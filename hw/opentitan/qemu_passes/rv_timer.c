/*
 * Auto-generated QEMU device: rv_timer
 *
 * Bus interface: protocol-agnostic (inferred from IR)
 *   addr   -> s->tl_i_a_address
 *   wdata  -> s->tl_i_a_data
 *   rdata  <- s->u_reg_tl_o_d_data
 *   valid  -> s->tl_i_a_valid  (pulsed 1 per txn — TL-UL a_valid)
 *   opcode -> s->tl_i_a_opcode (write=0 PutFullData, read=4 Get)
 *
 * The MMIO callbacks directly bridge QEMU's hwaddr/value to the
 * internal flattened state variables, then invoke update_state()
 * to propagate combinational logic. This design erases the bus
 * protocol struct layer (TL-UL / APB / AXI).
 */

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "rv_timer.h"

#pragma GCC diagnostic ignored "-Wbool-operation"
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#pragma GCC diagnostic ignored "-Wtype-limits"

/* Bug G: extract <=64 bits from a wide uint64_t arr[nWords]. */
static inline uint64_t qpw_wide_extract(const uint64_t *arr,
                                        unsigned nWords,
                                        unsigned shift,
                                        unsigned width)
{
    if (width == 0) return 0;
    unsigned wordIdx = shift / 64;
    unsigned bitIdx  = shift % 64;
    uint64_t mask = (width >= 64) ? ~(uint64_t)0
                                  : (((uint64_t)1 << width) - 1);
    if (wordIdx >= nWords) return 0;
    uint64_t lo = arr[wordIdx] >> bitIdx;
    if (bitIdx + width > 64 && wordIdx + 1 < nWords) {
        lo |= arr[wordIdx + 1] << (64 - bitIdx);
    }
    return lo & mask;
}

/* Memory arrays: write <=64 bits into a wide uint64_t arr[nWords]. */
static inline void qpw_wide_insert(uint64_t *arr, unsigned nWords,
                                   unsigned shift, unsigned width,
                                   uint64_t val)
{
    if (width == 0) return;
    unsigned wordIdx = shift / 64;
    unsigned bitIdx  = shift % 64;
    uint64_t mask = (width >= 64) ? ~(uint64_t)0
                                  : (((uint64_t)1 << width) - 1);
    if (wordIdx >= nWords) return;
    val &= mask;
    arr[wordIdx] = (arr[wordIdx] & ~(mask << bitIdx)) | (val << bitIdx);
    if (bitIdx + width > 64 && wordIdx + 1 < nWords) {
        unsigned hi = bitIdx + width - 64;   /* bits spilling into next word */
        uint64_t hmask = (((uint64_t)1 << hi) - 1);
        arr[wordIdx + 1] = (arr[wordIdx + 1] & ~hmask) | ((val >> (64 - bitIdx)) & hmask);
    }
}

/* Low 128 bits of a wide uint64_t arr[nWords] as a scalar (65..128-bit
 * expression contexts reading a wide field whole). */
static inline __uint128_t qpw_wide_read128(const uint64_t *arr,
                                           unsigned nWords)
{
    __uint128_t v = arr[0];
    if (nWords > 1) v |= ((__uint128_t)arr[1]) << 64;
    return v;
}

static void update_state_once(rv_timer_state *s);
typedef struct QPSettleFingerprint {
    uint64_t first;
    uint64_t second;
} QPSettleFingerprint;
static QPSettleFingerprint qp_settle_fingerprint(const rv_timer_state *s);
static void update_state(rv_timer_state *s)
{
    update_state_once(s);
    update_state_once(s);
    update_state_once(s);
}
static bool tick(rv_timer_state *s);
/* One clock + per-clock observer hook (organs needing edge visibility). */
static inline bool qp_tick(rv_timer_state *s)
{
    if (s->_qp_before_tick) {
        s->_qp_before_tick(s->_qp_before_tick_ctx);
        update_state(s);
    }
    bool _qp_ch = tick(s);
    if (s->_qp_on_tick) s->_qp_on_tick(s->_qp_on_tick_ctx);
    /* Activity bit.  Written AFTER the observer so an organ's
     * whole-core snapshot restore (spi_host byte replay) cannot
     * revert it, and a rewind counts as activity.
     * CONTRACT: consumed ONLY by a ring PUMP, outside every settle.
     * A settle hook must NEVER read it: a model with no fixed point
     * would pin it at 1 and hold every MMIO settle open to its cap. */
    s->_qp_active = (_qp_ch || s->_qp_rewound) ? 1 : 0;
    return _qp_ch;
}

/* Single settle bound per MMIO access — responsiveness only, not
 * correctness: leftover work continues on the next access/poll. */
#define QP_SETTLE_BUDGET 65536u
/* Cross-model tier: a hook-extended settle's continuation is the ring
 * PUMP, which runs outside the settle — the same argument the 256 tier
 * carries for firmware polling.  65536 survives only for an organ
 * holding an atomic protocol unit, where the MODEL owns the
 * continuation. */
#define QP_EXT_BUDGET     4096u
#define QP_EXT_STRIKES       3u

static QPSettleFingerprint qp_settle_fingerprint(const rv_timer_state *s)
{
    const uint8_t *bytes = (const uint8_t *)s;
    QPSettleFingerprint fp = {
        UINT64_C(1469598103934665603),
        UINT64_C(1099511628211),
    };

    for (size_t i = 0; i < sizeof(*s); ++i) {
        fp.first ^= bytes[i];
        fp.first *= UINT64_C(1099511628211);
        fp.second ^= (uint64_t)bytes[i] + UINT64_C(0x9e3779b97f4a7c15)
                     + (fp.second << 6) + (fp.second >> 2);
    }
    return fp;
}

/*
 * update_state() - Re-evaluate all combinational logic.
 *
 * Called after every MMIO write or read (for read-triggered outputs).
 * This is the "single-cycle evaluation" model: we assume all
 * combinational paths settle instantaneously (no multi-cycle handshake).
 */
static void update_state_once(rv_timer_state *s)
{
    s->gen_harts_0_u_intr_hw_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_rst_ni = (s->gen_harts_0_u_intr_hw_rst_ni) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_intr_o = (s->gen_harts_0_u_intr_hw_intr_o) & ((1ULL << 1) - 1);
    s->intr_out = (s->gen_harts_0_u_intr_hw_intr_o) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_core_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_core_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_core_rst_ni = (s->gen_harts_0_u_core_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->u_reg_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_tl_i_a_valid = (s->tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_tl_i_a_opcode = (s->tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_tl_i_a_param = (s->tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_tl_i_a_size = (s->tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_tl_i_a_source = s->tl_i_a_source;
    s->u_reg_tl_i_a_address = s->tl_i_a_address;
    s->u_reg_tl_i_a_mask = (s->tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_tl_i_a_data = s->tl_i_a_data;
    s->u_reg_tl_i_a_user_rsvd = (s->tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_tl_i_a_user_instr_type = (s->tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_tl_i_a_user_cmd_intg = (s->tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_tl_i_a_user_data_intg = (s->tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_tl_i_d_ready = (s->tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_racl_policies_i_0__write_perm = (s->racl_policies_i_0__write_perm) & ((1ULL << 2) - 1);
    s->u_reg_racl_policies_i_0__read_perm = (s->racl_policies_i_0__read_perm) & ((1ULL << 2) - 1);
    s->u_reg_intg_err = (0) & ((1ULL << 1) - 1);
    s->u_reg_racl_role_vec = (0) & ((1ULL << 2) - 1);
    s->u_reg_racl_addr_hit_read = (0) & ((1ULL << 10) - 1);
    s->u_reg_racl_addr_hit_write = (0) & ((1ULL << 10) - 1);
    s->u_reg_racl_error_o_racl_role = (0) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_overflow = (0) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_ctn_uid = (0) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_read_access = (0) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = 0;
    s->u_reg_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_racl_policies_i_0__write_perm = (s->u_reg_racl_policies_i_0__write_perm) & ((1ULL << 2) - 1);
    s->u_reg_racl_policies_i_0__read_perm = (s->u_reg_racl_policies_i_0__read_perm) & ((1ULL << 2) - 1);
    s->u_reg_u_chk_tl_i_a_valid = (s->u_reg_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_chk_tl_i_a_opcode = (s->u_reg_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_chk_tl_i_a_param = (s->u_reg_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_u_chk_tl_i_a_size = (s->u_reg_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_u_chk_tl_i_a_source = s->u_reg_tl_i_a_source;
    s->u_reg_u_chk_tl_i_a_address = s->u_reg_tl_i_a_address;
    s->u_reg_u_chk_tl_i_a_mask = (s->u_reg_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_u_chk_tl_i_a_data = s->u_reg_tl_i_a_data;
    s->u_reg_u_chk_tl_i_a_user_rsvd = (s->u_reg_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_u_chk_tl_i_a_user_instr_type = (s->u_reg_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_u_chk_tl_i_a_user_cmd_intg = (s->u_reg_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_chk_tl_i_a_user_data_intg = (s->u_reg_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_chk_tl_i_d_ready = (s->u_reg_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_chk_err_o = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_prim_reg_we_check_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_prim_reg_we_check_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_prim_reg_we_check_err_o = (0) & ((1ULL << 1) - 1);
    s->u_reg_reg_we_err = (s->u_reg_u_prim_reg_we_check_err_o) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_i_a_valid = (s->u_reg_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_i_a_opcode = (s->u_reg_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_tl_i_a_param = (s->u_reg_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_tl_i_a_size = (s->u_reg_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_tl_i_a_source = s->u_reg_tl_i_a_source;
    s->u_reg_u_reg_if_tl_i_a_address = s->u_reg_tl_i_a_address;
    s->u_reg_u_reg_if_tl_i_a_mask = (s->u_reg_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_tl_i_a_data = s->u_reg_tl_i_a_data;
    s->u_reg_u_reg_if_tl_i_a_user_rsvd = (s->u_reg_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_u_reg_if_tl_i_a_user_instr_type = (s->u_reg_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_tl_i_a_user_cmd_intg = (s->u_reg_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_tl_i_a_user_data_intg = (s->u_reg_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_tl_i_d_ready = (s->u_reg_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_en_ifetch_i = (9) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_busy_i = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_rst_ni = (s->u_reg_u_reg_if_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_i_a_valid = (s->u_reg_u_reg_if_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_i_a_opcode = (s->u_reg_u_reg_if_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_tl_i_a_param = (s->u_reg_u_reg_if_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_tl_i_a_size = (s->u_reg_u_reg_if_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_tl_i_a_source = s->u_reg_u_reg_if_tl_i_a_source;
    s->u_reg_u_reg_if_tl_i_a_address = s->u_reg_u_reg_if_tl_i_a_address;
    s->u_reg_u_reg_if_tl_i_a_mask = (s->u_reg_u_reg_if_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_tl_i_a_data = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_tl_i_a_user_rsvd = (s->u_reg_u_reg_if_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_u_reg_if_tl_i_a_user_instr_type = (s->u_reg_u_reg_if_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_tl_i_a_user_cmd_intg = (s->u_reg_u_reg_if_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_tl_i_a_user_data_intg = (s->u_reg_u_reg_if_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_tl_i_d_ready = (s->u_reg_u_reg_if_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_u_reg_if_outstanding_q) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_u_reg_if_rspop_q) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param = (0) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_u_reg_if_reqsz_q) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_reg_if_reqid_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_reg_if_rdata_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (0x0ULL) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg = (0x0ULL) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_u_reg_if_error_q) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready = (((((s->u_reg_u_reg_if_outstanding_q) | (s->u_reg_u_reg_if_busy_i))) ^ (1))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_rsp_intg = (0) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_data_intg = (0) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_d_ack = ((s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & (s->u_reg_u_reg_if_tl_i_d_ready)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_a_ack = ((s->u_reg_u_reg_if_tl_i_a_valid) & (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_wr_req = ((s->u_reg_u_reg_if_a_ack) & ((((s->u_reg_u_reg_if_tl_i_a_opcode) == (0))) | (((s->u_reg_u_reg_if_tl_i_a_opcode) == (1))))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_rd_req = ((s->u_reg_u_reg_if_a_ack) & (((s->u_reg_u_reg_if_tl_i_a_opcode) == (4)))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_addr_align_err = (((s->u_reg_u_reg_if_wr_req) ? (((((s->u_reg_u_reg_if_tl_i_a_address) & 0x3)) != (0))) : s->u_reg_u_reg_if_addr_align_err)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_addr_align_err = (((!(s->u_reg_u_reg_if_wr_req)) ? (0) : s->u_reg_u_reg_if_addr_align_err)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_clk_i = (s->u_reg_u_reg_if_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_rst_ni = (s->u_reg_u_reg_if_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_valid = (s->u_reg_u_reg_if_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_opcode = (s->u_reg_u_reg_if_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_param = (s->u_reg_u_reg_if_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_size = (s->u_reg_u_reg_if_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_source = s->u_reg_u_reg_if_tl_i_a_source;
    s->u_reg_u_reg_if_u_err_tl_i_a_address = s->u_reg_u_reg_if_tl_i_a_address;
    s->u_reg_u_reg_if_u_err_tl_i_a_mask = (s->u_reg_u_reg_if_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_data = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd = (s->u_reg_u_reg_if_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type = (s->u_reg_u_reg_if_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg = (s->u_reg_u_reg_if_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg = (s->u_reg_u_reg_if_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_d_ready = (s->u_reg_u_reg_if_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_mask_chk = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_fulldata_chk = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_valid = (s->u_reg_u_reg_if_u_err_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_opcode = (s->u_reg_u_reg_if_u_err_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_param = (s->u_reg_u_reg_if_u_err_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_size = (s->u_reg_u_reg_if_u_err_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = ((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0)))) ? (-1) : s->u_reg_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_source = s->u_reg_u_reg_if_u_err_tl_i_a_source;
    s->u_reg_u_reg_if_u_err_tl_i_a_address = s->u_reg_u_reg_if_u_err_tl_i_a_address;
    s->u_reg_u_reg_if_u_err_mask = ((1) << (((((uint64_t)(0)) << 2) | ((uint64_t)(((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 0x3)))))) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = (((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1)))) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 1)) ^ 1)) : s->u_reg_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = ((((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (2)))) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 0x3)) == (0))) : s->u_reg_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = ((((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_reg_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_mask = (s->u_reg_u_reg_if_u_err_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_u_err_mask_chk = ((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0)))) ? ((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & (((~(s->u_reg_u_reg_if_u_err_mask)) & 0xFULL))) == (0))) : s->u_reg_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_fulldata_chk = ((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0)))) ? ((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & (s->u_reg_u_reg_if_u_err_mask)) != (0))) : s->u_reg_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_mask_chk = (((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1)))) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (0))) : ((((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (0))))) : s->u_reg_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_fulldata_chk = (((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1)))) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (3))) : (((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (3))))) : s->u_reg_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_mask_chk = ((((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (2)))) ? (-1) : s->u_reg_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_fulldata_chk = ((((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (2)))) ? (((s->u_reg_u_reg_if_u_err_tl_i_a_mask) == (15))) : s->u_reg_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_mask_chk = ((((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_reg_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_fulldata_chk = ((((((s->u_reg_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_u_reg_if_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_reg_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_data = s->u_reg_u_reg_if_u_err_tl_i_a_data;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd = (s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type = (s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg = (s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg = (s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_u_err_tl_i_d_ready = (s->u_reg_u_reg_if_u_err_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_u_err_err_o = (((((((((((s->u_reg_u_reg_if_u_err_tl_i_a_opcode) == (0))) | (((s->u_reg_u_reg_if_u_err_tl_i_a_opcode) == (1))) | (((s->u_reg_u_reg_if_u_err_tl_i_a_opcode) == (4))))) & (s->u_reg_u_reg_if_u_err_addr_sz_chk) & (s->u_reg_u_reg_if_u_err_mask_chk) & (((((s->u_reg_u_reg_if_u_err_tl_i_a_opcode) == (4))) | (((s->u_reg_u_reg_if_u_err_tl_i_a_opcode) == (1))) | (s->u_reg_u_reg_if_u_err_fulldata_chk))))) ^ (1))) | (((((s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type) == (6))) & (((((s->u_reg_u_reg_if_u_err_tl_i_a_opcode) == (0))) | (((s->u_reg_u_reg_if_u_err_tl_i_a_opcode) == (1))))))) | (((((((s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type) == (6))) | (((s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type) == (9))))) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_err_internal = ((s->u_reg_u_reg_if_addr_align_err) | (s->u_reg_u_reg_if_u_err_err_o) | ((((s->u_reg_u_reg_if_tl_i_a_user_instr_type) == (6))) & (((s->u_reg_u_reg_if_en_ifetch_i) != (6))))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_o_d_valid = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_u_reg_if_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_o_d_opcode = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_u_reg_if_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_qpinl5_payload_opcode = (s->u_reg_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_tl_o_d_param = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_param = (s->u_reg_u_reg_if_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_param = (s->u_reg_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_u_reg_if_tl_o_d_size = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_u_reg_if_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_rsp_intg_gen_qpinl5_payload_size = (s->u_reg_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_u_reg_if_tl_o_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_reg_if_tl_o_d_source;
    s->u_reg_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_reg_if_tl_o_d_sink = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_sink = (s->u_reg_u_reg_if_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_sink = (s->u_reg_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_o_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_reg_if_tl_o_d_data;
    s->u_reg_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i = s->u_reg_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((((uint64_t)(0)) << 32) | ((uint64_t)(s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i)))) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x100000000ULL) | ((((__builtin_parityll((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (637975845)))) & 0x1ULL) << 32)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x200000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 4) & 0xFFFFFFFULL)) & (233547781))) << 4) | ((uint64_t)(0)))))) & 0x1ULL) << 33)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x400000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 31) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x3FFFFFFFULL)) & (547275989))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 34)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x800000000ULL) | ((((__builtin_parityll((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (824397521)))) & 0x1ULL) << 35)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x1000000000ULL) | ((((__builtin_parityll((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0xFFFFFFFFULL)) & (3267441211ULL)))) & 0x1ULL) << 36)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x2000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 30) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 2) & 0xFFFFFFFULL)) & (192092307))) << 2) | ((uint64_t)(0)))))) & 0x1ULL) << 37)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x4000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x7FFFFFFFULL)) & (1277700803))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 38)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) ^ (180388626432ULL)) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o = (s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_reg_u_rsp_intg_gen_data_intg = ((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o) >> 32) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_reg_if_tl_o_d_user_rsp_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (s->u_reg_u_reg_if_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_tl_o_d_user_data_intg = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg = (s->u_reg_u_reg_if_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg = (s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_u_rsp_intg_gen_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_reg_if_tl_o_d_error = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_u_reg_if_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_qpinl5_payload_error = (s->u_reg_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i = (((((((uint64_t)(s->u_reg_u_rsp_intg_gen_qpinl5_payload_error)) << 0) | (((uint64_t)(s->u_reg_u_rsp_intg_gen_qpinl5_payload_size)) << 1)) | (((uint64_t)(s->u_reg_u_rsp_intg_gen_qpinl5_payload_opcode)) << 3)) | (((uint64_t)(0)) << 6))) & ((1ULL << 57) - 1);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i) & ((1ULL << 57) - 1);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = ((((uint64_t)(0)) << 57) | ((uint64_t)(s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i)));
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x200000000000000ULL) | ((((__builtin_parityll((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (73183459585064959ULL)))) & 0x1ULL) << 57);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x400000000000000ULL) | ((((__builtin_parityll((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (106995641195921439ULL)))) & 0x1ULL) << 58);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x800000000000000ULL) | ((((__builtin_parityll((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (125504822018802145ULL)))) & 0x1ULL) << 59);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x1000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 1) & 0xFFFFFFFFFFFFFFULL)) & (67403489212122897ULL))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 60);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x2000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 2) & 0x7FFFFFFFFFFFFFULL)) & (34865184827919505ULL))) << 2) | ((uint64_t)(0)))))) & 0x1ULL) << 61);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x4000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 3) & 0x3FFFFFFFFFFFFFULL)) & (17723486863248017ULL))) << 3) | ((uint64_t)(0)))))) & 0x1ULL) << 62);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x8000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 4) & 0x1FFFFFFFFFFFFFULL)) & (8934470268372625ULL))) << 4) | ((uint64_t)(0)))))) & 0x1ULL) << 63);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) ^ (6052837899185946624ULL);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o;
    s->u_reg_u_rsp_intg_gen_rsp_intg = ((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 57) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_u_rsp_intg_gen_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_tl_o_a_ready = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_a_ready = (s->u_reg_u_reg_if_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_i_a_ready = (s->u_reg_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_intg_error_o = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_re_o = (((s->u_reg_u_reg_if_rd_req) & (((s->u_reg_u_reg_if_err_internal) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_we_o = (((s->u_reg_u_reg_if_wr_req) & (((s->u_reg_u_reg_if_err_internal) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_reg_we = (s->u_reg_u_reg_if_we_o) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_addr_o = (((((uint64_t)(0)) << 0) | (((uint64_t)((((s->u_reg_u_reg_if_tl_i_a_address) >> 2) & 0x7F))) << 2))) & ((1ULL << 9) - 1);
    s->u_reg_reg_addr = (s->u_reg_u_reg_if_addr_o) & ((1ULL << 9) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x1ULL) | (((((s->u_reg_reg_addr) == (0))) & 0x1ULL) << 0)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x2ULL) | (((((s->u_reg_reg_addr) == (4))) & 0x1ULL) << 1)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x4ULL) | (((((s->u_reg_reg_addr) == (256))) & 0x1ULL) << 2)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x8ULL) | (((((s->u_reg_reg_addr) == (260))) & 0x1ULL) << 3)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x10ULL) | (((((s->u_reg_reg_addr) == (264))) & 0x1ULL) << 4)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x20ULL) | (((((s->u_reg_reg_addr) == (268))) & 0x1ULL) << 5)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x40ULL) | (((((s->u_reg_reg_addr) == (272))) & 0x1ULL) << 6)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x80ULL) | (((((s->u_reg_reg_addr) == (276))) & 0x1ULL) << 7)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x100ULL) | (((((s->u_reg_reg_addr) == (280))) & 0x1ULL) << 8)) & ((1ULL << 10) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x200ULL) | (((((s->u_reg_reg_addr) == (284))) & 0x1ULL) << 9)) & ((1ULL << 10) - 1);
    s->u_reg_racl_addr_hit_read = (s->u_reg_addr_hit) & ((1ULL << 10) - 1);
    s->u_reg_racl_addr_hit_write = (s->u_reg_addr_hit) & ((1ULL << 10) - 1);
    s->u_reg_racl_error_o_valid = ((((s->u_reg_addr_hit) != (0))) & (((s->u_reg_u_reg_if_re_o) & (((s->u_reg_racl_addr_hit_read) == (0)))) | ((s->u_reg_reg_we) & (((s->u_reg_racl_addr_hit_write) == (0)))))) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_request_address = ((((uint64_t)(0)) << 9) | ((uint64_t)(s->u_reg_reg_addr)));
    s->u_reg_reg_rdata_next = (((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1)))) || (((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | (((0) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_prim_reg_we_check_en_i = (((s->u_reg_reg_we) & (((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((((s->u_reg_addr_hit) != (0))) ^ (1))))) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_wdata_o = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_be_o = (s->u_reg_u_reg_if_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_reg_be = (s->u_reg_u_reg_if_be_o) & ((1ULL << 4) - 1);
    s->u_reg_wr_err = ((s->u_reg_reg_we) & (((((s->u_reg_racl_addr_hit_write) & 1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 1) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 2) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 3) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 4) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 5) & 0x1)) & (((((s->u_reg_reg_be) & 0x7)) != (7)))) | (((((s->u_reg_racl_addr_hit_write) >> 6) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_racl_addr_hit_write) >> 7) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_racl_addr_hit_write) >> 8) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_racl_addr_hit_write) >> 9) & 0x1)) & (((s->u_reg_reg_be) != (15)))))) & ((1ULL << 1) - 1);
    s->u_reg_alert_test_we = ((((s->u_reg_racl_addr_hit_write) & 1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_we = (((((s->u_reg_racl_addr_hit_write) >> 1) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_intr_enable0_we = (((((s->u_reg_racl_addr_hit_write) >> 2) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_intr_state0_we = (((((s->u_reg_racl_addr_hit_write) >> 3) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_intr_test0_we = (((((s->u_reg_racl_addr_hit_write) >> 4) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_cfg0_we = (((((s->u_reg_racl_addr_hit_write) >> 5) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_timer_v_lower0_we = (((((s->u_reg_racl_addr_hit_write) >> 6) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_timer_v_upper0_we = (((((s->u_reg_racl_addr_hit_write) >> 7) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_compare_lower0_0_we = (((((s->u_reg_racl_addr_hit_write) >> 8) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_compare_upper0_0_we = (((((s->u_reg_racl_addr_hit_write) >> 9) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x1ULL) | (((s->u_reg_alert_test_we) & 0x1ULL) << 0)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x2ULL) | (((s->u_reg_ctrl_we) & 0x1ULL) << 1)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x4ULL) | (((s->u_reg_intr_enable0_we) & 0x1ULL) << 2)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x8ULL) | (((s->u_reg_intr_state0_we) & 0x1ULL) << 3)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x10ULL) | (((s->u_reg_intr_test0_we) & 0x1ULL) << 4)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x20ULL) | (((s->u_reg_cfg0_we) & 0x1ULL) << 5)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x40ULL) | (((s->u_reg_timer_v_lower0_we) & 0x1ULL) << 6)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x80ULL) | (((s->u_reg_timer_v_upper0_we) & 0x1ULL) << 7)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x100ULL) | (((s->u_reg_compare_lower0_0_we) & 0x1ULL) << 8)) & ((1ULL << 10) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x200ULL) | (((s->u_reg_compare_upper0_0_we) & 0x1ULL) << 9)) & ((1ULL << 10) - 1);
    s->u_reg_u_prim_reg_we_check_oh_i = (s->u_reg_reg_we_check) & ((1ULL << 10) - 1);
    s->u_reg_u_reg_if_error_i = (((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((((s->u_reg_addr_hit) != (0))) ^ (1))))) | (s->u_reg_wr_err) | (s->u_reg_intg_err))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_error_i = (s->u_reg_u_reg_if_error_i) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_re = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_we = (s->u_reg_alert_test_we) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_qe = (s->u_reg_u_alert_test_we) & ((1ULL << 1) - 1);
    s->u_reg_alert_test_flds_we = (s->u_reg_u_alert_test_qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_alert_test_qe = (s->u_reg_alert_test_flds_we) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_qre = (s->u_reg_u_alert_test_re) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_q = (s->u_reg_u_alert_test_wd) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_alert_test_q = (s->u_reg_u_alert_test_q) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_ds = (s->u_reg_u_alert_test_d) & ((1ULL << 1) - 1);
    s->u_reg_u_alert_test_qs = (s->u_reg_u_alert_test_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_rst_ni = (s->u_reg_u_ctrl_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_we = (s->u_reg_u_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_wd = (s->u_reg_u_ctrl_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_de = (s->u_reg_u_ctrl_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_d = (s->u_reg_u_ctrl_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_wd = (s->u_reg_u_ctrl_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_d = (s->u_reg_u_ctrl_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_wr_en_data_arb_we) | (s->u_reg_u_ctrl_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en = (s->u_reg_u_ctrl_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_data = (s->u_reg_u_ctrl_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_qe = (s->u_reg_u_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_q = (s->u_reg_u_ctrl_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_0__q = (s->u_reg_u_ctrl_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_qs = (s->u_reg_u_ctrl_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_wr_en_data_arb_q = (s->u_reg_u_ctrl_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_qs = (s->u_reg_u_ctrl_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_qs = (s->u_reg_u_ctrl_qs) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_ds = (((s->u_reg_u_ctrl_wr_en) ? (s->u_reg_u_ctrl_wr_data) : (s->u_reg_u_ctrl_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_we = (s->u_reg_intr_enable0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_rst_ni = (s->u_reg_u_intr_enable0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_we = (s->u_reg_u_intr_enable0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_wd = (s->u_reg_u_intr_enable0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_de = (s->u_reg_u_intr_enable0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_d = (s->u_reg_u_intr_enable0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_wd = (s->u_reg_u_intr_enable0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_d = (s->u_reg_u_intr_enable0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_wr_en = (((s->u_reg_u_intr_enable0_wr_en_data_arb_we) | (s->u_reg_u_intr_enable0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en = (s->u_reg_u_intr_enable0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_wr_data = (((s->u_reg_u_intr_enable0_wr_en_data_arb_we) ? (s->u_reg_u_intr_enable0_wr_en_data_arb_wd) : (s->u_reg_u_intr_enable0_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_data = (s->u_reg_u_intr_enable0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_qe = (s->u_reg_u_intr_enable0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_q = (s->u_reg_u_intr_enable0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_enable0_0__q = (s->u_reg_u_intr_enable0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_qs = (s->u_reg_u_intr_enable0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_wr_en_data_arb_q = (s->u_reg_u_intr_enable0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_qs = (s->u_reg_u_intr_enable0_qs) & ((1ULL << 1) - 1);
    s->u_reg_intr_enable0_qs = (s->u_reg_u_intr_enable0_qs) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable0_ds = (((s->u_reg_u_intr_enable0_wr_en) ? (s->u_reg_u_intr_enable0_wr_data) : (s->u_reg_u_intr_enable0_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_we = (s->u_reg_intr_state0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_rst_ni = (s->u_reg_u_intr_state0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_we = (s->u_reg_u_intr_state0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_wd = (s->u_reg_u_intr_state0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_wd = (s->u_reg_u_intr_state0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_qe = (s->u_reg_u_intr_state0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_q = (s->u_reg_u_intr_state0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_state0_0__q = (s->u_reg_u_intr_state0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_qs = (s->u_reg_u_intr_state0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_q = (s->u_reg_u_intr_state0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_q = (s->u_reg_u_intr_state0_wr_en_data_arb_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_qs = (s->u_reg_u_intr_state0_qs) & ((1ULL << 1) - 1);
    s->u_reg_intr_state0_qs = (s->u_reg_u_intr_state0_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))) || ((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1)))) || ((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1)))) && ((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1)))) || (((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) || ((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && ((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1)))))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | ((((((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))) ? (s->u_reg_intr_state0_qs) : ((((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))) ? (s->u_reg_intr_enable0_qs) : (s->u_reg_ctrl_qs))))) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_intr_test0_re = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_we = (s->u_reg_intr_test0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_qe = (s->u_reg_u_intr_test0_we) & ((1ULL << 1) - 1);
    s->u_reg_intr_test0_flds_we = (s->u_reg_u_intr_test0_qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test0_0__qe = (s->u_reg_intr_test0_flds_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_qre = (s->u_reg_u_intr_test0_re) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_q = (s->u_reg_u_intr_test0_wd) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test0_0__q = (s->u_reg_u_intr_test0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_ds = (s->u_reg_u_intr_test0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test0_qs = (s->u_reg_u_intr_test0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_we = (s->u_reg_cfg0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0xFFF)) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_d = (0) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_rst_ni = (s->u_reg_u_cfg0_prescale_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_we = (s->u_reg_u_cfg0_prescale_we) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd = (s->u_reg_u_cfg0_prescale_wd) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_de = (s->u_reg_u_cfg0_prescale_de) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_d = (s->u_reg_u_cfg0_prescale_d) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd = (s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_d = (s->u_reg_u_cfg0_prescale_wr_en_data_arb_d) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_en = (((s->u_reg_u_cfg0_prescale_wr_en_data_arb_we) | (s->u_reg_u_cfg0_prescale_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_wr_en = (s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_data = (((s->u_reg_u_cfg0_prescale_wr_en_data_arb_we) ? (s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd) : (s->u_reg_u_cfg0_prescale_wr_en_data_arb_d))) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_wr_data = (s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_data) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_qe = (s->u_reg_u_cfg0_prescale_we) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_prescale_q = (s->u_reg_u_cfg0_prescale_q) & ((1ULL << 12) - 1);
    s->u_reg_reg2hw_cfg0_prescale_q = (s->u_reg_u_cfg0_prescale_q) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_qs = (s->u_reg_u_cfg0_prescale_q) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_q = (s->u_reg_u_cfg0_prescale_q) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_prescale_qs = (s->u_reg_u_cfg0_prescale_qs) & ((1ULL << 12) - 1);
    s->u_reg_cfg0_prescale_qs = (s->u_reg_u_cfg0_prescale_qs) & ((1ULL << 12) - 1);
    s->u_reg_reg_rdata_next = (((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0xFFFULL) | (((s->u_reg_cfg0_prescale_qs) & 0xFFFULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_cfg0_prescale_ds = (((s->u_reg_u_cfg0_prescale_wr_en) ? (s->u_reg_u_cfg0_prescale_wr_data) : (s->u_reg_u_cfg0_prescale_qs))) & ((1ULL << 12) - 1);
    s->u_reg_u_cfg0_step_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_we = (s->u_reg_cfg0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_wd = (((s->u_reg_u_reg_if_wdata_o) >> 16) & 0xFF);
    s->u_reg_u_cfg0_step_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_d = 0;
    s->u_reg_u_cfg0_step_rst_ni = (s->u_reg_u_cfg0_step_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_wr_en_data_arb_we = (s->u_reg_u_cfg0_step_we) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_wr_en_data_arb_wd = s->u_reg_u_cfg0_step_wd;
    s->u_reg_u_cfg0_step_wr_en_data_arb_de = (s->u_reg_u_cfg0_step_de) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_wr_en_data_arb_d = s->u_reg_u_cfg0_step_d;
    s->u_reg_u_cfg0_step_wr_en_data_arb_wd = s->u_reg_u_cfg0_step_wr_en_data_arb_wd;
    s->u_reg_u_cfg0_step_wr_en_data_arb_d = s->u_reg_u_cfg0_step_wr_en_data_arb_d;
    s->u_reg_u_cfg0_step_wr_en_data_arb_wr_en = (((s->u_reg_u_cfg0_step_wr_en_data_arb_we) | (s->u_reg_u_cfg0_step_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_wr_en = (s->u_reg_u_cfg0_step_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_wr_en_data_arb_wr_data = ((s->u_reg_u_cfg0_step_wr_en_data_arb_we) ? (s->u_reg_u_cfg0_step_wr_en_data_arb_wd) : (s->u_reg_u_cfg0_step_wr_en_data_arb_d));
    s->u_reg_u_cfg0_step_wr_data = s->u_reg_u_cfg0_step_wr_en_data_arb_wr_data;
    s->u_reg_u_cfg0_step_qe = (s->u_reg_u_cfg0_step_we) & ((1ULL << 1) - 1);
    s->u_reg_u_cfg0_step_q = s->u_reg_u_cfg0_step_q;
    s->u_reg_reg2hw_cfg0_step_q = s->u_reg_u_cfg0_step_q;
    s->u_reg_u_cfg0_step_qs = s->u_reg_u_cfg0_step_q;
    s->u_reg_u_cfg0_step_wr_en_data_arb_q = s->u_reg_u_cfg0_step_q;
    s->u_reg_u_cfg0_step_qs = s->u_reg_u_cfg0_step_qs;
    s->u_reg_cfg0_step_qs = s->u_reg_u_cfg0_step_qs;
    s->u_reg_reg_rdata_next = (((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0xFF0000ULL) | (((s->u_reg_cfg0_step_qs) & 0xFFULL) << 16)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_cfg0_step_ds = ((s->u_reg_u_cfg0_step_wr_en) ? (s->u_reg_u_cfg0_step_wr_data) : (s->u_reg_u_cfg0_step_qs));
    s->u_reg_u_timer_v_lower0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_we = (s->u_reg_timer_v_lower0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_timer_v_lower0_rst_ni = (s->u_reg_u_timer_v_lower0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_we = (s->u_reg_u_timer_v_lower0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd = s->u_reg_u_timer_v_lower0_wd;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd = s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd;
    s->u_reg_u_timer_v_lower0_qe = (s->u_reg_u_timer_v_lower0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_q = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_reg2hw_timer_v_lower0_q = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_u_timer_v_lower0_qs = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_q = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_u_timer_v_lower0_qs = s->u_reg_u_timer_v_lower0_qs;
    s->u_reg_timer_v_lower0_qs = s->u_reg_u_timer_v_lower0_qs;
    s->u_reg_u_timer_v_upper0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_we = (s->u_reg_timer_v_upper0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_timer_v_upper0_rst_ni = (s->u_reg_u_timer_v_upper0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_we = (s->u_reg_u_timer_v_upper0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd = s->u_reg_u_timer_v_upper0_wd;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd = s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd;
    s->u_reg_u_timer_v_upper0_qe = (s->u_reg_u_timer_v_upper0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_q = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_reg2hw_timer_v_upper0_q = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_u_timer_v_upper0_qs = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_q = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_u_timer_v_upper0_qs = s->u_reg_u_timer_v_upper0_qs;
    s->u_reg_timer_v_upper0_qs = s->u_reg_u_timer_v_upper0_qs;
    s->u_reg_u_compare_lower0_00_qe_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_00_qe_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_00_qe_rst_ni = (s->u_reg_u_compare_lower0_00_qe_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_00_qe_q_o = (s->u_reg_u_compare_lower0_00_qe_q_o) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_compare_lower0_0_qe = (s->u_reg_u_compare_lower0_00_qe_q_o) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_we = (s->u_reg_compare_lower0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_compare_lower0_0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_d = 0;
    s->u_reg_u_compare_lower0_0_rst_ni = (s->u_reg_u_compare_lower0_0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_we = (s->u_reg_u_compare_lower0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd = s->u_reg_u_compare_lower0_0_wd;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_de = (s->u_reg_u_compare_lower0_0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_d = s->u_reg_u_compare_lower0_0_d;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd = s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_d = s->u_reg_u_compare_lower0_0_wr_en_data_arb_d;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_en = (((s->u_reg_u_compare_lower0_0_wr_en_data_arb_we) | (s->u_reg_u_compare_lower0_0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_wr_en = (s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_data = ((s->u_reg_u_compare_lower0_0_wr_en_data_arb_we) ? (s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd) : (s->u_reg_u_compare_lower0_0_wr_en_data_arb_d));
    s->u_reg_u_compare_lower0_0_wr_data = s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_data;
    s->u_reg_u_compare_lower0_0_qe = (s->u_reg_u_compare_lower0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_compare_lower0_0_flds_we = (s->u_reg_u_compare_lower0_0_qe) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_00_qe_d_i = (s->u_reg_compare_lower0_0_flds_we) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_00_qe_d_i = (s->u_reg_u_compare_lower0_00_qe_d_i) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_lower0_0_q = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_reg2hw_compare_lower0_0_q = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_u_compare_lower0_0_qs = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_q = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_u_compare_lower0_0_qs = s->u_reg_u_compare_lower0_0_qs;
    s->u_reg_compare_lower0_0_qs = s->u_reg_u_compare_lower0_0_qs;
    s->u_reg_u_compare_lower0_0_ds = ((s->u_reg_u_compare_lower0_0_wr_en) ? (s->u_reg_u_compare_lower0_0_wr_data) : (s->u_reg_u_compare_lower0_0_qs));
    s->u_reg_u_compare_upper0_00_qe_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_00_qe_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_00_qe_rst_ni = (s->u_reg_u_compare_upper0_00_qe_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_00_qe_q_o = (s->u_reg_u_compare_upper0_00_qe_q_o) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_compare_upper0_0_qe = (s->u_reg_u_compare_upper0_00_qe_q_o) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_we = (s->u_reg_compare_upper0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_compare_upper0_0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_d = 0;
    s->u_reg_u_compare_upper0_0_rst_ni = (s->u_reg_u_compare_upper0_0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_we = (s->u_reg_u_compare_upper0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd = s->u_reg_u_compare_upper0_0_wd;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_de = (s->u_reg_u_compare_upper0_0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_d = s->u_reg_u_compare_upper0_0_d;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd = s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_d = s->u_reg_u_compare_upper0_0_wr_en_data_arb_d;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_en = (((s->u_reg_u_compare_upper0_0_wr_en_data_arb_we) | (s->u_reg_u_compare_upper0_0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_wr_en = (s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_data = ((s->u_reg_u_compare_upper0_0_wr_en_data_arb_we) ? (s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd) : (s->u_reg_u_compare_upper0_0_wr_en_data_arb_d));
    s->u_reg_u_compare_upper0_0_wr_data = s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_data;
    s->u_reg_u_compare_upper0_0_qe = (s->u_reg_u_compare_upper0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_compare_upper0_0_flds_we = (s->u_reg_u_compare_upper0_0_qe) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_00_qe_d_i = (s->u_reg_compare_upper0_0_flds_we) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_00_qe_d_i = (s->u_reg_u_compare_upper0_00_qe_d_i) & ((1ULL << 1) - 1);
    s->u_reg_u_compare_upper0_0_q = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_reg2hw_compare_upper0_0_q = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_u_compare_upper0_0_qs = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_q = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_u_compare_upper0_0_qs = s->u_reg_u_compare_upper0_0_qs;
    s->u_reg_compare_upper0_0_qs = s->u_reg_u_compare_upper0_0_qs;
    s->u_reg_reg_rdata_next = (((((((((((s->u_reg_racl_addr_hit_read) >> 9) & 0x1)) == (1))) || ((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1)))) || ((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1)))) || ((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1)))) && (((((((((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 9) & 0x1)) == (1)))) || (((((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1))))) || ((((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1))))) || (((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1)))))) ? ((((((((s->u_reg_racl_addr_hit_read) >> 9) & 0x1)) == (1))) ? (s->u_reg_compare_upper0_0_qs) : ((((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1))) ? (s->u_reg_compare_lower0_0_qs) : ((((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1))) ? (s->u_reg_timer_v_upper0_qs) : (s->u_reg_timer_v_lower0_qs))))))) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((((((((!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1)))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 9) & 0x1)) == (1))))) ? (-1) : s->u_reg_reg_rdata_next);
    s->u_reg_u_reg_if_rdata_i = s->u_reg_reg_rdata_next;
    s->u_reg_u_reg_if_rdata_i = s->u_reg_u_reg_if_rdata_i;
    s->u_reg_u_compare_upper0_0_ds = ((s->u_reg_u_compare_upper0_0_wr_en) ? (s->u_reg_u_compare_upper0_0_wr_data) : (s->u_reg_u_compare_upper0_0_qs));
    s->u_reg_tl_o_d_valid = (s->u_reg_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_tl_o_d_opcode = (s->u_reg_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_tl_o_d_param = (s->u_reg_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_tl_o_d_size = (s->u_reg_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_tl_o_d_source = s->u_reg_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_tl_o_d_sink = (s->u_reg_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_tl_o_d_data = s->u_reg_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_tl_o_d_user_rsp_intg = (s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_tl_o_d_user_data_intg = (s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_tl_o_d_error = (s->u_reg_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_tl_o_a_ready = (s->u_reg_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_alert_test_q = (s->u_reg_reg2hw_alert_test_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_alert_test_qe = (s->u_reg_reg2hw_alert_test_qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_0__q = (((((uint64_t)(s->u_reg_reg2hw_ctrl_0__q)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->active = (s->u_reg_reg2hw_ctrl_0__q) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_core_active = (s->active) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_core_active = (s->gen_harts_0_u_core_active) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_enable0_0__q = (((((uint64_t)(s->u_reg_reg2hw_intr_enable0_0__q)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i = (s->u_reg_reg2hw_intr_enable0_0__q) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i = (s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_state0_0__q = (((((uint64_t)(s->u_reg_reg2hw_intr_state0_0__q)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_reg2hw_intr_state_q_i = (s->u_reg_reg2hw_intr_state0_0__q) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_status = (s->gen_harts_0_u_intr_hw_reg2hw_intr_state_q_i) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test0_0__q = (((((uint64_t)(s->u_reg_reg2hw_intr_test0_0__q)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_reg2hw_intr_test_q_i = (s->u_reg_reg2hw_intr_test0_0__q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test0_0__qe = (((((uint64_t)(s->u_reg_reg2hw_intr_test0_0__qe)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_reg2hw_intr_test_qe_i = (s->u_reg_reg2hw_intr_test0_0__qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_cfg0_step_q = s->u_reg_reg2hw_cfg0_step_q;
    s->gen_harts_0_u_core_step = s->u_reg_reg2hw_cfg0_step_q;
    s->u_reg_reg2hw_cfg0_prescale_q = (s->u_reg_reg2hw_cfg0_prescale_q) & ((1ULL << 12) - 1);
    s->gen_harts_0_u_core_prescaler = (s->u_reg_reg2hw_cfg0_prescale_q) & ((1ULL << 12) - 1);
    s->gen_harts_0_u_core_prescaler = (s->gen_harts_0_u_core_prescaler) & ((1ULL << 12) - 1);
    s->gen_harts_0_u_core_tick = (((s->gen_harts_0_u_core_active) & (((s->gen_harts_0_u_core_tick_count) >= (s->gen_harts_0_u_core_prescaler))))) & ((1ULL << 1) - 1);
    s->tick = (s->gen_harts_0_u_core_tick) & ((1ULL << 1) - 1);
    s->hw2reg_timer_v_upper0_de = (s->tick) & ((1ULL << 1) - 1);
    s->hw2reg_timer_v_lower0_de = (s->tick) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_timer_v_lower0_de = (s->hw2reg_timer_v_lower0_de) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_timer_v_upper0_de = (s->hw2reg_timer_v_upper0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_de = (s->u_reg_hw2reg_timer_v_lower0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_de = (s->u_reg_u_timer_v_lower0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_en = (((s->u_reg_u_timer_v_lower0_wr_en_data_arb_we) | (s->u_reg_u_timer_v_lower0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_lower0_wr_en = (s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_de = (s->u_reg_hw2reg_timer_v_upper0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_de = (s->u_reg_u_timer_v_upper0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_en = (((s->u_reg_u_timer_v_upper0_wr_en_data_arb_we) | (s->u_reg_u_timer_v_upper0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_timer_v_upper0_wr_en = (s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_timer_v_lower0_q = s->u_reg_reg2hw_timer_v_lower0_q;
    s->u_reg_reg2hw_timer_v_upper0_q = s->u_reg_reg2hw_timer_v_upper0_q;
    s->mtime_0_ = ((((uint64_t)(s->u_reg_reg2hw_timer_v_upper0_q)) << 32) | ((uint64_t)(s->u_reg_reg2hw_timer_v_lower0_q)));
    s->gen_harts_0_u_core_mtime = s->mtime_0_;
    s->gen_harts_0_u_core_mtime_d = ((s->gen_harts_0_u_core_mtime) + ((s->_qp_pump) ? (((((uint64_t)(s->gen_harts_0_u_core_step)) << 0) | (((uint64_t)(0)) << 8))) : 0));
    s->mtime_d_0_ = s->gen_harts_0_u_core_mtime_d;
    s->hw2reg_timer_v_upper0_d = (((s->mtime_d_0_) >> 32) & 0xFFFFFFFFULL);
    s->hw2reg_timer_v_lower0_d = ((s->mtime_d_0_) & 0xFFFFFFFFULL);
    s->u_reg_hw2reg_timer_v_lower0_d = s->hw2reg_timer_v_lower0_d;
    s->u_reg_hw2reg_timer_v_upper0_d = s->hw2reg_timer_v_upper0_d;
    s->u_reg_u_timer_v_lower0_d = s->u_reg_hw2reg_timer_v_lower0_d;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_d = s->u_reg_u_timer_v_lower0_d;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_d = s->u_reg_u_timer_v_lower0_wr_en_data_arb_d;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_data = ((s->u_reg_u_timer_v_lower0_wr_en_data_arb_we) ? (s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd) : (s->u_reg_u_timer_v_lower0_wr_en_data_arb_d));
    s->u_reg_u_timer_v_lower0_wr_data = s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_data;
    s->u_reg_u_timer_v_lower0_ds = ((s->u_reg_u_timer_v_lower0_wr_en) ? (s->u_reg_u_timer_v_lower0_wr_data) : (s->u_reg_u_timer_v_lower0_qs));
    s->u_reg_u_timer_v_upper0_d = s->u_reg_hw2reg_timer_v_upper0_d;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_d = s->u_reg_u_timer_v_upper0_d;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_d = s->u_reg_u_timer_v_upper0_wr_en_data_arb_d;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_data = ((s->u_reg_u_timer_v_upper0_wr_en_data_arb_we) ? (s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd) : (s->u_reg_u_timer_v_upper0_wr_en_data_arb_d));
    s->u_reg_u_timer_v_upper0_wr_data = s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_data;
    s->u_reg_u_timer_v_upper0_ds = ((s->u_reg_u_timer_v_upper0_wr_en) ? (s->u_reg_u_timer_v_upper0_wr_data) : (s->u_reg_u_timer_v_upper0_qs));
    s->u_reg_reg2hw_compare_lower0_0_q = s->u_reg_reg2hw_compare_lower0_0_q;
    s->u_reg_reg2hw_compare_lower0_0_qe = (s->u_reg_reg2hw_compare_lower0_0_qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_compare_upper0_0_q = s->u_reg_reg2hw_compare_upper0_0_q;
    s->gen_harts_0_u_core_mtimecmp_0_ = ((((uint64_t)(s->u_reg_reg2hw_compare_lower0_0_q)) << 0) | (((uint64_t)(s->u_reg_reg2hw_compare_upper0_0_q)) << 32));
    s->gen_harts_0_u_core_intr = ((s->gen_harts_0_u_core_active) & (((s->gen_harts_0_u_core_mtime) >= (s->gen_harts_0_u_core_mtimecmp_0_)))) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_core_intr = (s->gen_harts_0_u_core_intr) & ((1ULL << 1) - 1);
    s->intr_timer_set = (s->gen_harts_0_u_core_intr) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_event_intr_i = (s->intr_timer_set) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_hw2reg_intr_state_de_o = (((((s->gen_harts_0_u_intr_hw_reg2hw_intr_test_qe_i) & (s->gen_harts_0_u_intr_hw_reg2hw_intr_test_q_i))) | (s->gen_harts_0_u_intr_hw_event_intr_i))) & ((1ULL << 1) - 1);
    s->gen_harts_0_u_intr_hw_hw2reg_intr_state_d_o = (((((((s->gen_harts_0_u_intr_hw_reg2hw_intr_test_qe_i) & (s->gen_harts_0_u_intr_hw_reg2hw_intr_test_q_i))) | (s->gen_harts_0_u_intr_hw_event_intr_i))) | (s->gen_harts_0_u_intr_hw_reg2hw_intr_state_q_i))) & ((1ULL << 1) - 1);
    s->intr_timer_state_d = (s->gen_harts_0_u_intr_hw_hw2reg_intr_state_d_o) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_compare_upper0_0_qe = (s->u_reg_reg2hw_compare_upper0_0_qe) & ((1ULL << 1) - 1);
    s->mtimecmp_update_0__0_ = ((s->u_reg_reg2hw_compare_upper0_0_qe) | (s->u_reg_reg2hw_compare_lower0_0_qe)) & ((1ULL << 1) - 1);
    s->hw2reg_intr_state0_0__de = ((s->gen_harts_0_u_intr_hw_hw2reg_intr_state_de_o) | (s->mtimecmp_update_0__0_)) & ((1ULL << 1) - 1);
    s->hw2reg_intr_state0_0__d = ((s->intr_timer_state_d) & (((s->mtimecmp_update_0__0_) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_intr_state0_0__d = (((((uint64_t)(s->hw2reg_intr_state0_0__d)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_intr_state0_0__de = (((((uint64_t)(s->hw2reg_intr_state0_0__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_de = (s->u_reg_hw2reg_intr_state0_0__de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_d = (s->u_reg_hw2reg_intr_state0_0__d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_de = (s->u_reg_u_intr_state0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_d = (s->u_reg_u_intr_state0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_d = (s->u_reg_u_intr_state0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_wr_en = (((s->u_reg_u_intr_state0_wr_en_data_arb_we) | (s->u_reg_u_intr_state0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en = (s->u_reg_u_intr_state0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_en_data_arb_wr_data = (((((s->u_reg_u_intr_state0_wr_en_data_arb_de) ? (s->u_reg_u_intr_state0_wr_en_data_arb_d) : (s->u_reg_u_intr_state0_wr_en_data_arb_q))) & (((((s->u_reg_u_intr_state0_wr_en_data_arb_we) ^ (1))) | (((s->u_reg_u_intr_state0_wr_en_data_arb_wd) ^ (1))))))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_wr_data = (s->u_reg_u_intr_state0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state0_ds = (((s->u_reg_u_intr_state0_wr_en) ? (s->u_reg_u_intr_state0_wr_data) : (s->u_reg_u_intr_state0_qs))) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_valid = (s->u_reg_racl_error_o_valid) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_overflow = (s->u_reg_racl_error_o_overflow) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_racl_role = (s->u_reg_racl_error_o_racl_role) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_ctn_uid = (s->u_reg_racl_error_o_ctn_uid) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_read_access = (s->u_reg_racl_error_o_read_access) & ((1ULL << 1) - 1);
    s->u_reg_racl_error_o_request_address = s->u_reg_racl_error_o_request_address;
    s->u_reg_intg_err_o = (((s->u_reg_err_q) | (s->u_reg_intg_err) | (s->u_reg_reg_we_err))) & ((1ULL << 1) - 1);
    s->alerts = (s->u_reg_intg_err_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_i = (((s->u_reg_reg2hw_alert_test_q) & (s->u_reg_reg2hw_alert_test_qe))) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_req_i = (s->alerts) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_p = (s->alert_rx_i_0__ping_p) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_n = (s->alert_rx_i_0__ping_n) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_p = (s->alert_rx_i_0__ack_p) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_n = (s->alert_rx_i_0__ack_n) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger = ((s->gen_alert_tx_0_u_prim_alert_sender_alert_test_i) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = (s->gen_alert_tx_0_u_prim_alert_sender_state_q) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = (-1) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_clr = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_clr = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_in_i = (((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_n)) << 1))) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_pi = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_ni = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_pi) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_p_edge = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_n_edge = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = ((((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = (((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_n_edge)))) && (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) ? (0) : (1))) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = (((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_n_edge)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) ? (0) : (1)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = ((((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) + (1)) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_event_o = (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o))) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger = ((s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_event_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_in_i = (((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_n)) << 1))) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_pi = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_ni = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_pi) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_p_edge = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_n_edge = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = ((((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = (((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_n_edge)))) && (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) ? (0) : (1))) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = (((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_n_edge)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) ? (0) : (1)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = ((((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) + (1)) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_ack_level = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_clr = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_alert_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_clr = ((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_ping_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_event_o = (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o))) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_clr = (((s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_ping_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_set_d = ((((s->gen_alert_tx_0_u_prim_alert_sender_ping_clr) ^ 1)) & (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_clr = (((s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_d = ((((s->gen_alert_tx_0_u_prim_alert_sender_alert_clr) ^ 1)) & (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_in_i = (s->gen_alert_tx_0_u_prim_alert_sender_alert_req_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_in_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_set_d = ((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q)) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = ((((((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? (((((uint64_t)(0)) << 2) | (((uint64_t)(((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) ^ 1))) << 1) | ((uint64_t)(1)))) : s->gen_alert_tx_0_u_prim_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = ((((((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_alert_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = ((((((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = (((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4))))) || ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) && (((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4))))) || (((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) && (s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) || (((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) ? (((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4))))) ? (((((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (5))) ? (6) : (0))) : ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) ? (4) : (2))))) : s->gen_alert_tx_0_u_prim_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = ((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) || (((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_alert_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = ((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) || (((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = ((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-3) : s->gen_alert_tx_0_u_prim_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = ((((((((!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-3) : s->gen_alert_tx_0_u_prim_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = (((s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = (((s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = (((s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_d_i = (((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_pd)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_nd)) << 1))) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_clk_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_d_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o) & 1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_ack_o = (((s->gen_alert_tx_0_u_prim_alert_sender_alert_clr) & (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q))) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_state_o = (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p = (s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_p = (s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n = (s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_n = (s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n) & ((1ULL << 1) - 1);
    s->tl_o_d_valid = (s->u_reg_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->tl_o_d_opcode = (s->u_reg_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->tl_o_d_param = (s->u_reg_tl_o_d_param) & ((1ULL << 3) - 1);
    s->tl_o_d_size = (s->u_reg_tl_o_d_size) & ((1ULL << 2) - 1);
    s->tl_o_d_source = s->u_reg_tl_o_d_source;
    s->tl_o_d_sink = (s->u_reg_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->tl_o_d_data = s->u_reg_tl_o_d_data;
    s->tl_o_d_user_rsp_intg = (s->u_reg_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->tl_o_d_user_data_intg = (s->u_reg_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->tl_o_d_error = (s->u_reg_tl_o_d_error) & ((1ULL << 1) - 1);
    s->tl_o_a_ready = (s->u_reg_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_p = (((((uint64_t)(s->alert_tx_o_0__alert_p)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_n = (((((uint64_t)(s->alert_tx_o_0__alert_n)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->racl_error_o_valid = (s->u_reg_racl_error_o_valid) & ((1ULL << 1) - 1);
    s->racl_error_o_overflow = (s->u_reg_racl_error_o_overflow) & ((1ULL << 1) - 1);
    s->racl_error_o_racl_role = (s->u_reg_racl_error_o_racl_role) & ((1ULL << 1) - 1);
    s->racl_error_o_ctn_uid = (s->u_reg_racl_error_o_ctn_uid) & ((1ULL << 1) - 1);
    s->racl_error_o_read_access = (s->u_reg_racl_error_o_read_access) & ((1ULL << 1) - 1);
    s->racl_error_o_request_address = s->u_reg_racl_error_o_request_address;
    s->intr_timer_expired_hart0_timer0_o = (s->intr_out) & ((1ULL << 1) - 1);
}

/*
 * tick() - Evaluate and atomically commit one sequential edge.
 *
 * Phase 1 computes every next-state value from the same old state.
 * Phase 2 commits all registers together, matching Verilog NBA
 * semantics. The return value reports whether sequential state changed.
 * ACCUMULATE counters are handled by ptimer instead.
 */
static bool tick(rv_timer_state *s)
{
    bool _qp_changed = false;
    /* Phase 1: snapshot old state into next-state temporaries. */
    uint8_t _qp_next_gen_harts_0_u_intr_hw_intr_o = s->gen_harts_0_u_intr_hw_intr_o;
    uint16_t _qp_next_gen_harts_0_u_core_tick_count = s->gen_harts_0_u_core_tick_count;
    uint8_t _qp_next_u_reg_err_q = s->u_reg_err_q;
    uint8_t _qp_next_u_reg_u_reg_if_outstanding_q = s->u_reg_u_reg_if_outstanding_q;
    uint8_t _qp_next_u_reg_u_reg_if_reqid_q = s->u_reg_u_reg_if_reqid_q;
    uint8_t _qp_next_u_reg_u_reg_if_reqsz_q = s->u_reg_u_reg_if_reqsz_q;
    uint8_t _qp_next_u_reg_u_reg_if_rspop_q = s->u_reg_u_reg_if_rspop_q;
    uint32_t _qp_next_u_reg_u_reg_if_rdata_q = s->u_reg_u_reg_if_rdata_q;
    uint8_t _qp_next_u_reg_u_reg_if_error_q = s->u_reg_u_reg_if_error_q;
    uint8_t _qp_next_u_reg_u_ctrl_q = s->u_reg_u_ctrl_q;
    uint8_t _qp_next_u_reg_u_intr_enable0_q = s->u_reg_u_intr_enable0_q;
    uint8_t _qp_next_u_reg_u_intr_state0_q = s->u_reg_u_intr_state0_q;
    uint16_t _qp_next_u_reg_u_cfg0_prescale_q = s->u_reg_u_cfg0_prescale_q;
    uint8_t _qp_next_u_reg_u_cfg0_step_q = s->u_reg_u_cfg0_step_q;
    uint32_t _qp_next_u_reg_u_timer_v_lower0_q = s->u_reg_u_timer_v_lower0_q;
    uint32_t _qp_next_u_reg_u_timer_v_upper0_q = s->u_reg_u_timer_v_upper0_q;
    uint8_t _qp_next_u_reg_u_compare_lower0_00_qe_q_o = s->u_reg_u_compare_lower0_00_qe_q_o;
    uint32_t _qp_next_u_reg_u_compare_lower0_0_q = s->u_reg_u_compare_lower0_0_q;
    uint8_t _qp_next_u_reg_u_compare_upper0_00_qe_q_o = s->u_reg_u_compare_upper0_00_qe_q_o;
    uint32_t _qp_next_u_reg_u_compare_upper0_0_q = s->u_reg_u_compare_upper0_0_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q = s->gen_alert_tx_0_u_prim_alert_sender_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q = s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q = s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;

    /* Evaluate all next-state expressions from pre-edge state. */
    _qp_next_gen_harts_0_u_intr_hw_intr_o = (((((s->gen_harts_0_u_intr_hw_rst_ni) ^ 1)) ? (0) : _qp_next_gen_harts_0_u_intr_hw_intr_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_harts_0_u_intr_hw_intr_o = (((!(((s->gen_harts_0_u_intr_hw_rst_ni) ^ 1))) ? ((s->gen_harts_0_u_intr_hw_status) & (s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i)) : _qp_next_gen_harts_0_u_intr_hw_intr_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_harts_0_u_core_tick_count = ((((((((!(((s->gen_harts_0_u_core_rst_ni) ^ 1))) && (!(((s->gen_harts_0_u_core_active) ^ 1)))) && (((s->gen_harts_0_u_core_tick_count) == (s->gen_harts_0_u_core_prescaler)))) || ((!(((s->gen_harts_0_u_core_rst_ni) ^ 1))) && (((s->gen_harts_0_u_core_active) ^ 1)))) || (((s->gen_harts_0_u_core_rst_ni) ^ 1))) && (s->_qp_pump)) ? (0) : _qp_next_gen_harts_0_u_core_tick_count)) & ((1ULL << 12) - 1);
    _qp_next_gen_harts_0_u_core_tick_count = ((((((!(((s->gen_harts_0_u_core_rst_ni) ^ 1))) && (!(((s->gen_harts_0_u_core_active) ^ 1)))) && (!(((s->gen_harts_0_u_core_tick_count) == (s->gen_harts_0_u_core_prescaler))))) && (s->_qp_pump)) ? (s->gen_harts_0_u_core_tick_count + 1) : _qp_next_gen_harts_0_u_core_tick_count)) & ((1ULL << 12) - 1);
    _qp_next_u_reg_err_q = ((((((!(((s->u_reg_rst_ni) ^ 1))) && ((s->u_reg_intg_err) | (s->u_reg_reg_we_err))) || (((s->u_reg_rst_ni) ^ 1))) && (((!(((s->u_reg_rst_ni) ^ 1))) && ((s->u_reg_intg_err) | (s->u_reg_reg_we_err))) || (((s->u_reg_rst_ni) ^ 1)))) ? ((((!(((s->u_reg_rst_ni) ^ 1))) && ((s->u_reg_intg_err) | (s->u_reg_reg_we_err))) ? (1) : (0))) : _qp_next_u_reg_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_reg_if_outstanding_q = ((((((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (!(s->u_reg_u_reg_if_a_ack))) && (s->u_reg_u_reg_if_d_ack)) || ((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack))) || (((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (!(s->u_reg_u_reg_if_a_ack))) && (s->u_reg_u_reg_if_d_ack)) || ((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack))) || (((s->u_reg_u_reg_if_rst_ni) ^ 1)))) ? ((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (1) : (0))) : _qp_next_u_reg_u_reg_if_outstanding_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_reg_if_reqid_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_reqid_q);
    _qp_next_u_reg_u_reg_if_reqsz_q = (((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_reqsz_q)) & ((1ULL << 2) - 1);
    _qp_next_u_reg_u_reg_if_rspop_q = (((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_rspop_q)) & ((1ULL << 3) - 1);
    _qp_next_u_reg_u_reg_if_reqid_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (s->u_reg_u_reg_if_tl_i_a_source) : _qp_next_u_reg_u_reg_if_reqid_q);
    _qp_next_u_reg_u_reg_if_reqsz_q = ((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (s->u_reg_u_reg_if_tl_i_a_size) : _qp_next_u_reg_u_reg_if_reqsz_q)) & ((1ULL << 2) - 1);
    _qp_next_u_reg_u_reg_if_rspop_q = ((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (((((uint64_t)(0)) << 1) | ((uint64_t)(s->u_reg_u_reg_if_rd_req)))) : _qp_next_u_reg_u_reg_if_rspop_q)) & ((1ULL << 3) - 1);
    _qp_next_u_reg_u_reg_if_rdata_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_rdata_q);
    _qp_next_u_reg_u_reg_if_error_q = (((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_reg_if_rdata_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((((s->u_reg_u_reg_if_error_i) | (s->u_reg_u_reg_if_err_internal) | (s->u_reg_u_reg_if_wr_req)) ? (4294967295ULL) : (s->u_reg_u_reg_if_rdata_i))) : _qp_next_u_reg_u_reg_if_rdata_q);
    _qp_next_u_reg_u_reg_if_error_q = ((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((s->u_reg_u_reg_if_error_i) | (s->u_reg_u_reg_if_err_internal)) : _qp_next_u_reg_u_reg_if_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_q = (((((s->u_reg_u_ctrl_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_q = ((((!(((s->u_reg_u_ctrl_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_wr_en)) ? (s->u_reg_u_ctrl_wr_data) : _qp_next_u_reg_u_ctrl_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_enable0_q = (((((s->u_reg_u_intr_enable0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_enable0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_enable0_q = ((((!(((s->u_reg_u_intr_enable0_rst_ni) ^ 1))) && (s->u_reg_u_intr_enable0_wr_en)) ? (s->u_reg_u_intr_enable0_wr_data) : _qp_next_u_reg_u_intr_enable0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_state0_q = (((((s->u_reg_u_intr_state0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_state0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_state0_q = ((((!(((s->u_reg_u_intr_state0_rst_ni) ^ 1))) && (s->u_reg_u_intr_state0_wr_en)) ? (s->u_reg_u_intr_state0_wr_data) : _qp_next_u_reg_u_intr_state0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_cfg0_prescale_q = (((((s->u_reg_u_cfg0_prescale_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_cfg0_prescale_q)) & ((1ULL << 12) - 1);
    _qp_next_u_reg_u_cfg0_prescale_q = ((((!(((s->u_reg_u_cfg0_prescale_rst_ni) ^ 1))) && (s->u_reg_u_cfg0_prescale_wr_en)) ? (s->u_reg_u_cfg0_prescale_wr_data) : _qp_next_u_reg_u_cfg0_prescale_q)) & ((1ULL << 12) - 1);
    _qp_next_u_reg_u_cfg0_step_q = ((((s->u_reg_u_cfg0_step_rst_ni) ^ 1)) ? (1) : _qp_next_u_reg_u_cfg0_step_q);
    _qp_next_u_reg_u_cfg0_step_q = (((!(((s->u_reg_u_cfg0_step_rst_ni) ^ 1))) && (s->u_reg_u_cfg0_step_wr_en)) ? (s->u_reg_u_cfg0_step_wr_data) : _qp_next_u_reg_u_cfg0_step_q);
    _qp_next_u_reg_u_timer_v_lower0_q = ((((s->u_reg_u_timer_v_lower0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_timer_v_lower0_q);
    _qp_next_u_reg_u_timer_v_lower0_q = (((!(((s->u_reg_u_timer_v_lower0_rst_ni) ^ 1))) && (s->u_reg_u_timer_v_lower0_wr_en)) ? (s->u_reg_u_timer_v_lower0_wr_data) : _qp_next_u_reg_u_timer_v_lower0_q);
    _qp_next_u_reg_u_timer_v_upper0_q = ((((s->u_reg_u_timer_v_upper0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_timer_v_upper0_q);
    _qp_next_u_reg_u_timer_v_upper0_q = (((!(((s->u_reg_u_timer_v_upper0_rst_ni) ^ 1))) && (s->u_reg_u_timer_v_upper0_wr_en)) ? (s->u_reg_u_timer_v_upper0_wr_data) : _qp_next_u_reg_u_timer_v_upper0_q);
    _qp_next_u_reg_u_compare_lower0_00_qe_q_o = (((((s->u_reg_u_compare_lower0_00_qe_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_compare_lower0_00_qe_q_o)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_compare_lower0_00_qe_q_o = (((!(((s->u_reg_u_compare_lower0_00_qe_rst_ni) ^ 1))) ? (s->u_reg_u_compare_lower0_00_qe_d_i) : _qp_next_u_reg_u_compare_lower0_00_qe_q_o)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_compare_lower0_0_q = ((((s->u_reg_u_compare_lower0_0_rst_ni) ^ 1)) ? (-1) : _qp_next_u_reg_u_compare_lower0_0_q);
    _qp_next_u_reg_u_compare_lower0_0_q = (((!(((s->u_reg_u_compare_lower0_0_rst_ni) ^ 1))) && (s->u_reg_u_compare_lower0_0_wr_en)) ? (s->u_reg_u_compare_lower0_0_wr_data) : _qp_next_u_reg_u_compare_lower0_0_q);
    _qp_next_u_reg_u_compare_upper0_00_qe_q_o = (((((s->u_reg_u_compare_upper0_00_qe_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_compare_upper0_00_qe_q_o)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_compare_upper0_00_qe_q_o = (((!(((s->u_reg_u_compare_upper0_00_qe_rst_ni) ^ 1))) ? (s->u_reg_u_compare_upper0_00_qe_d_i) : _qp_next_u_reg_u_compare_upper0_00_qe_q_o)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_compare_upper0_0_q = ((((s->u_reg_u_compare_upper0_0_rst_ni) ^ 1)) ? (-1) : _qp_next_u_reg_u_compare_upper0_0_q);
    _qp_next_u_reg_u_compare_upper0_0_q = (((!(((s->u_reg_u_compare_upper0_0_rst_ni) ^ 1))) && (s->u_reg_u_compare_upper0_0_wr_en)) ? (s->u_reg_u_compare_upper0_0_wr_data) : _qp_next_u_reg_u_compare_upper0_0_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q)) & ((1ULL << 3) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_state_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q)) & ((1ULL << 3) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_ping_set_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = (((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1)) ? (-2) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = (((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o)) & ((1ULL << 2) - 1);

    /* Detect changes before committing ordinary registers. */
    _qp_changed |= _qp_next_gen_harts_0_u_intr_hw_intr_o != s->gen_harts_0_u_intr_hw_intr_o;
    _qp_changed |= _qp_next_gen_harts_0_u_core_tick_count != s->gen_harts_0_u_core_tick_count;
    _qp_changed |= _qp_next_u_reg_err_q != s->u_reg_err_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_outstanding_q != s->u_reg_u_reg_if_outstanding_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_reqid_q != s->u_reg_u_reg_if_reqid_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_reqsz_q != s->u_reg_u_reg_if_reqsz_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_rspop_q != s->u_reg_u_reg_if_rspop_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_rdata_q != s->u_reg_u_reg_if_rdata_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_error_q != s->u_reg_u_reg_if_error_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_q != s->u_reg_u_ctrl_q;
    _qp_changed |= _qp_next_u_reg_u_intr_enable0_q != s->u_reg_u_intr_enable0_q;
    _qp_changed |= _qp_next_u_reg_u_intr_state0_q != s->u_reg_u_intr_state0_q;
    _qp_changed |= _qp_next_u_reg_u_cfg0_prescale_q != s->u_reg_u_cfg0_prescale_q;
    _qp_changed |= _qp_next_u_reg_u_cfg0_step_q != s->u_reg_u_cfg0_step_q;
    _qp_changed |= _qp_next_u_reg_u_timer_v_lower0_q != s->u_reg_u_timer_v_lower0_q;
    _qp_changed |= _qp_next_u_reg_u_timer_v_upper0_q != s->u_reg_u_timer_v_upper0_q;
    _qp_changed |= _qp_next_u_reg_u_compare_lower0_00_qe_q_o != s->u_reg_u_compare_lower0_00_qe_q_o;
    _qp_changed |= _qp_next_u_reg_u_compare_lower0_0_q != s->u_reg_u_compare_lower0_0_q;
    _qp_changed |= _qp_next_u_reg_u_compare_upper0_00_qe_q_o != s->u_reg_u_compare_upper0_00_qe_q_o;
    _qp_changed |= _qp_next_u_reg_u_compare_upper0_0_q != s->u_reg_u_compare_upper0_0_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q != s->gen_alert_tx_0_u_prim_alert_sender_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q != s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q != s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q != s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;

    /* Phase 2: commit all ordinary registers simultaneously. */
    s->gen_harts_0_u_intr_hw_intr_o = _qp_next_gen_harts_0_u_intr_hw_intr_o;
    s->gen_harts_0_u_core_tick_count = _qp_next_gen_harts_0_u_core_tick_count;
    s->u_reg_err_q = _qp_next_u_reg_err_q;
    s->u_reg_u_reg_if_outstanding_q = _qp_next_u_reg_u_reg_if_outstanding_q;
    s->u_reg_u_reg_if_reqid_q = _qp_next_u_reg_u_reg_if_reqid_q;
    s->u_reg_u_reg_if_reqsz_q = _qp_next_u_reg_u_reg_if_reqsz_q;
    s->u_reg_u_reg_if_rspop_q = _qp_next_u_reg_u_reg_if_rspop_q;
    s->u_reg_u_reg_if_rdata_q = _qp_next_u_reg_u_reg_if_rdata_q;
    s->u_reg_u_reg_if_error_q = _qp_next_u_reg_u_reg_if_error_q;
    s->u_reg_u_ctrl_q = _qp_next_u_reg_u_ctrl_q;
    s->u_reg_u_intr_enable0_q = _qp_next_u_reg_u_intr_enable0_q;
    s->u_reg_u_intr_state0_q = _qp_next_u_reg_u_intr_state0_q;
    s->u_reg_u_cfg0_prescale_q = _qp_next_u_reg_u_cfg0_prescale_q;
    s->u_reg_u_cfg0_step_q = _qp_next_u_reg_u_cfg0_step_q;
    s->u_reg_u_timer_v_lower0_q = _qp_next_u_reg_u_timer_v_lower0_q;
    s->u_reg_u_timer_v_upper0_q = _qp_next_u_reg_u_timer_v_upper0_q;
    s->u_reg_u_compare_lower0_00_qe_q_o = _qp_next_u_reg_u_compare_lower0_00_qe_q_o;
    s->u_reg_u_compare_lower0_0_q = _qp_next_u_reg_u_compare_lower0_0_q;
    s->u_reg_u_compare_upper0_00_qe_q_o = _qp_next_u_reg_u_compare_upper0_00_qe_q_o;
    s->u_reg_u_compare_upper0_0_q = _qp_next_u_reg_u_compare_upper0_0_q;
    s->gen_alert_tx_0_u_prim_alert_sender_state_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;

    return _qp_changed;
}

void rv_timer_advance_tick(rv_timer_state *s)
{
    s->_qp_pump = 1;
    update_state(s);
    qp_tick(s);
    s->_qp_pump = 0;
    {
    if (!s->_qp_busy) {
        s->_qp_busy = 1;
        unsigned _qp_ticks = 0;
        update_state(s);
        QPSettleFingerprint _qp_base = qp_settle_fingerprint(s);
        unsigned _qp_lam = 0, _qp_pow = 1;
        bool _qp_ext = false;
        unsigned _qp_cap = 256u;
        while (_qp_ticks < _qp_cap) {
            bool _qp_ch = qp_tick(s);
            bool _qp_rw = s->_qp_rewound != 0;
            if (_qp_rw) { s->_qp_rewound = 0; _qp_ch = true; }
            if (!_qp_ch && !_qp_ext)
                break;  /* sequential fixed point reached */
            ++_qp_ticks;
            update_state(s);
            /* settle hook AFTER tick+update: the machine co-steps
             * the OTHER ring members (this model is _qp_busy and is
             * skipped there) against this model's FRESH post-tick
             * outputs — the same phase as a pump costep, so a
             * one-clock request pulse is seen exactly once (the
             * before-tick placement let a pulse live two partner
             * ticks: a boot INS was delivered twice).  Nonzero
             * return = cross-model transaction in flight: keeps the
             * loop alive past a local fixed point. */
            int _qp_hk = s->_qp_settle_hook ?
                         s->_qp_settle_hook(s->_qp_settle_hook_ctx) : 0;
            /* muzzled: the machine still co-steps (the ring must not
             * stall), but its verdict can no longer widen the budget. */
            _qp_ext = _qp_hk && !s->_qp_hook_muzzle;
            /* re-mirror after the hook wired fresh partner outputs
             * onto this model's inputs: without this, the next tick
             * still sees pre-hook mirrors and a one-clock handshake
             * (req/ready) takes one beat too long — the boot INS
             * was pushed twice.  Costs only while cross-model
             * traffic is live. */
            if (_qp_ext) {
                update_state(s);
            }
            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            if (_qp_rw) {  /* deliberate repeat: move the camera here */
                _qp_base = _qp_now; _qp_lam = 0; _qp_pow = 1;
                continue;
            }
            ++_qp_lam;
            if (_qp_now.first == _qp_base.first &&
                _qp_now.second == _qp_base.second &&
                !s->_qp_hold_settle && !_qp_ext)
                break;  /* state revisited: periodic, no fixed point exists */
            if (_qp_lam == _qp_pow) {  /* Brent: move camera, double the wait */
                _qp_base = _qp_now; _qp_lam = 0;
                if (_qp_pow < (1u << 30)) _qp_pow <<= 1;
            }
            _qp_cap = s->_qp_hold_settle ? QP_SETTLE_BUDGET
                    : (_qp_ext ? QP_EXT_BUDGET : 256u);
        }
        s->_qp_last_ticks = (uint16_t)(_qp_ticks > 0xFFFFu ? 0xFFFFu : _qp_ticks);
        if (_qp_ticks >= _qp_cap) {
            s->_qp_budget_hit = 1;
            if (_qp_cap == QP_EXT_BUDGET) {
                if (s->_qp_ext_strikes < 255u) s->_qp_ext_strikes++;
                if (s->_qp_ext_strikes >= QP_EXT_STRIKES && !s->_qp_hook_muzzle) {
                    s->_qp_hook_muzzle = 1;
                    qemu_log_mask(LOG_GUEST_ERROR,
                        "qp settle: hook MUZZLED after %u capped extended settles; "
                        "co-stepping continues, budget reverts to the 256 tier\n",
                        (unsigned)QP_EXT_STRIKES);
                }
            }
            qemu_log_mask(LOG_UNIMP, "qp settle: budget %u exhausted, state still changing\n", _qp_cap);
        } else if (_qp_cap == QP_EXT_BUDGET) {
            s->_qp_ext_strikes = 0;  /* an extended settle that CONVERGED */
        }
        s->_qp_busy = 0;
    }
    }
    update_state(s);

}

/* Settle to quiescence (no bus access). */
void rv_timer_settle(rv_timer_state *s)
{
    {
    if (!s->_qp_busy) {
        s->_qp_busy = 1;
        unsigned _qp_ticks = 0;
        update_state(s);
        QPSettleFingerprint _qp_base = qp_settle_fingerprint(s);
        unsigned _qp_lam = 0, _qp_pow = 1;
        bool _qp_ext = false;
        unsigned _qp_cap = 256u;
        while (_qp_ticks < _qp_cap) {
            bool _qp_ch = qp_tick(s);
            bool _qp_rw = s->_qp_rewound != 0;
            if (_qp_rw) { s->_qp_rewound = 0; _qp_ch = true; }
            if (!_qp_ch && !_qp_ext)
                break;  /* sequential fixed point reached */
            ++_qp_ticks;
            update_state(s);
            /* settle hook AFTER tick+update: the machine co-steps
             * the OTHER ring members (this model is _qp_busy and is
             * skipped there) against this model's FRESH post-tick
             * outputs — the same phase as a pump costep, so a
             * one-clock request pulse is seen exactly once (the
             * before-tick placement let a pulse live two partner
             * ticks: a boot INS was delivered twice).  Nonzero
             * return = cross-model transaction in flight: keeps the
             * loop alive past a local fixed point. */
            int _qp_hk = s->_qp_settle_hook ?
                         s->_qp_settle_hook(s->_qp_settle_hook_ctx) : 0;
            /* muzzled: the machine still co-steps (the ring must not
             * stall), but its verdict can no longer widen the budget. */
            _qp_ext = _qp_hk && !s->_qp_hook_muzzle;
            /* re-mirror after the hook wired fresh partner outputs
             * onto this model's inputs: without this, the next tick
             * still sees pre-hook mirrors and a one-clock handshake
             * (req/ready) takes one beat too long — the boot INS
             * was pushed twice.  Costs only while cross-model
             * traffic is live. */
            if (_qp_ext) {
                update_state(s);
            }
            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            if (_qp_rw) {  /* deliberate repeat: move the camera here */
                _qp_base = _qp_now; _qp_lam = 0; _qp_pow = 1;
                continue;
            }
            ++_qp_lam;
            if (_qp_now.first == _qp_base.first &&
                _qp_now.second == _qp_base.second &&
                !s->_qp_hold_settle && !_qp_ext)
                break;  /* state revisited: periodic, no fixed point exists */
            if (_qp_lam == _qp_pow) {  /* Brent: move camera, double the wait */
                _qp_base = _qp_now; _qp_lam = 0;
                if (_qp_pow < (1u << 30)) _qp_pow <<= 1;
            }
            _qp_cap = s->_qp_hold_settle ? QP_SETTLE_BUDGET
                    : (_qp_ext ? QP_EXT_BUDGET : 256u);
        }
        s->_qp_last_ticks = (uint16_t)(_qp_ticks > 0xFFFFu ? 0xFFFFu : _qp_ticks);
        if (_qp_ticks >= _qp_cap) {
            s->_qp_budget_hit = 1;
            if (_qp_cap == QP_EXT_BUDGET) {
                if (s->_qp_ext_strikes < 255u) s->_qp_ext_strikes++;
                if (s->_qp_ext_strikes >= QP_EXT_STRIKES && !s->_qp_hook_muzzle) {
                    s->_qp_hook_muzzle = 1;
                    qemu_log_mask(LOG_GUEST_ERROR,
                        "qp settle: hook MUZZLED after %u capped extended settles; "
                        "co-stepping continues, budget reverts to the 256 tier\n",
                        (unsigned)QP_EXT_STRIKES);
                }
            }
            qemu_log_mask(LOG_UNIMP, "qp settle: budget %u exhausted, state still changing\n", _qp_cap);
        } else if (_qp_cap == QP_EXT_BUDGET) {
            s->_qp_ext_strikes = 0;  /* an extended settle that CONVERGED */
        }
        s->_qp_busy = 0;
    }
    }
    update_state(s);

}

/* Advance exactly one model clock.  Pin-level transport bridges use
 * this instead of holding an MMIO settle loop open. */
void rv_timer_step(rv_timer_state *s)
{
    update_state(s);
    qp_tick(s);
    update_state(s);
}

/* Fine-grained co-step primitives: a machine-level bridge that
 * LOCK-STEPS two generated models (device-to-device signal links)
 * must interleave update/tick across the pair — a coarse
 * step()+step() sequence skews the handshake phases (the keymgr x
 * kmac app channel only converges under update-update, tick-tick,
 * update-update ordering). */
void rv_timer_update(rv_timer_state *s)
{
    update_state(s);
}

void rv_timer_tick(rv_timer_state *s)
{
    qp_tick(s);
}

void rv_timer_step_many(rv_timer_state *s, unsigned count)
{
    update_state(s);
    while (count--) { qp_tick(s); update_state(s); }
}

/* Pulse reset to commit RESVALs into every prim_subreg. */
void rv_timer_reset(rv_timer_state *s)
{
    s->rst_ni = 0;
    update_state(s);
    qp_tick(s);
    update_state(s);
    s->rst_ni = 1;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

/*
 * MMIO Read — Protocol-agnostic bus transaction
 *
 * Instead of a switch(addr) with hardcoded register offsets,
 * we inject the address into the internal addr signal and let
 * update_state() drive the correct value to rdata.
 */
uint64_t rv_timer_read(void *opaque, hwaddr addr, unsigned size)
{
    rv_timer_state *s = opaque;
    s->_qp_access_gen++;   /* organs: state snapshots older than this are stale */

    /* Inject QEMU address into the internal address signal */
    s->tl_i_a_address = (uint32_t)addr;

    /* TL-UL: assert request valid + Get opcode + always-ready response acceptor */
    s->tl_i_a_valid = 1;
    s->tl_i_d_ready = 1;
    s->tl_i_a_opcode = (uint8_t)4;  /* Get */
    s->tl_i_a_size = (uint8_t)(size >= 4 ? 2 : size == 2 ? 1 : 0);  /* log2(bytes) */
    s->tl_i_a_mask = (uint8_t)(((1u << (size >= 4 ? 4 : size)) - 1u) << (addr & 3u));  /* byte lanes */
    s->tl_i_a_user_instr_type = (uint8_t)9;  /* MuBi4False: data access */

    /* Multi-pass settle, same shape as the write callback.
     *
     * The read path also has seq drvs that must re-fire under
     * the read-cycle's opcode (e.g. rdata_q in tlul_adapter_reg
     * gates on `(error||wr_req) ? -1 : rdata_i` and is itself a
     * seq drv).  Without ticking on the read, rdata_q remains
     * at whatever value the previous write callback left it.
     *
     * Ticking during read is safe for register storage: with
     * we==0 (read cycle), every prim_subreg's `q <= we?wd:q`
     * is a no-op, so register state is preserved.  ACCUMULATE
     * counters go through ptimer (not tick), also unaffected.
     */
    s->_qp_in_request = 1;
    update_state(s);
    qp_tick(s);
    s->tl_i_a_valid = 0;   /* request lasted one clock */
    s->_qp_in_request = 0;
    s->_qp_rd_cap = 0;
    {
    if (!s->_qp_busy) {
        s->_qp_busy = 1;
        unsigned _qp_ticks = 0;
        update_state(s);
            if (!s->_qp_rd_cap && s->u_reg_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->u_reg_tl_o_d_data; }
        QPSettleFingerprint _qp_base = qp_settle_fingerprint(s);
        unsigned _qp_lam = 0, _qp_pow = 1;
        bool _qp_ext = false;
        unsigned _qp_cap = 256u;
        while (_qp_ticks < _qp_cap) {
            bool _qp_ch = qp_tick(s);
            bool _qp_rw = s->_qp_rewound != 0;
            if (_qp_rw) { s->_qp_rewound = 0; _qp_ch = true; }
            if (!_qp_ch && !_qp_ext)
                break;  /* sequential fixed point reached */
            ++_qp_ticks;
            update_state(s);
            if (!s->_qp_rd_cap && s->u_reg_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->u_reg_tl_o_d_data; }
            /* settle hook AFTER tick+update: the machine co-steps
             * the OTHER ring members (this model is _qp_busy and is
             * skipped there) against this model's FRESH post-tick
             * outputs — the same phase as a pump costep, so a
             * one-clock request pulse is seen exactly once (the
             * before-tick placement let a pulse live two partner
             * ticks: a boot INS was delivered twice).  Nonzero
             * return = cross-model transaction in flight: keeps the
             * loop alive past a local fixed point. */
            int _qp_hk = s->_qp_settle_hook ?
                         s->_qp_settle_hook(s->_qp_settle_hook_ctx) : 0;
            /* muzzled: the machine still co-steps (the ring must not
             * stall), but its verdict can no longer widen the budget. */
            _qp_ext = _qp_hk && !s->_qp_hook_muzzle;
            /* re-mirror after the hook wired fresh partner outputs
             * onto this model's inputs: without this, the next tick
             * still sees pre-hook mirrors and a one-clock handshake
             * (req/ready) takes one beat too long — the boot INS
             * was pushed twice.  Costs only while cross-model
             * traffic is live. */
            if (_qp_ext) {
                update_state(s);
            if (!s->_qp_rd_cap && s->u_reg_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->u_reg_tl_o_d_data; }
            }
            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            if (_qp_rw) {  /* deliberate repeat: move the camera here */
                _qp_base = _qp_now; _qp_lam = 0; _qp_pow = 1;
                continue;
            }
            ++_qp_lam;
            if (_qp_now.first == _qp_base.first &&
                _qp_now.second == _qp_base.second &&
                !s->_qp_hold_settle && !_qp_ext)
                break;  /* state revisited: periodic, no fixed point exists */
            if (_qp_lam == _qp_pow) {  /* Brent: move camera, double the wait */
                _qp_base = _qp_now; _qp_lam = 0;
                if (_qp_pow < (1u << 30)) _qp_pow <<= 1;
            }
            _qp_cap = s->_qp_hold_settle ? QP_SETTLE_BUDGET
                    : (_qp_ext ? QP_EXT_BUDGET : 256u);
        }
        s->_qp_last_ticks = (uint16_t)(_qp_ticks > 0xFFFFu ? 0xFFFFu : _qp_ticks);
        if (_qp_ticks >= _qp_cap) {
            s->_qp_budget_hit = 1;
            if (_qp_cap == QP_EXT_BUDGET) {
                if (s->_qp_ext_strikes < 255u) s->_qp_ext_strikes++;
                if (s->_qp_ext_strikes >= QP_EXT_STRIKES && !s->_qp_hook_muzzle) {
                    s->_qp_hook_muzzle = 1;
                    qemu_log_mask(LOG_GUEST_ERROR,
                        "qp settle: hook MUZZLED after %u capped extended settles; "
                        "co-stepping continues, budget reverts to the 256 tier\n",
                        (unsigned)QP_EXT_STRIKES);
                }
            }
            qemu_log_mask(LOG_UNIMP, "qp settle: budget %u exhausted, state still changing\n", _qp_cap);
        } else if (_qp_cap == QP_EXT_BUDGET) {
            s->_qp_ext_strikes = 0;  /* an extended settle that CONVERGED */
        }
        s->_qp_busy = 0;
    }
    }
    update_state(s);

    uint32_t _qp_rv = s->_qp_rd_cap ? s->_qp_rd_capv : s->u_reg_tl_o_d_data;
    if (size < 4)
        return ((uint64_t)_qp_rv >> (8u * (addr & 3u))) & ((1ULL << (8u * size)) - 1u);
    return (uint64_t)_qp_rv;
}

/*
 * MMIO Write — Protocol-agnostic bus transaction
 *
 * Instead of a switch(addr) dispatching to individual registers,
 * we inject addr+value into the internal bus signals and let
 * update_state() propagate the write to the correct target.
 */
void rv_timer_write(void *opaque, hwaddr addr,
                uint64_t value, unsigned size)
{
    rv_timer_state *s = opaque;
    s->_qp_access_gen++;   /* organs: state snapshots older than this are stale */

    /* Inject QEMU address into the internal address signal */
    s->tl_i_a_address = (uint32_t)addr;

    /* Inject QEMU write data into the internal wdata signal
     * (sub-word writes: data sits in the addressed byte lanes) */
    s->tl_i_a_data = (uint32_t)(size < 4 ? (value << (8u * (addr & 3u))) : value);

    /* Set write mask (byte-enable bits for the access size, in the
     * addressed lanes: a byte write at +1 -> a_mask = 0x2) */
    s->tl_i_a_mask = (uint8_t)(((1u << (size >= 4 ? 4 : size)) - 1u) << (addr & 3u));

    /* TL-UL: assert request valid + PutFullData opcode + always-ready response acceptor */
    s->tl_i_a_valid = 1;
    s->tl_i_d_ready = 1;
    s->tl_i_a_opcode = (uint8_t)0;  /* PutFullData */
    s->tl_i_a_size = (uint8_t)(size >= 4 ? 2 : size == 2 ? 1 : 0);  /* log2(bytes) */
    s->tl_i_a_user_instr_type = (uint8_t)9;  /* MuBi4False: data access */

    /* Multi-pass comb-seq settle to a steady state.
     *
     * The OT register block has a multi-stage chain that needs
     * several update_state↔tick rounds to converge:
     *   - Round 1: comb (addr decode, racl gates from prev cycle
     *     state) → tick (addr_hit and racl_addr_hit_write update,
     *     register q's commit using stale ctrl_we).
     *   - Round 2: comb (now ctrl_we is true since racl_addr_hit
     *     is fresh) → tick (q's commit with fresh ctrl_we).
     *   - Round 3: comb (qs refreshes from new q) → tick
     *     (reg_rdata_next mux reads fresh qs).
     *   - Final comb: propagate reg_rdata_next forward through
     *     u_reg_if's d_data, rsp_intg, instance bridges, up to
     *     the top-level rdata BIP.
     *
     * The loop exits when sequential state stops changing.  If a full
     * state repeats, the trajectory is periodic and the current settle
     * exits early; 256 changing edges remain the final guard.  Autonomous
     * counters and timers should be classified out of tick and driven by
     * a QEMU timing backend rather than relying on this safety path.
     */
    s->_qp_in_request = 1;
    update_state(s);
    qp_tick(s);
    s->tl_i_a_valid = 0;   /* request lasted one clock */
    s->_qp_in_request = 0;
    {
    if (!s->_qp_busy) {
        s->_qp_busy = 1;
        unsigned _qp_ticks = 0;
        update_state(s);
        QPSettleFingerprint _qp_base = qp_settle_fingerprint(s);
        unsigned _qp_lam = 0, _qp_pow = 1;
        bool _qp_ext = false;
        unsigned _qp_cap = 256u;
        while (_qp_ticks < _qp_cap) {
            bool _qp_ch = qp_tick(s);
            bool _qp_rw = s->_qp_rewound != 0;
            if (_qp_rw) { s->_qp_rewound = 0; _qp_ch = true; }
            if (!_qp_ch && !_qp_ext)
                break;  /* sequential fixed point reached */
            ++_qp_ticks;
            update_state(s);
            /* settle hook AFTER tick+update: the machine co-steps
             * the OTHER ring members (this model is _qp_busy and is
             * skipped there) against this model's FRESH post-tick
             * outputs — the same phase as a pump costep, so a
             * one-clock request pulse is seen exactly once (the
             * before-tick placement let a pulse live two partner
             * ticks: a boot INS was delivered twice).  Nonzero
             * return = cross-model transaction in flight: keeps the
             * loop alive past a local fixed point. */
            int _qp_hk = s->_qp_settle_hook ?
                         s->_qp_settle_hook(s->_qp_settle_hook_ctx) : 0;
            /* muzzled: the machine still co-steps (the ring must not
             * stall), but its verdict can no longer widen the budget. */
            _qp_ext = _qp_hk && !s->_qp_hook_muzzle;
            /* re-mirror after the hook wired fresh partner outputs
             * onto this model's inputs: without this, the next tick
             * still sees pre-hook mirrors and a one-clock handshake
             * (req/ready) takes one beat too long — the boot INS
             * was pushed twice.  Costs only while cross-model
             * traffic is live. */
            if (_qp_ext) {
                update_state(s);
            }
            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            if (_qp_rw) {  /* deliberate repeat: move the camera here */
                _qp_base = _qp_now; _qp_lam = 0; _qp_pow = 1;
                continue;
            }
            ++_qp_lam;
            if (_qp_now.first == _qp_base.first &&
                _qp_now.second == _qp_base.second &&
                !s->_qp_hold_settle && !_qp_ext)
                break;  /* state revisited: periodic, no fixed point exists */
            if (_qp_lam == _qp_pow) {  /* Brent: move camera, double the wait */
                _qp_base = _qp_now; _qp_lam = 0;
                if (_qp_pow < (1u << 30)) _qp_pow <<= 1;
            }
            _qp_cap = s->_qp_hold_settle ? QP_SETTLE_BUDGET
                    : (_qp_ext ? QP_EXT_BUDGET : 256u);
        }
        s->_qp_last_ticks = (uint16_t)(_qp_ticks > 0xFFFFu ? 0xFFFFu : _qp_ticks);
        if (_qp_ticks >= _qp_cap) {
            s->_qp_budget_hit = 1;
            if (_qp_cap == QP_EXT_BUDGET) {
                if (s->_qp_ext_strikes < 255u) s->_qp_ext_strikes++;
                if (s->_qp_ext_strikes >= QP_EXT_STRIKES && !s->_qp_hook_muzzle) {
                    s->_qp_hook_muzzle = 1;
                    qemu_log_mask(LOG_GUEST_ERROR,
                        "qp settle: hook MUZZLED after %u capped extended settles; "
                        "co-stepping continues, budget reverts to the 256 tier\n",
                        (unsigned)QP_EXT_STRIKES);
                }
            }
            qemu_log_mask(LOG_UNIMP, "qp settle: budget %u exhausted, state still changing\n", _qp_cap);
        } else if (_qp_cap == QP_EXT_BUDGET) {
            s->_qp_ext_strikes = 0;  /* an extended settle that CONVERGED */
        }
        s->_qp_busy = 0;
    }
    }
    update_state(s);

}

static const MemoryRegionOps rv_timer_ops = {
    .read  = rv_timer_read,
    .write = rv_timer_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void rv_timer_realize(DeviceState *dev, Error **errp)
{
    rv_timer_state *s = RV_TIMER(dev);

    memory_region_init_io(&s->mmio, OBJECT(dev), &rv_timer_ops, s,
                          "rv_timer", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    /* Initialize state to zero */
    update_state(s);
}

static void rv_timer_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = rv_timer_realize;
}

static const TypeInfo rv_timer_info = {
    .name          = TYPE_RV_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(rv_timer_state),
    .class_init    = rv_timer_class_init,
};

static void rv_timer_register_types(void)
{
    type_register_static(&rv_timer_info);
}

type_init(rv_timer_register_types)

/*
 * Public input setters — Phase 2.d
 *
 * Each setter writes one host-driven input field, then runs the
 * `update_state → tick → update_state` settling sequence so the
 * new value propagates through comb + seq logic before the call
 * returns.  Use these from the shim to bridge QEMU back-end
 * events (chardev RX, qemu_irq pin changes, alert handler
 * responses, ...) into the simulated device state.
 */
void rv_timer_set_alert_rx_i_0__ping_p(rv_timer_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_p = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rv_timer_set_alert_rx_i_0__ping_n(rv_timer_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_n = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rv_timer_set_alert_rx_i_0__ack_p(rv_timer_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_p = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rv_timer_set_alert_rx_i_0__ack_n(rv_timer_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_n = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rv_timer_set_racl_policies_i_0__write_perm(rv_timer_state *s, uint8_t value)
{
    s->racl_policies_i_0__write_perm = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rv_timer_set_racl_policies_i_0__read_perm(rv_timer_state *s, uint8_t value)
{
    s->racl_policies_i_0__read_perm = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

