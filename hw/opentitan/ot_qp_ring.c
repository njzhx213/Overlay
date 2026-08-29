/*
 * QP RING ENGINE — see include/hw/opentitan/ot_qp_ring.h
 *
 * Copyright (c) 2026 qemu-passes
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/opentitan/ot_qp_ring.h"

/* Pump invocations a ring may be kept alive by ACTIVITY alone before the
 * valve forces it quiet.  Generous: a legitimate crunch that outruns its
 * own settle finishes in a handful of invocations. */
#define QP_ACT_INVOCATIONS 1024u

/* Samples an `outstanding` transaction may live before the engine assumes a
 * one-clock CLOSES was missed and heals itself.  Far longer than any real
 * transaction (a csrng GEN is thousands of beats), short enough that a wedge
 * cannot outlive one gate target. */
#define QP_OUT_TTL 200000u

static bool qp_row_asserted(const qp_wire *w)
{
    const uint8_t *p = w->src;
    size_t total = (size_t)w->bytes * w->n;

    if (!p) {
        return false;
    }
    for (size_t i = 0; i < total; i++) {
        if (p[i]) {
            return true;
        }
    }
    return false;
}

void qp_ring_sample(qp_ring *r)
{
    for (unsigned l = 0; l < r->n_legs; l++) {
        r->leg[l].req = false;
    }

    /* OPENS first, REQ next, CLOSES last: a same-beat open+close nets
     * closed, which is what a one-clock request/ack pair means. */
    for (unsigned i = 0; i < r->n_rows; i++) {
        const qp_wire *w = &r->rows[i];
        if (!r->leg[w->leg].present || !(w->flags & QP_W_OPENS)) {
            continue;
        }
        if (qp_row_asserted(w)) {
            if (!r->leg[w->leg].outstanding) {
                r->leg[w->leg].out_ttl = QP_OUT_TTL;
            }
            r->leg[w->leg].outstanding = true;
        }
    }
    for (unsigned i = 0; i < r->n_rows; i++) {
        const qp_wire *w = &r->rows[i];
        if (!r->leg[w->leg].present || w->role != QP_ROLE_REQ) {
            continue;
        }
        if (qp_row_asserted(w)) {
            r->leg[w->leg].req = true;
        }
    }
    for (unsigned i = 0; i < r->n_rows; i++) {
        const qp_wire *w = &r->rows[i];
        if (!r->leg[w->leg].present || !(w->flags & QP_W_CLOSES)) {
            continue;
        }
        if (qp_row_asserted(w)) {
            r->leg[w->leg].outstanding = false;
        }
    }

    for (unsigned l = 0; l < r->n_legs; l++) {
        qp_leg *g = &r->leg[l];
        g->hot = g->present &&
                 (g->req || g->outstanding || (g->warm != 0u));
    }
}

void qp_ring_beat(qp_ring *r)
{
    qp_ring_sample(r);
    for (unsigned l = 0; l < r->n_legs; l++) {
        qp_leg *g = &r->leg[l];
        if (!g->present) {
            continue;
        }
        if (g->outstanding && g->out_ttl && --g->out_ttl == 0u) {
            g->outstanding = false;   /* self-heal: CLOSES never observed */
            qemu_log_mask(LOG_UNIMP, "qp ring %s leg %s: outstanding TTL "
                          "expired (a close pulse was missed)\n",
                          r->name, g->name);
        }
        if (g->req || g->outstanding) {
            g->warm = g->warm_reload;
        } else if (g->warm) {
            g->warm--;
        }
        g->hot = g->req || g->outstanding || (g->warm != 0u);
    }
}

bool qp_ring_hot(const qp_ring *r)
{
    for (unsigned l = 0; l < r->n_legs; l++) {
        if (r->leg[l].present && r->leg[l].hot) {
            return true;
        }
    }
    return false;
}

void qp_ring_wire(qp_ring *r)
{
    for (unsigned i = 0; i < r->n_rows; i++) {
        const qp_wire *w = &r->rows[i];
        if (!w->dst || !w->src || !r->leg[w->leg].present) {
            continue;
        }
        memcpy(w->dst, w->src, (size_t)w->bytes * w->n);
    }
}

void qp_ring_freeze(qp_ring *r)
{
    for (unsigned i = 0; i < r->n_rows; i++) {
        const qp_wire *w = &r->rows[i];
        if (!w->dst || !(w->flags & QP_W_FREEZE) || !r->leg[w->leg].present) {
            continue;
        }
        for (unsigned e = 0; e < w->n; e++) {
            void *p = (uint8_t *)w->dst + (size_t)e * w->bytes;
            switch (w->bytes) {
            case 1: *(uint8_t *)p = (uint8_t)w->idle; break;
            case 2: *(uint16_t *)p = (uint16_t)w->idle; break;
            case 4: *(uint32_t *)p = (uint32_t)w->idle; break;
            case 8: *(uint64_t *)p = w->idle; break;
            default:
                /* wider leaf (e.g. a 128-bit bus): only an all-zero idle is
                 * representable, which is what every declared row uses */
                g_assert(w->idle == 0);
                memset(p, 0, w->bytes);
                break;
            }
        }
    }
}

static bool qp_member_steps(const qp_ring *r, const qp_member *p)
{
    if (!p->present || *p->busy) {
        return false;
    }
    /* an optional member (a leg the ring may or may not carry) is stepped
     * only while its own leg is hot: an idle one is never ticked */
    return p->gate_leg < 0 || r->leg[p->gate_leg].hot;
}

void qp_ring_step(qp_ring *r)
{
    /* One update / tick / update over the union of present members, each
     * guarded by its own _qp_busy: the model whose settle we are inside is
     * ticked by that settle loop, never here (phase triad, rule i/ii). */
    for (unsigned m = 0; m < r->n_members; m++) {
        qp_member *p = &r->member[m];
        if (qp_member_steps(r, p)) {
            p->update(p->st);
        }
    }
    for (unsigned m = 0; m < r->n_members; m++) {
        qp_member *p = &r->member[m];
        if (qp_member_steps(r, p)) {
            p->tick(p->st);
        }
    }
    for (unsigned m = 0; m < r->n_members; m++) {
        qp_member *p = &r->member[m];
        if (qp_member_steps(r, p)) {
            p->update(p->st);
            /* A member that has once reached a fixed point is eligible to
             * contribute its activity bit; one that never does (a free-
             * running counter, a limit cycle) excludes itself forever. */
            if (!*p->active) {
                p->ever_quiet = true;
            }
        }
    }
    r->costeps++;
    /* every beat is scanned: a one-clock pulse can only be produced by a
     * beat, so this is what makes OPENS/CLOSES observation complete */
    qp_ring_beat(r);
}

bool qp_ring_any_active(qp_ring *r)
{
    for (unsigned m = 0; m < r->n_members; m++) {
        const qp_member *p = &r->member[m];
        if (p->present && p->ever_quiet && *p->active) {
            return true;
        }
    }
    return false;
}

bool qp_ring_busy(qp_ring *r, bool extra)
{
    return qp_ring_hot(r) || extra || qp_ring_any_active(r);
}

bool qp_ring_act_valve(qp_ring *r, bool hot_or_extra)
{
    if (hot_or_extra) {
        r->act_only = 0;
        return false;
    }
    if (!qp_ring_any_active(r)) {
        r->act_only = 0;
        return false;
    }
    if (r->act_only < QP_ACT_INVOCATIONS) {
        r->act_only++;
        return false;
    }
    if (!r->valve_fired) {
        r->valve_fired = true;
        qemu_log_mask(LOG_UNIMP,
                      "qp ring %s: activity-only valve forced quiet after %u "
                      "pump invocations\n", r->name, QP_ACT_INVOCATIONS);
    }
    return true;  /* force quiet */
}
