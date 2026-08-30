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

/* OT_RING_STATS=1 prints the audit totals at exit.  settle_traffic is the
 * number the gate cannot produce: how often cross-model traffic arises INSIDE
 * a settle.  Zero across a workload means the settle hook is unexercised by
 * it — a measurement, where before there was only the observation that the
 * gate passes with hooks disabled, which could equally have meant the hook was
 * redundant or the workload never reached for it. */
#define QP_MAX_TRACKED 8
static qp_ring *qp_tracked[QP_MAX_TRACKED];
static unsigned qp_n_tracked;

static void qp_ring_report(void)
{
    for (unsigned i = 0; i < qp_n_tracked; i++) {
        const qp_ring *r = qp_tracked[i];
        fprintf(stderr,
                "qp_ring[%s]: hook_calls=%llu fires=%llu costeps=%llu "
                "pump_beats=%llu settle_traffic=%llu missed_beats=%llu\n",
                r->name ? r->name : "?",
                (unsigned long long)r->hook_calls,
                (unsigned long long)r->hook_fires,
                (unsigned long long)r->costeps,
                (unsigned long long)r->pump_beats,
                (unsigned long long)r->settle_traffic,
                (unsigned long long)r->missed_beats);
    }
}

void qp_ring_audit(qp_ring *r, bool hot)
{
    /* hook_calls / hook_fires were declared with the struct but NOTHING ever
     * incremented them, so every reading of them was 0 and meant nothing.
     * The audit runs once per hook invocation, which is exactly the place. */
    r->hook_calls++;
    if (hot) {
        r->hook_fires++;
    }

    if (getenv("OT_RING_STATS")) {
        bool known = false;
        for (unsigned i = 0; i < qp_n_tracked; i++) {
            if (qp_tracked[i] == r) { known = true; break; }
        }
        if (!known && qp_n_tracked < QP_MAX_TRACKED) {
            if (qp_n_tracked == 0) {
                atexit(qp_ring_report);
            }
            qp_tracked[qp_n_tracked++] = r;
        }
    }

    uint64_t cur[4] = { 0, 0, 0, 0 };
    bool changed = false;

    for (unsigned i = 0; i < r->n_rows && i < 256u; i++) {
        const qp_wire *w = &r->rows[i];
        if (!r->leg[w->leg].present || w->role != QP_ROLE_REQ) {
            continue;
        }
        if (qp_row_asserted(w)) {
            cur[i >> 6] |= 1ULL << (i & 63u);
        }
    }
    if (r->snap_valid) {
        for (unsigned k = 0; k < 4; k++) {
            /* RISING edges only.  A REQ row going 1 -> 0 is traffic ENDING,
             * and a quiet verdict at that moment is correct, not a miss.
             * Counting any change made the detector fire exactly twice on
             * every workload — its first reading was its own false positive,
             * which is what a new instrument is for. */
            if (cur[k] & ~r->snap[k]) {
                changed = true;
            }
        }
    }
    for (unsigned k = 0; k < 4; k++) {
        r->snap[k] = cur[k];
    }
    r->snap_valid = true;

    /* OT_FORCE_MISSED_BEAT=1 fires the detector on purpose.  A detector that
     * has never been seen to fire is indistinguishable from one that cannot,
     * and this project has shipped exactly that kind of ornament before, so
     * the positive control is part of the mechanism rather than a one-off
     * experiment someone has to reconstruct. */
    if (getenv("OT_FORCE_MISSED_BEAT")) {
        changed = true;
        hot = false;
    }
    if (!changed) {
        return;
    }
    r->settle_traffic++;
    if (hot) {
        return;                 /* traffic seen AND served — the normal case */
    }
    if (++r->missed_beats == 1) {
        fprintf(stderr,
                "qp_ring[%s]: MISSED BEAT - a declared REQ row moved inside a "
                "settle while the hook verdict was quiet.  A leg can wedge "
                "silently from here; check the row roles and the beat scan.\n",
                r->name ? r->name : "?");
    }
}
