/*
 * QP RING ENGINE — generic cross-model liveness for qemu-passes models.
 *
 * A "ring" is a set of generated models the machine wires together.  The
 * engine expresses ring liveness over a CLOSED SET of top-level port rows
 * whose role is DECLARED DATA, so the machine needs no device knowledge:
 * no internal (u_*) signal references and no FSM state constants.
 *
 * The one thing ports cannot express — "a partner still has local work" —
 * is answered by the models' own activity bit (_qp_active) and is used
 * ONLY by a ring PUMP, outside every settle.  A settle hook must never
 * consult it: a model with no fixed point would pin it at 1 and hold every
 * MMIO settle open to its cap.
 *
 * Copyright (c) 2026 qemu-passes
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef HW_OPENTITAN_OT_QP_RING_H
#define HW_OPENTITAN_OT_QP_RING_H

/* Role selects the SAMPLING domain of a row.  Declared, never derived:
 * a ready/gnt line idles high and a data/qualifier leaf can hold a value
 * forever, so neither can be distinguished from traffic by observation. */
typedef enum {
    QP_ROLE_DATA  = 0,  /* payload/qualifier: wired, never sampled */
    QP_ROLE_REQ   = 1,  /* one-clock or level request/ack: sampled */
    QP_ROLE_READY = 2,  /* backpressure line (idles high): never sampled */
} qp_wrole;

#define QP_W_OPENS   (1u << 0)  /* asserting this row opens an outstanding txn */
#define QP_W_CLOSES  (1u << 1)  /* asserting this row closes it */
#define QP_W_FREEZE  (1u << 2)  /* quiet path writes `idle` into dst */

#define QP_MAX_LEGS     4u
#define QP_MAX_MEMBERS  6u

/* One wired leaf (or leaf array).  src/dst are DIRECT MEMBER POINTERS, so
 * every row is compile-checked against the generated struct. */
typedef struct {
    unsigned    leg;    /* owning leg index */
    const void *src;    /* &producer->out_leaf (NULL = freeze-only row) */
    void       *dst;    /* &consumer->in_leaf  (NULL = sample-only row) */
    unsigned    bytes;  /* 1/2/4/8 per element */
    unsigned    n;      /* element count (1 for scalars) */
    unsigned    role;   /* qp_wrole */
    unsigned    flags;  /* QP_W_* */
    uint64_t    idle;   /* value the freeze writes */
} qp_wire;

/* A leg is a transaction domain: its rows open/close together and it warms
 * for warm_reload beats after the last assertion (multi-beat bursts leave
 * gaps where no row is asserted). */
typedef struct {
    const char *name;
    unsigned    warm_reload;
    bool        present;
    /* runtime */
    bool        req;
    bool        outstanding;
    unsigned    out_ttl;    /* samples an outstanding txn may live (self-heal:
                             * a missed one-clock CLOSES must not wedge hot) */
    unsigned    warm;
    bool        hot;
} qp_leg;

/* A ring member is a model the engine may step. */
typedef struct {
    const char *name;
    void       *st;         /* model state */
    uint8_t    *busy;       /* &st->_qp_busy   */
    uint8_t    *active;     /* &st->_qp_active */
    void      (*update)(void *);
    void      (*tick)(void *);
    bool        present;
    int         gate_leg;   /* >=0: step only while that leg is hot (an idle
                             * optional member is never ticked); -1: always */
    /* runtime */
    bool        ever_quiet; /* observed _qp_active == 0 at least once */
} qp_member;

typedef struct {
    const char *name;
    qp_wire    *rows;
    unsigned    n_rows;
    qp_leg      leg[QP_MAX_LEGS];
    unsigned    n_legs;
    qp_member   member[QP_MAX_MEMBERS];
    unsigned    n_members;
    /* runtime */
    unsigned    act_only;   /* consecutive activity-only pump invocations */
    bool        valve_fired;
    /* telemetry */
    uint64_t    hook_calls, hook_fires, costeps, pump_beats;
    /* missed-beat self-check (see qp_ring_audit) */
    uint64_t    snap[4];        /* per-row asserted bitmap, up to 256 rows */
    bool        snap_valid;
    uint64_t    settle_traffic; /* REQ-row transitions observed inside settles */
    uint64_t    missed_beats;   /* ... of those, ones the verdict called quiet */
} qp_ring;

/* Idempotent level scan: recompute req/outstanding/hot from the rows.  Safe
 * to call any number of times per beat (no warm decay here). */
void qp_ring_sample(qp_ring *r);
/* Exactly once per RING BEAT (a costep): age the warm windows.  A pulse can
 * only be produced by a beat, so scanning on every beat is what guarantees a
 * one-clock CLOSES is never missed. */
void qp_ring_beat(qp_ring *r);
/* Any leg hot? (the settle hook's verdict; ports + warm only) */
bool qp_ring_hot(const qp_ring *r);
/* Copy every row that has a destination. */
void qp_ring_wire(qp_ring *r);
/* Write `idle` into every QP_W_FREEZE row's destination. */
void qp_ring_freeze(qp_ring *r);
/* One update/tick/update over the union of present members (busy-skipped). */
void qp_ring_step(qp_ring *r);
/* PUMP ONLY: has any member that is known to be able to quiesce reported
 * activity?  Members that never quiesce auto-exclude themselves. */
bool qp_ring_any_active(qp_ring *r);
/* PUMP ONLY: hot || activity.  `extra` folds in machine-side pending state
 * (e.g. an organ holding an unaccepted sample). */
bool qp_ring_busy(qp_ring *r, bool extra);
/* PUMP ONLY safety valve: force quiet if the ring has been kept alive by
 * activity alone for QP_ACT_INVOCATIONS pump invocations. */
bool qp_ring_act_valve(qp_ring *r, bool hot_or_extra);

/* MISSED-BEAT SELF-CHECK.  Call once per settle-hook invocation, passing the
 * verdict the hook is about to act on.
 *
 * The most expensive failure mode this engine has is SILENT: cross-model
 * traffic occurs inside one model's MMIO settle, the hook's level sample does
 * not see it, the hook reports quiet, and the leg wedges.  Diagnosing one of
 * those cost an afternoon; a printed line costs nothing.  The signature is
 * cheap: a row whose role is DECLARED REQ changed since the previous hook call
 * while the verdict for this call was NOT hot — traffic happened and we did
 * not serve it.
 *
 * Roles are what make this device-agnostic: a DATA leaf can hold a value
 * forever and a READY line idles high, so neither's movement means traffic.
 * Only REQ rows are sampled — the same declared column the engine already
 * trusts for its own liveness.
 *
 * LIMIT, stated because a detector you over-trust is worse than none: a pulse
 * that rises AND falls entirely between two samples is invisible to any
 * sampler.  That is exactly why the engine scans on every ring BEAT rather
 * than only when the hook fires; this check is a backstop for the sampling
 * path, not a proof of its absence.
 *
 * It also answers a question the gate cannot: settle_traffic counts how often
 * cross-model traffic arises INSIDE a settle at all.  If that stays zero
 * across a workload, the hook is unexercised by it — which is a measurement,
 * not an inference. */
void qp_ring_audit(qp_ring *r, bool hot);

#endif /* HW_OPENTITAN_OT_QP_RING_H */
