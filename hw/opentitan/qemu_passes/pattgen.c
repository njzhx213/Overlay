/*
 * Auto-generated QEMU device: pattgen
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
#include "pattgen.h"

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

static void update_state_once(pattgen_state *s);
typedef struct QPSettleFingerprint {
    uint64_t first;
    uint64_t second;
} QPSettleFingerprint;
static QPSettleFingerprint qp_settle_fingerprint(const pattgen_state *s);
static void update_state(pattgen_state *s)
{
    update_state_once(s);
    update_state_once(s);
    update_state_once(s);
}
static bool tick(pattgen_state *s);
/* One clock + per-clock observer hook (organs needing edge visibility). */
static inline bool qp_tick(pattgen_state *s)
{
    if (s->_qp_before_tick) {
        s->_qp_before_tick(s->_qp_before_tick_ctx);
        update_state(s);
    }
    bool _qp_ch = tick(s);
    if (s->_qp_on_tick) s->_qp_on_tick(s->_qp_on_tick_ctx);
    return _qp_ch;
}

/* Single settle bound per MMIO access — responsiveness only, not
 * correctness: leftover work continues on the next access/poll. */
#define QP_SETTLE_BUDGET 65536u

static QPSettleFingerprint qp_settle_fingerprint(const pattgen_state *s)
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
static void update_state_once(pattgen_state *s)
{
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
    s->u_reg_intg_err = (0) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = 0;
    s->u_reg_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
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
    s->u_reg_u_reg_if_addr_o = (((((uint64_t)(0)) << 0) | (((uint64_t)((((s->u_reg_u_reg_if_tl_i_a_address) >> 2) & 0xF))) << 2))) & ((1ULL << 6) - 1);
    s->u_reg_reg_addr = (s->u_reg_u_reg_if_addr_o) & ((1ULL << 6) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x1ULL) | (((((s->u_reg_reg_addr) == (0))) & 0x1ULL) << 0)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x2ULL) | (((((s->u_reg_reg_addr) == (4))) & 0x1ULL) << 1)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x4ULL) | (((((s->u_reg_reg_addr) == (8))) & 0x1ULL) << 2)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x8ULL) | (((((s->u_reg_reg_addr) == (12))) & 0x1ULL) << 3)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x10ULL) | (((((s->u_reg_reg_addr) == (16))) & 0x1ULL) << 4)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x20ULL) | (((((s->u_reg_reg_addr) == (20))) & 0x1ULL) << 5)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x40ULL) | (((((s->u_reg_reg_addr) == (24))) & 0x1ULL) << 6)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x80ULL) | (((((s->u_reg_reg_addr) == (28))) & 0x1ULL) << 7)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x100ULL) | (((((s->u_reg_reg_addr) == (32))) & 0x1ULL) << 8)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x200ULL) | (((((s->u_reg_reg_addr) == (36))) & 0x1ULL) << 9)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x400ULL) | (((((s->u_reg_reg_addr) == (40))) & 0x1ULL) << 10)) & ((1ULL << 12) - 1);
    s->u_reg_addr_hit = ((s->u_reg_addr_hit & ~0x800ULL) | (((((s->u_reg_reg_addr) == (44))) & 0x1ULL) << 11)) & ((1ULL << 12) - 1);
    s->u_reg_u_prim_reg_we_check_en_i = (((s->u_reg_reg_we) & (((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((s->u_reg_addr_hit) == (0))))) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_wdata_o = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_be_o = (s->u_reg_u_reg_if_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_reg_be = (s->u_reg_u_reg_if_be_o) & ((1ULL << 4) - 1);
    s->u_reg_wr_err = ((s->u_reg_reg_we) & (((((s->u_reg_addr_hit) & 1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 1) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 2) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 3) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 4) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 5) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 6) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 7) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 8) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 9) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 10) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 11) & 0x1)) & (((s->u_reg_reg_be) != (15)))))) & ((1ULL << 1) - 1);
    s->u_reg_intr_state_we = ((((s->u_reg_addr_hit) & 1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_intr_enable_we = (((((s->u_reg_addr_hit) >> 1) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_intr_test_we = (((((s->u_reg_addr_hit) >> 2) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_alert_test_we = (((((s->u_reg_addr_hit) >> 3) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_we = (((((s->u_reg_addr_hit) >> 4) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_prediv_ch0_we = (((((s->u_reg_addr_hit) >> 5) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_prediv_ch1_we = (((((s->u_reg_addr_hit) >> 6) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_data_ch0_0_we = (((((s->u_reg_addr_hit) >> 7) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_data_ch0_1_we = (((((s->u_reg_addr_hit) >> 8) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_data_ch1_0_we = (((((s->u_reg_addr_hit) >> 9) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_data_ch1_1_we = (((((s->u_reg_addr_hit) >> 10) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_size_we = (((((s->u_reg_addr_hit) >> 11) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x1ULL) | (((s->u_reg_intr_state_we) & 0x1ULL) << 0)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x2ULL) | (((s->u_reg_intr_enable_we) & 0x1ULL) << 1)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x4ULL) | (((s->u_reg_intr_test_we) & 0x1ULL) << 2)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x8ULL) | (((s->u_reg_alert_test_we) & 0x1ULL) << 3)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x10ULL) | (((s->u_reg_ctrl_we) & 0x1ULL) << 4)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x20ULL) | (((s->u_reg_prediv_ch0_we) & 0x1ULL) << 5)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x40ULL) | (((s->u_reg_prediv_ch1_we) & 0x1ULL) << 6)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x80ULL) | (((s->u_reg_data_ch0_0_we) & 0x1ULL) << 7)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x100ULL) | (((s->u_reg_data_ch0_1_we) & 0x1ULL) << 8)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x200ULL) | (((s->u_reg_data_ch1_0_we) & 0x1ULL) << 9)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x400ULL) | (((s->u_reg_data_ch1_1_we) & 0x1ULL) << 10)) & ((1ULL << 12) - 1);
    s->u_reg_reg_we_check = ((s->u_reg_reg_we_check & ~0x800ULL) | (((s->u_reg_size_we) & 0x1ULL) << 11)) & ((1ULL << 12) - 1);
    s->u_reg_u_prim_reg_we_check_oh_i = (s->u_reg_reg_we_check) & ((1ULL << 12) - 1);
    s->u_reg_u_reg_if_error_i = (((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((s->u_reg_addr_hit) == (0))))) | (s->u_reg_wr_err) | (s->u_reg_intg_err))) & ((1ULL << 1) - 1);
    s->u_reg_u_reg_if_error_i = (s->u_reg_u_reg_if_error_i) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_we = (s->u_reg_intr_state_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_rst_ni = (s->u_reg_u_intr_state_done_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_we = (s->u_reg_u_intr_state_done_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd = (s->u_reg_u_intr_state_done_ch0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd = (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_qe = (s->u_reg_u_intr_state_done_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_q = (s->u_reg_u_intr_state_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_state_done_ch0_q = (s->u_reg_u_intr_state_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_qs = (s->u_reg_u_intr_state_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q = (s->u_reg_u_intr_state_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q = (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_qs = (s->u_reg_u_intr_state_done_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_intr_state_done_ch0_qs = (s->u_reg_u_intr_state_done_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_we = (s->u_reg_intr_state_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_rst_ni = (s->u_reg_u_intr_state_done_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_we = (s->u_reg_u_intr_state_done_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd = (s->u_reg_u_intr_state_done_ch1_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd = (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_qe = (s->u_reg_u_intr_state_done_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_q = (s->u_reg_u_intr_state_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_state_done_ch1_q = (s->u_reg_u_intr_state_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_qs = (s->u_reg_u_intr_state_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q = (s->u_reg_u_intr_state_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q = (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_qs = (s->u_reg_u_intr_state_done_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_intr_state_done_ch1_qs = (s->u_reg_u_intr_state_done_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_we = (s->u_reg_intr_enable_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_rst_ni = (s->u_reg_u_intr_enable_done_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_we = (s->u_reg_u_intr_enable_done_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd = (s->u_reg_u_intr_enable_done_ch0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_de = (s->u_reg_u_intr_enable_done_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d = (s->u_reg_u_intr_enable_done_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd = (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d = (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_we) | (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en = (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_data = (((s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_we) ? (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd) : (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_data = (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_qe = (s->u_reg_u_intr_enable_done_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_q = (s->u_reg_u_intr_enable_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_enable_done_ch0_q = (s->u_reg_u_intr_enable_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_qs = (s->u_reg_u_intr_enable_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_q = (s->u_reg_u_intr_enable_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch0_qs = (s->u_reg_u_intr_enable_done_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_intr_enable_done_ch0_qs = (s->u_reg_u_intr_enable_done_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = (((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) || (((((s->u_reg_addr_hit) & 1)) == (1)))) && (((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && ((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1)))) || (((((s->u_reg_addr_hit) & 1)) == (1))))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | ((((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) ? (s->u_reg_intr_enable_done_ch0_qs) : (s->u_reg_intr_state_done_ch0_qs))) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_intr_enable_done_ch0_ds = (((s->u_reg_u_intr_enable_done_ch0_wr_en) ? (s->u_reg_u_intr_enable_done_ch0_wr_data) : (s->u_reg_u_intr_enable_done_ch0_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_we = (s->u_reg_intr_enable_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_rst_ni = (s->u_reg_u_intr_enable_done_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_we = (s->u_reg_u_intr_enable_done_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd = (s->u_reg_u_intr_enable_done_ch1_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_de = (s->u_reg_u_intr_enable_done_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d = (s->u_reg_u_intr_enable_done_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd = (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d = (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_we) | (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en = (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_data = (((s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_we) ? (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd) : (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_data = (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_qe = (s->u_reg_u_intr_enable_done_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_q = (s->u_reg_u_intr_enable_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_enable_done_ch1_q = (s->u_reg_u_intr_enable_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_qs = (s->u_reg_u_intr_enable_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_q = (s->u_reg_u_intr_enable_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_enable_done_ch1_qs = (s->u_reg_u_intr_enable_done_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_intr_enable_done_ch1_qs = (s->u_reg_u_intr_enable_done_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = (((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) || (((((s->u_reg_addr_hit) & 1)) == (1)))) && (((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && ((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1)))) || (((((s->u_reg_addr_hit) & 1)) == (1))))) ? ((s->u_reg_reg_rdata_next & ~0x2ULL) | ((((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) ? (s->u_reg_intr_enable_done_ch1_qs) : (s->u_reg_intr_state_done_ch1_qs))) & 0x1ULL) << 1)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = ((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | (((0) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = ((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x2ULL) | (((0) & 0x1ULL) << 1)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | (((0) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_intr_enable_done_ch1_ds = (((s->u_reg_u_intr_enable_done_ch1_wr_en) ? (s->u_reg_u_intr_enable_done_ch1_wr_data) : (s->u_reg_u_intr_enable_done_ch1_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_re = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_we = (s->u_reg_intr_test_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_qe = (s->u_reg_u_intr_test_done_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_intr_test_flds_we = ((s->u_reg_intr_test_flds_we & ~0x1ULL) | (((s->u_reg_u_intr_test_done_ch0_qe) & 0x1ULL) << 0)) & ((1ULL << 2) - 1);
    s->u_reg_u_intr_test_done_ch0_qre = (s->u_reg_u_intr_test_done_ch0_re) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_q = (s->u_reg_u_intr_test_done_ch0_wd) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test_done_ch0_q = (s->u_reg_u_intr_test_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_ds = (s->u_reg_u_intr_test_done_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch0_qs = (s->u_reg_u_intr_test_done_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_re = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_we = (s->u_reg_intr_test_we) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_qe = (s->u_reg_u_intr_test_done_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_intr_test_flds_we = ((s->u_reg_intr_test_flds_we & ~0x2ULL) | (((s->u_reg_u_intr_test_done_ch1_qe) & 0x1ULL) << 1)) & ((1ULL << 2) - 1);
    s->u_reg_reg2hw_intr_test_done_ch0_qe = (((s->u_reg_intr_test_flds_we) == (3))) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test_done_ch1_qe = (((s->u_reg_intr_test_flds_we) == (3))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_qre = (s->u_reg_u_intr_test_done_ch1_re) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_q = (s->u_reg_u_intr_test_done_ch1_wd) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test_done_ch1_q = (s->u_reg_u_intr_test_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_ds = (s->u_reg_u_intr_test_done_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_test_done_ch1_qs = (s->u_reg_u_intr_test_done_ch1_d) & ((1ULL << 1) - 1);
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
    s->u_reg_u_ctrl_enable_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_rst_ni = (s->u_reg_u_ctrl_enable_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_we = (s->u_reg_u_ctrl_enable_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_enable_ch0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_de = (s->u_reg_u_ctrl_enable_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_enable_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en = (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_data = (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_qe = (s->u_reg_u_ctrl_enable_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_q = (s->u_reg_u_ctrl_enable_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_enable_ch0_q = (s->u_reg_u_ctrl_enable_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_qs = (s->u_reg_u_ctrl_enable_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_q = (s->u_reg_u_ctrl_enable_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch0_qs = (s->u_reg_u_ctrl_enable_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_enable_ch0_qs = (s->u_reg_u_ctrl_enable_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | (((s->u_reg_ctrl_enable_ch0_qs) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_enable_ch0_ds = (((s->u_reg_u_ctrl_enable_ch0_wr_en) ? (s->u_reg_u_ctrl_enable_ch0_wr_data) : (s->u_reg_u_ctrl_enable_ch0_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_rst_ni = (s->u_reg_u_ctrl_enable_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_we = (s->u_reg_u_ctrl_enable_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_enable_ch1_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_de = (s->u_reg_u_ctrl_enable_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_enable_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en = (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_data = (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_qe = (s->u_reg_u_ctrl_enable_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_q = (s->u_reg_u_ctrl_enable_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_enable_ch1_q = (s->u_reg_u_ctrl_enable_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_qs = (s->u_reg_u_ctrl_enable_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_q = (s->u_reg_u_ctrl_enable_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_enable_ch1_qs = (s->u_reg_u_ctrl_enable_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_enable_ch1_qs = (s->u_reg_u_ctrl_enable_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x2ULL) | (((s->u_reg_ctrl_enable_ch1_qs) & 0x1ULL) << 1)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_enable_ch1_ds = (((s->u_reg_u_ctrl_enable_ch1_wr_en) ? (s->u_reg_u_ctrl_enable_ch1_wr_data) : (s->u_reg_u_ctrl_enable_ch1_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 2) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_rst_ni = (s->u_reg_u_ctrl_polarity_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_we = (s->u_reg_u_ctrl_polarity_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_polarity_ch0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_de = (s->u_reg_u_ctrl_polarity_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_polarity_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en = (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_data = (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_qe = (s->u_reg_u_ctrl_polarity_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_q = (s->u_reg_u_ctrl_polarity_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_polarity_ch0_q = (s->u_reg_u_ctrl_polarity_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_qs = (s->u_reg_u_ctrl_polarity_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_q = (s->u_reg_u_ctrl_polarity_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch0_qs = (s->u_reg_u_ctrl_polarity_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_polarity_ch0_qs = (s->u_reg_u_ctrl_polarity_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x4ULL) | (((s->u_reg_ctrl_polarity_ch0_qs) & 0x1ULL) << 2)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_polarity_ch0_ds = (((s->u_reg_u_ctrl_polarity_ch0_wr_en) ? (s->u_reg_u_ctrl_polarity_ch0_wr_data) : (s->u_reg_u_ctrl_polarity_ch0_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 3) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_rst_ni = (s->u_reg_u_ctrl_polarity_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_we = (s->u_reg_u_ctrl_polarity_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_polarity_ch1_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_de = (s->u_reg_u_ctrl_polarity_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_polarity_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en = (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_data = (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_qe = (s->u_reg_u_ctrl_polarity_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_q = (s->u_reg_u_ctrl_polarity_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_polarity_ch1_q = (s->u_reg_u_ctrl_polarity_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_qs = (s->u_reg_u_ctrl_polarity_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_q = (s->u_reg_u_ctrl_polarity_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_polarity_ch1_qs = (s->u_reg_u_ctrl_polarity_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_polarity_ch1_qs = (s->u_reg_u_ctrl_polarity_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x8ULL) | (((s->u_reg_ctrl_polarity_ch1_qs) & 0x1ULL) << 3)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_polarity_ch1_ds = (((s->u_reg_u_ctrl_polarity_ch1_wr_en) ? (s->u_reg_u_ctrl_polarity_ch1_wr_data) : (s->u_reg_u_ctrl_polarity_ch1_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 4) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_we = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_de = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_data = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_qe = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_q = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch0_q = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_q = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_inactive_level_pcl_ch0_qs = (s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x10ULL) | (((s->u_reg_ctrl_inactive_level_pcl_ch0_qs) & 0x1ULL) << 4)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_ds = (((s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_data) : (s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 5) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni = (s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_we = (s->u_reg_u_ctrl_inactive_level_pda_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pda_ch0_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_de = (s->u_reg_u_ctrl_inactive_level_pda_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pda_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en = (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_data = (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_qe = (s->u_reg_u_ctrl_inactive_level_pda_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_q = (s->u_reg_u_ctrl_inactive_level_pda_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pda_ch0_q = (s->u_reg_u_ctrl_inactive_level_pda_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_qs = (s->u_reg_u_ctrl_inactive_level_pda_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_q = (s->u_reg_u_ctrl_inactive_level_pda_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_qs = (s->u_reg_u_ctrl_inactive_level_pda_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_inactive_level_pda_ch0_qs = (s->u_reg_u_ctrl_inactive_level_pda_ch0_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x20ULL) | (((s->u_reg_ctrl_inactive_level_pda_ch0_qs) & 0x1ULL) << 5)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_ds = (((s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_data) : (s->u_reg_u_ctrl_inactive_level_pda_ch0_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 6) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_we = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_de = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_data = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_qe = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_q = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch1_q = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_q = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_inactive_level_pcl_ch1_qs = (s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x40ULL) | (((s->u_reg_ctrl_inactive_level_pcl_ch1_qs) & 0x1ULL) << 6)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_ds = (((s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_data) : (s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_we = (s->u_reg_ctrl_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 7) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni = (s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_we = (s->u_reg_u_ctrl_inactive_level_pda_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pda_ch1_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_de = (s->u_reg_u_ctrl_inactive_level_pda_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pda_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd = (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d = (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en = (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_data = (((s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d))) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_data = (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_qe = (s->u_reg_u_ctrl_inactive_level_pda_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_q = (s->u_reg_u_ctrl_inactive_level_pda_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pda_ch1_q = (s->u_reg_u_ctrl_inactive_level_pda_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_qs = (s->u_reg_u_ctrl_inactive_level_pda_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_q = (s->u_reg_u_ctrl_inactive_level_pda_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_qs = (s->u_reg_u_ctrl_inactive_level_pda_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_ctrl_inactive_level_pda_ch1_qs = (s->u_reg_u_ctrl_inactive_level_pda_ch1_qs) & ((1ULL << 1) - 1);
    s->u_reg_reg_rdata_next = ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x80ULL) | (((s->u_reg_ctrl_inactive_level_pda_ch1_qs) & 0x1ULL) << 7)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_ds = (((s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_data) : (s->u_reg_u_ctrl_inactive_level_pda_ch1_qs))) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_we = (s->u_reg_prediv_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_prediv_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_d = 0;
    s->u_reg_u_prediv_ch0_rst_ni = (s->u_reg_u_prediv_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_wr_en_data_arb_we = (s->u_reg_u_prediv_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wd = s->u_reg_u_prediv_ch0_wd;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_de = (s->u_reg_u_prediv_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_wr_en_data_arb_d = s->u_reg_u_prediv_ch0_d;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wd = s->u_reg_u_prediv_ch0_wr_en_data_arb_wd;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_d = s->u_reg_u_prediv_ch0_wr_en_data_arb_d;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_prediv_ch0_wr_en_data_arb_we) | (s->u_reg_u_prediv_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_wr_en = (s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_prediv_ch0_wr_en_data_arb_we) ? (s->u_reg_u_prediv_ch0_wr_en_data_arb_wd) : (s->u_reg_u_prediv_ch0_wr_en_data_arb_d));
    s->u_reg_u_prediv_ch0_wr_data = s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_prediv_ch0_qe = (s->u_reg_u_prediv_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch0_q = s->u_reg_u_prediv_ch0_q;
    s->u_reg_reg2hw_prediv_ch0_q = s->u_reg_u_prediv_ch0_q;
    s->u_reg_u_prediv_ch0_qs = s->u_reg_u_prediv_ch0_q;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_q = s->u_reg_u_prediv_ch0_q;
    s->u_reg_u_prediv_ch0_qs = s->u_reg_u_prediv_ch0_qs;
    s->u_reg_prediv_ch0_qs = s->u_reg_u_prediv_ch0_qs;
    s->u_reg_u_prediv_ch0_ds = ((s->u_reg_u_prediv_ch0_wr_en) ? (s->u_reg_u_prediv_ch0_wr_data) : (s->u_reg_u_prediv_ch0_qs));
    s->u_reg_u_prediv_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_we = (s->u_reg_prediv_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_prediv_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_d = 0;
    s->u_reg_u_prediv_ch1_rst_ni = (s->u_reg_u_prediv_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_wr_en_data_arb_we = (s->u_reg_u_prediv_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wd = s->u_reg_u_prediv_ch1_wd;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_de = (s->u_reg_u_prediv_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_wr_en_data_arb_d = s->u_reg_u_prediv_ch1_d;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wd = s->u_reg_u_prediv_ch1_wr_en_data_arb_wd;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_d = s->u_reg_u_prediv_ch1_wr_en_data_arb_d;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_prediv_ch1_wr_en_data_arb_we) | (s->u_reg_u_prediv_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_wr_en = (s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_prediv_ch1_wr_en_data_arb_we) ? (s->u_reg_u_prediv_ch1_wr_en_data_arb_wd) : (s->u_reg_u_prediv_ch1_wr_en_data_arb_d));
    s->u_reg_u_prediv_ch1_wr_data = s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_prediv_ch1_qe = (s->u_reg_u_prediv_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_prediv_ch1_q = s->u_reg_u_prediv_ch1_q;
    s->u_reg_reg2hw_prediv_ch1_q = s->u_reg_u_prediv_ch1_q;
    s->u_reg_u_prediv_ch1_qs = s->u_reg_u_prediv_ch1_q;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_q = s->u_reg_u_prediv_ch1_q;
    s->u_reg_u_prediv_ch1_qs = s->u_reg_u_prediv_ch1_qs;
    s->u_reg_prediv_ch1_qs = s->u_reg_u_prediv_ch1_qs;
    s->u_reg_u_prediv_ch1_ds = ((s->u_reg_u_prediv_ch1_wr_en) ? (s->u_reg_u_prediv_ch1_wr_data) : (s->u_reg_u_prediv_ch1_qs));
    s->u_reg_u_data_ch0_0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_we = (s->u_reg_data_ch0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch0_0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_d = 0;
    s->u_reg_u_data_ch0_0_rst_ni = (s->u_reg_u_data_ch0_0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_wr_en_data_arb_we = (s->u_reg_u_data_ch0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wd = s->u_reg_u_data_ch0_0_wd;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_de = (s->u_reg_u_data_ch0_0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_wr_en_data_arb_d = s->u_reg_u_data_ch0_0_d;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wd = s->u_reg_u_data_ch0_0_wr_en_data_arb_wd;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_d = s->u_reg_u_data_ch0_0_wr_en_data_arb_d;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_en = (((s->u_reg_u_data_ch0_0_wr_en_data_arb_we) | (s->u_reg_u_data_ch0_0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_wr_en = (s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch0_0_wr_en_data_arb_we) ? (s->u_reg_u_data_ch0_0_wr_en_data_arb_wd) : (s->u_reg_u_data_ch0_0_wr_en_data_arb_d));
    s->u_reg_u_data_ch0_0_wr_data = s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch0_0_qe = (s->u_reg_u_data_ch0_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_0_q = s->u_reg_u_data_ch0_0_q;
    s->u_reg_reg2hw_data_ch0_0__q = s->u_reg_u_data_ch0_0_q;
    s->u_reg_u_data_ch0_0_qs = s->u_reg_u_data_ch0_0_q;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_q = s->u_reg_u_data_ch0_0_q;
    s->u_reg_u_data_ch0_0_qs = s->u_reg_u_data_ch0_0_qs;
    s->u_reg_data_ch0_0_qs = s->u_reg_u_data_ch0_0_qs;
    s->u_reg_u_data_ch0_0_ds = ((s->u_reg_u_data_ch0_0_wr_en) ? (s->u_reg_u_data_ch0_0_wr_data) : (s->u_reg_u_data_ch0_0_qs));
    s->u_reg_u_data_ch0_1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_we = (s->u_reg_data_ch0_1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch0_1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_d = 0;
    s->u_reg_u_data_ch0_1_rst_ni = (s->u_reg_u_data_ch0_1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_wr_en_data_arb_we = (s->u_reg_u_data_ch0_1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wd = s->u_reg_u_data_ch0_1_wd;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_de = (s->u_reg_u_data_ch0_1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_wr_en_data_arb_d = s->u_reg_u_data_ch0_1_d;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wd = s->u_reg_u_data_ch0_1_wr_en_data_arb_wd;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_d = s->u_reg_u_data_ch0_1_wr_en_data_arb_d;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_en = (((s->u_reg_u_data_ch0_1_wr_en_data_arb_we) | (s->u_reg_u_data_ch0_1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_wr_en = (s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch0_1_wr_en_data_arb_we) ? (s->u_reg_u_data_ch0_1_wr_en_data_arb_wd) : (s->u_reg_u_data_ch0_1_wr_en_data_arb_d));
    s->u_reg_u_data_ch0_1_wr_data = s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch0_1_qe = (s->u_reg_u_data_ch0_1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch0_1_q = s->u_reg_u_data_ch0_1_q;
    s->u_reg_reg2hw_data_ch0_1__q = s->u_reg_u_data_ch0_1_q;
    s->u_reg_u_data_ch0_1_qs = s->u_reg_u_data_ch0_1_q;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_q = s->u_reg_u_data_ch0_1_q;
    s->u_reg_u_data_ch0_1_qs = s->u_reg_u_data_ch0_1_qs;
    s->u_reg_data_ch0_1_qs = s->u_reg_u_data_ch0_1_qs;
    s->u_reg_u_data_ch0_1_ds = ((s->u_reg_u_data_ch0_1_wr_en) ? (s->u_reg_u_data_ch0_1_wr_data) : (s->u_reg_u_data_ch0_1_qs));
    s->u_reg_u_data_ch1_0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_we = (s->u_reg_data_ch1_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch1_0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_d = 0;
    s->u_reg_u_data_ch1_0_rst_ni = (s->u_reg_u_data_ch1_0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_wr_en_data_arb_we = (s->u_reg_u_data_ch1_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wd = s->u_reg_u_data_ch1_0_wd;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_de = (s->u_reg_u_data_ch1_0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_wr_en_data_arb_d = s->u_reg_u_data_ch1_0_d;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wd = s->u_reg_u_data_ch1_0_wr_en_data_arb_wd;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_d = s->u_reg_u_data_ch1_0_wr_en_data_arb_d;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_en = (((s->u_reg_u_data_ch1_0_wr_en_data_arb_we) | (s->u_reg_u_data_ch1_0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_wr_en = (s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch1_0_wr_en_data_arb_we) ? (s->u_reg_u_data_ch1_0_wr_en_data_arb_wd) : (s->u_reg_u_data_ch1_0_wr_en_data_arb_d));
    s->u_reg_u_data_ch1_0_wr_data = s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch1_0_qe = (s->u_reg_u_data_ch1_0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_0_q = s->u_reg_u_data_ch1_0_q;
    s->u_reg_reg2hw_data_ch1_0__q = s->u_reg_u_data_ch1_0_q;
    s->u_reg_u_data_ch1_0_qs = s->u_reg_u_data_ch1_0_q;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_q = s->u_reg_u_data_ch1_0_q;
    s->u_reg_u_data_ch1_0_qs = s->u_reg_u_data_ch1_0_qs;
    s->u_reg_data_ch1_0_qs = s->u_reg_u_data_ch1_0_qs;
    s->u_reg_u_data_ch1_0_ds = ((s->u_reg_u_data_ch1_0_wr_en) ? (s->u_reg_u_data_ch1_0_wr_data) : (s->u_reg_u_data_ch1_0_qs));
    s->u_reg_u_data_ch1_1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_we = (s->u_reg_data_ch1_1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch1_1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_d = 0;
    s->u_reg_u_data_ch1_1_rst_ni = (s->u_reg_u_data_ch1_1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_wr_en_data_arb_we = (s->u_reg_u_data_ch1_1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wd = s->u_reg_u_data_ch1_1_wd;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_de = (s->u_reg_u_data_ch1_1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_wr_en_data_arb_d = s->u_reg_u_data_ch1_1_d;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wd = s->u_reg_u_data_ch1_1_wr_en_data_arb_wd;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_d = s->u_reg_u_data_ch1_1_wr_en_data_arb_d;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_en = (((s->u_reg_u_data_ch1_1_wr_en_data_arb_we) | (s->u_reg_u_data_ch1_1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_wr_en = (s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch1_1_wr_en_data_arb_we) ? (s->u_reg_u_data_ch1_1_wr_en_data_arb_wd) : (s->u_reg_u_data_ch1_1_wr_en_data_arb_d));
    s->u_reg_u_data_ch1_1_wr_data = s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch1_1_qe = (s->u_reg_u_data_ch1_1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_data_ch1_1_q = s->u_reg_u_data_ch1_1_q;
    s->u_reg_reg2hw_data_ch1_1__q = s->u_reg_u_data_ch1_1_q;
    s->u_reg_u_data_ch1_1_qs = s->u_reg_u_data_ch1_1_q;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_q = s->u_reg_u_data_ch1_1_q;
    s->u_reg_u_data_ch1_1_qs = s->u_reg_u_data_ch1_1_qs;
    s->u_reg_data_ch1_1_qs = s->u_reg_u_data_ch1_1_qs;
    s->u_reg_reg_rdata_next = (((((((((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))) || ((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1)))) && ((((((((((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1)))) || ((((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) || (((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) || ((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) || (((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) || ((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1)))))) ? ((((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))) ? (s->u_reg_data_ch1_1_qs) : ((((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))) ? (s->u_reg_data_ch1_0_qs) : ((((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))) ? (s->u_reg_data_ch0_1_qs) : ((((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))) ? (s->u_reg_data_ch0_0_qs) : ((((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))) ? (s->u_reg_prediv_ch1_qs) : (s->u_reg_prediv_ch0_qs))))))))))) : s->u_reg_reg_rdata_next);
    s->u_reg_u_data_ch1_1_ds = ((s->u_reg_u_data_ch1_1_wr_en) ? (s->u_reg_u_data_ch1_1_wr_data) : (s->u_reg_u_data_ch1_1_qs));
    s->u_reg_u_size_len_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_we = (s->u_reg_size_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x3F)) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_d = (0) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_rst_ni = (s->u_reg_u_size_len_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_we = (s->u_reg_u_size_len_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wd = (s->u_reg_u_size_len_ch0_wd) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_de = (s->u_reg_u_size_len_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_d = (s->u_reg_u_size_len_ch0_d) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wd = (s->u_reg_u_size_len_ch0_wr_en_data_arb_wd) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_d = (s->u_reg_u_size_len_ch0_wr_en_data_arb_d) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_size_len_ch0_wr_en_data_arb_we) | (s->u_reg_u_size_len_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_wr_en = (s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_data = (((s->u_reg_u_size_len_ch0_wr_en_data_arb_we) ? (s->u_reg_u_size_len_ch0_wr_en_data_arb_wd) : (s->u_reg_u_size_len_ch0_wr_en_data_arb_d))) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_wr_data = (s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_data) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_qe = (s->u_reg_u_size_len_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch0_q = (s->u_reg_u_size_len_ch0_q) & ((1ULL << 6) - 1);
    s->u_reg_reg2hw_size_len_ch0_q = (s->u_reg_u_size_len_ch0_q) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_qs = (s->u_reg_u_size_len_ch0_q) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_wr_en_data_arb_q = (s->u_reg_u_size_len_ch0_q) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch0_qs = (s->u_reg_u_size_len_ch0_qs) & ((1ULL << 6) - 1);
    s->u_reg_size_len_ch0_qs = (s->u_reg_u_size_len_ch0_qs) & ((1ULL << 6) - 1);
    s->u_reg_reg_rdata_next = (((((((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x3FULL) | (((s->u_reg_size_len_ch0_qs) & 0x3FULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_size_len_ch0_ds = (((s->u_reg_u_size_len_ch0_wr_en) ? (s->u_reg_u_size_len_ch0_wr_data) : (s->u_reg_u_size_len_ch0_qs))) & ((1ULL << 6) - 1);
    s->u_reg_u_size_reps_ch0_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_we = (s->u_reg_size_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 6) & 0x3FF)) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_d = (0) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_rst_ni = (s->u_reg_u_size_reps_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_we = (s->u_reg_u_size_reps_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd = (s->u_reg_u_size_reps_ch0_wd) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_de = (s->u_reg_u_size_reps_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_d = (s->u_reg_u_size_reps_ch0_d) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd = (s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_d = (s->u_reg_u_size_reps_ch0_wr_en_data_arb_d) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_size_reps_ch0_wr_en_data_arb_we) | (s->u_reg_u_size_reps_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_wr_en = (s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_data = (((s->u_reg_u_size_reps_ch0_wr_en_data_arb_we) ? (s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd) : (s->u_reg_u_size_reps_ch0_wr_en_data_arb_d))) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_wr_data = (s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_data) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_qe = (s->u_reg_u_size_reps_ch0_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch0_q = (s->u_reg_u_size_reps_ch0_q) & ((1ULL << 10) - 1);
    s->u_reg_reg2hw_size_reps_ch0_q = (s->u_reg_u_size_reps_ch0_q) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_qs = (s->u_reg_u_size_reps_ch0_q) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_q = (s->u_reg_u_size_reps_ch0_q) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch0_qs = (s->u_reg_u_size_reps_ch0_qs) & ((1ULL << 10) - 1);
    s->u_reg_size_reps_ch0_qs = (s->u_reg_u_size_reps_ch0_qs) & ((1ULL << 10) - 1);
    s->u_reg_reg_rdata_next = (((((((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0xFFC0ULL) | (((s->u_reg_size_reps_ch0_qs) & 0x3FFULL) << 6)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_size_reps_ch0_ds = (((s->u_reg_u_size_reps_ch0_wr_en) ? (s->u_reg_u_size_reps_ch0_wr_data) : (s->u_reg_u_size_reps_ch0_qs))) & ((1ULL << 10) - 1);
    s->u_reg_u_size_len_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_we = (s->u_reg_size_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 16) & 0x3F)) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_d = (0) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_rst_ni = (s->u_reg_u_size_len_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_we = (s->u_reg_u_size_len_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wd = (s->u_reg_u_size_len_ch1_wd) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_de = (s->u_reg_u_size_len_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_d = (s->u_reg_u_size_len_ch1_d) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wd = (s->u_reg_u_size_len_ch1_wr_en_data_arb_wd) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_d = (s->u_reg_u_size_len_ch1_wr_en_data_arb_d) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_size_len_ch1_wr_en_data_arb_we) | (s->u_reg_u_size_len_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_wr_en = (s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_data = (((s->u_reg_u_size_len_ch1_wr_en_data_arb_we) ? (s->u_reg_u_size_len_ch1_wr_en_data_arb_wd) : (s->u_reg_u_size_len_ch1_wr_en_data_arb_d))) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_wr_data = (s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_data) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_qe = (s->u_reg_u_size_len_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_len_ch1_q = (s->u_reg_u_size_len_ch1_q) & ((1ULL << 6) - 1);
    s->u_reg_reg2hw_size_len_ch1_q = (s->u_reg_u_size_len_ch1_q) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_qs = (s->u_reg_u_size_len_ch1_q) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_wr_en_data_arb_q = (s->u_reg_u_size_len_ch1_q) & ((1ULL << 6) - 1);
    s->u_reg_u_size_len_ch1_qs = (s->u_reg_u_size_len_ch1_qs) & ((1ULL << 6) - 1);
    s->u_reg_size_len_ch1_qs = (s->u_reg_u_size_len_ch1_qs) & ((1ULL << 6) - 1);
    s->u_reg_reg_rdata_next = (((((((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x3F0000ULL) | (((s->u_reg_size_len_ch1_qs) & 0x3FULL) << 16)) : s->u_reg_reg_rdata_next);
    s->u_reg_u_size_len_ch1_ds = (((s->u_reg_u_size_len_ch1_wr_en) ? (s->u_reg_u_size_len_ch1_wr_data) : (s->u_reg_u_size_len_ch1_qs))) & ((1ULL << 6) - 1);
    s->u_reg_u_size_reps_ch1_clk_i = (s->u_reg_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_rst_ni = (s->u_reg_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_we = (s->u_reg_size_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_wd = ((((s->u_reg_u_reg_if_wdata_o) >> 22) & 0x3FF)) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_de = (0) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_d = (0) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_rst_ni = (s->u_reg_u_size_reps_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_we = (s->u_reg_u_size_reps_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd = (s->u_reg_u_size_reps_ch1_wd) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_de = (s->u_reg_u_size_reps_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_d = (s->u_reg_u_size_reps_ch1_d) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd = (s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_d = (s->u_reg_u_size_reps_ch1_wr_en_data_arb_d) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_size_reps_ch1_wr_en_data_arb_we) | (s->u_reg_u_size_reps_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_wr_en = (s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_data = (((s->u_reg_u_size_reps_ch1_wr_en_data_arb_we) ? (s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd) : (s->u_reg_u_size_reps_ch1_wr_en_data_arb_d))) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_wr_data = (s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_data) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_qe = (s->u_reg_u_size_reps_ch1_we) & ((1ULL << 1) - 1);
    s->u_reg_u_size_reps_ch1_q = (s->u_reg_u_size_reps_ch1_q) & ((1ULL << 10) - 1);
    s->u_reg_reg2hw_size_reps_ch1_q = (s->u_reg_u_size_reps_ch1_q) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_qs = (s->u_reg_u_size_reps_ch1_q) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_q = (s->u_reg_u_size_reps_ch1_q) & ((1ULL << 10) - 1);
    s->u_reg_u_size_reps_ch1_qs = (s->u_reg_u_size_reps_ch1_qs) & ((1ULL << 10) - 1);
    s->u_reg_size_reps_ch1_qs = (s->u_reg_u_size_reps_ch1_qs) & ((1ULL << 10) - 1);
    s->u_reg_reg_rdata_next = (((((((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0xFFC00000ULL) | (((s->u_reg_size_reps_ch1_qs) & 0x3FFULL) << 22)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((((((((((!(((((s->u_reg_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1))))) ? (-1) : s->u_reg_reg_rdata_next);
    s->u_reg_u_reg_if_rdata_i = s->u_reg_reg_rdata_next;
    s->u_reg_u_reg_if_rdata_i = s->u_reg_u_reg_if_rdata_i;
    s->u_reg_u_size_reps_ch1_ds = (((s->u_reg_u_size_reps_ch1_wr_en) ? (s->u_reg_u_size_reps_ch1_wr_data) : (s->u_reg_u_size_reps_ch1_qs))) & ((1ULL << 10) - 1);
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
    s->u_reg_reg2hw_intr_state_done_ch1_q = (s->u_reg_reg2hw_intr_state_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_state_done_ch0_q = (s->u_reg_reg2hw_intr_state_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_enable_done_ch1_q = (s->u_reg_reg2hw_intr_enable_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_enable_done_ch0_q = (s->u_reg_reg2hw_intr_enable_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test_done_ch1_q = (s->u_reg_reg2hw_intr_test_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test_done_ch1_qe = (s->u_reg_reg2hw_intr_test_done_ch1_qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test_done_ch0_q = (s->u_reg_reg2hw_intr_test_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_intr_test_done_ch0_qe = (s->u_reg_reg2hw_intr_test_done_ch0_qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_alert_test_q = (s->u_reg_reg2hw_alert_test_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_alert_test_qe = (s->u_reg_reg2hw_alert_test_qe) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pda_ch1_q = (s->u_reg_reg2hw_ctrl_inactive_level_pda_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch1_q = (s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pda_ch0_q = (s->u_reg_reg2hw_ctrl_inactive_level_pda_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch0_q = (s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_polarity_ch1_q = (s->u_reg_reg2hw_ctrl_polarity_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_polarity_ch0_q = (s->u_reg_reg2hw_ctrl_polarity_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_enable_ch1_q = (s->u_reg_reg2hw_ctrl_enable_ch1_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_ctrl_enable_ch0_q = (s->u_reg_reg2hw_ctrl_enable_ch0_q) & ((1ULL << 1) - 1);
    s->u_reg_reg2hw_prediv_ch0_q = s->u_reg_reg2hw_prediv_ch0_q;
    s->u_reg_reg2hw_prediv_ch1_q = s->u_reg_reg2hw_prediv_ch1_q;
    s->u_reg_reg2hw_data_ch0_1__q = ((((uint64_t)(s->u_reg_reg2hw_data_ch0_1__q)) & ((1ULL << 32) - 1)));
    s->u_reg_reg2hw_data_ch0_0__q = ((((uint64_t)(s->u_reg_reg2hw_data_ch0_0__q)) & ((1ULL << 32) - 1)));
    s->u_reg_reg2hw_data_ch1_1__q = ((((uint64_t)(s->u_reg_reg2hw_data_ch1_1__q)) & ((1ULL << 32) - 1)));
    s->u_reg_reg2hw_data_ch1_0__q = ((((uint64_t)(s->u_reg_reg2hw_data_ch1_0__q)) & ((1ULL << 32) - 1)));
    s->u_reg_reg2hw_size_reps_ch1_q = (s->u_reg_reg2hw_size_reps_ch1_q) & ((1ULL << 10) - 1);
    s->u_reg_reg2hw_size_len_ch1_q = (s->u_reg_reg2hw_size_len_ch1_q) & ((1ULL << 6) - 1);
    s->u_reg_reg2hw_size_reps_ch0_q = (s->u_reg_reg2hw_size_reps_ch0_q) & ((1ULL << 10) - 1);
    s->u_reg_reg2hw_size_len_ch0_q = (s->u_reg_reg2hw_size_len_ch0_q) & ((1ULL << 6) - 1);
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
    s->u_pattgen_core_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_state_done_ch1_q = (s->u_reg_reg2hw_intr_state_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_state_done_ch0_q = (s->u_reg_reg2hw_intr_state_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_enable_done_ch1_q = (s->u_reg_reg2hw_intr_enable_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_enable_done_ch0_q = (s->u_reg_reg2hw_intr_enable_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_test_done_ch1_q = (s->u_reg_reg2hw_intr_test_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_test_done_ch1_qe = (s->u_reg_reg2hw_intr_test_done_ch1_qe) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_test_done_ch0_q = (s->u_reg_reg2hw_intr_test_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_intr_test_done_ch0_qe = (s->u_reg_reg2hw_intr_test_done_ch0_qe) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_alert_test_q = (s->u_reg_reg2hw_alert_test_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_alert_test_qe = (s->u_reg_reg2hw_alert_test_qe) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch1_q = (s->u_reg_reg2hw_ctrl_inactive_level_pda_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch1_q = (s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch0_q = (s->u_reg_reg2hw_ctrl_inactive_level_pda_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch0_q = (s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_polarity_ch1_q = (s->u_reg_reg2hw_ctrl_polarity_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_polarity_ch0_q = (s->u_reg_reg2hw_ctrl_polarity_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_enable_ch1_q = (s->u_reg_reg2hw_ctrl_enable_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_ctrl_enable_ch0_q = (s->u_reg_reg2hw_ctrl_enable_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_reg2hw_prediv_ch0_q = s->u_reg_reg2hw_prediv_ch0_q;
    s->u_pattgen_core_reg2hw_prediv_ch1_q = s->u_reg_reg2hw_prediv_ch1_q;
    s->u_pattgen_core_reg2hw_data_ch0_0__q = s->u_reg_reg2hw_data_ch0_0__q;
    s->u_pattgen_core_reg2hw_data_ch0_1__q = s->u_reg_reg2hw_data_ch0_1__q;
    s->u_pattgen_core_reg2hw_data_ch1_0__q = s->u_reg_reg2hw_data_ch1_0__q;
    s->u_pattgen_core_reg2hw_data_ch1_1__q = s->u_reg_reg2hw_data_ch1_1__q;
    s->u_pattgen_core_reg2hw_size_reps_ch1_q = (s->u_reg_reg2hw_size_reps_ch1_q) & ((1ULL << 10) - 1);
    s->u_pattgen_core_reg2hw_size_len_ch1_q = (s->u_reg_reg2hw_size_len_ch1_q) & ((1ULL << 6) - 1);
    s->u_pattgen_core_reg2hw_size_reps_ch0_q = (s->u_reg_reg2hw_size_reps_ch0_q) & ((1ULL << 10) - 1);
    s->u_pattgen_core_reg2hw_size_len_ch0_q = (s->u_reg_reg2hw_size_len_ch0_q) & ((1ULL << 6) - 1);
    s->u_pattgen_core_ch0_ctrl_enable = (s->u_pattgen_core_reg2hw_ctrl_enable_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch0_ctrl_polarity = (s->u_pattgen_core_reg2hw_ctrl_polarity_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch0_ctrl_inactive_level_pcl = (s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch0_ctrl_inactive_level_pda = (s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch0_ctrl_data = (s->u_pattgen_core_ch0_ctrl_data & ~0xFFFFFFFF00000000ULL) | (((s->u_pattgen_core_reg2hw_data_ch0_1__q) & 0xFFFFFFFFULL) << 32);
    s->u_pattgen_core_ch0_ctrl_data = (s->u_pattgen_core_ch0_ctrl_data & ~0xFFFFFFFFULL) | (((s->u_pattgen_core_reg2hw_data_ch0_0__q) & 0xFFFFFFFFULL) << 0);
    s->u_pattgen_core_ch0_ctrl_prediv = s->u_pattgen_core_reg2hw_prediv_ch0_q;
    s->u_pattgen_core_ch0_ctrl_len = (s->u_pattgen_core_reg2hw_size_len_ch0_q) & ((1ULL << 6) - 1);
    s->u_pattgen_core_ch0_ctrl_reps = (s->u_pattgen_core_reg2hw_size_reps_ch0_q) & ((1ULL << 10) - 1);
    s->u_pattgen_core_ch1_ctrl_enable = (s->u_pattgen_core_reg2hw_ctrl_enable_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch1_ctrl_polarity = (s->u_pattgen_core_reg2hw_ctrl_polarity_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch1_ctrl_inactive_level_pcl = (s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch1_ctrl_inactive_level_pda = (s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_ch1_ctrl_data = (s->u_pattgen_core_ch1_ctrl_data & ~0xFFFFFFFF00000000ULL) | (((s->u_pattgen_core_reg2hw_data_ch1_1__q) & 0xFFFFFFFFULL) << 32);
    s->u_pattgen_core_ch1_ctrl_data = (s->u_pattgen_core_ch1_ctrl_data & ~0xFFFFFFFFULL) | (((s->u_pattgen_core_reg2hw_data_ch1_0__q) & 0xFFFFFFFFULL) << 0);
    s->u_pattgen_core_ch1_ctrl_prediv = s->u_pattgen_core_reg2hw_prediv_ch1_q;
    s->u_pattgen_core_ch1_ctrl_len = (s->u_pattgen_core_reg2hw_size_len_ch1_q) & ((1ULL << 6) - 1);
    s->u_pattgen_core_ch1_ctrl_reps = (s->u_pattgen_core_reg2hw_size_reps_ch1_q) & ((1ULL << 10) - 1);
    s->u_pattgen_core_chan0_clk_i = (s->u_pattgen_core_clk_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_rst_ni = (s->u_pattgen_core_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_enable = (s->u_pattgen_core_ch0_ctrl_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_polarity = (s->u_pattgen_core_ch0_ctrl_polarity) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl = (s->u_pattgen_core_ch0_ctrl_inactive_level_pcl) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda = (s->u_pattgen_core_ch0_ctrl_inactive_level_pda) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_prediv = s->u_pattgen_core_ch0_ctrl_prediv;
    s->u_pattgen_core_chan0_ctrl_i_data = s->u_pattgen_core_ch0_ctrl_data;
    s->u_pattgen_core_chan0_ctrl_i_len = (s->u_pattgen_core_ch0_ctrl_len) & ((1ULL << 6) - 1);
    s->u_pattgen_core_chan0_ctrl_i_reps = (s->u_pattgen_core_ch0_ctrl_reps) & ((1ULL << 10) - 1);
    s->u_pattgen_core_chan0_clk_en = (((s->u_pattgen_core_chan0_complete_q) ^ 1)) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_prediv_clk_rollover = (((s->u_pattgen_core_chan0_clk_cnt_q) == (s->u_pattgen_core_chan0_prediv_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_rst_ni = (s->u_pattgen_core_chan0_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_enable = (s->u_pattgen_core_chan0_ctrl_i_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_enable = (s->u_pattgen_core_chan0_ctrl_i_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_clk_cnt_d = (((((s->u_pattgen_core_chan0_enable) ^ 1)) | (s->u_pattgen_core_chan0_prediv_clk_rollover)) ? (0) : ((s->u_pattgen_core_chan0_clk_cnt_q) + (1)));
    s->u_pattgen_core_chan0_pcl_int_d = ((s->u_pattgen_core_chan0_enable) & ((s->u_pattgen_core_chan0_prediv_clk_rollover) ^ (s->u_pattgen_core_chan0_pcl_int_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_bit_cnt_en = (((s->u_pattgen_core_chan0_pcl_int_q) & (s->u_pattgen_core_chan0_prediv_clk_rollover)) | (((s->u_pattgen_core_chan0_enable) ^ 1))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_bit_cnt_d = ((((((s->u_pattgen_core_chan0_enable) ^ 1)) | (((s->u_pattgen_core_chan0_bit_cnt_q) == (s->u_pattgen_core_chan0_len_q)))) ? (0) : ((s->u_pattgen_core_chan0_bit_cnt_q) + (1)))) & ((1ULL << 6) - 1);
    s->u_pattgen_core_chan0_rep_cnt_en = (((s->u_pattgen_core_chan0_bit_cnt_en) & (((s->u_pattgen_core_chan0_bit_cnt_q) == (s->u_pattgen_core_chan0_len_q)))) | (((s->u_pattgen_core_chan0_enable) ^ 1))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_rep_cnt_d = ((((((s->u_pattgen_core_chan0_enable) ^ 1)) | (((s->u_pattgen_core_chan0_rep_cnt_q) == (s->u_pattgen_core_chan0_reps_q)))) ? (0) : ((s->u_pattgen_core_chan0_rep_cnt_q) + (1)))) & ((1ULL << 10) - 1);
    s->u_pattgen_core_chan0_complete_en = (((s->u_pattgen_core_chan0_rep_cnt_en) & (((s->u_pattgen_core_chan0_rep_cnt_q) == (s->u_pattgen_core_chan0_reps_q)))) | (((s->u_pattgen_core_chan0_enable) ^ 1))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_complete_d = (s->u_pattgen_core_chan0_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_active_d = ((((s->u_pattgen_core_chan0_complete_q) ^ 1)) & ((s->u_pattgen_core_chan0_enable) | (s->u_pattgen_core_chan0_active_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_active = (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_active_d) : (s->u_pattgen_core_chan0_active_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_polarity = (s->u_pattgen_core_chan0_ctrl_i_polarity) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl = (s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda = (s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_ctrl_i_prediv = s->u_pattgen_core_chan0_ctrl_i_prediv;
    s->u_pattgen_core_chan0_ctrl_i_data = s->u_pattgen_core_chan0_ctrl_i_data;
    s->u_pattgen_core_chan0_ctrl_i_len = (s->u_pattgen_core_chan0_ctrl_i_len) & ((1ULL << 6) - 1);
    s->u_pattgen_core_chan0_ctrl_i_reps = (s->u_pattgen_core_chan0_ctrl_i_reps) & ((1ULL << 10) - 1);
    s->u_pattgen_core_chan0_pda_o = (s->u_pattgen_core_chan0_pda_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_pcl_o = (s->u_pattgen_core_chan0_pcl_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan0_event_done_o = (((s->u_pattgen_core_chan0_complete_q) & (((s->u_pattgen_core_chan0_complete_q2) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_clk_i = (s->u_pattgen_core_clk_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_rst_ni = (s->u_pattgen_core_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_enable = (s->u_pattgen_core_ch1_ctrl_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_polarity = (s->u_pattgen_core_ch1_ctrl_polarity) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl = (s->u_pattgen_core_ch1_ctrl_inactive_level_pcl) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda = (s->u_pattgen_core_ch1_ctrl_inactive_level_pda) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_prediv = s->u_pattgen_core_ch1_ctrl_prediv;
    s->u_pattgen_core_chan1_ctrl_i_data = s->u_pattgen_core_ch1_ctrl_data;
    s->u_pattgen_core_chan1_ctrl_i_len = (s->u_pattgen_core_ch1_ctrl_len) & ((1ULL << 6) - 1);
    s->u_pattgen_core_chan1_ctrl_i_reps = (s->u_pattgen_core_ch1_ctrl_reps) & ((1ULL << 10) - 1);
    s->u_pattgen_core_chan1_clk_en = (((s->u_pattgen_core_chan1_complete_q) ^ 1)) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_prediv_clk_rollover = (((s->u_pattgen_core_chan1_clk_cnt_q) == (s->u_pattgen_core_chan1_prediv_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_rst_ni = (s->u_pattgen_core_chan1_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_enable = (s->u_pattgen_core_chan1_ctrl_i_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_enable = (s->u_pattgen_core_chan1_ctrl_i_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_clk_cnt_d = (((((s->u_pattgen_core_chan1_enable) ^ 1)) | (s->u_pattgen_core_chan1_prediv_clk_rollover)) ? (0) : ((s->u_pattgen_core_chan1_clk_cnt_q) + (1)));
    s->u_pattgen_core_chan1_pcl_int_d = ((s->u_pattgen_core_chan1_enable) & ((s->u_pattgen_core_chan1_prediv_clk_rollover) ^ (s->u_pattgen_core_chan1_pcl_int_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_bit_cnt_en = (((s->u_pattgen_core_chan1_pcl_int_q) & (s->u_pattgen_core_chan1_prediv_clk_rollover)) | (((s->u_pattgen_core_chan1_enable) ^ 1))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_bit_cnt_d = ((((((s->u_pattgen_core_chan1_enable) ^ 1)) | (((s->u_pattgen_core_chan1_bit_cnt_q) == (s->u_pattgen_core_chan1_len_q)))) ? (0) : ((s->u_pattgen_core_chan1_bit_cnt_q) + (1)))) & ((1ULL << 6) - 1);
    s->u_pattgen_core_chan1_rep_cnt_en = (((s->u_pattgen_core_chan1_bit_cnt_en) & (((s->u_pattgen_core_chan1_bit_cnt_q) == (s->u_pattgen_core_chan1_len_q)))) | (((s->u_pattgen_core_chan1_enable) ^ 1))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_rep_cnt_d = ((((((s->u_pattgen_core_chan1_enable) ^ 1)) | (((s->u_pattgen_core_chan1_rep_cnt_q) == (s->u_pattgen_core_chan1_reps_q)))) ? (0) : ((s->u_pattgen_core_chan1_rep_cnt_q) + (1)))) & ((1ULL << 10) - 1);
    s->u_pattgen_core_chan1_complete_en = (((s->u_pattgen_core_chan1_rep_cnt_en) & (((s->u_pattgen_core_chan1_rep_cnt_q) == (s->u_pattgen_core_chan1_reps_q)))) | (((s->u_pattgen_core_chan1_enable) ^ 1))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_complete_d = (s->u_pattgen_core_chan1_enable) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_active_d = ((((s->u_pattgen_core_chan1_complete_q) ^ 1)) & ((s->u_pattgen_core_chan1_enable) | (s->u_pattgen_core_chan1_active_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_active = (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_active_d) : (s->u_pattgen_core_chan1_active_q))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_polarity = (s->u_pattgen_core_chan1_ctrl_i_polarity) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl = (s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda = (s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_ctrl_i_prediv = s->u_pattgen_core_chan1_ctrl_i_prediv;
    s->u_pattgen_core_chan1_ctrl_i_data = s->u_pattgen_core_chan1_ctrl_i_data;
    s->u_pattgen_core_chan1_ctrl_i_len = (s->u_pattgen_core_chan1_ctrl_i_len) & ((1ULL << 6) - 1);
    s->u_pattgen_core_chan1_ctrl_i_reps = (s->u_pattgen_core_chan1_ctrl_i_reps) & ((1ULL << 10) - 1);
    s->u_pattgen_core_chan1_pda_o = (s->u_pattgen_core_chan1_pda_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_pcl_o = (s->u_pattgen_core_chan1_pcl_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_chan1_event_done_o = (((s->u_pattgen_core_chan1_complete_q) & (((s->u_pattgen_core_chan1_complete_q2) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_clk_i = (s->u_pattgen_core_clk_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_rst_ni = (s->u_pattgen_core_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_event_intr_i = (s->u_pattgen_core_chan0_event_done_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i = (s->u_pattgen_core_reg2hw_intr_enable_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_q_i = (s->u_pattgen_core_reg2hw_intr_test_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_qe_i = (s->u_pattgen_core_reg2hw_intr_test_done_ch0_qe) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_state_q_i = (s->u_pattgen_core_reg2hw_intr_state_done_ch0_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_status = (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_state_q_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_rst_ni = (s->u_pattgen_core_intr_hw_done_ch0_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i = (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_de_o = (((((s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch0_event_intr_i))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_de = (s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_de_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_d_o = (((((((s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch0_event_intr_i))) | (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_state_q_i))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_d = (s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_d_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch0_intr_o = (s->u_pattgen_core_intr_hw_done_ch0_intr_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_clk_i = (s->u_pattgen_core_clk_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_rst_ni = (s->u_pattgen_core_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_event_intr_i = (s->u_pattgen_core_chan1_event_done_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i = (s->u_pattgen_core_reg2hw_intr_enable_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_q_i = (s->u_pattgen_core_reg2hw_intr_test_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_qe_i = (s->u_pattgen_core_reg2hw_intr_test_done_ch1_qe) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_state_q_i = (s->u_pattgen_core_reg2hw_intr_state_done_ch1_q) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_status = (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_state_q_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_rst_ni = (s->u_pattgen_core_intr_hw_done_ch1_rst_ni) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i = (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_de_o = (((((s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch1_event_intr_i))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_de = (s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_de_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_d_o = (((((((s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch1_event_intr_i))) | (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_state_q_i))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_d = (s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_d_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_hw_done_ch1_intr_o = (s->u_pattgen_core_intr_hw_done_ch1_intr_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_d = (s->u_pattgen_core_hw2reg_intr_state_done_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_intr_state_done_ch1_d = (s->u_pattgen_core_hw2reg_intr_state_done_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_d = (s->u_reg_hw2reg_intr_state_done_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d = (s->u_reg_u_intr_state_done_ch1_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d = (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_de = (s->u_pattgen_core_hw2reg_intr_state_done_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_intr_state_done_ch1_de = (s->u_pattgen_core_hw2reg_intr_state_done_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_de = (s->u_reg_hw2reg_intr_state_done_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_de = (s->u_reg_u_intr_state_done_ch1_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_en = (((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_we) | (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en = (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_data = (((((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_de) ? (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d) : (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q))) & (((((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_we) ^ (1))) | (((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd) ^ (1))))))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_wr_data = (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch1_ds = (((s->u_reg_u_intr_state_done_ch1_wr_en) ? (s->u_reg_u_intr_state_done_ch1_wr_data) : (s->u_reg_u_intr_state_done_ch1_qs))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_d = (s->u_pattgen_core_hw2reg_intr_state_done_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_intr_state_done_ch0_d = (s->u_pattgen_core_hw2reg_intr_state_done_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_d = (s->u_reg_hw2reg_intr_state_done_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d = (s->u_reg_u_intr_state_done_ch0_d) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d = (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_de = (s->u_pattgen_core_hw2reg_intr_state_done_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_hw2reg_intr_state_done_ch0_de = (s->u_pattgen_core_hw2reg_intr_state_done_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_de = (s->u_reg_hw2reg_intr_state_done_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_de = (s->u_reg_u_intr_state_done_ch0_de) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_en = (((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_we) | (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_de))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en = (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_data = (((((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_de) ? (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d) : (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q))) & (((((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_we) ^ (1))) | (((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd) ^ (1))))))) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_wr_data = (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_u_intr_state_done_ch0_ds = (((s->u_reg_u_intr_state_done_ch0_wr_en) ? (s->u_reg_u_intr_state_done_ch0_wr_data) : (s->u_reg_u_intr_state_done_ch0_qs))) & ((1ULL << 1) - 1);
    s->u_pattgen_core_pda0_tx_o = (s->u_pattgen_core_chan0_pda_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_pcl0_tx_o = (s->u_pattgen_core_chan0_pcl_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_pda1_tx_o = (s->u_pattgen_core_chan1_pda_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_pcl1_tx_o = (s->u_pattgen_core_chan1_pcl_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_done_ch0_o = (s->u_pattgen_core_intr_hw_done_ch0_intr_o) & ((1ULL << 1) - 1);
    s->u_pattgen_core_intr_done_ch1_o = (s->u_pattgen_core_intr_hw_done_ch1_intr_o) & ((1ULL << 1) - 1);
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
    s->cio_pda0_tx_o = (s->u_pattgen_core_pda0_tx_o) & ((1ULL << 1) - 1);
    s->cio_pcl0_tx_o = (s->u_pattgen_core_pcl0_tx_o) & ((1ULL << 1) - 1);
    s->cio_pda1_tx_o = (s->u_pattgen_core_pda1_tx_o) & ((1ULL << 1) - 1);
    s->cio_pcl1_tx_o = (s->u_pattgen_core_pcl1_tx_o) & ((1ULL << 1) - 1);
    s->cio_pda0_tx_en_o = (1) & ((1ULL << 1) - 1);
    s->cio_pcl0_tx_en_o = (1) & ((1ULL << 1) - 1);
    s->cio_pda1_tx_en_o = (1) & ((1ULL << 1) - 1);
    s->cio_pcl1_tx_en_o = (1) & ((1ULL << 1) - 1);
    s->intr_done_ch0_o = (s->u_pattgen_core_intr_done_ch0_o) & ((1ULL << 1) - 1);
    s->intr_done_ch1_o = (s->u_pattgen_core_intr_done_ch1_o) & ((1ULL << 1) - 1);
}

/*
 * tick() - Evaluate and atomically commit one sequential edge.
 *
 * Phase 1 computes every next-state value from the same old state.
 * Phase 2 commits all registers together, matching Verilog NBA
 * semantics. The return value reports whether sequential state changed.
 * ACCUMULATE counters are handled by ptimer instead.
 */
static bool tick(pattgen_state *s)
{
    bool _qp_changed = false;
    /* Phase 1: snapshot old state into next-state temporaries. */
    uint8_t _qp_next_u_reg_err_q = s->u_reg_err_q;
    uint8_t _qp_next_u_reg_u_reg_if_outstanding_q = s->u_reg_u_reg_if_outstanding_q;
    uint8_t _qp_next_u_reg_u_reg_if_reqid_q = s->u_reg_u_reg_if_reqid_q;
    uint8_t _qp_next_u_reg_u_reg_if_reqsz_q = s->u_reg_u_reg_if_reqsz_q;
    uint8_t _qp_next_u_reg_u_reg_if_rspop_q = s->u_reg_u_reg_if_rspop_q;
    uint32_t _qp_next_u_reg_u_reg_if_rdata_q = s->u_reg_u_reg_if_rdata_q;
    uint8_t _qp_next_u_reg_u_reg_if_error_q = s->u_reg_u_reg_if_error_q;
    uint8_t _qp_next_u_reg_u_intr_state_done_ch0_q = s->u_reg_u_intr_state_done_ch0_q;
    uint8_t _qp_next_u_reg_u_intr_state_done_ch1_q = s->u_reg_u_intr_state_done_ch1_q;
    uint8_t _qp_next_u_reg_u_intr_enable_done_ch0_q = s->u_reg_u_intr_enable_done_ch0_q;
    uint8_t _qp_next_u_reg_u_intr_enable_done_ch1_q = s->u_reg_u_intr_enable_done_ch1_q;
    uint8_t _qp_next_u_reg_u_ctrl_enable_ch0_q = s->u_reg_u_ctrl_enable_ch0_q;
    uint8_t _qp_next_u_reg_u_ctrl_enable_ch1_q = s->u_reg_u_ctrl_enable_ch1_q;
    uint8_t _qp_next_u_reg_u_ctrl_polarity_ch0_q = s->u_reg_u_ctrl_polarity_ch0_q;
    uint8_t _qp_next_u_reg_u_ctrl_polarity_ch1_q = s->u_reg_u_ctrl_polarity_ch1_q;
    uint8_t _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q = s->u_reg_u_ctrl_inactive_level_pcl_ch0_q;
    uint8_t _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q = s->u_reg_u_ctrl_inactive_level_pda_ch0_q;
    uint8_t _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q = s->u_reg_u_ctrl_inactive_level_pcl_ch1_q;
    uint8_t _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q = s->u_reg_u_ctrl_inactive_level_pda_ch1_q;
    uint32_t _qp_next_u_reg_u_prediv_ch0_q = s->u_reg_u_prediv_ch0_q;
    uint32_t _qp_next_u_reg_u_prediv_ch1_q = s->u_reg_u_prediv_ch1_q;
    uint32_t _qp_next_u_reg_u_data_ch0_0_q = s->u_reg_u_data_ch0_0_q;
    uint32_t _qp_next_u_reg_u_data_ch0_1_q = s->u_reg_u_data_ch0_1_q;
    uint32_t _qp_next_u_reg_u_data_ch1_0_q = s->u_reg_u_data_ch1_0_q;
    uint32_t _qp_next_u_reg_u_data_ch1_1_q = s->u_reg_u_data_ch1_1_q;
    uint8_t _qp_next_u_reg_u_size_len_ch0_q = s->u_reg_u_size_len_ch0_q;
    uint16_t _qp_next_u_reg_u_size_reps_ch0_q = s->u_reg_u_size_reps_ch0_q;
    uint8_t _qp_next_u_reg_u_size_len_ch1_q = s->u_reg_u_size_len_ch1_q;
    uint16_t _qp_next_u_reg_u_size_reps_ch1_q = s->u_reg_u_size_reps_ch1_q;
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
    uint8_t _qp_next_u_pattgen_core_chan0_polarity_q = s->u_pattgen_core_chan0_polarity_q;
    uint8_t _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q = s->u_pattgen_core_chan0_inactive_level_pcl_q;
    uint8_t _qp_next_u_pattgen_core_chan0_inactive_level_pda_q = s->u_pattgen_core_chan0_inactive_level_pda_q;
    uint32_t _qp_next_u_pattgen_core_chan0_prediv_q = s->u_pattgen_core_chan0_prediv_q;
    uint64_t _qp_next_u_pattgen_core_chan0_data_q = s->u_pattgen_core_chan0_data_q;
    uint8_t _qp_next_u_pattgen_core_chan0_len_q = s->u_pattgen_core_chan0_len_q;
    uint16_t _qp_next_u_pattgen_core_chan0_reps_q = s->u_pattgen_core_chan0_reps_q;
    uint8_t _qp_next_u_pattgen_core_chan0_pcl_o = s->u_pattgen_core_chan0_pcl_o;
    uint8_t _qp_next_u_pattgen_core_chan0_pda_o = s->u_pattgen_core_chan0_pda_o;
    uint32_t _qp_next_u_pattgen_core_chan0_clk_cnt_q = s->u_pattgen_core_chan0_clk_cnt_q;
    uint8_t _qp_next_u_pattgen_core_chan0_pcl_int_q = s->u_pattgen_core_chan0_pcl_int_q;
    uint8_t _qp_next_u_pattgen_core_chan0_bit_cnt_q = s->u_pattgen_core_chan0_bit_cnt_q;
    uint16_t _qp_next_u_pattgen_core_chan0_rep_cnt_q = s->u_pattgen_core_chan0_rep_cnt_q;
    uint8_t _qp_next_u_pattgen_core_chan0_complete_q = s->u_pattgen_core_chan0_complete_q;
    uint8_t _qp_next_u_pattgen_core_chan0_complete_q2 = s->u_pattgen_core_chan0_complete_q2;
    uint8_t _qp_next_u_pattgen_core_chan0_active_q = s->u_pattgen_core_chan0_active_q;
    uint8_t _qp_next_u_pattgen_core_chan1_polarity_q = s->u_pattgen_core_chan1_polarity_q;
    uint8_t _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q = s->u_pattgen_core_chan1_inactive_level_pcl_q;
    uint8_t _qp_next_u_pattgen_core_chan1_inactive_level_pda_q = s->u_pattgen_core_chan1_inactive_level_pda_q;
    uint32_t _qp_next_u_pattgen_core_chan1_prediv_q = s->u_pattgen_core_chan1_prediv_q;
    uint64_t _qp_next_u_pattgen_core_chan1_data_q = s->u_pattgen_core_chan1_data_q;
    uint8_t _qp_next_u_pattgen_core_chan1_len_q = s->u_pattgen_core_chan1_len_q;
    uint16_t _qp_next_u_pattgen_core_chan1_reps_q = s->u_pattgen_core_chan1_reps_q;
    uint8_t _qp_next_u_pattgen_core_chan1_pcl_o = s->u_pattgen_core_chan1_pcl_o;
    uint8_t _qp_next_u_pattgen_core_chan1_pda_o = s->u_pattgen_core_chan1_pda_o;
    uint32_t _qp_next_u_pattgen_core_chan1_clk_cnt_q = s->u_pattgen_core_chan1_clk_cnt_q;
    uint8_t _qp_next_u_pattgen_core_chan1_pcl_int_q = s->u_pattgen_core_chan1_pcl_int_q;
    uint8_t _qp_next_u_pattgen_core_chan1_bit_cnt_q = s->u_pattgen_core_chan1_bit_cnt_q;
    uint16_t _qp_next_u_pattgen_core_chan1_rep_cnt_q = s->u_pattgen_core_chan1_rep_cnt_q;
    uint8_t _qp_next_u_pattgen_core_chan1_complete_q = s->u_pattgen_core_chan1_complete_q;
    uint8_t _qp_next_u_pattgen_core_chan1_complete_q2 = s->u_pattgen_core_chan1_complete_q2;
    uint8_t _qp_next_u_pattgen_core_chan1_active_q = s->u_pattgen_core_chan1_active_q;
    uint8_t _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o = s->u_pattgen_core_intr_hw_done_ch0_intr_o;
    uint8_t _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o = s->u_pattgen_core_intr_hw_done_ch1_intr_o;

    /* Evaluate all next-state expressions from pre-edge state. */
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
    _qp_next_u_reg_u_intr_state_done_ch0_q = (((((s->u_reg_u_intr_state_done_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_state_done_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_state_done_ch0_q = ((((!(((s->u_reg_u_intr_state_done_ch0_rst_ni) ^ 1))) && (s->u_reg_u_intr_state_done_ch0_wr_en)) ? (s->u_reg_u_intr_state_done_ch0_wr_data) : _qp_next_u_reg_u_intr_state_done_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_state_done_ch1_q = (((((s->u_reg_u_intr_state_done_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_state_done_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_state_done_ch1_q = ((((!(((s->u_reg_u_intr_state_done_ch1_rst_ni) ^ 1))) && (s->u_reg_u_intr_state_done_ch1_wr_en)) ? (s->u_reg_u_intr_state_done_ch1_wr_data) : _qp_next_u_reg_u_intr_state_done_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_enable_done_ch0_q = (((((s->u_reg_u_intr_enable_done_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_enable_done_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_enable_done_ch0_q = ((((!(((s->u_reg_u_intr_enable_done_ch0_rst_ni) ^ 1))) && (s->u_reg_u_intr_enable_done_ch0_wr_en)) ? (s->u_reg_u_intr_enable_done_ch0_wr_data) : _qp_next_u_reg_u_intr_enable_done_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_enable_done_ch1_q = (((((s->u_reg_u_intr_enable_done_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_enable_done_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_intr_enable_done_ch1_q = ((((!(((s->u_reg_u_intr_enable_done_ch1_rst_ni) ^ 1))) && (s->u_reg_u_intr_enable_done_ch1_wr_en)) ? (s->u_reg_u_intr_enable_done_ch1_wr_data) : _qp_next_u_reg_u_intr_enable_done_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_enable_ch0_q = (((((s->u_reg_u_ctrl_enable_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_enable_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_enable_ch0_q = ((((!(((s->u_reg_u_ctrl_enable_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_enable_ch0_wr_en)) ? (s->u_reg_u_ctrl_enable_ch0_wr_data) : _qp_next_u_reg_u_ctrl_enable_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_enable_ch1_q = (((((s->u_reg_u_ctrl_enable_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_enable_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_enable_ch1_q = ((((!(((s->u_reg_u_ctrl_enable_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_enable_ch1_wr_en)) ? (s->u_reg_u_ctrl_enable_ch1_wr_data) : _qp_next_u_reg_u_ctrl_enable_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_polarity_ch0_q = (((((s->u_reg_u_ctrl_polarity_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_polarity_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_polarity_ch0_q = ((((!(((s->u_reg_u_ctrl_polarity_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_polarity_ch0_wr_en)) ? (s->u_reg_u_ctrl_polarity_ch0_wr_data) : _qp_next_u_reg_u_ctrl_polarity_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_polarity_ch1_q = (((((s->u_reg_u_ctrl_polarity_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_polarity_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_polarity_ch1_q = ((((!(((s->u_reg_u_ctrl_polarity_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_polarity_ch1_wr_en)) ? (s->u_reg_u_ctrl_polarity_ch1_wr_data) : _qp_next_u_reg_u_ctrl_polarity_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q = (((((s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q = ((((!(((s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q = (((((s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q = ((((!(((s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q = (((((s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q = ((((!(((s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q = (((((s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q = ((((!(((s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_u_prediv_ch0_q = ((((s->u_reg_u_prediv_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_prediv_ch0_q);
    _qp_next_u_reg_u_prediv_ch0_q = (((!(((s->u_reg_u_prediv_ch0_rst_ni) ^ 1))) && (s->u_reg_u_prediv_ch0_wr_en)) ? (s->u_reg_u_prediv_ch0_wr_data) : _qp_next_u_reg_u_prediv_ch0_q);
    _qp_next_u_reg_u_prediv_ch1_q = ((((s->u_reg_u_prediv_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_prediv_ch1_q);
    _qp_next_u_reg_u_prediv_ch1_q = (((!(((s->u_reg_u_prediv_ch1_rst_ni) ^ 1))) && (s->u_reg_u_prediv_ch1_wr_en)) ? (s->u_reg_u_prediv_ch1_wr_data) : _qp_next_u_reg_u_prediv_ch1_q);
    _qp_next_u_reg_u_data_ch0_0_q = ((((s->u_reg_u_data_ch0_0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_data_ch0_0_q);
    _qp_next_u_reg_u_data_ch0_0_q = (((!(((s->u_reg_u_data_ch0_0_rst_ni) ^ 1))) && (s->u_reg_u_data_ch0_0_wr_en)) ? (s->u_reg_u_data_ch0_0_wr_data) : _qp_next_u_reg_u_data_ch0_0_q);
    _qp_next_u_reg_u_data_ch0_1_q = ((((s->u_reg_u_data_ch0_1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_data_ch0_1_q);
    _qp_next_u_reg_u_data_ch0_1_q = (((!(((s->u_reg_u_data_ch0_1_rst_ni) ^ 1))) && (s->u_reg_u_data_ch0_1_wr_en)) ? (s->u_reg_u_data_ch0_1_wr_data) : _qp_next_u_reg_u_data_ch0_1_q);
    _qp_next_u_reg_u_data_ch1_0_q = ((((s->u_reg_u_data_ch1_0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_data_ch1_0_q);
    _qp_next_u_reg_u_data_ch1_0_q = (((!(((s->u_reg_u_data_ch1_0_rst_ni) ^ 1))) && (s->u_reg_u_data_ch1_0_wr_en)) ? (s->u_reg_u_data_ch1_0_wr_data) : _qp_next_u_reg_u_data_ch1_0_q);
    _qp_next_u_reg_u_data_ch1_1_q = ((((s->u_reg_u_data_ch1_1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_data_ch1_1_q);
    _qp_next_u_reg_u_data_ch1_1_q = (((!(((s->u_reg_u_data_ch1_1_rst_ni) ^ 1))) && (s->u_reg_u_data_ch1_1_wr_en)) ? (s->u_reg_u_data_ch1_1_wr_data) : _qp_next_u_reg_u_data_ch1_1_q);
    _qp_next_u_reg_u_size_len_ch0_q = (((((s->u_reg_u_size_len_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_len_ch0_q)) & ((1ULL << 6) - 1);
    _qp_next_u_reg_u_size_len_ch0_q = ((((!(((s->u_reg_u_size_len_ch0_rst_ni) ^ 1))) && (s->u_reg_u_size_len_ch0_wr_en)) ? (s->u_reg_u_size_len_ch0_wr_data) : _qp_next_u_reg_u_size_len_ch0_q)) & ((1ULL << 6) - 1);
    _qp_next_u_reg_u_size_reps_ch0_q = (((((s->u_reg_u_size_reps_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_reps_ch0_q)) & ((1ULL << 10) - 1);
    _qp_next_u_reg_u_size_reps_ch0_q = ((((!(((s->u_reg_u_size_reps_ch0_rst_ni) ^ 1))) && (s->u_reg_u_size_reps_ch0_wr_en)) ? (s->u_reg_u_size_reps_ch0_wr_data) : _qp_next_u_reg_u_size_reps_ch0_q)) & ((1ULL << 10) - 1);
    _qp_next_u_reg_u_size_len_ch1_q = (((((s->u_reg_u_size_len_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_len_ch1_q)) & ((1ULL << 6) - 1);
    _qp_next_u_reg_u_size_len_ch1_q = ((((!(((s->u_reg_u_size_len_ch1_rst_ni) ^ 1))) && (s->u_reg_u_size_len_ch1_wr_en)) ? (s->u_reg_u_size_len_ch1_wr_data) : _qp_next_u_reg_u_size_len_ch1_q)) & ((1ULL << 6) - 1);
    _qp_next_u_reg_u_size_reps_ch1_q = (((((s->u_reg_u_size_reps_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_reps_ch1_q)) & ((1ULL << 10) - 1);
    _qp_next_u_reg_u_size_reps_ch1_q = ((((!(((s->u_reg_u_size_reps_ch1_rst_ni) ^ 1))) && (s->u_reg_u_size_reps_ch1_wr_en)) ? (s->u_reg_u_size_reps_ch1_wr_data) : _qp_next_u_reg_u_size_reps_ch1_q)) & ((1ULL << 10) - 1);
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
    _qp_next_u_pattgen_core_chan0_polarity_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_polarity_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_inactive_level_pda_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_inactive_level_pda_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_prediv_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_prediv_q);
    _qp_next_u_pattgen_core_chan0_data_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_data_q);
    _qp_next_u_pattgen_core_chan0_len_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_len_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan0_reps_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_reps_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan0_polarity_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_polarity_q) : (s->u_pattgen_core_chan0_ctrl_i_polarity))) : _qp_next_u_pattgen_core_chan0_polarity_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_inactive_level_pcl_q) : (s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl))) : _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_inactive_level_pda_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_inactive_level_pda_q) : (s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda))) : _qp_next_u_pattgen_core_chan0_inactive_level_pda_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_prediv_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_prediv_q) : (s->u_pattgen_core_chan0_ctrl_i_prediv))) : _qp_next_u_pattgen_core_chan0_prediv_q);
    _qp_next_u_pattgen_core_chan0_data_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_data_q) : (s->u_pattgen_core_chan0_ctrl_i_data))) : _qp_next_u_pattgen_core_chan0_data_q);
    _qp_next_u_pattgen_core_chan0_len_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_len_q) : (s->u_pattgen_core_chan0_ctrl_i_len))) : _qp_next_u_pattgen_core_chan0_len_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan0_reps_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_reps_q) : (s->u_pattgen_core_chan0_ctrl_i_reps))) : _qp_next_u_pattgen_core_chan0_reps_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan0_pcl_o = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_pcl_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_pda_o = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_pda_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_pcl_o = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_active) ? ((s->u_pattgen_core_chan0_polarity_q) ^ (s->u_pattgen_core_chan0_pcl_int_q)) : (s->u_pattgen_core_chan0_inactive_level_pcl_q))) : _qp_next_u_pattgen_core_chan0_pcl_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_pda_o = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_active) ? ((((s->u_pattgen_core_chan0_data_q) >> (((((uint64_t)(0)) << 6) | ((uint64_t)(s->u_pattgen_core_chan0_bit_cnt_q))))) & 1)) : (s->u_pattgen_core_chan0_inactive_level_pda_q))) : _qp_next_u_pattgen_core_chan0_pda_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_clk_cnt_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_clk_cnt_q);
    _qp_next_u_pattgen_core_chan0_clk_cnt_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_clk_en) ? (s->u_pattgen_core_chan0_clk_cnt_d) : (s->u_pattgen_core_chan0_clk_cnt_q))) : _qp_next_u_pattgen_core_chan0_clk_cnt_q);
    _qp_next_u_pattgen_core_chan0_pcl_int_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_pcl_int_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_pcl_int_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_clk_en) ? (s->u_pattgen_core_chan0_pcl_int_d) : (s->u_pattgen_core_chan0_pcl_int_q))) : _qp_next_u_pattgen_core_chan0_pcl_int_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_bit_cnt_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_bit_cnt_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan0_bit_cnt_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_bit_cnt_en) ? (s->u_pattgen_core_chan0_bit_cnt_d) : (s->u_pattgen_core_chan0_bit_cnt_q))) : _qp_next_u_pattgen_core_chan0_bit_cnt_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan0_rep_cnt_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_rep_cnt_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan0_rep_cnt_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_rep_cnt_en) ? (s->u_pattgen_core_chan0_rep_cnt_d) : (s->u_pattgen_core_chan0_rep_cnt_q))) : _qp_next_u_pattgen_core_chan0_rep_cnt_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan0_complete_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_complete_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_complete_q2 = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_complete_q2)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_complete_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_complete_en) ? (s->u_pattgen_core_chan0_complete_d) : (s->u_pattgen_core_chan0_complete_q))) : _qp_next_u_pattgen_core_chan0_complete_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_complete_q2 = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan0_complete_q) : _qp_next_u_pattgen_core_chan0_complete_q2)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_active_q = (((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_active_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan0_active_q = (((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan0_active_d) : _qp_next_u_pattgen_core_chan0_active_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_polarity_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_polarity_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_inactive_level_pda_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_inactive_level_pda_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_prediv_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_prediv_q);
    _qp_next_u_pattgen_core_chan1_data_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_data_q);
    _qp_next_u_pattgen_core_chan1_len_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_len_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan1_reps_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_reps_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan1_polarity_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_polarity_q) : (s->u_pattgen_core_chan1_ctrl_i_polarity))) : _qp_next_u_pattgen_core_chan1_polarity_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_inactive_level_pcl_q) : (s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl))) : _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_inactive_level_pda_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_inactive_level_pda_q) : (s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda))) : _qp_next_u_pattgen_core_chan1_inactive_level_pda_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_prediv_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_prediv_q) : (s->u_pattgen_core_chan1_ctrl_i_prediv))) : _qp_next_u_pattgen_core_chan1_prediv_q);
    _qp_next_u_pattgen_core_chan1_data_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_data_q) : (s->u_pattgen_core_chan1_ctrl_i_data))) : _qp_next_u_pattgen_core_chan1_data_q);
    _qp_next_u_pattgen_core_chan1_len_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_len_q) : (s->u_pattgen_core_chan1_ctrl_i_len))) : _qp_next_u_pattgen_core_chan1_len_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan1_reps_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_reps_q) : (s->u_pattgen_core_chan1_ctrl_i_reps))) : _qp_next_u_pattgen_core_chan1_reps_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan1_pcl_o = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_pcl_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_pda_o = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_pda_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_pcl_o = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_active) ? ((s->u_pattgen_core_chan1_polarity_q) ^ (s->u_pattgen_core_chan1_pcl_int_q)) : (s->u_pattgen_core_chan1_inactive_level_pcl_q))) : _qp_next_u_pattgen_core_chan1_pcl_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_pda_o = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_active) ? ((((s->u_pattgen_core_chan1_data_q) >> (((((uint64_t)(0)) << 6) | ((uint64_t)(s->u_pattgen_core_chan1_bit_cnt_q))))) & 1)) : (s->u_pattgen_core_chan1_inactive_level_pda_q))) : _qp_next_u_pattgen_core_chan1_pda_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_clk_cnt_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_clk_cnt_q);
    _qp_next_u_pattgen_core_chan1_clk_cnt_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_clk_en) ? (s->u_pattgen_core_chan1_clk_cnt_d) : (s->u_pattgen_core_chan1_clk_cnt_q))) : _qp_next_u_pattgen_core_chan1_clk_cnt_q);
    _qp_next_u_pattgen_core_chan1_pcl_int_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_pcl_int_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_pcl_int_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_clk_en) ? (s->u_pattgen_core_chan1_pcl_int_d) : (s->u_pattgen_core_chan1_pcl_int_q))) : _qp_next_u_pattgen_core_chan1_pcl_int_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_bit_cnt_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_bit_cnt_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan1_bit_cnt_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_bit_cnt_en) ? (s->u_pattgen_core_chan1_bit_cnt_d) : (s->u_pattgen_core_chan1_bit_cnt_q))) : _qp_next_u_pattgen_core_chan1_bit_cnt_q)) & ((1ULL << 6) - 1);
    _qp_next_u_pattgen_core_chan1_rep_cnt_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_rep_cnt_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan1_rep_cnt_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_rep_cnt_en) ? (s->u_pattgen_core_chan1_rep_cnt_d) : (s->u_pattgen_core_chan1_rep_cnt_q))) : _qp_next_u_pattgen_core_chan1_rep_cnt_q)) & ((1ULL << 10) - 1);
    _qp_next_u_pattgen_core_chan1_complete_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_complete_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_complete_q2 = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_complete_q2)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_complete_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_complete_en) ? (s->u_pattgen_core_chan1_complete_d) : (s->u_pattgen_core_chan1_complete_q))) : _qp_next_u_pattgen_core_chan1_complete_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_complete_q2 = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan1_complete_q) : _qp_next_u_pattgen_core_chan1_complete_q2)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_active_q = (((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_active_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_chan1_active_q = (((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan1_active_d) : _qp_next_u_pattgen_core_chan1_active_q)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o = (((((s->u_pattgen_core_intr_hw_done_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o = (((!(((s->u_pattgen_core_intr_hw_done_ch0_rst_ni) ^ 1))) ? ((s->u_pattgen_core_intr_hw_done_ch0_status) & (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i)) : _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o = (((((s->u_pattgen_core_intr_hw_done_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o)) & ((1ULL << 1) - 1);
    _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o = (((!(((s->u_pattgen_core_intr_hw_done_ch1_rst_ni) ^ 1))) ? ((s->u_pattgen_core_intr_hw_done_ch1_status) & (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i)) : _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o)) & ((1ULL << 1) - 1);

    /* Detect changes before committing ordinary registers. */
    _qp_changed |= _qp_next_u_reg_err_q != s->u_reg_err_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_outstanding_q != s->u_reg_u_reg_if_outstanding_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_reqid_q != s->u_reg_u_reg_if_reqid_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_reqsz_q != s->u_reg_u_reg_if_reqsz_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_rspop_q != s->u_reg_u_reg_if_rspop_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_rdata_q != s->u_reg_u_reg_if_rdata_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_error_q != s->u_reg_u_reg_if_error_q;
    _qp_changed |= _qp_next_u_reg_u_intr_state_done_ch0_q != s->u_reg_u_intr_state_done_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_intr_state_done_ch1_q != s->u_reg_u_intr_state_done_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_intr_enable_done_ch0_q != s->u_reg_u_intr_enable_done_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_intr_enable_done_ch1_q != s->u_reg_u_intr_enable_done_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_enable_ch0_q != s->u_reg_u_ctrl_enable_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_enable_ch1_q != s->u_reg_u_ctrl_enable_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_polarity_ch0_q != s->u_reg_u_ctrl_polarity_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_polarity_ch1_q != s->u_reg_u_ctrl_polarity_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q != s->u_reg_u_ctrl_inactive_level_pcl_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q != s->u_reg_u_ctrl_inactive_level_pda_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q != s->u_reg_u_ctrl_inactive_level_pcl_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q != s->u_reg_u_ctrl_inactive_level_pda_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_prediv_ch0_q != s->u_reg_u_prediv_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_prediv_ch1_q != s->u_reg_u_prediv_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_data_ch0_0_q != s->u_reg_u_data_ch0_0_q;
    _qp_changed |= _qp_next_u_reg_u_data_ch0_1_q != s->u_reg_u_data_ch0_1_q;
    _qp_changed |= _qp_next_u_reg_u_data_ch1_0_q != s->u_reg_u_data_ch1_0_q;
    _qp_changed |= _qp_next_u_reg_u_data_ch1_1_q != s->u_reg_u_data_ch1_1_q;
    _qp_changed |= _qp_next_u_reg_u_size_len_ch0_q != s->u_reg_u_size_len_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_size_reps_ch0_q != s->u_reg_u_size_reps_ch0_q;
    _qp_changed |= _qp_next_u_reg_u_size_len_ch1_q != s->u_reg_u_size_len_ch1_q;
    _qp_changed |= _qp_next_u_reg_u_size_reps_ch1_q != s->u_reg_u_size_reps_ch1_q;
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
    _qp_changed |= _qp_next_u_pattgen_core_chan0_polarity_q != s->u_pattgen_core_chan0_polarity_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q != s->u_pattgen_core_chan0_inactive_level_pcl_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_inactive_level_pda_q != s->u_pattgen_core_chan0_inactive_level_pda_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_prediv_q != s->u_pattgen_core_chan0_prediv_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_data_q != s->u_pattgen_core_chan0_data_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_len_q != s->u_pattgen_core_chan0_len_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_reps_q != s->u_pattgen_core_chan0_reps_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_pcl_o != s->u_pattgen_core_chan0_pcl_o;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_pda_o != s->u_pattgen_core_chan0_pda_o;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_clk_cnt_q != s->u_pattgen_core_chan0_clk_cnt_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_pcl_int_q != s->u_pattgen_core_chan0_pcl_int_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_bit_cnt_q != s->u_pattgen_core_chan0_bit_cnt_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_rep_cnt_q != s->u_pattgen_core_chan0_rep_cnt_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_complete_q != s->u_pattgen_core_chan0_complete_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_complete_q2 != s->u_pattgen_core_chan0_complete_q2;
    _qp_changed |= _qp_next_u_pattgen_core_chan0_active_q != s->u_pattgen_core_chan0_active_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_polarity_q != s->u_pattgen_core_chan1_polarity_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q != s->u_pattgen_core_chan1_inactive_level_pcl_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_inactive_level_pda_q != s->u_pattgen_core_chan1_inactive_level_pda_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_prediv_q != s->u_pattgen_core_chan1_prediv_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_data_q != s->u_pattgen_core_chan1_data_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_len_q != s->u_pattgen_core_chan1_len_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_reps_q != s->u_pattgen_core_chan1_reps_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_pcl_o != s->u_pattgen_core_chan1_pcl_o;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_pda_o != s->u_pattgen_core_chan1_pda_o;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_clk_cnt_q != s->u_pattgen_core_chan1_clk_cnt_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_pcl_int_q != s->u_pattgen_core_chan1_pcl_int_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_bit_cnt_q != s->u_pattgen_core_chan1_bit_cnt_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_rep_cnt_q != s->u_pattgen_core_chan1_rep_cnt_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_complete_q != s->u_pattgen_core_chan1_complete_q;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_complete_q2 != s->u_pattgen_core_chan1_complete_q2;
    _qp_changed |= _qp_next_u_pattgen_core_chan1_active_q != s->u_pattgen_core_chan1_active_q;
    _qp_changed |= _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o != s->u_pattgen_core_intr_hw_done_ch0_intr_o;
    _qp_changed |= _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o != s->u_pattgen_core_intr_hw_done_ch1_intr_o;

    /* Phase 2: commit all ordinary registers simultaneously. */
    s->u_reg_err_q = _qp_next_u_reg_err_q;
    s->u_reg_u_reg_if_outstanding_q = _qp_next_u_reg_u_reg_if_outstanding_q;
    s->u_reg_u_reg_if_reqid_q = _qp_next_u_reg_u_reg_if_reqid_q;
    s->u_reg_u_reg_if_reqsz_q = _qp_next_u_reg_u_reg_if_reqsz_q;
    s->u_reg_u_reg_if_rspop_q = _qp_next_u_reg_u_reg_if_rspop_q;
    s->u_reg_u_reg_if_rdata_q = _qp_next_u_reg_u_reg_if_rdata_q;
    s->u_reg_u_reg_if_error_q = _qp_next_u_reg_u_reg_if_error_q;
    s->u_reg_u_intr_state_done_ch0_q = _qp_next_u_reg_u_intr_state_done_ch0_q;
    s->u_reg_u_intr_state_done_ch1_q = _qp_next_u_reg_u_intr_state_done_ch1_q;
    s->u_reg_u_intr_enable_done_ch0_q = _qp_next_u_reg_u_intr_enable_done_ch0_q;
    s->u_reg_u_intr_enable_done_ch1_q = _qp_next_u_reg_u_intr_enable_done_ch1_q;
    s->u_reg_u_ctrl_enable_ch0_q = _qp_next_u_reg_u_ctrl_enable_ch0_q;
    s->u_reg_u_ctrl_enable_ch1_q = _qp_next_u_reg_u_ctrl_enable_ch1_q;
    s->u_reg_u_ctrl_polarity_ch0_q = _qp_next_u_reg_u_ctrl_polarity_ch0_q;
    s->u_reg_u_ctrl_polarity_ch1_q = _qp_next_u_reg_u_ctrl_polarity_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_q = _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_q = _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_q = _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_q = _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q;
    s->u_reg_u_prediv_ch0_q = _qp_next_u_reg_u_prediv_ch0_q;
    s->u_reg_u_prediv_ch1_q = _qp_next_u_reg_u_prediv_ch1_q;
    s->u_reg_u_data_ch0_0_q = _qp_next_u_reg_u_data_ch0_0_q;
    s->u_reg_u_data_ch0_1_q = _qp_next_u_reg_u_data_ch0_1_q;
    s->u_reg_u_data_ch1_0_q = _qp_next_u_reg_u_data_ch1_0_q;
    s->u_reg_u_data_ch1_1_q = _qp_next_u_reg_u_data_ch1_1_q;
    s->u_reg_u_size_len_ch0_q = _qp_next_u_reg_u_size_len_ch0_q;
    s->u_reg_u_size_reps_ch0_q = _qp_next_u_reg_u_size_reps_ch0_q;
    s->u_reg_u_size_len_ch1_q = _qp_next_u_reg_u_size_len_ch1_q;
    s->u_reg_u_size_reps_ch1_q = _qp_next_u_reg_u_size_reps_ch1_q;
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
    s->u_pattgen_core_chan0_polarity_q = _qp_next_u_pattgen_core_chan0_polarity_q;
    s->u_pattgen_core_chan0_inactive_level_pcl_q = _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q;
    s->u_pattgen_core_chan0_inactive_level_pda_q = _qp_next_u_pattgen_core_chan0_inactive_level_pda_q;
    s->u_pattgen_core_chan0_prediv_q = _qp_next_u_pattgen_core_chan0_prediv_q;
    s->u_pattgen_core_chan0_data_q = _qp_next_u_pattgen_core_chan0_data_q;
    s->u_pattgen_core_chan0_len_q = _qp_next_u_pattgen_core_chan0_len_q;
    s->u_pattgen_core_chan0_reps_q = _qp_next_u_pattgen_core_chan0_reps_q;
    s->u_pattgen_core_chan0_pcl_o = _qp_next_u_pattgen_core_chan0_pcl_o;
    s->u_pattgen_core_chan0_pda_o = _qp_next_u_pattgen_core_chan0_pda_o;
    s->u_pattgen_core_chan0_clk_cnt_q = _qp_next_u_pattgen_core_chan0_clk_cnt_q;
    s->u_pattgen_core_chan0_pcl_int_q = _qp_next_u_pattgen_core_chan0_pcl_int_q;
    s->u_pattgen_core_chan0_bit_cnt_q = _qp_next_u_pattgen_core_chan0_bit_cnt_q;
    s->u_pattgen_core_chan0_rep_cnt_q = _qp_next_u_pattgen_core_chan0_rep_cnt_q;
    s->u_pattgen_core_chan0_complete_q = _qp_next_u_pattgen_core_chan0_complete_q;
    s->u_pattgen_core_chan0_complete_q2 = _qp_next_u_pattgen_core_chan0_complete_q2;
    s->u_pattgen_core_chan0_active_q = _qp_next_u_pattgen_core_chan0_active_q;
    s->u_pattgen_core_chan1_polarity_q = _qp_next_u_pattgen_core_chan1_polarity_q;
    s->u_pattgen_core_chan1_inactive_level_pcl_q = _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q;
    s->u_pattgen_core_chan1_inactive_level_pda_q = _qp_next_u_pattgen_core_chan1_inactive_level_pda_q;
    s->u_pattgen_core_chan1_prediv_q = _qp_next_u_pattgen_core_chan1_prediv_q;
    s->u_pattgen_core_chan1_data_q = _qp_next_u_pattgen_core_chan1_data_q;
    s->u_pattgen_core_chan1_len_q = _qp_next_u_pattgen_core_chan1_len_q;
    s->u_pattgen_core_chan1_reps_q = _qp_next_u_pattgen_core_chan1_reps_q;
    s->u_pattgen_core_chan1_pcl_o = _qp_next_u_pattgen_core_chan1_pcl_o;
    s->u_pattgen_core_chan1_pda_o = _qp_next_u_pattgen_core_chan1_pda_o;
    s->u_pattgen_core_chan1_clk_cnt_q = _qp_next_u_pattgen_core_chan1_clk_cnt_q;
    s->u_pattgen_core_chan1_pcl_int_q = _qp_next_u_pattgen_core_chan1_pcl_int_q;
    s->u_pattgen_core_chan1_bit_cnt_q = _qp_next_u_pattgen_core_chan1_bit_cnt_q;
    s->u_pattgen_core_chan1_rep_cnt_q = _qp_next_u_pattgen_core_chan1_rep_cnt_q;
    s->u_pattgen_core_chan1_complete_q = _qp_next_u_pattgen_core_chan1_complete_q;
    s->u_pattgen_core_chan1_complete_q2 = _qp_next_u_pattgen_core_chan1_complete_q2;
    s->u_pattgen_core_chan1_active_q = _qp_next_u_pattgen_core_chan1_active_q;
    s->u_pattgen_core_intr_hw_done_ch0_intr_o = _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o;
    s->u_pattgen_core_intr_hw_done_ch1_intr_o = _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o;

    return _qp_changed;
}

/* Settle to quiescence (no bus access). */
void pattgen_settle(pattgen_state *s)
{
    {
    if (!s->_qp_busy) {
        s->_qp_busy = 1;
        unsigned _qp_ticks = 0;
        update_state(s);
        QPSettleFingerprint _qp_base = qp_settle_fingerprint(s);
        unsigned _qp_lam = 0, _qp_pow = 1;
        while (_qp_ticks < (s->_qp_hold_settle ? QP_SETTLE_BUDGET : 256u)) {
            bool _qp_ch = qp_tick(s);
            bool _qp_rw = s->_qp_rewound != 0;
            if (_qp_rw) { s->_qp_rewound = 0; _qp_ch = true; }
            if (!_qp_ch)
                break;  /* sequential fixed point reached */
            ++_qp_ticks;
            update_state(s);
            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            if (_qp_rw) {  /* deliberate repeat: move the camera here */
                _qp_base = _qp_now; _qp_lam = 0; _qp_pow = 1;
                continue;
            }
            ++_qp_lam;
            if (_qp_now.first == _qp_base.first &&
                _qp_now.second == _qp_base.second &&
                !s->_qp_hold_settle)
                break;  /* state revisited: periodic, no fixed point exists */
            if (_qp_lam == _qp_pow) {  /* Brent: move camera, double the wait */
                _qp_base = _qp_now; _qp_lam = 0;
                if (_qp_pow < (1u << 30)) _qp_pow <<= 1;
            }
        }
        if (_qp_ticks >= QP_SETTLE_BUDGET)
            qemu_log_mask(LOG_UNIMP, "qp settle: budget exhausted, state still changing (continues on next access)\n");
        s->_qp_busy = 0;
    }
    }
    update_state(s);

}

/* Advance exactly one model clock.  Pin-level transport bridges use
 * this instead of holding an MMIO settle loop open. */
void pattgen_step(pattgen_state *s)
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
void pattgen_update(pattgen_state *s)
{
    update_state(s);
}

void pattgen_tick(pattgen_state *s)
{
    qp_tick(s);
}

void pattgen_step_many(pattgen_state *s, unsigned count)
{
    update_state(s);
    while (count--) { qp_tick(s); update_state(s); }
}

/* Pulse reset to commit RESVALs into every prim_subreg. */
void pattgen_reset(pattgen_state *s)
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
uint64_t pattgen_read(void *opaque, hwaddr addr, unsigned size)
{
    pattgen_state *s = opaque;
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
        while (_qp_ticks < (s->_qp_hold_settle ? QP_SETTLE_BUDGET : 256u)) {
            bool _qp_ch = qp_tick(s);
            bool _qp_rw = s->_qp_rewound != 0;
            if (_qp_rw) { s->_qp_rewound = 0; _qp_ch = true; }
            if (!_qp_ch)
                break;  /* sequential fixed point reached */
            ++_qp_ticks;
            update_state(s);
            if (!s->_qp_rd_cap && s->u_reg_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->u_reg_tl_o_d_data; }
            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            if (_qp_rw) {  /* deliberate repeat: move the camera here */
                _qp_base = _qp_now; _qp_lam = 0; _qp_pow = 1;
                continue;
            }
            ++_qp_lam;
            if (_qp_now.first == _qp_base.first &&
                _qp_now.second == _qp_base.second &&
                !s->_qp_hold_settle)
                break;  /* state revisited: periodic, no fixed point exists */
            if (_qp_lam == _qp_pow) {  /* Brent: move camera, double the wait */
                _qp_base = _qp_now; _qp_lam = 0;
                if (_qp_pow < (1u << 30)) _qp_pow <<= 1;
            }
        }
        if (_qp_ticks >= QP_SETTLE_BUDGET)
            qemu_log_mask(LOG_UNIMP, "qp settle: budget exhausted, state still changing (continues on next access)\n");
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
void pattgen_write(void *opaque, hwaddr addr,
               uint64_t value, unsigned size)
{
    pattgen_state *s = opaque;
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
        while (_qp_ticks < (s->_qp_hold_settle ? QP_SETTLE_BUDGET : 256u)) {
            bool _qp_ch = qp_tick(s);
            bool _qp_rw = s->_qp_rewound != 0;
            if (_qp_rw) { s->_qp_rewound = 0; _qp_ch = true; }
            if (!_qp_ch)
                break;  /* sequential fixed point reached */
            ++_qp_ticks;
            update_state(s);
            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            if (_qp_rw) {  /* deliberate repeat: move the camera here */
                _qp_base = _qp_now; _qp_lam = 0; _qp_pow = 1;
                continue;
            }
            ++_qp_lam;
            if (_qp_now.first == _qp_base.first &&
                _qp_now.second == _qp_base.second &&
                !s->_qp_hold_settle)
                break;  /* state revisited: periodic, no fixed point exists */
            if (_qp_lam == _qp_pow) {  /* Brent: move camera, double the wait */
                _qp_base = _qp_now; _qp_lam = 0;
                if (_qp_pow < (1u << 30)) _qp_pow <<= 1;
            }
        }
        if (_qp_ticks >= QP_SETTLE_BUDGET)
            qemu_log_mask(LOG_UNIMP, "qp settle: budget exhausted, state still changing (continues on next access)\n");
        s->_qp_busy = 0;
    }
    }
    update_state(s);

}

static const MemoryRegionOps pattgen_ops = {
    .read  = pattgen_read,
    .write = pattgen_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void pattgen_realize(DeviceState *dev, Error **errp)
{
    pattgen_state *s = PATTGEN(dev);

    memory_region_init_io(&s->mmio, OBJECT(dev), &pattgen_ops, s,
                          "pattgen", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    /* Initialize state to zero */
    update_state(s);
}

static void pattgen_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = pattgen_realize;
}

static const TypeInfo pattgen_info = {
    .name          = TYPE_PATTGEN,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(pattgen_state),
    .class_init    = pattgen_class_init,
};

static void pattgen_register_types(void)
{
    type_register_static(&pattgen_info);
}

type_init(pattgen_register_types)

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
void pattgen_set_alert_rx_i_0__ping_p(pattgen_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_p = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void pattgen_set_alert_rx_i_0__ping_n(pattgen_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_n = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void pattgen_set_alert_rx_i_0__ack_p(pattgen_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_p = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void pattgen_set_alert_rx_i_0__ack_n(pattgen_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_n = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

