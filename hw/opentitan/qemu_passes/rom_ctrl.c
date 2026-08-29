/*
 * Auto-generated QEMU device: rom_ctrl
 *
 * Bus interface: protocol-agnostic (inferred from IR)
 *   addr   -> s->rom_tl_i_a_address
 *   wdata  -> s->rom_tl_i_a_data
 *   rdata  <- s->u_tl_adapter_rom_tl_o_d_data
 *   valid  -> s->rom_tl_i_a_valid  (pulsed 1 per txn — TL-UL a_valid)
 *   opcode -> s->rom_tl_i_a_opcode (write=0 PutFullData, read=4 Get)
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
#include "rom_ctrl.h"

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

static void update_state_once(rom_ctrl_state *s);
typedef struct QPSettleFingerprint {
    uint64_t first;
    uint64_t second;
} QPSettleFingerprint;
static QPSettleFingerprint qp_settle_fingerprint(const rom_ctrl_state *s);
static void update_state(rom_ctrl_state *s)
{
    update_state_once(s);
    update_state_once(s);
    update_state_once(s);
}
static bool tick(rom_ctrl_state *s);
/* One clock + per-clock observer hook (organs needing edge visibility). */
static inline bool qp_tick(rom_ctrl_state *s)
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

static QPSettleFingerprint qp_settle_fingerprint(const rom_ctrl_state *s)
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
static void update_state_once(rom_ctrl_state *s)
{
    s->rom_tl_i_a_valid = (s->rom_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->rom_tl_i_a_opcode = (s->rom_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->rom_tl_i_a_param = (s->rom_tl_i_a_param) & ((1ULL << 3) - 1);
    s->rom_tl_i_a_size = (s->rom_tl_i_a_size) & ((1ULL << 2) - 1);
    s->rom_tl_i_a_source = s->rom_tl_i_a_source;
    s->rom_tl_i_a_address = s->rom_tl_i_a_address;
    s->rom_tl_i_a_mask = (s->rom_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->rom_tl_i_a_data = s->rom_tl_i_a_data;
    s->rom_tl_i_a_user_rsvd = (s->rom_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->rom_tl_i_a_user_instr_type = (s->rom_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->rom_tl_i_a_user_cmd_intg = (s->rom_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->rom_tl_i_a_user_data_intg = (s->rom_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->rom_tl_i_d_ready = (s->rom_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_tl_rom_h2d_buf_in_i = (((((((((((((((__uint128_t)(s->rom_tl_i_d_ready)) << 0) | (((__uint128_t)(s->rom_tl_i_a_user_data_intg)) << 1)) | (((__uint128_t)(s->rom_tl_i_a_user_cmd_intg)) << 8)) | (((__uint128_t)(s->rom_tl_i_a_user_instr_type)) << 15)) | (((__uint128_t)(s->rom_tl_i_a_user_rsvd)) << 19)) | (((__uint128_t)(s->rom_tl_i_a_data)) << 24)) | (((__uint128_t)(s->rom_tl_i_a_mask)) << 56)) | (((__uint128_t)(s->rom_tl_i_a_address)) << 60)) | (((__uint128_t)(s->rom_tl_i_a_source)) << 92)) | (((__uint128_t)(s->rom_tl_i_a_size)) << 100)) | (((__uint128_t)(s->rom_tl_i_a_param)) << 102)) | (((__uint128_t)(s->rom_tl_i_a_opcode)) << 105)) | (((__uint128_t)(s->rom_tl_i_a_valid)) << 108));
    s->u_tl_rom_h2d_buf_out_o = s->u_tl_rom_h2d_buf_in_i;
    s->u_tl_adapter_rom_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_tl_i_a_valid = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 108) & ((1ULL << 1) - 1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_tl_i_a_opcode = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 105) & ((1ULL << 3) - 1))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_tl_i_a_param = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 102) & ((1ULL << 3) - 1))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_tl_i_a_size = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 100) & ((1ULL << 2) - 1))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_tl_i_a_source = ((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 92) & ((1ULL << 8) - 1));
    s->u_tl_adapter_rom_tl_i_a_address = ((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 60) & ((1ULL << 32) - 1));
    s->u_tl_adapter_rom_tl_i_a_mask = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 56) & ((1ULL << 4) - 1))) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_tl_i_a_data = ((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 24) & ((1ULL << 32) - 1));
    s->u_tl_adapter_rom_tl_i_a_user_rsvd = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 19) & ((1ULL << 5) - 1))) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_tl_i_a_user_instr_type = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 15) & ((1ULL << 4) - 1))) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_tl_i_a_user_cmd_intg = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 8) & ((1ULL << 7) - 1))) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_tl_i_a_user_data_intg = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 1) & ((1ULL << 7) - 1))) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_tl_i_d_ready = (((uint64_t)(((__uint128_t)s->u_tl_rom_h2d_buf_out_o) >> 0) & ((1ULL << 1) - 1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_en_ifetch_i = (6) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_rerror_i = (0) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_readback_en_i = (9) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_wr_collision_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_write_pending_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_sram_addr_rdata = (0) & ((1ULL << 12) - 1);
    s->u_tl_adapter_rom_d_valid = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_d_error = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_woffset = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_wmask_int_0_ = 0;
    s->u_tl_adapter_rom_wdata_int_0_ = 0;
    s->u_tl_adapter_rom_wmask_intg_0_ = (0) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_wdata_intg_0_ = (0) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_rdata_tlword = (180388626432) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_rst_ni = (s->u_tl_adapter_rom_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_valid = (s->u_tl_adapter_rom_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_opcode = (s->u_tl_adapter_rom_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_param = (s->u_tl_adapter_rom_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_size = (s->u_tl_adapter_rom_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_source = s->u_tl_adapter_rom_tl_i_a_source;
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_address = s->u_tl_adapter_rom_tl_i_a_address;
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_mask = (s->u_tl_adapter_rom_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_data = s->u_tl_adapter_rom_tl_i_a_data;
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_user_rsvd = (s->u_tl_adapter_rom_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_user_instr_type = (s->u_tl_adapter_rom_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_user_cmd_intg = (s->u_tl_adapter_rom_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_a_user_data_intg = (s->u_tl_adapter_rom_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_tl_i_d_ready = (s->u_tl_adapter_rom_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_err_o = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_intg_error = (s->u_tl_adapter_rom_gen_cmd_intg_check_u_cmd_intg_chk_err_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_clk_i = (s->u_tl_adapter_rom_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_rst_ni = (s->u_tl_adapter_rom_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_valid = (s->u_tl_adapter_rom_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_opcode = (s->u_tl_adapter_rom_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_param = (s->u_tl_adapter_rom_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_size = (s->u_tl_adapter_rom_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_source = s->u_tl_adapter_rom_tl_i_a_source;
    s->u_tl_adapter_rom_u_err_tl_i_a_address = s->u_tl_adapter_rom_tl_i_a_address;
    s->u_tl_adapter_rom_u_err_tl_i_a_mask = (s->u_tl_adapter_rom_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_data = s->u_tl_adapter_rom_tl_i_a_data;
    s->u_tl_adapter_rom_u_err_tl_i_a_user_rsvd = (s->u_tl_adapter_rom_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_user_instr_type = (s->u_tl_adapter_rom_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_user_cmd_intg = (s->u_tl_adapter_rom_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_user_data_intg = (s->u_tl_adapter_rom_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_d_ready = (s->u_tl_adapter_rom_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_addr_sz_chk = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_mask_chk = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_fulldata_chk = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_valid = (s->u_tl_adapter_rom_u_err_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_opcode = (s->u_tl_adapter_rom_u_err_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_param = (s->u_tl_adapter_rom_u_err_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_size = (s->u_tl_adapter_rom_u_err_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_err_addr_sz_chk = ((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0)))) ? (-1) : s->u_tl_adapter_rom_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_source = s->u_tl_adapter_rom_u_err_tl_i_a_source;
    s->u_tl_adapter_rom_u_err_tl_i_a_address = s->u_tl_adapter_rom_u_err_tl_i_a_address;
    s->u_tl_adapter_rom_u_err_mask = ((1) << (((((uint64_t)(0)) << 2) | ((uint64_t)(((s->u_tl_adapter_rom_u_err_tl_i_a_address) & 0x3)))))) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_err_addr_sz_chk = (((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1)))) ? (((((s->u_tl_adapter_rom_u_err_tl_i_a_address) & 1)) ^ 1)) : s->u_tl_adapter_rom_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_addr_sz_chk = ((((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (2)))) ? (((((s->u_tl_adapter_rom_u_err_tl_i_a_address) & 0x3)) == (0))) : s->u_tl_adapter_rom_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_addr_sz_chk = ((((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_tl_adapter_rom_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_mask = (s->u_tl_adapter_rom_u_err_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_err_mask_chk = ((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0)))) ? ((((s->u_tl_adapter_rom_u_err_tl_i_a_mask) & (((~(s->u_tl_adapter_rom_u_err_mask)) & 0xFULL))) == (0))) : s->u_tl_adapter_rom_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_fulldata_chk = ((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0)))) ? ((((s->u_tl_adapter_rom_u_err_tl_i_a_mask) & (s->u_tl_adapter_rom_u_err_mask)) != (0))) : s->u_tl_adapter_rom_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_mask_chk = (((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1)))) ? ((((((s->u_tl_adapter_rom_u_err_tl_i_a_address) >> 1) & 0x1)) ? (((((s->u_tl_adapter_rom_u_err_tl_i_a_mask) & 0x3)) == (0))) : ((((((s->u_tl_adapter_rom_u_err_tl_i_a_mask) >> 2) & 0x3)) == (0))))) : s->u_tl_adapter_rom_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_fulldata_chk = (((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1)))) ? ((((((s->u_tl_adapter_rom_u_err_tl_i_a_address) >> 1) & 0x1)) ? ((((((s->u_tl_adapter_rom_u_err_tl_i_a_mask) >> 2) & 0x3)) == (3))) : (((((s->u_tl_adapter_rom_u_err_tl_i_a_mask) & 0x3)) == (3))))) : s->u_tl_adapter_rom_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_mask_chk = ((((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (2)))) ? (-1) : s->u_tl_adapter_rom_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_fulldata_chk = ((((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (2)))) ? (((s->u_tl_adapter_rom_u_err_tl_i_a_mask) == (15))) : s->u_tl_adapter_rom_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_mask_chk = ((((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_tl_adapter_rom_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_fulldata_chk = ((((((s->u_tl_adapter_rom_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_tl_adapter_rom_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_data = s->u_tl_adapter_rom_u_err_tl_i_a_data;
    s->u_tl_adapter_rom_u_err_tl_i_a_user_rsvd = (s->u_tl_adapter_rom_u_err_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_user_instr_type = (s->u_tl_adapter_rom_u_err_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_user_cmd_intg = (s->u_tl_adapter_rom_u_err_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_a_user_data_intg = (s->u_tl_adapter_rom_u_err_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_err_tl_i_d_ready = (s->u_tl_adapter_rom_u_err_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_err_err_o = (((((((((((s->u_tl_adapter_rom_u_err_tl_i_a_opcode) == (0))) | (((s->u_tl_adapter_rom_u_err_tl_i_a_opcode) == (1))) | (((s->u_tl_adapter_rom_u_err_tl_i_a_opcode) == (4))))) & (s->u_tl_adapter_rom_u_err_addr_sz_chk) & (s->u_tl_adapter_rom_u_err_mask_chk) & (((((s->u_tl_adapter_rom_u_err_tl_i_a_opcode) == (4))) | (((s->u_tl_adapter_rom_u_err_tl_i_a_opcode) == (1))) | (s->u_tl_adapter_rom_u_err_fulldata_chk))))) ^ (1))) | (((((s->u_tl_adapter_rom_u_err_tl_i_a_user_instr_type) == (6))) & (((((s->u_tl_adapter_rom_u_err_tl_i_a_opcode) == (0))) | (((s->u_tl_adapter_rom_u_err_tl_i_a_opcode) == (1))))))) | (((((((s->u_tl_adapter_rom_u_err_tl_i_a_user_instr_type) == (6))) | (((s->u_tl_adapter_rom_u_err_tl_i_a_user_instr_type) == (9))))) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_clk_i = (s->u_tl_adapter_rom_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_rst_ni = (s->u_tl_adapter_rom_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_valid = (s->u_tl_adapter_rom_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_opcode = (s->u_tl_adapter_rom_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_param = (s->u_tl_adapter_rom_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_size = (s->u_tl_adapter_rom_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_source = s->u_tl_adapter_rom_tl_i_a_source;
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_address = s->u_tl_adapter_rom_tl_i_a_address;
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_mask = (s->u_tl_adapter_rom_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_data = s->u_tl_adapter_rom_tl_i_a_data;
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_rsvd = (s->u_tl_adapter_rom_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_instr_type = (s->u_tl_adapter_rom_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_cmd_intg = (s->u_tl_adapter_rom_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_data_intg = (s->u_tl_adapter_rom_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_i_d_ready = (s->u_tl_adapter_rom_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_param = (0) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_sink = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_user_rsp_intg = (0) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_error_i = (((((((((s->u_tl_adapter_rom_tl_i_a_opcode) == (0))) | (((s->u_tl_adapter_rom_tl_i_a_opcode) == (1))))) & (((((s->u_tl_adapter_rom_tl_i_a_mask) != (15))) | (((s->u_tl_adapter_rom_tl_i_a_size) != (2))))))) | (((s->u_tl_adapter_rom_tl_i_a_opcode) != (4))) | (((((((s->u_tl_adapter_rom_tl_i_a_user_instr_type) == (6))) | (((s->u_tl_adapter_rom_tl_i_a_user_instr_type) == (9))))) ^ (1))) | (((((s->u_tl_adapter_rom_tl_i_a_user_instr_type) == (6))) & (((s->u_tl_adapter_rom_en_ifetch_i) != (6))))) | (s->u_tl_adapter_rom_u_err_err_o) | (s->u_tl_adapter_rom_intg_error))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_readback_en_i = (s->u_tl_adapter_rom_readback_en_i) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_sram_byte_wr_collision_i = (s->u_tl_adapter_rom_wr_collision_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_write_pending_i = (s->u_tl_adapter_rom_write_pending_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_param = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_param = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_param = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_param = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_param = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_sink = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_sink) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_sink = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_sink = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_sink = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_sink = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_user_rsp_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_rsp_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_rsp_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_rsp_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_valid = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_valid = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_wmask_intg_0_ = ((((s->u_tl_adapter_rom_tl_i_int_a_valid) && (((((((s->u_tl_adapter_rom_woffset) ^ 1)) ? (((s->u_tl_adapter_rom_woffset) & 0x0)) : (0))) == (0)))) ? (-1) : s->u_tl_adapter_rom_wmask_intg_0_)) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_opcode = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_opcode = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_we_o = ((s->u_tl_adapter_rom_tl_i_int_a_valid) & ((((s->u_tl_adapter_rom_tl_i_int_a_opcode) == (0))) | (((s->u_tl_adapter_rom_tl_i_int_a_opcode) == (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_param = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_param = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_size = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_size = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_source = s->u_tl_adapter_rom_u_sram_byte_tl_i_a_source;
    s->u_tl_adapter_rom_tl_i_int_a_source = s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_source;
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_address = s->u_tl_adapter_rom_u_sram_byte_tl_i_a_address;
    s->u_tl_adapter_rom_tl_i_int_a_address = s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_address;
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_mask = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_mask = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_mask) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_wmask_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wmask_int_0_ & ~0xFFULL) | (((((((s->u_tl_adapter_rom_tl_i_int_a_mask) & 1)) ? 0xFF : 0)) & 0xFFULL) << 0)) : s->u_tl_adapter_rom_wmask_int_0_);
    s->u_tl_adapter_rom_wmask_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wmask_int_0_ & ~0xFF00ULL) | ((((((((s->u_tl_adapter_rom_tl_i_int_a_mask) >> 1) & 0x1)) ? 0xFF : 0)) & 0xFFULL) << 8)) : s->u_tl_adapter_rom_wmask_int_0_);
    s->u_tl_adapter_rom_wmask_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wmask_int_0_ & ~0xFF0000ULL) | ((((((((s->u_tl_adapter_rom_tl_i_int_a_mask) >> 2) & 0x1)) ? 0xFF : 0)) & 0xFFULL) << 16)) : s->u_tl_adapter_rom_wmask_int_0_);
    s->u_tl_adapter_rom_wmask_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wmask_int_0_ & ~0xFF000000ULL) | ((((((((s->u_tl_adapter_rom_tl_i_int_a_mask) >> 3) & 0x1)) ? 0xFF : 0)) & 0xFFULL) << 24)) : s->u_tl_adapter_rom_wmask_int_0_);
    s->u_tl_adapter_rom_wmask_combined_0_ = (((((uint64_t)(s->u_tl_adapter_rom_wmask_intg_0_)) << 32) | ((uint64_t)(s->u_tl_adapter_rom_wmask_int_0_)))) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_data = s->u_tl_adapter_rom_u_sram_byte_tl_i_a_data;
    s->u_tl_adapter_rom_tl_i_int_a_data = s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_data;
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_rsvd = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_user_rsvd = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_instr_type = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_user_instr_type = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_cmd_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_user_cmd_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_data_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_tl_i_int_a_user_data_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_wdata_intg_0_ = ((((s->u_tl_adapter_rom_tl_i_int_a_valid) && (((((((s->u_tl_adapter_rom_woffset) ^ 1)) ? (((s->u_tl_adapter_rom_woffset) & 0x0)) : (0))) == (0)))) ? (s->u_tl_adapter_rom_tl_i_int_a_user_data_intg) : s->u_tl_adapter_rom_wdata_intg_0_)) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_d_ready = (s->u_tl_adapter_rom_u_sram_byte_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_tl_i_int_d_ready = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_o_d_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_error_o = (s->u_tl_adapter_rom_u_sram_byte_error_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_alert_o = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_compound_txn_in_progress_o = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_data_i = 0;
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_i = s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_data_i;
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_i = s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_i;
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = (((((uint64_t)(0)) << 32) | ((uint64_t)(s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_i)))) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o & ~0x100000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (637975845)))) & 0x1ULL) << 32)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o & ~0x200000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) >> 4) & 0xFFFFFFFULL)) & (233547781))) << 4) | ((uint64_t)(0)))))) & 0x1ULL) << 33)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o & ~0x400000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 31) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) >> 1) & 0x3FFFFFFFULL)) & (547275989))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 34)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o & ~0x800000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (824397521)))) & 0x1ULL) << 35)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o & ~0x1000000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) & 0xFFFFFFFFULL)) & (3267441211ULL)))) & 0x1ULL) << 36)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o & ~0x2000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 30) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) >> 2) & 0xFFFFFFFULL)) & (192092307))) << 2) | ((uint64_t)(0)))))) & 0x1ULL) << 37)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o & ~0x4000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) >> 1) & 0x7FFFFFFFULL)) & (1277700803))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 38)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) ^ (180388626432ULL)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o = (s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_data_intg_o = (s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_error_instr_integ = ((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_instr_data_intg_o) >> 32) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_data_i = 4294967295ULL;
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_i = s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_data_i;
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_i = s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_i;
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = (((((uint64_t)(0)) << 32) | ((uint64_t)(s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_i)))) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o & ~0x100000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (637975845)))) & 0x1ULL) << 32)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o & ~0x200000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) >> 4) & 0xFFFFFFFULL)) & (233547781))) << 4) | ((uint64_t)(0)))))) & 0x1ULL) << 33)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o & ~0x400000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 31) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) >> 1) & 0x3FFFFFFFULL)) & (547275989))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 34)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o & ~0x800000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (824397521)))) & 0x1ULL) << 35)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o & ~0x1000000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) & 0xFFFFFFFFULL)) & (3267441211ULL)))) & 0x1ULL) << 36)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o & ~0x2000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 30) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) >> 2) & 0xFFFFFFFULL)) & (192092307))) << 2) | ((uint64_t)(0)))))) & 0x1ULL) << 37)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o & ~0x4000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) >> 1) & 0x7FFFFFFFULL)) & (1277700803))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 38)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = ((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) ^ (180388626432ULL)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o = (s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_data_intg_o = (s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_error_data_integ = ((((s->u_tl_adapter_rom_u_tlul_data_integ_enc_data_data_intg_o) >> 32) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_reqfifo_clk_i = (s->u_tl_adapter_rom_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_rst_ni = (s->u_tl_adapter_rom_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_clr_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_wdata_i = (((((((uint64_t)(s->u_tl_adapter_rom_tl_i_int_a_source)) << 0) | (((uint64_t)(s->u_tl_adapter_rom_tl_i_int_a_size)) << 8)) | (((uint64_t)(s->u_tl_adapter_rom_tl_i_int_a_user_instr_type)) << 10)) | (((uint64_t)(s->u_tl_adapter_rom_u_sram_byte_error_o)) << 14)) | (((uint64_t)(((s->u_tl_adapter_rom_tl_i_int_a_opcode) == (4)))) << 15));
    s->u_tl_adapter_rom_u_reqfifo_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_wdata_i = s->u_tl_adapter_rom_u_reqfifo_wdata_i;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_clk_i = (s->u_tl_adapter_rom_u_reqfifo_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_clr_i = (s->u_tl_adapter_rom_u_reqfifo_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i = (1) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i = (1) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_ = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_ = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_d = ((((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))))) != (3))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb = ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i = (((((uint64_t)(0)) << 0) | (((uint64_t)(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb) ^ (1)))) << 1))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_set_val = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_set_val = ((3) - (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i)) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i = (1) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i = (1) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_ = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_ = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_d = ((((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))))) != (3))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb = ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i = (((((uint64_t)(0)) << 0) | (((uint64_t)(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb) ^ (1)))) << 1))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_set_val = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_set_val = ((3) - (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i)) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_fifo_wptr = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_rdata_int = ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o) ^ 1)) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_));
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_full_o = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) ^ (2))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_empty_o = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_depth_o = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) ^ (2))))) ? (2) : (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb) == (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb))) ? (((((((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o)) << 0) | (((uint64_t)(0)) << 1))) - (((((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o)) << 0) | (((uint64_t)(0)) << 1))))) : (((((2) - (((((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o)) << 0) | (((uint64_t)(0)) << 1))))) + (((((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o)) << 0) | (((uint64_t)(0)) << 1))))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_err_o = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_o) | (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_wready_o = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_full_o) ^ (1))) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_rvalid_o = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_empty_o) ^ (1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_reqfifo_rvalid = (s->u_tl_adapter_rom_u_reqfifo_rvalid_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_rdata_o = ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_empty_o) ? (0) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_rdata_int));
    s->u_tl_adapter_rom_reqfifo_rdata_is_read = ((((s->u_tl_adapter_rom_u_reqfifo_rdata_o) >> 15) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_reqfifo_rdata_error = ((((s->u_tl_adapter_rom_u_reqfifo_rdata_o) >> 14) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_reqfifo_rdata_instr_type = ((((s->u_tl_adapter_rom_u_reqfifo_rdata_o) >> 10) & 0xF)) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_reqfifo_rdata_size = ((((s->u_tl_adapter_rom_u_reqfifo_rdata_o) >> 8) & 0x3)) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_reqfifo_rdata_source = ((s->u_tl_adapter_rom_u_reqfifo_rdata_o) & 0xFF);
    s->u_tl_adapter_rom_d_valid = ((((((!(s->u_tl_adapter_rom_reqfifo_rvalid)) || (((s->u_tl_adapter_rom_reqfifo_rvalid) && (!(s->u_tl_adapter_rom_reqfifo_rdata_error))) && (!(s->u_tl_adapter_rom_reqfifo_rdata_is_read)))) || ((s->u_tl_adapter_rom_reqfifo_rvalid) && (s->u_tl_adapter_rom_reqfifo_rdata_error))) && (((!(s->u_tl_adapter_rom_reqfifo_rvalid)) || (((s->u_tl_adapter_rom_reqfifo_rvalid) && (!(s->u_tl_adapter_rom_reqfifo_rdata_error))) && (!(s->u_tl_adapter_rom_reqfifo_rdata_is_read)))) || ((s->u_tl_adapter_rom_reqfifo_rvalid) && (s->u_tl_adapter_rom_reqfifo_rdata_error)))) ? (((!(s->u_tl_adapter_rom_reqfifo_rvalid)) ? (0) : (1))) : s->u_tl_adapter_rom_d_valid)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_error_blanking_data = ((((s->u_tl_adapter_rom_reqfifo_rdata_instr_type) == (6))) ? (0) : (4294967295ULL));
    s->u_tl_adapter_rom_error_blanking_integ = (((((s->u_tl_adapter_rom_reqfifo_rdata_instr_type) == (6))) ? (s->u_tl_adapter_rom_error_instr_integ) : (s->u_tl_adapter_rom_error_data_integ))) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_reqfifo_full_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_full_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_depth_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_depth_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_err_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_err_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_reqfifo_error = (s->u_tl_adapter_rom_u_reqfifo_err_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_clk_i = (s->u_tl_adapter_rom_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_rst_ni = (s->u_tl_adapter_rom_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_clr_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_wdata_i = (((((uint64_t)(s->u_tl_adapter_rom_woffset)) << 0) | (((uint64_t)(s->u_tl_adapter_rom_tl_i_int_a_mask)) << 1))) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_wdata_i = (s->u_tl_adapter_rom_u_sramreqfifo_wdata_i) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_clk_i = (s->u_tl_adapter_rom_u_sramreqfifo_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_clr_i = (s->u_tl_adapter_rom_u_sramreqfifo_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i = (1) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i = (1) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_ = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_ = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_d = ((((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))))) != (3))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i = (((((uint64_t)(0)) << 0) | (((uint64_t)(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb) ^ (1)))) << 1))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_set_val = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_set_val = ((3) - (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i)) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i = (1) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i = (1) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_ = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_ = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_d = ((((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))))) != (3))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i = (((((uint64_t)(0)) << 0) | (((uint64_t)(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb) ^ (1)))) << 1))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_set_val = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_set_val = ((3) - (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i)) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_fifo_wptr = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_rdata_int = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o) ^ 1)) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_))) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_full_o = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) ^ (2))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_empty_o = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_depth_o = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) ^ (2))))) ? (2) : (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb) == (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb))) ? (((((((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o)) << 0) | (((uint64_t)(0)) << 1))) - (((((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o)) << 0) | (((uint64_t)(0)) << 1))))) : (((((2) - (((((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o)) << 0) | (((uint64_t)(0)) << 1))))) + (((((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o)) << 0) | (((uint64_t)(0)) << 1))))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_err_o = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_o) | (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_wready_o = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_full_o) ^ (1))) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_rvalid_o = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_empty_o) ^ (1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_rdata_o = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_empty_o) ? (0) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_rdata_int))) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_sram_req_rdata_mask = ((((s->u_tl_adapter_rom_u_sramreqfifo_rdata_o) >> 1) & 0xF)) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_sram_req_rdata_woffset = (((s->u_tl_adapter_rom_u_sramreqfifo_rdata_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_full_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_full_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_depth_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_depth_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_err_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_err_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_sramreqfifo_error = (s->u_tl_adapter_rom_u_sramreqfifo_err_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_clk_i = (s->u_tl_adapter_rom_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_rst_ni = (s->u_tl_adapter_rom_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_clr_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_clk_i = (s->u_tl_adapter_rom_u_rspfifo_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_clr_i = (s->u_tl_adapter_rom_u_rspfifo_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i = (1) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i = (1) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_ = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_ = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_d = ((((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))))) != (3))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_o = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i = (((((uint64_t)(0)) << 0) | (((uint64_t)(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb) ^ (1)))) << 1))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_set_val = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_set_val = ((3) - (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_cnt_i)) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_clr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i = (0) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i = (1) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i = (1) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_ = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_clk_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clk_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_ = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_d = ((((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))))) != (3))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_o = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i = (((((uint64_t)(0)) << 0) | (((uint64_t)(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb) ^ (1)))) << 1))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_set_val = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_set_val = ((3) - (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_cnt_i)) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_fifo_wptr = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_rdata = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_o) ^ 1)) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_))) & ((1ULL << 40) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_full_o = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) ^ (2))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_empty_o = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_depth_o = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_o) == (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_o) ^ (2))))) ? (2) : (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_wrap_msb) == (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_wrap_msb))) ? (((((((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_o)) << 0) | (((uint64_t)(0)) << 1))) - (((((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_o)) << 0) | (((uint64_t)(0)) << 1))))) : (((((2) - (((((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_o)) << 0) | (((uint64_t)(0)) << 1))))) + (((((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_o)) << 0) | (((uint64_t)(0)) << 1))))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_err_o = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_o) | (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_wready_o = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_full_o) ^ (1))) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_full_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_full_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_depth_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_depth_o) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_err_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_err_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_rsp_fifo_error = (s->u_tl_adapter_rom_u_rspfifo_err_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_tl_o_d_param = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_tl_o_d_sink = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_req_o = (((s->u_tl_adapter_rom_tl_i_int_a_valid) & (s->u_tl_adapter_rom_u_reqfifo_wready_o) & (((s->u_tl_adapter_rom_u_sram_byte_error_o) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_req_type_o = (s->u_tl_adapter_rom_tl_i_int_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_tl_adapter_rom_we_o = (s->u_tl_adapter_rom_we_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_wdata_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wdata_int_0_ & ~0xFFULL) | ((((((((s->u_tl_adapter_rom_tl_i_int_a_mask) & 1)) & (s->u_tl_adapter_rom_we_o)) ? (((s->u_tl_adapter_rom_tl_i_int_a_data) & 0xFF)) : (0))) & 0xFFULL) << 0)) : s->u_tl_adapter_rom_wdata_int_0_);
    s->u_tl_adapter_rom_wdata_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wdata_int_0_ & ~0xFF00ULL) | (((((((((s->u_tl_adapter_rom_tl_i_int_a_mask) >> 1) & 0x1)) & (s->u_tl_adapter_rom_we_o)) ? ((((s->u_tl_adapter_rom_tl_i_int_a_data) >> 8) & 0xFF)) : (0))) & 0xFFULL) << 8)) : s->u_tl_adapter_rom_wdata_int_0_);
    s->u_tl_adapter_rom_wdata_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wdata_int_0_ & ~0xFF0000ULL) | (((((((((s->u_tl_adapter_rom_tl_i_int_a_mask) >> 2) & 0x1)) & (s->u_tl_adapter_rom_we_o)) ? ((((s->u_tl_adapter_rom_tl_i_int_a_data) >> 16) & 0xFF)) : (0))) & 0xFFULL) << 16)) : s->u_tl_adapter_rom_wdata_int_0_);
    s->u_tl_adapter_rom_wdata_int_0_ = ((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((s->u_tl_adapter_rom_wdata_int_0_ & ~0xFF000000ULL) | (((((((((s->u_tl_adapter_rom_tl_i_int_a_mask) >> 3) & 0x1)) & (s->u_tl_adapter_rom_we_o)) ? ((((s->u_tl_adapter_rom_tl_i_int_a_data) >> 24) & 0xFF)) : (0))) & 0xFFULL) << 24)) : s->u_tl_adapter_rom_wdata_int_0_);
    s->u_tl_adapter_rom_wdata_combined_0_ = (((((uint64_t)(s->u_tl_adapter_rom_wdata_intg_0_)) << 32) | ((uint64_t)(s->u_tl_adapter_rom_wdata_int_0_)))) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_addr_o = (((s->u_tl_adapter_rom_tl_i_int_a_valid) ? ((((s->u_tl_adapter_rom_tl_i_int_a_address) >> 2) & 0x1FFF)) : (0))) & ((1ULL << 13) - 1);
    s->u_tl_adapter_rom_wdata_o = (s->u_tl_adapter_rom_wdata_combined_0_) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_wmask_o = (s->u_tl_adapter_rom_wmask_combined_0_) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_intg_error_o = (((s->u_tl_adapter_rom_intg_error) | (s->u_tl_adapter_rom_rsp_fifo_error) | (s->u_tl_adapter_rom_sramreqfifo_error) | (s->u_tl_adapter_rom_reqfifo_error) | (s->u_tl_adapter_rom_intg_error_q))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_user_rsvd_o = (((s->u_tl_adapter_rom_tl_i_int_a_valid) ? (s->u_tl_adapter_rom_tl_i_int_a_user_rsvd) : (0))) & ((1ULL << 5) - 1);
    s->u_tl_adapter_rom_compound_txn_in_progress_o = (s->u_tl_adapter_rom_u_sram_byte_compound_txn_in_progress_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_readback_error_o = (0) & ((1ULL << 1) - 1);
    s->u_mux_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->u_mux_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->u_mux_bus_rom_addr_i = (s->u_tl_adapter_rom_addr_o) & ((1ULL << 13) - 1);
    s->u_mux_bus_prince_addr_i = (((s->rom_tl_i_a_valid) ? ((((s->rom_tl_i_a_address) >> 2) & 0x1FFF)) : (0))) & ((1ULL << 13) - 1);
    s->u_mux_bus_req_i = (s->u_tl_adapter_rom_req_o) & ((1ULL << 1) - 1);
    s->u_mux_rst_ni = (s->u_mux_rst_ni) & ((1ULL << 1) - 1);
    s->u_mux_bus_rom_addr_i = (s->u_mux_bus_rom_addr_i) & ((1ULL << 13) - 1);
    s->u_mux_bus_prince_addr_i = (s->u_mux_bus_prince_addr_i) & ((1ULL << 13) - 1);
    s->u_mux_bus_req_i = (s->u_mux_bus_req_i) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_q_flop_clk_i = (s->u_mux_clk_i) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_q_flop_rst_ni = (s->u_mux_rst_ni) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_q_flop_rst_ni = (s->u_mux_u_sel_bus_q_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_q_flop_q_o = (s->u_mux_u_sel_bus_q_flop_q_o) & ((1ULL << 4) - 1);
    s->u_mux_u_sel_bus_qq_flop_clk_i = (s->u_mux_clk_i) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_qq_flop_rst_ni = (s->u_mux_rst_ni) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_qq_flop_d_i = (s->u_mux_u_sel_bus_q_flop_q_o) & ((1ULL << 4) - 1);
    s->u_mux_u_sel_bus_qq_flop_rst_ni = (s->u_mux_u_sel_bus_qq_flop_rst_ni) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_qq_flop_d_i = (s->u_mux_u_sel_bus_qq_flop_d_i) & ((1ULL << 4) - 1);
    s->u_mux_u_sel_bus_qq_flop_q_o = (s->u_mux_u_sel_bus_qq_flop_q_o) & ((1ULL << 4) - 1);
    s->u_mux_alert_o = (s->u_mux_alert_q) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_cfg_i_test = (s->rom_cfg_i_test) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_cfg_i_cfg_en = (s->rom_cfg_i_cfg_en) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_cfg_i_cfg = (s->rom_cfg_i_cfg) & ((1ULL << 4) - 1);
    memset(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_in_i, 0, sizeof(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_in_i));
    memcpy(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_u_secure_anchor_buf_in_i, s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_in_i, sizeof(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_u_secure_anchor_buf_in_i));
    memcpy(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_u_secure_anchor_buf_out_o, s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_u_secure_anchor_buf_in_i, sizeof(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_u_secure_anchor_buf_out_o));
    memcpy(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_out_o, s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_u_secure_anchor_buf_out_o, sizeof(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_out_o));
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_key_i = ((((s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_out_o[2]) >> 51) & 0x1FFFULL)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_key_i = (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_key_i) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_clk_i = (s->gen_rom_scramble_enabled_u_rom_clk_i) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni = (s->gen_rom_scramble_enabled_u_rom_rst_ni) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_key_i = ((((__uint128_t)qpw_wide_extract(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_out_o, 3, (0) + 64, 64)) << 64) | qpw_wide_extract(s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_out_o, 3, 0, 64));
    s->gen_rom_scramble_enabled_u_rom_u_prince_dec_i = (0) & ((1ULL << 1) - 1);
    /* comb SCC -20: 42 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xFULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) & 0xFF)) & (189)) & 0xF)) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 8) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 13) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF0ULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 4) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 9) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF00ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 5) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 8) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 8) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 1) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 4) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 4) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 12) & 0x7)))))) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF0000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 17) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 20) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 20) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 28) & 0x7)))))) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF00000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 16) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 16) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 24) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 29) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 16) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 20) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 25) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 16) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF0000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 16) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 21) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 24) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 24) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF00000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 33) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 36) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 36) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 44) & 0x7)))))) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 32) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 32) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 40) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 45) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF0000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 32) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 36) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 41) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 32) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF00000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 32) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 37) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 40) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 40) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 48) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 48) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 56) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 61) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF0000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 48) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 52) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 57) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 48) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF00000000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 48) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 53) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 56) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 56) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out & ~0xF000000000000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 49) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 52) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 52) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q) >> 60) & 0x7)))))) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl31_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl15_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_0_ = s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle;
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1 = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_0_) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q) ^ (14448342753998945364ULL);
    /* comb SCC -21: 59 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xFULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 48) & 0xF)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF0ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 36) & 0xF)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF00ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 24) & 0xF)) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 12) & 0xF)) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF0000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) & 0xF)) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF00000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 52) & 0xF)) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 40) & 0xF)) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF0000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 28) & 0xF)) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF00000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 16) & 0xF)) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 4) & 0xF)) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF0000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 56) & 0xF)) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF00000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 44) & 0xF)) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 32) & 0xF)) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF0000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 20) & 0xF)) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF00000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 8) & 0xF)) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out & ~0xF000000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_xor1) >> 60) & 0xF)) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl16_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xFULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) & 0xFF)) & (189)) & 0xF)) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 8) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 13) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF0ULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 4) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 9) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF00ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 5) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 8) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 8) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 1) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 4) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 4) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 12) & 0x7)))))) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF0000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 17) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 20) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 20) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 28) & 0x7)))))) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF00000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 16) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 16) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 24) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 29) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 16) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 20) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 25) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 16) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF0000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 16) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 21) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 24) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 24) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF00000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 33) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 36) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 36) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 44) & 0x7)))))) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 32) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 32) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 40) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 45) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF0000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 32) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 36) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 41) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 32) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF00000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 32) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 37) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 40) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 40) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 48) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 48) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 56) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 61) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF0000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 48) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 52) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 57) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 48) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF00000000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 48) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 53) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 56) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 56) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out & ~0xF000000000000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 49) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 52) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 52) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 60) & 0x7)))))) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl32_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_1_data_state_bwd) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_1_ = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl17_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1 = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_1_) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k1_q) ^ (7252222110370849037ULL);
    /* comb SCC -22: 59 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xFULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 48) & 0xF)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF0ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 36) & 0xF)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF00ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 24) & 0xF)) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 12) & 0xF)) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF0000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) & 0xF)) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF00000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 52) & 0xF)) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 40) & 0xF)) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF0000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 28) & 0xF)) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF00000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 16) & 0xF)) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 4) & 0xF)) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF0000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 56) & 0xF)) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF00000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 44) & 0xF)) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 32) & 0xF)) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF0000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 20) & 0xF)) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF00000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 8) & 0xF)) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out & ~0xF000000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_xor1) >> 60) & 0xF)) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl18_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xFULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) & 0xFF)) & (189)) & 0xF)) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 8) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 13) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF0ULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 4) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 9) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF00ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 5) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 8) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 8) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 1) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 4) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 4) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 12) & 0x7)))))) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF0000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 17) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 20) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 20) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 28) & 0x7)))))) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF00000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 16) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 16) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 24) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 29) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 16) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 20) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 25) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 16) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF0000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 16) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 21) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 24) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 24) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF00000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 33) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 36) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 36) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 44) & 0x7)))))) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 32) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 32) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 40) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 45) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF0000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 32) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 36) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 41) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 32) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF00000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 32) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 37) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 40) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 40) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 48) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 48) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 56) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 61) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF0000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 48) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 52) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 57) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 48) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF00000000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 48) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 53) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 56) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 56) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out & ~0xF000000000000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 49) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 52) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 52) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 60) & 0x7)))))) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl33_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_2_data_state_bwd) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_2_ = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl19_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1 = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_2_) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q) ^ (15255279193702540185ULL);
    /* comb SCC -23: 59 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xFULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 48) & 0xF)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF0ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 36) & 0xF)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF00ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 24) & 0xF)) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 12) & 0xF)) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF0000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) & 0xF)) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF00000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 52) & 0xF)) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 40) & 0xF)) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF0000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 28) & 0xF)) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF00000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 16) & 0xF)) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 4) & 0xF)) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF0000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 56) & 0xF)) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF00000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 44) & 0xF)) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 32) & 0xF)) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF0000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 20) & 0xF)) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF00000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 8) & 0xF)) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out & ~0xF000000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_xor1) >> 60) & 0xF)) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl20_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xFULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) & 0xFF)) & (189)) & 0xF)) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 8) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 13) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF0ULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 4) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 9) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF00ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 5) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 8) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 8) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 1) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 4) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 4) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 12) & 0x7)))))) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF0000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 17) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 20) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 20) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 28) & 0x7)))))) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF00000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 16) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 16) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 24) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 29) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 16) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 20) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 25) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 16) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF0000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 16) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 21) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 24) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 24) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF00000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 33) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 36) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 36) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 44) & 0x7)))))) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 32) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 32) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 40) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 45) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF0000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 32) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 36) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 41) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 32) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF00000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 32) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 37) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 40) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 40) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 48) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 48) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 56) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 61) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF0000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 48) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 52) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 57) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 48) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF00000000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 48) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 53) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 56) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 56) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out & ~0xF000000000000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 49) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 52) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 52) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 60) & 0x7)))))) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl34_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0x7ULL,0x3ULL,0x2ULL,0xFULL,0xDULL,0x8ULL,0x9ULL,0xAULL,0x6ULL,0x4ULL,0x0ULL,0x5ULL,0xEULL,0xCULL,0x1ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_bwd_pass_3_data_state_bwd) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_3_ = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl21_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_o = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_hi_3_) ^ (13883517620612518109ULL);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_o = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_o) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k1_q);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_o = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_o) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q);
    s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni = (s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_key_i = s->gen_rom_scramble_enabled_u_rom_u_prince_key_i;
    /* comb SCC -14: 3 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0 = (((s->gen_rom_scramble_enabled_u_rom_u_prince_key_i) >> 64) & 0xFFFFFFFFFFFFFFFFULL);
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_d = ((((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_k0) & 1))) << 63) | (((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_k0) >> 2) & 0x3FFFFFFFFFFFFFFFULL))) << 1) | ((uint64_t)(((((s->gen_rom_scramble_enabled_u_rom_u_prince_k0) >> 63) & 0x1)) ^ ((((s->gen_rom_scramble_enabled_u_rom_u_prince_k0) >> 1) & 0x1)))));
    s->gen_rom_scramble_enabled_u_rom_u_prince_k1_d = ((s->gen_rom_scramble_enabled_u_rom_u_prince_key_i) & 0xFFFFFFFFFFFFFFFFULL);
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_d = (((s->gen_rom_scramble_enabled_u_rom_u_prince_key_i) >> 64) & 0xFFFFFFFFFFFFFFFFULL);
    s->gen_rom_scramble_enabled_u_rom_u_prince_dec_i = (s->gen_rom_scramble_enabled_u_rom_u_prince_dec_i) & ((1ULL << 1) - 1);
    /* comb SCC -14: 3 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0 = ((s->gen_rom_scramble_enabled_u_rom_u_prince_dec_i) ? (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_d) : s->gen_rom_scramble_enabled_u_rom_u_prince_k0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_d = ((s->gen_rom_scramble_enabled_u_rom_u_prince_dec_i) ? ((((s->gen_rom_scramble_enabled_u_rom_u_prince_key_i) >> 64) & 0xFFFFFFFFFFFFFFFFULL)) : s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_d);
    s->gen_rom_scramble_enabled_u_rom_u_prince_k1_d = ((s->gen_rom_scramble_enabled_u_rom_u_prince_dec_i) ? ((s->gen_rom_scramble_enabled_u_rom_u_prince_k1_d) ^ (13883517620612518109ULL)) : s->gen_rom_scramble_enabled_u_rom_u_prince_k1_d);
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_d = ((s->gen_rom_scramble_enabled_u_rom_u_prince_dec_i) ? ((s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_d) ^ (13883517620612518109ULL)) : s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_d);
    s->gen_rom_scramble_enabled_u_rom_u_prince_valid_o = (s->gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_o = s->gen_rom_scramble_enabled_u_rom_u_prince_data_o;
    s->gen_rom_scramble_enabled_u_rom_u_rom_clk_i = (s->gen_rom_scramble_enabled_u_rom_clk_i) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_rst_ni = (s->gen_rom_scramble_enabled_u_rom_rst_ni) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_cfg_i_test = (s->gen_rom_scramble_enabled_u_rom_cfg_i_test) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_cfg_i_cfg_en = (s->gen_rom_scramble_enabled_u_rom_cfg_i_cfg_en) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_cfg_i_cfg = (s->gen_rom_scramble_enabled_u_rom_cfg_i_cfg) & ((1ULL << 4) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_clk_i = (s->gen_rom_scramble_enabled_u_rom_u_rom_clk_i) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_rst_ni = (s->gen_rom_scramble_enabled_u_rom_u_rom_rst_ni) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_cfg_i_test = (s->gen_rom_scramble_enabled_u_rom_u_rom_cfg_i_test) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_cfg_i_cfg_en = (s->gen_rom_scramble_enabled_u_rom_u_rom_cfg_i_cfg_en) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_cfg_i_cfg = (s->gen_rom_scramble_enabled_u_rom_u_rom_cfg_i_cfg) & ((1ULL << 4) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_rvalid_o = (s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_rvalid_o) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_rdata_o = (s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_rdata_o) & ((1ULL << 39) - 1);
    s->gen_rom_scramble_enabled_u_rom_rvalid_o = (s->gen_rom_scramble_enabled_u_rom_u_rom_rvalid_o) & ((1ULL << 1) - 1);
    s->u_mux_rom_rvalid_i = (s->gen_rom_scramble_enabled_u_rom_rvalid_o) & ((1ULL << 1) - 1);
    s->u_mux_bus_rvalid_o = (((((s->u_mux_u_sel_bus_q_flop_q_o) == (6))) & (s->u_mux_rom_rvalid_i))) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_scr_rdata_o = (s->gen_rom_scramble_enabled_u_rom_u_rom_rdata_o) & ((1ULL << 39) - 1);
    s->u_mux_rom_scr_rdata_i = (s->gen_rom_scramble_enabled_u_rom_scr_rdata_o) & ((1ULL << 39) - 1);
    s->u_mux_chk_rdata_o = (s->u_mux_rom_scr_rdata_i) & ((1ULL << 39) - 1);
    s->gen_rom_scramble_enabled_u_rom_clr_rdata_o = (((s->gen_rom_scramble_enabled_u_rom_u_rom_rdata_o) ^ ((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_o) >> 0) & 0x7FFFFFFFFF)))) & ((1ULL << 39) - 1);
    s->u_mux_rom_clr_rdata_i = (s->gen_rom_scramble_enabled_u_rom_clr_rdata_o) & ((1ULL << 39) - 1);
    s->u_mux_bus_rdata_o = (s->u_mux_rom_clr_rdata_i) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_rdata_i = (s->u_mux_bus_rdata_o) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_rdata_reshaped_0_ = (((((uint64_t)(s->u_tl_adapter_rom_rdata_i)) >> 0) & ((1ULL << 39) - 1))) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_rdata_tlword = (((((s->u_tl_adapter_rom_sram_req_rdata_mask) != (0))) ? (s->u_tl_adapter_rom_rdata_reshaped_0_) : s->u_tl_adapter_rom_rdata_tlword)) & ((1ULL << 39) - 1);
    s->u_tl_adapter_rom_u_rspfifo_wdata_i = ((((((uint64_t)((((s->u_tl_adapter_rom_rerror_i) >> 1) & 0x1))) << 0) | (((uint64_t)((((s->u_tl_adapter_rom_rdata_tlword) >> 32) & 0x7F))) << 1)) | (((uint64_t)((((s->u_tl_adapter_rom_rdata_tlword) >> 0) & 0xFFFFFFFF))) << 8))) & ((1ULL << 40) - 1);
    s->u_tl_adapter_rom_u_rspfifo_wdata_i = (s->u_tl_adapter_rom_u_rspfifo_wdata_i) & ((1ULL << 40) - 1);
    s->u_reg_regs_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_tl_i_a_valid = (s->regs_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_tl_i_a_opcode = (s->regs_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_tl_i_a_param = (s->regs_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_tl_i_a_size = (s->regs_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_tl_i_a_source = s->regs_tl_i_a_source;
    s->u_reg_regs_tl_i_a_address = s->regs_tl_i_a_address;
    s->u_reg_regs_tl_i_a_mask = (s->regs_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_regs_tl_i_a_data = s->regs_tl_i_a_data;
    s->u_reg_regs_tl_i_a_user_rsvd = (s->regs_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_regs_tl_i_a_user_instr_type = (s->regs_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_regs_tl_i_a_user_cmd_intg = (s->regs_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_tl_i_a_user_data_intg = (s->regs_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_tl_i_d_ready = (s->regs_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_intg_err = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg_rdata_next = 0;
    s->u_reg_regs_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_chk_tl_i_a_valid = (s->u_reg_regs_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_chk_tl_i_a_opcode = (s->u_reg_regs_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_chk_tl_i_a_param = (s->u_reg_regs_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_chk_tl_i_a_size = (s->u_reg_regs_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_chk_tl_i_a_source = s->u_reg_regs_tl_i_a_source;
    s->u_reg_regs_u_chk_tl_i_a_address = s->u_reg_regs_tl_i_a_address;
    s->u_reg_regs_u_chk_tl_i_a_mask = (s->u_reg_regs_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_chk_tl_i_a_data = s->u_reg_regs_tl_i_a_data;
    s->u_reg_regs_u_chk_tl_i_a_user_rsvd = (s->u_reg_regs_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_regs_u_chk_tl_i_a_user_instr_type = (s->u_reg_regs_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_chk_tl_i_a_user_cmd_intg = (s->u_reg_regs_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_chk_tl_i_a_user_data_intg = (s->u_reg_regs_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_chk_tl_i_d_ready = (s->u_reg_regs_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_chk_err_o = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_prim_reg_we_check_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_prim_reg_we_check_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_prim_reg_we_check_err_o = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg_we_err = (s->u_reg_regs_u_prim_reg_we_check_err_o) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_valid = (s->u_reg_regs_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_opcode = (s->u_reg_regs_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_param = (s->u_reg_regs_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_size = (s->u_reg_regs_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_source = s->u_reg_regs_tl_i_a_source;
    s->u_reg_regs_u_reg_if_tl_i_a_address = s->u_reg_regs_tl_i_a_address;
    s->u_reg_regs_u_reg_if_tl_i_a_mask = (s->u_reg_regs_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_data = s->u_reg_regs_tl_i_a_data;
    s->u_reg_regs_u_reg_if_tl_i_a_user_rsvd = (s->u_reg_regs_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_user_instr_type = (s->u_reg_regs_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_user_cmd_intg = (s->u_reg_regs_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_user_data_intg = (s->u_reg_regs_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_tl_i_d_ready = (s->u_reg_regs_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_en_ifetch_i = (9) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_busy_i = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_rst_ni = (s->u_reg_regs_u_reg_if_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_valid = (s->u_reg_regs_u_reg_if_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_opcode = (s->u_reg_regs_u_reg_if_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_param = (s->u_reg_regs_u_reg_if_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_size = (s->u_reg_regs_u_reg_if_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_source = s->u_reg_regs_u_reg_if_tl_i_a_source;
    s->u_reg_regs_u_reg_if_tl_i_a_address = s->u_reg_regs_u_reg_if_tl_i_a_address;
    s->u_reg_regs_u_reg_if_tl_i_a_mask = (s->u_reg_regs_u_reg_if_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_data = s->u_reg_regs_u_reg_if_tl_i_a_data;
    s->u_reg_regs_u_reg_if_tl_i_a_user_rsvd = (s->u_reg_regs_u_reg_if_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_user_instr_type = (s->u_reg_regs_u_reg_if_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_user_cmd_intg = (s->u_reg_regs_u_reg_if_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_tl_i_a_user_data_intg = (s->u_reg_regs_u_reg_if_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_tl_i_d_ready = (s->u_reg_regs_u_reg_if_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_regs_u_reg_if_outstanding_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_regs_u_reg_if_rspop_q) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_param = (0) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_regs_u_reg_if_reqsz_q) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_source = s->u_reg_regs_u_reg_if_reqid_q;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_sink = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_data = s->u_reg_regs_u_reg_if_rdata_q;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (0x0ULL) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg = (0x0ULL) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_regs_u_reg_if_error_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_a_ready = (((((s->u_reg_regs_u_reg_if_outstanding_q) | (s->u_reg_regs_u_reg_if_busy_i))) ^ (1))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_rsp_intg = (0) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_data_intg = (0) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_param = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_source = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_source = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_sink = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_data = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_data = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_a_ready = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_d_ack = ((s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & (s->u_reg_regs_u_reg_if_tl_i_d_ready)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_source = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_data = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_a_ack = ((s->u_reg_regs_u_reg_if_tl_i_a_valid) & (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_a_ready)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_wr_req = ((s->u_reg_regs_u_reg_if_a_ack) & ((((s->u_reg_regs_u_reg_if_tl_i_a_opcode) == (0))) | (((s->u_reg_regs_u_reg_if_tl_i_a_opcode) == (1))))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_rd_req = ((s->u_reg_regs_u_reg_if_a_ack) & (((s->u_reg_regs_u_reg_if_tl_i_a_opcode) == (4)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_addr_align_err = (((s->u_reg_regs_u_reg_if_wr_req) ? (((((s->u_reg_regs_u_reg_if_tl_i_a_address) & 0x3)) != (0))) : s->u_reg_regs_u_reg_if_addr_align_err)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_addr_align_err = (((!(s->u_reg_regs_u_reg_if_wr_req)) ? (0) : s->u_reg_regs_u_reg_if_addr_align_err)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_clk_i = (s->u_reg_regs_u_reg_if_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_rst_ni = (s->u_reg_regs_u_reg_if_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid = (s->u_reg_regs_u_reg_if_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode = (s->u_reg_regs_u_reg_if_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_param = (s->u_reg_regs_u_reg_if_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_size = (s->u_reg_regs_u_reg_if_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_source = s->u_reg_regs_u_reg_if_tl_i_a_source;
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_address = s->u_reg_regs_u_reg_if_tl_i_a_address;
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask = (s->u_reg_regs_u_reg_if_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_data = s->u_reg_regs_u_reg_if_tl_i_a_data;
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_rsvd = (s->u_reg_regs_u_reg_if_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_instr_type = (s->u_reg_regs_u_reg_if_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_cmd_intg = (s->u_reg_regs_u_reg_if_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_data_intg = (s->u_reg_regs_u_reg_if_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_d_ready = (s->u_reg_regs_u_reg_if_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_addr_sz_chk = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_mask_chk = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_fulldata_chk = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_param = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_size = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_u_err_addr_sz_chk = ((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0)))) ? (-1) : s->u_reg_regs_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_source = s->u_reg_regs_u_reg_if_u_err_tl_i_a_source;
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_address = s->u_reg_regs_u_reg_if_u_err_tl_i_a_address;
    s->u_reg_regs_u_reg_if_u_err_mask = ((1) << (((((uint64_t)(0)) << 2) | ((uint64_t)(((s->u_reg_regs_u_reg_if_u_err_tl_i_a_address) & 0x3)))))) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_u_err_addr_sz_chk = (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1)))) ? (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_address) & 1)) ^ 1)) : s->u_reg_regs_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_addr_sz_chk = ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (2)))) ? (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_address) & 0x3)) == (0))) : s->u_reg_regs_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_addr_sz_chk = ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_reg_regs_u_reg_if_u_err_addr_sz_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_u_err_mask_chk = ((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0)))) ? ((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) & (((~(s->u_reg_regs_u_reg_if_u_err_mask)) & 0xFULL))) == (0))) : s->u_reg_regs_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_fulldata_chk = ((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0)))) ? ((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) & (s->u_reg_regs_u_reg_if_u_err_mask)) != (0))) : s->u_reg_regs_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_mask_chk = (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1)))) ? ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (0))) : ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (0))))) : s->u_reg_regs_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_fulldata_chk = (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1)))) ? ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (3))) : (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (3))))) : s->u_reg_regs_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_mask_chk = ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (2)))) ? (-1) : s->u_reg_regs_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_fulldata_chk = ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (2)))) ? (((s->u_reg_regs_u_reg_if_u_err_tl_i_a_mask) == (15))) : s->u_reg_regs_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_mask_chk = ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_reg_regs_u_reg_if_u_err_mask_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_fulldata_chk = ((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_valid) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (0))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (1))))) && (!(((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_reg_regs_u_reg_if_u_err_tl_i_a_size)))) == (2))))) ? (0) : s->u_reg_regs_u_reg_if_u_err_fulldata_chk)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_data = s->u_reg_regs_u_reg_if_u_err_tl_i_a_data;
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_rsvd = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_rsvd) & ((1ULL << 5) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_instr_type = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_instr_type) & ((1ULL << 4) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_cmd_intg = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_cmd_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_data_intg = (s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_u_err_tl_i_d_ready = (s->u_reg_regs_u_reg_if_u_err_tl_i_d_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_u_err_err_o = (((((((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) == (0))) | (((s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) == (1))) | (((s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) == (4))))) & (s->u_reg_regs_u_reg_if_u_err_addr_sz_chk) & (s->u_reg_regs_u_reg_if_u_err_mask_chk) & (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) == (4))) | (((s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) == (1))) | (s->u_reg_regs_u_reg_if_u_err_fulldata_chk))))) ^ (1))) | (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_instr_type) == (6))) & (((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) == (0))) | (((s->u_reg_regs_u_reg_if_u_err_tl_i_a_opcode) == (1))))))) | (((((((s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_instr_type) == (6))) | (((s->u_reg_regs_u_reg_if_u_err_tl_i_a_user_instr_type) == (9))))) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_err_internal = ((s->u_reg_regs_u_reg_if_addr_align_err) | (s->u_reg_regs_u_reg_if_u_err_err_o) | ((((s->u_reg_regs_u_reg_if_tl_i_a_user_instr_type) == (6))) & (((s->u_reg_regs_u_reg_if_en_ifetch_i) != (6))))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_valid = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_regs_u_reg_if_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_valid = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_valid = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_opcode = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_regs_u_reg_if_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_opcode = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_qpinl11_payload_opcode = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_opcode = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_param = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_param = (s->u_reg_regs_u_reg_if_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_param = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_param = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_size = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_regs_u_reg_if_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_size = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_rsp_intg_gen_qpinl11_payload_size = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_size = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_source = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_source = s->u_reg_regs_u_reg_if_tl_o_d_source;
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_source = s->u_reg_regs_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_source = s->u_reg_regs_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_source = s->u_reg_regs_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_regs_u_reg_if_tl_o_d_sink = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_sink = (s->u_reg_regs_u_reg_if_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_sink = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_sink = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_data = s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_data = s->u_reg_regs_u_reg_if_tl_o_d_data;
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_data = s->u_reg_regs_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_data = s->u_reg_regs_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i = s->u_reg_regs_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i;
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i;
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((((uint64_t)(0)) << 32) | ((uint64_t)(s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i)))) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x100000000ULL) | ((((__builtin_parityll((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (637975845)))) & 0x1ULL) << 32)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x200000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 4) & 0xFFFFFFFULL)) & (233547781))) << 4) | ((uint64_t)(0)))))) & 0x1ULL) << 33)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x400000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 31) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x3FFFFFFFULL)) & (547275989))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 34)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x800000000ULL) | ((((__builtin_parityll((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (824397521)))) & 0x1ULL) << 35)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x1000000000ULL) | ((((__builtin_parityll((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0xFFFFFFFFULL)) & (3267441211ULL)))) & 0x1ULL) << 36)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x2000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 30) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 2) & 0xFFFFFFFULL)) & (192092307))) << 2) | ((uint64_t)(0)))))) & 0x1ULL) << 37)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x4000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 32) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x7FFFFFFFULL)) & (1277700803))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 38)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = ((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) ^ (180388626432ULL)) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o = (s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & ((1ULL << 39) - 1);
    s->u_reg_regs_u_rsp_intg_gen_data_intg = ((((s->u_reg_regs_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o) >> 32) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_data = s->u_reg_regs_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_regs_u_reg_if_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (s->u_reg_regs_u_reg_if_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_rsp_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_user_data_intg = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_data_intg = (s->u_reg_regs_u_reg_if_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_data_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_regs_u_rsp_intg_gen_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_data_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_reg_if_tl_o_d_error = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_regs_u_reg_if_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_d_error = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_qpinl11_payload_error = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_regs_u_rsp_intg_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i = (((((((uint64_t)(s->u_reg_regs_u_rsp_intg_gen_qpinl11_payload_error)) << 0) | (((uint64_t)(s->u_reg_regs_u_rsp_intg_gen_qpinl11_payload_size)) << 1)) | (((uint64_t)(s->u_reg_regs_u_rsp_intg_gen_qpinl11_payload_opcode)) << 3)) | (((uint64_t)(0)) << 6))) & ((1ULL << 57) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i) & ((1ULL << 57) - 1);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = ((((uint64_t)(0)) << 57) | ((uint64_t)(s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i)));
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x200000000000000ULL) | ((((__builtin_parityll((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (73183459585064959ULL)))) & 0x1ULL) << 57);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x400000000000000ULL) | ((((__builtin_parityll((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (106995641195921439ULL)))) & 0x1ULL) << 58);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x800000000000000ULL) | ((((__builtin_parityll((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (125504822018802145ULL)))) & 0x1ULL) << 59);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x1000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 1) & 0xFFFFFFFFFFFFFFULL)) & (67403489212122897ULL))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 60);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x2000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 2) & 0x7FFFFFFFFFFFFFULL)) & (34865184827919505ULL))) << 2) | ((uint64_t)(0)))))) & 0x1ULL) << 61);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x4000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 3) & 0x3FFFFFFFFFFFFFULL)) & (17723486863248017ULL))) << 3) | ((uint64_t)(0)))))) & 0x1ULL) << 62);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x8000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 4) & 0x1FFFFFFFFFFFFFULL)) & (8934470268372625ULL))) << 4) | ((uint64_t)(0)))))) & 0x1ULL) << 63);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) ^ (6052837899185946624ULL);
    s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o;
    s->u_reg_regs_u_rsp_intg_gen_rsp_intg = ((((s->u_reg_regs_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 57) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_rsp_intg_gen_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_d_error = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_tl_o_a_ready = (s->u_reg_regs_u_reg_if_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_a_ready = (s->u_reg_regs_u_reg_if_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_i_a_ready = (s->u_reg_regs_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_regs_u_rsp_intg_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_rsp_intg_gen_tl_o_a_ready = (s->u_reg_regs_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_intg_error_o = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_re_o = (((s->u_reg_regs_u_reg_if_rd_req) & (((s->u_reg_regs_u_reg_if_err_internal) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_we_o = (((s->u_reg_regs_u_reg_if_wr_req) & (((s->u_reg_regs_u_reg_if_err_internal) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg_we = (s->u_reg_regs_u_reg_if_we_o) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_addr_o = (((((uint64_t)(0)) << 0) | (((uint64_t)((((s->u_reg_regs_u_reg_if_tl_i_a_address) >> 2) & 0x1F))) << 2))) & ((1ULL << 7) - 1);
    s->u_reg_regs_reg_addr = (s->u_reg_regs_u_reg_if_addr_o) & ((1ULL << 7) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x1ULL) | (((((s->u_reg_regs_reg_addr) == (0))) & 0x1ULL) << 0)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x2ULL) | (((((s->u_reg_regs_reg_addr) == (4))) & 0x1ULL) << 1)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x4ULL) | (((((s->u_reg_regs_reg_addr) == (8))) & 0x1ULL) << 2)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x8ULL) | (((((s->u_reg_regs_reg_addr) == (12))) & 0x1ULL) << 3)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x10ULL) | (((((s->u_reg_regs_reg_addr) == (16))) & 0x1ULL) << 4)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x20ULL) | (((((s->u_reg_regs_reg_addr) == (20))) & 0x1ULL) << 5)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x40ULL) | (((((s->u_reg_regs_reg_addr) == (24))) & 0x1ULL) << 6)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x80ULL) | (((((s->u_reg_regs_reg_addr) == (28))) & 0x1ULL) << 7)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x100ULL) | (((((s->u_reg_regs_reg_addr) == (32))) & 0x1ULL) << 8)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x200ULL) | (((((s->u_reg_regs_reg_addr) == (36))) & 0x1ULL) << 9)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x400ULL) | (((((s->u_reg_regs_reg_addr) == (40))) & 0x1ULL) << 10)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x800ULL) | (((((s->u_reg_regs_reg_addr) == (44))) & 0x1ULL) << 11)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x1000ULL) | (((((s->u_reg_regs_reg_addr) == (48))) & 0x1ULL) << 12)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x2000ULL) | (((((s->u_reg_regs_reg_addr) == (52))) & 0x1ULL) << 13)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x4000ULL) | (((((s->u_reg_regs_reg_addr) == (56))) & 0x1ULL) << 14)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x8000ULL) | (((((s->u_reg_regs_reg_addr) == (60))) & 0x1ULL) << 15)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x10000ULL) | (((((s->u_reg_regs_reg_addr) == (64))) & 0x1ULL) << 16)) & ((1ULL << 18) - 1);
    s->u_reg_regs_addr_hit = ((s->u_reg_regs_addr_hit & ~0x20000ULL) | (((((s->u_reg_regs_reg_addr) == (68))) & 0x1ULL) << 17)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_rdata_next = ((((((s->u_reg_regs_addr_hit) & 1)) == (1))) ? ((s->u_reg_regs_reg_rdata_next & ~0x1ULL) | (((0) & 0x1ULL) << 0)) : s->u_reg_regs_reg_rdata_next);
    s->u_reg_regs_u_prim_reg_we_check_en_i = (((s->u_reg_regs_reg_we) & (((((((s->u_reg_regs_u_reg_if_re_o) | (s->u_reg_regs_reg_we))) & (((s->u_reg_regs_addr_hit) == (0))))) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_wdata_o = s->u_reg_regs_u_reg_if_tl_i_a_data;
    s->u_reg_regs_u_reg_if_be_o = (s->u_reg_regs_u_reg_if_tl_i_a_mask) & ((1ULL << 4) - 1);
    s->u_reg_regs_reg_be = (s->u_reg_regs_u_reg_if_be_o) & ((1ULL << 4) - 1);
    s->u_reg_regs_wr_err = ((s->u_reg_regs_reg_we) & (((((s->u_reg_regs_addr_hit) & 1)) & (((((s->u_reg_regs_reg_be) & 1)) ^ 1))) | (((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) & (((((s->u_reg_regs_reg_be) & 1)) ^ 1))) | (((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 15) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 16) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))) | (((((s->u_reg_regs_addr_hit) >> 17) & 0x1)) & (((s->u_reg_regs_reg_be) != (15)))))) & ((1ULL << 1) - 1);
    s->u_reg_regs_alert_test_we = ((((s->u_reg_regs_addr_hit) & 1)) & (s->u_reg_regs_reg_we) & ((((((s->u_reg_regs_u_reg_if_re_o) | (s->u_reg_regs_reg_we)) & (((s->u_reg_regs_addr_hit) == (0)))) | (s->u_reg_regs_wr_err) | (s->u_reg_regs_intg_err)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x1ULL) | (((s->u_reg_regs_alert_test_we) & 0x1ULL) << 0)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x2ULL) | (((0) & 0x1ULL) << 1)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x4ULL) | (((0) & 0x1ULL) << 2)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x8ULL) | (((0) & 0x1ULL) << 3)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x10ULL) | (((0) & 0x1ULL) << 4)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x20ULL) | (((0) & 0x1ULL) << 5)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x40ULL) | (((0) & 0x1ULL) << 6)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x80ULL) | (((0) & 0x1ULL) << 7)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x100ULL) | (((0) & 0x1ULL) << 8)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x200ULL) | (((0) & 0x1ULL) << 9)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x400ULL) | (((0) & 0x1ULL) << 10)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x800ULL) | (((0) & 0x1ULL) << 11)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x1000ULL) | (((0) & 0x1ULL) << 12)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x2000ULL) | (((0) & 0x1ULL) << 13)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x4000ULL) | (((0) & 0x1ULL) << 14)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x8000ULL) | (((0) & 0x1ULL) << 15)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x10000ULL) | (((0) & 0x1ULL) << 16)) & ((1ULL << 18) - 1);
    s->u_reg_regs_reg_we_check = ((s->u_reg_regs_reg_we_check & ~0x20000ULL) | (((0) & 0x1ULL) << 17)) & ((1ULL << 18) - 1);
    s->u_reg_regs_u_prim_reg_we_check_oh_i = (s->u_reg_regs_reg_we_check) & ((1ULL << 18) - 1);
    s->u_reg_regs_u_reg_if_error_i = (((((((s->u_reg_regs_u_reg_if_re_o) | (s->u_reg_regs_reg_we))) & (((s->u_reg_regs_addr_hit) == (0))))) | (s->u_reg_regs_wr_err) | (s->u_reg_regs_intg_err))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_reg_if_error_i = (s->u_reg_regs_u_reg_if_error_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_re = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_we = (s->u_reg_regs_alert_test_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_wd = ((((s->u_reg_regs_u_reg_if_wdata_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_d = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_qe = (s->u_reg_regs_u_alert_test_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_alert_test_flds_we = (s->u_reg_regs_u_alert_test_qe) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg2hw_alert_test_qe = (s->u_reg_regs_alert_test_flds_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_qre = (s->u_reg_regs_u_alert_test_re) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_q = (s->u_reg_regs_u_alert_test_wd) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg2hw_alert_test_q = (s->u_reg_regs_u_alert_test_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_ds = (s->u_reg_regs_u_alert_test_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_alert_test_qs = (s->u_reg_regs_u_alert_test_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wd = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_rst_ni = (s->u_reg_regs_u_fatal_alert_cause_checker_error_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_we = (s->u_reg_regs_u_fatal_alert_cause_checker_error_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_wd = (s->u_reg_regs_u_fatal_alert_cause_checker_error_wd) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_qe = (s->u_reg_regs_u_fatal_alert_cause_checker_error_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_q = (s->u_reg_regs_u_fatal_alert_cause_checker_error_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_qs = (s->u_reg_regs_u_fatal_alert_cause_checker_error_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_q = (s->u_reg_regs_u_fatal_alert_cause_checker_error_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_qs = (s->u_reg_regs_u_fatal_alert_cause_checker_error_qs) & ((1ULL << 1) - 1);
    s->u_reg_regs_fatal_alert_cause_checker_error_qs = (s->u_reg_regs_u_fatal_alert_cause_checker_error_qs) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg_rdata_next = (((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && ((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1)))) ? ((s->u_reg_regs_reg_rdata_next & ~0x1ULL) | (((s->u_reg_regs_fatal_alert_cause_checker_error_qs) & 0x1ULL) << 0)) : s->u_reg_regs_reg_rdata_next);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wd = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_rst_ni = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_we = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_wd = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wd) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_qe = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_q = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_qs = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_q = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_qs = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_qs) & ((1ULL << 1) - 1);
    s->u_reg_regs_fatal_alert_cause_integrity_error_qs = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_qs) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg_rdata_next = (((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && ((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1)))) ? ((s->u_reg_regs_reg_rdata_next & ~0x2ULL) | (((s->u_reg_regs_fatal_alert_cause_integrity_error_qs) & 0x1ULL) << 1)) : s->u_reg_regs_reg_rdata_next);
    s->u_reg_regs_u_digest_0_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_wd = 0;
    s->u_reg_regs_u_digest_0_rst_ni = (s->u_reg_regs_u_digest_0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_wr_en_data_arb_we = (s->u_reg_regs_u_digest_0_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_wr_en_data_arb_wd = s->u_reg_regs_u_digest_0_wd;
    s->u_reg_regs_u_digest_0_qe = (s->u_reg_regs_u_digest_0_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_q = s->u_reg_regs_u_digest_0_q;
    s->u_reg_regs_reg2hw_digest_0__q = s->u_reg_regs_u_digest_0_q;
    s->u_reg_regs_u_digest_0_qs = s->u_reg_regs_u_digest_0_q;
    s->u_reg_regs_u_digest_0_wr_en_data_arb_q = s->u_reg_regs_u_digest_0_q;
    s->u_reg_regs_u_digest_0_qs = s->u_reg_regs_u_digest_0_qs;
    s->u_reg_regs_digest_0_qs = s->u_reg_regs_u_digest_0_qs;
    s->u_reg_regs_u_digest_1_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_wd = 0;
    s->u_reg_regs_u_digest_1_rst_ni = (s->u_reg_regs_u_digest_1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_wr_en_data_arb_we = (s->u_reg_regs_u_digest_1_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_wr_en_data_arb_wd = s->u_reg_regs_u_digest_1_wd;
    s->u_reg_regs_u_digest_1_qe = (s->u_reg_regs_u_digest_1_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_q = s->u_reg_regs_u_digest_1_q;
    s->u_reg_regs_reg2hw_digest_1__q = s->u_reg_regs_u_digest_1_q;
    s->u_reg_regs_u_digest_1_qs = s->u_reg_regs_u_digest_1_q;
    s->u_reg_regs_u_digest_1_wr_en_data_arb_q = s->u_reg_regs_u_digest_1_q;
    s->u_reg_regs_u_digest_1_qs = s->u_reg_regs_u_digest_1_qs;
    s->u_reg_regs_digest_1_qs = s->u_reg_regs_u_digest_1_qs;
    s->u_reg_regs_u_digest_2_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_wd = 0;
    s->u_reg_regs_u_digest_2_rst_ni = (s->u_reg_regs_u_digest_2_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_wr_en_data_arb_we = (s->u_reg_regs_u_digest_2_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_wr_en_data_arb_wd = s->u_reg_regs_u_digest_2_wd;
    s->u_reg_regs_u_digest_2_qe = (s->u_reg_regs_u_digest_2_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_q = s->u_reg_regs_u_digest_2_q;
    s->u_reg_regs_reg2hw_digest_2__q = s->u_reg_regs_u_digest_2_q;
    s->u_reg_regs_u_digest_2_qs = s->u_reg_regs_u_digest_2_q;
    s->u_reg_regs_u_digest_2_wr_en_data_arb_q = s->u_reg_regs_u_digest_2_q;
    s->u_reg_regs_u_digest_2_qs = s->u_reg_regs_u_digest_2_qs;
    s->u_reg_regs_digest_2_qs = s->u_reg_regs_u_digest_2_qs;
    s->u_reg_regs_u_digest_3_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_wd = 0;
    s->u_reg_regs_u_digest_3_rst_ni = (s->u_reg_regs_u_digest_3_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_wr_en_data_arb_we = (s->u_reg_regs_u_digest_3_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_wr_en_data_arb_wd = s->u_reg_regs_u_digest_3_wd;
    s->u_reg_regs_u_digest_3_qe = (s->u_reg_regs_u_digest_3_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_q = s->u_reg_regs_u_digest_3_q;
    s->u_reg_regs_reg2hw_digest_3__q = s->u_reg_regs_u_digest_3_q;
    s->u_reg_regs_u_digest_3_qs = s->u_reg_regs_u_digest_3_q;
    s->u_reg_regs_u_digest_3_wr_en_data_arb_q = s->u_reg_regs_u_digest_3_q;
    s->u_reg_regs_u_digest_3_qs = s->u_reg_regs_u_digest_3_qs;
    s->u_reg_regs_digest_3_qs = s->u_reg_regs_u_digest_3_qs;
    s->u_reg_regs_u_digest_4_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_wd = 0;
    s->u_reg_regs_u_digest_4_rst_ni = (s->u_reg_regs_u_digest_4_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_wr_en_data_arb_we = (s->u_reg_regs_u_digest_4_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_wr_en_data_arb_wd = s->u_reg_regs_u_digest_4_wd;
    s->u_reg_regs_u_digest_4_qe = (s->u_reg_regs_u_digest_4_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_q = s->u_reg_regs_u_digest_4_q;
    s->u_reg_regs_reg2hw_digest_4__q = s->u_reg_regs_u_digest_4_q;
    s->u_reg_regs_u_digest_4_qs = s->u_reg_regs_u_digest_4_q;
    s->u_reg_regs_u_digest_4_wr_en_data_arb_q = s->u_reg_regs_u_digest_4_q;
    s->u_reg_regs_u_digest_4_qs = s->u_reg_regs_u_digest_4_qs;
    s->u_reg_regs_digest_4_qs = s->u_reg_regs_u_digest_4_qs;
    s->u_reg_regs_u_digest_5_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_wd = 0;
    s->u_reg_regs_u_digest_5_rst_ni = (s->u_reg_regs_u_digest_5_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_wr_en_data_arb_we = (s->u_reg_regs_u_digest_5_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_wr_en_data_arb_wd = s->u_reg_regs_u_digest_5_wd;
    s->u_reg_regs_u_digest_5_qe = (s->u_reg_regs_u_digest_5_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_q = s->u_reg_regs_u_digest_5_q;
    s->u_reg_regs_reg2hw_digest_5__q = s->u_reg_regs_u_digest_5_q;
    s->u_reg_regs_u_digest_5_qs = s->u_reg_regs_u_digest_5_q;
    s->u_reg_regs_u_digest_5_wr_en_data_arb_q = s->u_reg_regs_u_digest_5_q;
    s->u_reg_regs_u_digest_5_qs = s->u_reg_regs_u_digest_5_qs;
    s->u_reg_regs_digest_5_qs = s->u_reg_regs_u_digest_5_qs;
    s->u_reg_regs_u_digest_6_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_wd = 0;
    s->u_reg_regs_u_digest_6_rst_ni = (s->u_reg_regs_u_digest_6_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_wr_en_data_arb_we = (s->u_reg_regs_u_digest_6_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_wr_en_data_arb_wd = s->u_reg_regs_u_digest_6_wd;
    s->u_reg_regs_u_digest_6_qe = (s->u_reg_regs_u_digest_6_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_q = s->u_reg_regs_u_digest_6_q;
    s->u_reg_regs_reg2hw_digest_6__q = s->u_reg_regs_u_digest_6_q;
    s->u_reg_regs_u_digest_6_qs = s->u_reg_regs_u_digest_6_q;
    s->u_reg_regs_u_digest_6_wr_en_data_arb_q = s->u_reg_regs_u_digest_6_q;
    s->u_reg_regs_u_digest_6_qs = s->u_reg_regs_u_digest_6_qs;
    s->u_reg_regs_digest_6_qs = s->u_reg_regs_u_digest_6_qs;
    s->u_reg_regs_u_digest_7_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_wd = 0;
    s->u_reg_regs_u_digest_7_rst_ni = (s->u_reg_regs_u_digest_7_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_wr_en_data_arb_we = (s->u_reg_regs_u_digest_7_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_wr_en_data_arb_wd = s->u_reg_regs_u_digest_7_wd;
    s->u_reg_regs_u_digest_7_qe = (s->u_reg_regs_u_digest_7_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_q = s->u_reg_regs_u_digest_7_q;
    s->u_reg_regs_reg2hw_digest_7__q = s->u_reg_regs_u_digest_7_q;
    s->u_reg_regs_u_digest_7_qs = s->u_reg_regs_u_digest_7_q;
    s->u_reg_regs_u_digest_7_wr_en_data_arb_q = s->u_reg_regs_u_digest_7_q;
    s->u_reg_regs_u_digest_7_qs = s->u_reg_regs_u_digest_7_qs;
    s->u_reg_regs_digest_7_qs = s->u_reg_regs_u_digest_7_qs;
    s->u_reg_regs_u_exp_digest_0_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_wd = 0;
    s->u_reg_regs_u_exp_digest_0_rst_ni = (s->u_reg_regs_u_exp_digest_0_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_0_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_0_wd;
    s->u_reg_regs_u_exp_digest_0_qe = (s->u_reg_regs_u_exp_digest_0_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_q = s->u_reg_regs_u_exp_digest_0_q;
    s->u_reg_regs_reg2hw_exp_digest_0__q = s->u_reg_regs_u_exp_digest_0_q;
    s->u_reg_regs_u_exp_digest_0_qs = s->u_reg_regs_u_exp_digest_0_q;
    s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_0_q;
    s->u_reg_regs_u_exp_digest_0_qs = s->u_reg_regs_u_exp_digest_0_qs;
    s->u_reg_regs_exp_digest_0_qs = s->u_reg_regs_u_exp_digest_0_qs;
    s->u_reg_regs_u_exp_digest_1_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_wd = 0;
    s->u_reg_regs_u_exp_digest_1_rst_ni = (s->u_reg_regs_u_exp_digest_1_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_1_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_1_wd;
    s->u_reg_regs_u_exp_digest_1_qe = (s->u_reg_regs_u_exp_digest_1_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_q = s->u_reg_regs_u_exp_digest_1_q;
    s->u_reg_regs_reg2hw_exp_digest_1__q = s->u_reg_regs_u_exp_digest_1_q;
    s->u_reg_regs_u_exp_digest_1_qs = s->u_reg_regs_u_exp_digest_1_q;
    s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_1_q;
    s->u_reg_regs_u_exp_digest_1_qs = s->u_reg_regs_u_exp_digest_1_qs;
    s->u_reg_regs_exp_digest_1_qs = s->u_reg_regs_u_exp_digest_1_qs;
    s->u_reg_regs_u_exp_digest_2_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_wd = 0;
    s->u_reg_regs_u_exp_digest_2_rst_ni = (s->u_reg_regs_u_exp_digest_2_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_2_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_2_wd;
    s->u_reg_regs_u_exp_digest_2_qe = (s->u_reg_regs_u_exp_digest_2_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_q = s->u_reg_regs_u_exp_digest_2_q;
    s->u_reg_regs_reg2hw_exp_digest_2__q = s->u_reg_regs_u_exp_digest_2_q;
    s->u_reg_regs_u_exp_digest_2_qs = s->u_reg_regs_u_exp_digest_2_q;
    s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_2_q;
    s->u_reg_regs_u_exp_digest_2_qs = s->u_reg_regs_u_exp_digest_2_qs;
    s->u_reg_regs_exp_digest_2_qs = s->u_reg_regs_u_exp_digest_2_qs;
    s->u_reg_regs_u_exp_digest_3_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_wd = 0;
    s->u_reg_regs_u_exp_digest_3_rst_ni = (s->u_reg_regs_u_exp_digest_3_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_3_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_3_wd;
    s->u_reg_regs_u_exp_digest_3_qe = (s->u_reg_regs_u_exp_digest_3_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_q = s->u_reg_regs_u_exp_digest_3_q;
    s->u_reg_regs_reg2hw_exp_digest_3__q = s->u_reg_regs_u_exp_digest_3_q;
    s->u_reg_regs_u_exp_digest_3_qs = s->u_reg_regs_u_exp_digest_3_q;
    s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_3_q;
    s->u_reg_regs_u_exp_digest_3_qs = s->u_reg_regs_u_exp_digest_3_qs;
    s->u_reg_regs_exp_digest_3_qs = s->u_reg_regs_u_exp_digest_3_qs;
    s->u_reg_regs_u_exp_digest_4_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_wd = 0;
    s->u_reg_regs_u_exp_digest_4_rst_ni = (s->u_reg_regs_u_exp_digest_4_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_4_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_4_wd;
    s->u_reg_regs_u_exp_digest_4_qe = (s->u_reg_regs_u_exp_digest_4_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_q = s->u_reg_regs_u_exp_digest_4_q;
    s->u_reg_regs_reg2hw_exp_digest_4__q = s->u_reg_regs_u_exp_digest_4_q;
    s->u_reg_regs_u_exp_digest_4_qs = s->u_reg_regs_u_exp_digest_4_q;
    s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_4_q;
    s->u_reg_regs_u_exp_digest_4_qs = s->u_reg_regs_u_exp_digest_4_qs;
    s->u_reg_regs_exp_digest_4_qs = s->u_reg_regs_u_exp_digest_4_qs;
    s->u_reg_regs_u_exp_digest_5_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_wd = 0;
    s->u_reg_regs_u_exp_digest_5_rst_ni = (s->u_reg_regs_u_exp_digest_5_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_5_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_5_wd;
    s->u_reg_regs_u_exp_digest_5_qe = (s->u_reg_regs_u_exp_digest_5_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_q = s->u_reg_regs_u_exp_digest_5_q;
    s->u_reg_regs_reg2hw_exp_digest_5__q = s->u_reg_regs_u_exp_digest_5_q;
    s->u_reg_regs_u_exp_digest_5_qs = s->u_reg_regs_u_exp_digest_5_q;
    s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_5_q;
    s->u_reg_regs_u_exp_digest_5_qs = s->u_reg_regs_u_exp_digest_5_qs;
    s->u_reg_regs_exp_digest_5_qs = s->u_reg_regs_u_exp_digest_5_qs;
    s->u_reg_regs_u_exp_digest_6_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_wd = 0;
    s->u_reg_regs_u_exp_digest_6_rst_ni = (s->u_reg_regs_u_exp_digest_6_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_6_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_6_wd;
    s->u_reg_regs_u_exp_digest_6_qe = (s->u_reg_regs_u_exp_digest_6_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_q = s->u_reg_regs_u_exp_digest_6_q;
    s->u_reg_regs_reg2hw_exp_digest_6__q = s->u_reg_regs_u_exp_digest_6_q;
    s->u_reg_regs_u_exp_digest_6_qs = s->u_reg_regs_u_exp_digest_6_q;
    s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_6_q;
    s->u_reg_regs_u_exp_digest_6_qs = s->u_reg_regs_u_exp_digest_6_qs;
    s->u_reg_regs_exp_digest_6_qs = s->u_reg_regs_u_exp_digest_6_qs;
    s->u_reg_regs_u_exp_digest_7_clk_i = (s->u_reg_regs_clk_i) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_rst_ni = (s->u_reg_regs_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_we = (0) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_wd = 0;
    s->u_reg_regs_u_exp_digest_7_rst_ni = (s->u_reg_regs_u_exp_digest_7_rst_ni) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_we = (s->u_reg_regs_u_exp_digest_7_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_wd = s->u_reg_regs_u_exp_digest_7_wd;
    s->u_reg_regs_u_exp_digest_7_qe = (s->u_reg_regs_u_exp_digest_7_we) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_q = s->u_reg_regs_u_exp_digest_7_q;
    s->u_reg_regs_reg2hw_exp_digest_7__q = s->u_reg_regs_u_exp_digest_7_q;
    s->u_reg_regs_u_exp_digest_7_qs = s->u_reg_regs_u_exp_digest_7_q;
    s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_q = s->u_reg_regs_u_exp_digest_7_q;
    s->u_reg_regs_u_exp_digest_7_qs = s->u_reg_regs_u_exp_digest_7_qs;
    s->u_reg_regs_exp_digest_7_qs = s->u_reg_regs_u_exp_digest_7_qs;
    s->u_reg_regs_reg_rdata_next = (((((((((((((((((((((((s->u_reg_regs_addr_hit) >> 17) & 0x1)) == (1))) || ((((((s->u_reg_regs_addr_hit) >> 16) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 15) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1)))) || ((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1)))) && (((((((((((((((((((((((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 15) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 16) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 17) & 0x1)) == (1)))) || (((((((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 15) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 16) & 0x1)) == (1))))) || ((((((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 15) & 0x1)) == (1))))) || (((((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) == (1))))) || ((((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1))))) || (((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))))) || ((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) || (((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) || ((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) || (((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) || ((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) || (((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) || ((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) || (((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) || ((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) || (((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && ((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1)))))) ? ((((((((s->u_reg_regs_addr_hit) >> 17) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_7_qs) : ((((((((s->u_reg_regs_addr_hit) >> 16) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_6_qs) : ((((((((s->u_reg_regs_addr_hit) >> 15) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_5_qs) : ((((((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_4_qs) : ((((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_3_qs) : ((((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_2_qs) : ((((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_1_qs) : ((((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))) ? (s->u_reg_regs_exp_digest_0_qs) : ((((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))) ? (s->u_reg_regs_digest_7_qs) : ((((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))) ? (s->u_reg_regs_digest_6_qs) : ((((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))) ? (s->u_reg_regs_digest_5_qs) : ((((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))) ? (s->u_reg_regs_digest_4_qs) : ((((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))) ? (s->u_reg_regs_digest_3_qs) : ((((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))) ? (s->u_reg_regs_digest_2_qs) : ((((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))) ? (s->u_reg_regs_digest_1_qs) : (s->u_reg_regs_digest_0_qs))))))))))))))))))))))))))))))) : s->u_reg_regs_reg_rdata_next);
    s->u_reg_regs_reg_rdata_next = (((((((((((((((((((!(((((s->u_reg_regs_addr_hit) & 1)) == (1)))) && (!((((((s->u_reg_regs_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 11) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 12) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 13) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 14) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 15) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 16) & 0x1)) == (1))))) && (!((((((s->u_reg_regs_addr_hit) >> 17) & 0x1)) == (1))))) ? (-1) : s->u_reg_regs_reg_rdata_next);
    s->u_reg_regs_u_reg_if_rdata_i = s->u_reg_regs_reg_rdata_next;
    s->u_reg_regs_u_reg_if_rdata_i = s->u_reg_regs_u_reg_if_rdata_i;
    s->u_reg_regs_tl_o_d_valid = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_reg_regs_tl_o_d_opcode = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_reg_regs_tl_o_d_param = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_param) & ((1ULL << 3) - 1);
    s->u_reg_regs_tl_o_d_size = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_reg_regs_tl_o_d_source = s->u_reg_regs_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_regs_tl_o_d_sink = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->u_reg_regs_tl_o_d_data = s->u_reg_regs_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_regs_tl_o_d_user_rsp_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_tl_o_d_user_data_intg = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_reg_regs_tl_o_d_error = (s->u_reg_regs_u_rsp_intg_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_tl_o_a_ready = (s->u_reg_regs_u_rsp_intg_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg2hw_alert_test_q = (s->u_reg_regs_reg2hw_alert_test_q) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg2hw_alert_test_qe = (s->u_reg_regs_reg2hw_alert_test_qe) & ((1ULL << 1) - 1);
    s->alert_test = ((s->u_reg_regs_reg2hw_alert_test_q) & (s->u_reg_regs_reg2hw_alert_test_qe)) & ((1ULL << 1) - 1);
    s->u_reg_regs_reg2hw_digest_7__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_7__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_digest_6__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_6__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_digest_5__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_5__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_digest_4__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_4__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_digest_3__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_3__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_digest_2__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_2__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_digest_1__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_1__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_digest_0__q = ((((uint64_t)(s->u_reg_regs_reg2hw_digest_0__q)) & ((1ULL << 32) - 1)));
    s->digest_q[0] = (s->digest_q[0] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_0__q) & 0xFFFFFFFFULL) << 0);
    s->digest_q[0] = (s->digest_q[0] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_1__q) & 0xFFFFFFFFULL) << 32);
    s->digest_q[1] = (s->digest_q[1] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_2__q) & 0xFFFFFFFFULL) << 0);
    s->digest_q[1] = (s->digest_q[1] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_3__q) & 0xFFFFFFFFULL) << 32);
    s->digest_q[2] = (s->digest_q[2] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_4__q) & 0xFFFFFFFFULL) << 0);
    s->digest_q[2] = (s->digest_q[2] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_5__q) & 0xFFFFFFFFULL) << 32);
    s->digest_q[3] = (s->digest_q[3] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_6__q) & 0xFFFFFFFFULL) << 0);
    s->digest_q[3] = (s->digest_q[3] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_digest_7__q) & 0xFFFFFFFFULL) << 32);
    s->u_reg_regs_reg2hw_exp_digest_7__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_7__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_exp_digest_6__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_6__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_exp_digest_5__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_5__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_exp_digest_4__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_4__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_exp_digest_3__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_3__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_exp_digest_2__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_2__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_exp_digest_1__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_1__q)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_reg2hw_exp_digest_0__q = ((((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_0__q)) & ((1ULL << 32) - 1)));
    s->exp_digest_q[0] = (s->exp_digest_q[0] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_0__q) & 0xFFFFFFFFULL) << 0);
    s->exp_digest_q[0] = (s->exp_digest_q[0] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_1__q) & 0xFFFFFFFFULL) << 32);
    s->exp_digest_q[1] = (s->exp_digest_q[1] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_2__q) & 0xFFFFFFFFULL) << 0);
    s->exp_digest_q[1] = (s->exp_digest_q[1] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_3__q) & 0xFFFFFFFFULL) << 32);
    s->exp_digest_q[2] = (s->exp_digest_q[2] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_4__q) & 0xFFFFFFFFULL) << 0);
    s->exp_digest_q[2] = (s->exp_digest_q[2] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_5__q) & 0xFFFFFFFFULL) << 32);
    s->exp_digest_q[3] = (s->exp_digest_q[3] & ~0xFFFFFFFFULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_6__q) & 0xFFFFFFFFULL) << 0);
    s->exp_digest_q[3] = (s->exp_digest_q[3] & ~0xFFFFFFFF00000000ULL) | (((uint64_t)(s->u_reg_regs_reg2hw_exp_digest_7__q) & 0xFFFFFFFFULL) << 32);
    s->u_reg_regs_intg_err_o = (((s->u_reg_regs_err_q) | (s->u_reg_regs_intg_err) | (s->u_reg_regs_reg_we_err))) & ((1ULL << 1) - 1);
    s->hw2reg_fatal_alert_cause_integrity_error_d = ((s->u_tl_adapter_rom_intg_error_o) | (s->u_reg_regs_intg_err_o)) & ((1ULL << 1) - 1);
    s->hw2reg_fatal_alert_cause_integrity_error_de = ((s->u_tl_adapter_rom_intg_error_o) | (s->u_reg_regs_intg_err_o)) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_fatal_alert_cause_integrity_error_d = (s->hw2reg_fatal_alert_cause_integrity_error_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_fatal_alert_cause_integrity_error_de = (s->hw2reg_fatal_alert_cause_integrity_error_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_de = (s->u_reg_regs_hw2reg_fatal_alert_cause_integrity_error_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_d = (s->u_reg_regs_hw2reg_fatal_alert_cause_integrity_error_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_de = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_d = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_wr_en = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_wr_data = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_data = (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_ds = (((s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en) ? (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_data) : (s->u_reg_regs_u_fatal_alert_cause_integrity_error_qs))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    memcpy(s->gen_fsm_scramble_enabled_u_checker_fsm_digest_i, s->digest_q, sizeof(s->gen_fsm_scramble_enabled_u_checker_fsm_digest_i));
    memcpy(s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_i, s->exp_digest_q, sizeof(s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_i));
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_rdy_i = (s->kmac_data_i_ready) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i = (s->kmac_data_i_done) & ((1ULL << 1) - 1);
    for (unsigned _qp_w = 0; _qp_w < 4; ++_qp_w)
        s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_digest_i[_qp_w] = (((s->kmac_data_i_digest_share0[_qp_w])) ^ ((s->kmac_data_i_digest_share1[_qp_w])));
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_err_i = (s->kmac_data_i_error) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_rom_data_i = (((s->u_mux_chk_rdata_o) >> 0) & 0xFFFFFFFF);
    s->gen_fsm_scramble_enabled_u_checker_fsm_fsm_alert = (0) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_d = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_rdy_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_rdy_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_rdy_i) ? (0) : s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_d)) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_err_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_err_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q) == (8191))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_d = ((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q) + (1)) & ((1ULL << 13) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q) == (8182))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_o) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_read_req_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_counter_read_req = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_read_req_o) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_data_addr_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q) & ((1ULL << 13) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_data_last_nontop_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_counter_lnt = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_data_last_nontop_o) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_start_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q) & ((1ULL << 1) - 1);
    memcpy(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_digest_i, s->gen_fsm_scramble_enabled_u_checker_fsm_digest_i, sizeof(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_digest_i));
    memcpy(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_exp_digest_i, s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_i, sizeof(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_exp_digest_i));
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_fsm_alert = (0) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_start_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_start_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_state_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_state_o) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_d = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_fsm_alert = (((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (4)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (18))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (25))))) ? (-1) : s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_fsm_alert)) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_clr_i = (0) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_i = (0) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_cnt_i = (0) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_decr_en_i = (0) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_step_i = (1) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_commit_i = (1) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_set_val = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_cnt_i) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_incr_en = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_decr_en_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_set_val = ((7) - (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_cnt_i)) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_step_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_step_i) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_ = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_ = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_d = ((((((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_)))) + (((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_))))) != (7))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_o) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_d = (((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (4)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (18)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q) == (7)))) || ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (4))) && (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_start_i))) && ((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (4)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (18)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q) == (7)))) || ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (4))) && (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_start_i)))) ? (((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (4)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (18)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q) == (7)))) ? (25) : (18))) : s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_d)) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_d = ((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q) & (((qpw_wide_extract(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_digest_i, 4, (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q)) << 5) | ((uint64_t)(31)))) + (225)) & 255ULL), 32)) == (qpw_wide_extract(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_exp_digest_i, 4, (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q)) << 5) | ((uint64_t)(31)))) + (225)) & 255ULL), 32))))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_state_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_d) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_state_i) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_d_i) & ((1ULL << 5) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_incr_en_i = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (18))) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q) != (7))))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_incr_en = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_incr_en_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_ext_cnt = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_decr_en_i) ? ((((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_)))) - (((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_step_i))))) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_incr_en) ? ((((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_)))) + (((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_step_i))))) : (((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_)))))))) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_oflow = ((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_incr_en) & ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_ext_cnt) >> 3) & 0x1))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_cnt_sat = ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_decr_en_i) & ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_ext_cnt) >> 3) & 0x1))) ? (0) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_oflow) ? (7) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_ext_cnt) & 0x7)))))) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_cnt_en = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_incr_en) ^ (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_decr_en_i)) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_incr_en) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_) != (7)))) | ((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_decr_en_i) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_) != (0)))))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_0_ = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_clr_i) ? (0) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_i) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_set_val) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_cnt_en) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_cnt_sat) : (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_))))))) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_committed_0_ = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_commit_i) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_0_) : (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_0_))) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_ext_cnt = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_incr_en_i) ? ((((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_)))) - (((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_step_i))))) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_incr_en) ? ((((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_)))) + (((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_step_i))))) : (((((uint64_t)(0)) << 3) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_)))))))) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_oflow = ((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_incr_en) & ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_ext_cnt) >> 3) & 0x1))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_cnt_sat = ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_incr_en_i) & ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_ext_cnt) >> 3) & 0x1))) ? (0) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_oflow) ? (7) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_ext_cnt) & 0x7)))))) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_cnt_en = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_incr_en) ^ (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_incr_en_i)) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_incr_en) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_) != (7)))) | ((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_incr_en_i) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_) != (0)))))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_1_ = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_clr_i) ? (7) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_set_i) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_set_val) : (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_cnt_en) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_cnt_sat) : (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_))))))) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_committed_1_ = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_commit_i) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_1_) : (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_q_1_))) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_committed_0_) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_d_i) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_committed_1_) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_d_i) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_after_commit_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_cnt_d_0_) & ((1ULL << 3) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_mubi_i = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q) ? (6) : (9))) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_mubi_i) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_d_i) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_mubi_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_done_o = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (25))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_checker_done = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_done_o) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_good_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_mubi_o) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_alert_o = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_fsm_alert) | (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_start_i) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) != (4))))) | (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (4))) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q) != (0))))) | (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (25))) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_addr_q) != (7))))) | (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_o))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_clk_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_clk_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_rst_ni = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_rst_ni) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_state_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_state_q = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_state_o) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_state_d = (s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_state_d = ((((((((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) || ((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_checker_done) : (s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) || (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185)))) && (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (1))))) || ((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201))) && ((s->gen_fsm_scramble_enabled_u_checker_fsm_counter_lnt) & (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_rdy_i)))) && (((((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) && (((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_checker_done) : (s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) || (((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633)))) && (((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_checker_done) : (s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done))))) || (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185)))) && (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (1))))) || ((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201))) && ((s->gen_fsm_scramble_enabled_u_checker_fsm_counter_lnt) & (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_rdy_i))))) ? (((((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) || ((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_checker_done) : (s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) ? (((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345)))) ? (518) : (345))) : (((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185)))) && (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (1)))) ? (921) : (185))))) : s->gen_fsm_scramble_enabled_u_checker_fsm_state_d)) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_state_d = ((((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921)))) && (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) || (((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185)))) && (!(((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (1))))) && (!(((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (2))))) && (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (3))))) || ((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185)))) && (!(((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (1))))) && (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (2))))) ? (((s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_err_i) ? (297) : ((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185)))) && (!(((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (1))))) && (((((((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i)) << 1) | ((uint64_t)(s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done)))) == (2)))) ? (633) : (345))))) : s->gen_fsm_scramble_enabled_u_checker_fsm_state_d)) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_fsm_alert = ((((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (518))))) ? (-1) : s->gen_fsm_scramble_enabled_u_checker_fsm_fsm_alert)) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_state_d = ((((((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345))))) && (!(((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (518))))) ? (297) : s->gen_fsm_scramble_enabled_u_checker_fsm_state_d)) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_state_d = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_checker_done) & ((((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (345))) | (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (518)))) ^ 1))) | ((s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201)))) | ((s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i) & ((((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))) | (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (921)))) ^ 1)))) ? (297) : s->gen_fsm_scramble_enabled_u_checker_fsm_state_d)) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_d = ((((s->gen_fsm_scramble_enabled_u_checker_fsm_counter_read_req) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (201))) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_counter_lnt) ^ 1))) ? (-1) : s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_d)) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o = ((s->gen_fsm_scramble_enabled_u_checker_fsm_fsm_alert) | (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_alert_o) | ((((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) & 0xF)) != (9))) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done) ^ 1)))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_data_rdy_i = (((s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_rdy_i) | (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))) | (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_go = ((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_data_rdy_i) & (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_d) ^ 1))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_read_addr_o = (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_go) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_d) : (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q))) & ((1ULL << 13) - 1);
    memcpy(s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o, s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_digest_i, sizeof(s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o));
    s->hw2reg_digest_0__d = ((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[0]) & 0xFFFFFFFFULL);
    s->hw2reg_digest_1__d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[0]) >> 32) & 0xFFFFFFFFULL);
    s->hw2reg_digest_2__d = ((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[1]) & 0xFFFFFFFFULL);
    s->hw2reg_digest_3__d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[1]) >> 32) & 0xFFFFFFFFULL);
    s->hw2reg_digest_4__d = ((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[2]) & 0xFFFFFFFFULL);
    s->hw2reg_digest_5__d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[2]) >> 32) & 0xFFFFFFFFULL);
    s->hw2reg_digest_6__d = ((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[3]) & 0xFFFFFFFFULL);
    s->hw2reg_digest_7__d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_digest_o[3]) >> 32) & 0xFFFFFFFFULL);
    s->u_reg_regs_hw2reg_digest_7__d = ((((uint64_t)(s->hw2reg_digest_7__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_digest_6__d = ((((uint64_t)(s->hw2reg_digest_6__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_digest_5__d = ((((uint64_t)(s->hw2reg_digest_5__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_digest_4__d = ((((uint64_t)(s->hw2reg_digest_4__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_digest_3__d = ((((uint64_t)(s->hw2reg_digest_3__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_digest_2__d = ((((uint64_t)(s->hw2reg_digest_2__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_digest_1__d = ((((uint64_t)(s->hw2reg_digest_1__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_digest_0__d = ((((uint64_t)(s->hw2reg_digest_0__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_u_digest_0_d = s->u_reg_regs_hw2reg_digest_0__d;
    s->u_reg_regs_u_digest_0_wr_en_data_arb_d = s->u_reg_regs_u_digest_0_d;
    s->u_reg_regs_u_digest_0_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_0_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_0_wr_data = s->u_reg_regs_u_digest_0_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_digest_1_d = s->u_reg_regs_hw2reg_digest_1__d;
    s->u_reg_regs_u_digest_1_wr_en_data_arb_d = s->u_reg_regs_u_digest_1_d;
    s->u_reg_regs_u_digest_1_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_1_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_1_wr_data = s->u_reg_regs_u_digest_1_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_digest_2_d = s->u_reg_regs_hw2reg_digest_2__d;
    s->u_reg_regs_u_digest_2_wr_en_data_arb_d = s->u_reg_regs_u_digest_2_d;
    s->u_reg_regs_u_digest_2_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_2_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_2_wr_data = s->u_reg_regs_u_digest_2_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_digest_3_d = s->u_reg_regs_hw2reg_digest_3__d;
    s->u_reg_regs_u_digest_3_wr_en_data_arb_d = s->u_reg_regs_u_digest_3_d;
    s->u_reg_regs_u_digest_3_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_3_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_3_wr_data = s->u_reg_regs_u_digest_3_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_digest_4_d = s->u_reg_regs_hw2reg_digest_4__d;
    s->u_reg_regs_u_digest_4_wr_en_data_arb_d = s->u_reg_regs_u_digest_4_d;
    s->u_reg_regs_u_digest_4_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_4_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_4_wr_data = s->u_reg_regs_u_digest_4_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_digest_5_d = s->u_reg_regs_hw2reg_digest_5__d;
    s->u_reg_regs_u_digest_5_wr_en_data_arb_d = s->u_reg_regs_u_digest_5_d;
    s->u_reg_regs_u_digest_5_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_5_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_5_wr_data = s->u_reg_regs_u_digest_5_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_digest_6_d = s->u_reg_regs_hw2reg_digest_6__d;
    s->u_reg_regs_u_digest_6_wr_en_data_arb_d = s->u_reg_regs_u_digest_6_d;
    s->u_reg_regs_u_digest_6_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_6_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_6_wr_data = s->u_reg_regs_u_digest_6_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_digest_7_d = s->u_reg_regs_hw2reg_digest_7__d;
    s->u_reg_regs_u_digest_7_wr_en_data_arb_d = s->u_reg_regs_u_digest_7_d;
    s->u_reg_regs_u_digest_7_wr_en_data_arb_wr_data = s->u_reg_regs_u_digest_7_wr_en_data_arb_d;
    s->u_reg_regs_u_digest_7_wr_data = s->u_reg_regs_u_digest_7_wr_en_data_arb_wr_data;
    s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_done_i) & ((1ULL << 1) - 1);
    s->hw2reg_digest_0__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->hw2reg_digest_1__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->hw2reg_digest_2__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->hw2reg_digest_3__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->hw2reg_digest_4__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->hw2reg_digest_5__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->hw2reg_digest_6__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->hw2reg_digest_7__de = (s->gen_fsm_scramble_enabled_u_checker_fsm_digest_vld_o) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_7__de = (((((uint64_t)(s->hw2reg_digest_7__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_6__de = (((((uint64_t)(s->hw2reg_digest_6__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_5__de = (((((uint64_t)(s->hw2reg_digest_5__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_4__de = (((((uint64_t)(s->hw2reg_digest_4__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_3__de = (((((uint64_t)(s->hw2reg_digest_3__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_2__de = (((((uint64_t)(s->hw2reg_digest_2__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_1__de = (((((uint64_t)(s->hw2reg_digest_1__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_digest_0__de = (((((uint64_t)(s->hw2reg_digest_0__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_de = (s->u_reg_regs_hw2reg_digest_0__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_wr_en_data_arb_de = (s->u_reg_regs_u_digest_0_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_0_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_wr_en = (s->u_reg_regs_u_digest_0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_0_ds = ((s->u_reg_regs_u_digest_0_wr_en) ? (s->u_reg_regs_u_digest_0_wr_data) : (s->u_reg_regs_u_digest_0_qs));
    s->u_reg_regs_u_digest_1_de = (s->u_reg_regs_hw2reg_digest_1__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_wr_en_data_arb_de = (s->u_reg_regs_u_digest_1_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_1_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_wr_en = (s->u_reg_regs_u_digest_1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_1_ds = ((s->u_reg_regs_u_digest_1_wr_en) ? (s->u_reg_regs_u_digest_1_wr_data) : (s->u_reg_regs_u_digest_1_qs));
    s->u_reg_regs_u_digest_2_de = (s->u_reg_regs_hw2reg_digest_2__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_wr_en_data_arb_de = (s->u_reg_regs_u_digest_2_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_2_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_wr_en = (s->u_reg_regs_u_digest_2_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_2_ds = ((s->u_reg_regs_u_digest_2_wr_en) ? (s->u_reg_regs_u_digest_2_wr_data) : (s->u_reg_regs_u_digest_2_qs));
    s->u_reg_regs_u_digest_3_de = (s->u_reg_regs_hw2reg_digest_3__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_wr_en_data_arb_de = (s->u_reg_regs_u_digest_3_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_3_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_wr_en = (s->u_reg_regs_u_digest_3_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_3_ds = ((s->u_reg_regs_u_digest_3_wr_en) ? (s->u_reg_regs_u_digest_3_wr_data) : (s->u_reg_regs_u_digest_3_qs));
    s->u_reg_regs_u_digest_4_de = (s->u_reg_regs_hw2reg_digest_4__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_wr_en_data_arb_de = (s->u_reg_regs_u_digest_4_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_4_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_wr_en = (s->u_reg_regs_u_digest_4_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_4_ds = ((s->u_reg_regs_u_digest_4_wr_en) ? (s->u_reg_regs_u_digest_4_wr_data) : (s->u_reg_regs_u_digest_4_qs));
    s->u_reg_regs_u_digest_5_de = (s->u_reg_regs_hw2reg_digest_5__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_wr_en_data_arb_de = (s->u_reg_regs_u_digest_5_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_5_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_wr_en = (s->u_reg_regs_u_digest_5_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_5_ds = ((s->u_reg_regs_u_digest_5_wr_en) ? (s->u_reg_regs_u_digest_5_wr_data) : (s->u_reg_regs_u_digest_5_qs));
    s->u_reg_regs_u_digest_6_de = (s->u_reg_regs_hw2reg_digest_6__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_wr_en_data_arb_de = (s->u_reg_regs_u_digest_6_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_6_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_wr_en = (s->u_reg_regs_u_digest_6_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_6_ds = ((s->u_reg_regs_u_digest_6_wr_en) ? (s->u_reg_regs_u_digest_6_wr_data) : (s->u_reg_regs_u_digest_6_qs));
    s->u_reg_regs_u_digest_7_de = (s->u_reg_regs_hw2reg_digest_7__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_wr_en_data_arb_de = (s->u_reg_regs_u_digest_7_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_wr_en_data_arb_wr_en = (s->u_reg_regs_u_digest_7_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_wr_en = (s->u_reg_regs_u_digest_7_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_digest_7_ds = ((s->u_reg_regs_u_digest_7_wr_en) ? (s->u_reg_regs_u_digest_7_wr_data) : (s->u_reg_regs_u_digest_7_qs));
    s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o = s->gen_fsm_scramble_enabled_u_checker_fsm_rom_data_i;
    s->hw2reg_exp_digest_0__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->hw2reg_exp_digest_1__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->hw2reg_exp_digest_2__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->hw2reg_exp_digest_3__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->hw2reg_exp_digest_4__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->hw2reg_exp_digest_5__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->hw2reg_exp_digest_6__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->hw2reg_exp_digest_7__d = s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_o;
    s->u_reg_regs_hw2reg_exp_digest_7__d = ((((uint64_t)(s->hw2reg_exp_digest_7__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_exp_digest_6__d = ((((uint64_t)(s->hw2reg_exp_digest_6__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_exp_digest_5__d = ((((uint64_t)(s->hw2reg_exp_digest_5__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_exp_digest_4__d = ((((uint64_t)(s->hw2reg_exp_digest_4__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_exp_digest_3__d = ((((uint64_t)(s->hw2reg_exp_digest_3__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_exp_digest_2__d = ((((uint64_t)(s->hw2reg_exp_digest_2__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_exp_digest_1__d = ((((uint64_t)(s->hw2reg_exp_digest_1__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_hw2reg_exp_digest_0__d = ((((uint64_t)(s->hw2reg_exp_digest_0__d)) & ((1ULL << 32) - 1)));
    s->u_reg_regs_u_exp_digest_0_d = s->u_reg_regs_hw2reg_exp_digest_0__d;
    s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_0_d;
    s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_0_wr_data = s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_exp_digest_1_d = s->u_reg_regs_hw2reg_exp_digest_1__d;
    s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_1_d;
    s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_1_wr_data = s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_exp_digest_2_d = s->u_reg_regs_hw2reg_exp_digest_2__d;
    s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_2_d;
    s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_2_wr_data = s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_exp_digest_3_d = s->u_reg_regs_hw2reg_exp_digest_3__d;
    s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_3_d;
    s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_3_wr_data = s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_exp_digest_4_d = s->u_reg_regs_hw2reg_exp_digest_4__d;
    s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_4_d;
    s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_4_wr_data = s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_exp_digest_5_d = s->u_reg_regs_hw2reg_exp_digest_5__d;
    s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_5_d;
    s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_5_wr_data = s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_exp_digest_6_d = s->u_reg_regs_hw2reg_exp_digest_6__d;
    s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_6_d;
    s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_6_wr_data = s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_wr_data;
    s->u_reg_regs_u_exp_digest_7_d = s->u_reg_regs_hw2reg_exp_digest_7__d;
    s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_d = s->u_reg_regs_u_exp_digest_7_d;
    s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_wr_data = s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_d;
    s->u_reg_regs_u_exp_digest_7_wr_data = s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_wr_data;
    s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o = (((((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (185))) | (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) == (633))))) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_counter_done) ^ (1))))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o = ((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_data_addr_o) >> 0) & 0x7)) & ((1ULL << 3) - 1);
    s->hw2reg_exp_digest_0__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (0)))) & ((1ULL << 1) - 1);
    s->hw2reg_exp_digest_1__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (1)))) & ((1ULL << 1) - 1);
    s->hw2reg_exp_digest_2__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (2)))) & ((1ULL << 1) - 1);
    s->hw2reg_exp_digest_3__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (3)))) & ((1ULL << 1) - 1);
    s->hw2reg_exp_digest_4__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (4)))) & ((1ULL << 1) - 1);
    s->hw2reg_exp_digest_5__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (5)))) & ((1ULL << 1) - 1);
    s->hw2reg_exp_digest_6__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (6)))) & ((1ULL << 1) - 1);
    s->hw2reg_exp_digest_7__de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_vld_o) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_exp_digest_idx_o) == (7)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_7__de = (((((uint64_t)(s->hw2reg_exp_digest_7__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_6__de = (((((uint64_t)(s->hw2reg_exp_digest_6__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_5__de = (((((uint64_t)(s->hw2reg_exp_digest_5__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_4__de = (((((uint64_t)(s->hw2reg_exp_digest_4__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_3__de = (((((uint64_t)(s->hw2reg_exp_digest_3__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_2__de = (((((uint64_t)(s->hw2reg_exp_digest_2__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_1__de = (((((uint64_t)(s->hw2reg_exp_digest_1__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_exp_digest_0__de = (((((uint64_t)(s->hw2reg_exp_digest_0__de)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_de = (s->u_reg_regs_hw2reg_exp_digest_0__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_0_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_wr_en = (s->u_reg_regs_u_exp_digest_0_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_0_ds = ((s->u_reg_regs_u_exp_digest_0_wr_en) ? (s->u_reg_regs_u_exp_digest_0_wr_data) : (s->u_reg_regs_u_exp_digest_0_qs));
    s->u_reg_regs_u_exp_digest_1_de = (s->u_reg_regs_hw2reg_exp_digest_1__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_1_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_wr_en = (s->u_reg_regs_u_exp_digest_1_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_1_ds = ((s->u_reg_regs_u_exp_digest_1_wr_en) ? (s->u_reg_regs_u_exp_digest_1_wr_data) : (s->u_reg_regs_u_exp_digest_1_qs));
    s->u_reg_regs_u_exp_digest_2_de = (s->u_reg_regs_hw2reg_exp_digest_2__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_2_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_wr_en = (s->u_reg_regs_u_exp_digest_2_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_2_ds = ((s->u_reg_regs_u_exp_digest_2_wr_en) ? (s->u_reg_regs_u_exp_digest_2_wr_data) : (s->u_reg_regs_u_exp_digest_2_qs));
    s->u_reg_regs_u_exp_digest_3_de = (s->u_reg_regs_hw2reg_exp_digest_3__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_3_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_wr_en = (s->u_reg_regs_u_exp_digest_3_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_3_ds = ((s->u_reg_regs_u_exp_digest_3_wr_en) ? (s->u_reg_regs_u_exp_digest_3_wr_data) : (s->u_reg_regs_u_exp_digest_3_qs));
    s->u_reg_regs_u_exp_digest_4_de = (s->u_reg_regs_hw2reg_exp_digest_4__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_4_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_wr_en = (s->u_reg_regs_u_exp_digest_4_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_4_ds = ((s->u_reg_regs_u_exp_digest_4_wr_en) ? (s->u_reg_regs_u_exp_digest_4_wr_data) : (s->u_reg_regs_u_exp_digest_4_qs));
    s->u_reg_regs_u_exp_digest_5_de = (s->u_reg_regs_hw2reg_exp_digest_5__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_5_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_wr_en = (s->u_reg_regs_u_exp_digest_5_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_5_ds = ((s->u_reg_regs_u_exp_digest_5_wr_en) ? (s->u_reg_regs_u_exp_digest_5_wr_data) : (s->u_reg_regs_u_exp_digest_5_qs));
    s->u_reg_regs_u_exp_digest_6_de = (s->u_reg_regs_hw2reg_exp_digest_6__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_6_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_wr_en = (s->u_reg_regs_u_exp_digest_6_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_6_ds = ((s->u_reg_regs_u_exp_digest_6_wr_en) ? (s->u_reg_regs_u_exp_digest_6_wr_data) : (s->u_reg_regs_u_exp_digest_6_qs));
    s->u_reg_regs_u_exp_digest_7_de = (s->u_reg_regs_hw2reg_exp_digest_7__de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_de = (s->u_reg_regs_u_exp_digest_7_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_wr_en = (s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_wr_en = (s->u_reg_regs_u_exp_digest_7_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_exp_digest_7_ds = ((s->u_reg_regs_u_exp_digest_7_wr_en) ? (s->u_reg_regs_u_exp_digest_7_wr_data) : (s->u_reg_regs_u_exp_digest_7_qs));
    s->gen_fsm_scramble_enabled_u_checker_fsm_pwrmgr_data_o_done = ((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) >> 0) & 0xF)) & ((1ULL << 4) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_pwrmgr_data_o_good = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_good_o) & ((1ULL << 4) - 1);
    memcpy(s->gen_fsm_scramble_enabled_u_checker_fsm_keymgr_data_o_data, s->gen_fsm_scramble_enabled_u_checker_fsm_digest_i, sizeof(s->gen_fsm_scramble_enabled_u_checker_fsm_keymgr_data_o_data));
    s->gen_fsm_scramble_enabled_u_checker_fsm_keymgr_data_o_valid = ((((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) >> 0) & 0xF)) != (9))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_last_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_counter_lnt) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_rom_select_bus_o = ((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) >> 0) & 0xF)) & ((1ULL << 4) - 1);
    s->u_mux_sel_bus_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_rom_select_bus_o) & ((1ULL << 4) - 1);
    s->u_mux_alert_d = (((((((s->u_mux_sel_bus_i) == (6))) | (((s->u_mux_sel_bus_i) == (9)))) ^ 1)) | ((((((s->u_mux_u_sel_bus_q_flop_q_o) == (6))) | (((s->u_mux_u_sel_bus_q_flop_q_o) == (9)))) ^ 1)) | ((((s->u_mux_u_sel_bus_q_flop_q_o) != (9))) & (((s->u_mux_sel_bus_i) != (6)))) | ((((s->u_mux_u_sel_bus_qq_flop_q_o) != (9))) & (((s->u_mux_u_sel_bus_q_flop_q_o) != (6))))) & ((1ULL << 1) - 1);
    s->u_mux_u_sel_bus_q_flop_d_i = (((((6) & ((s->u_mux_u_sel_bus_q_flop_q_o) | (s->u_mux_sel_bus_i))) | (~(6) & (s->u_mux_u_sel_bus_q_flop_q_o) & (s->u_mux_sel_bus_i))) & 0xFULL)) & ((1ULL << 4) - 1);
    s->u_mux_u_sel_bus_q_flop_d_i = (s->u_mux_u_sel_bus_q_flop_d_i) & ((1ULL << 4) - 1);
    s->u_mux_bus_gnt_o = (((s->u_mux_sel_bus_i) == (6))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_gnt_i = (s->u_mux_bus_gnt_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_missed_err_gnt_d = ((s->u_tl_adapter_rom_u_sram_byte_error_o) & (s->u_tl_adapter_rom_tl_i_int_a_valid) & (((((s->u_tl_adapter_rom_gnt_i) | (s->u_tl_adapter_rom_missed_err_gnt_q)) & (s->u_tl_adapter_rom_u_reqfifo_wready_o) & (s->u_tl_adapter_rom_u_sramreqfifo_wready_o)) ^ 1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_a_ready = (((((s->u_tl_adapter_rom_gnt_i) | (s->u_tl_adapter_rom_missed_err_gnt_q))) & (s->u_tl_adapter_rom_u_reqfifo_wready_o) & (s->u_tl_adapter_rom_u_sramreqfifo_wready_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_a_ready = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_a_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_a_ready = (s->u_tl_adapter_rom_u_sram_byte_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_a_ready = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_a_ready = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_a_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_a_ready = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_wvalid_i = (((s->u_tl_adapter_rom_tl_i_int_a_valid) & (((((s->u_tl_adapter_rom_gnt_i) | (s->u_tl_adapter_rom_missed_err_gnt_q))) & (s->u_tl_adapter_rom_u_reqfifo_wready_o) & (s->u_tl_adapter_rom_u_sramreqfifo_wready_o))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_fifo_incr_wptr = ((s->u_tl_adapter_rom_u_reqfifo_wvalid_i) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_full_o) ^ 1)) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst) ^ 1)))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_fifo_incr_wptr) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i) & (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_oflow = ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_sat = ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_oflow) ? (3) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_en = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) ^ (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i)) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) != (3)))) | ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_oflow = ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_sat = ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_oflow) ? (3) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_en = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) ^ (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i)) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_) != (3)))) | ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i) ? (0) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_set_val) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_en) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_sat) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_0_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_1_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i) ? (3) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_set_val) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_en) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_sat) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_1_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_1_) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_1_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_after_commit_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_wvalid_i = (((((s->u_tl_adapter_rom_tl_i_int_a_valid) & (s->u_tl_adapter_rom_u_reqfifo_wready_o) & (((s->u_tl_adapter_rom_u_sram_byte_error_o) ^ (1))))) & (s->u_tl_adapter_rom_gnt_i) & (((s->u_tl_adapter_rom_we_o) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_fifo_incr_wptr = ((s->u_tl_adapter_rom_u_sramreqfifo_wvalid_i) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_full_o) ^ 1)) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst) ^ 1)))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_fifo_incr_wptr) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i) & (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_wptr_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_oflow = ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_sat = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_oflow) ? (3) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_en = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) ^ (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i)) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) != (3)))) | ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_oflow = ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_sat = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_oflow) ? (3) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_en = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) ^ (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i)) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_) != (3)))) | ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i) ? (0) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_set_val) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_en) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_sat) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_0_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_1_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i) ? (3) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_set_val) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_en) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_sat) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_1_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_1_) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_1_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_after_commit_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_tl_o_a_ready = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_rom_addr_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_read_addr_o) & ((1ULL << 13) - 1);
    s->u_mux_chk_addr_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_rom_addr_o) & ((1ULL << 13) - 1);
    s->u_mux_chk_addr_i = (s->u_mux_chk_addr_i) & ((1ULL << 13) - 1);
    s->u_mux_rom_rom_addr_o = (((((s->u_mux_sel_bus_i) == (6))) ? (s->u_mux_bus_rom_addr_i) : (s->u_mux_chk_addr_i))) & ((1ULL << 13) - 1);
    s->u_mux_rom_prince_addr_o = (((((s->u_mux_sel_bus_i) == (6))) ? (s->u_mux_bus_prince_addr_i) : (s->u_mux_chk_addr_i))) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_rom_addr_i = (s->u_mux_rom_rom_addr_o) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_prince_addr_i = (s->u_mux_rom_prince_addr_o) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_i = (s->gen_rom_scramble_enabled_u_rom_rom_addr_i) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_state_0_ = (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_i) & ((1ULL << 13) - 1);
    /* comb SCC -12: 31 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_state_0_) ^ (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_key_i)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) & 0xF))) < 16u) ? ((const uint64_t[16]){0xCULL,0x5ULL,0x6ULL,0xBULL,0x9ULL,0x0ULL,0xAULL,0xDULL,0x3ULL,0xEULL,0xFULL,0x8ULL,0x4ULL,0x7ULL,0x1ULL,0x2ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) & 0xF)))] : 0)) & 0xFULL) << 0)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xCULL,0x5ULL,0x6ULL,0xBULL,0x9ULL,0x0ULL,0xAULL,0xDULL,0x3ULL,0xEULL,0xFULL,0x8ULL,0x4ULL,0x7ULL,0x1ULL,0x2ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0xF00ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xCULL,0x5ULL,0x6ULL,0xBULL,0x9ULL,0x0ULL,0xAULL,0xDULL,0x3ULL,0xEULL,0xFULL,0x8ULL,0x4ULL,0x7ULL,0x1ULL,0x2ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 8) & 0xF)))] : 0)) & 0xFULL) << 8)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x1000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) & 1)) & 0x1ULL) << 12)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x800ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 1) & 0x1)) & 0x1ULL) << 11)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x400ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 2) & 0x1)) & 0x1ULL) << 10)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x200ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 3) & 0x1)) & 0x1ULL) << 9)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x100ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 4) & 0x1)) & 0x1ULL) << 8)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x80ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 5) & 0x1)) & 0x1ULL) << 7)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x40ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 6) & 0x1)) & 0x1ULL) << 6)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x20ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 7) & 0x1)) & 0x1ULL) << 5)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x10ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 8) & 0x1)) & 0x1ULL) << 4)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x8ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 9) & 0x1)) & 0x1ULL) << 3)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x4ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 10) & 0x1)) & 0x1ULL) << 2)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x2ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 11) & 0x1)) & 0x1ULL) << 1)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped & ~0x1ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) >> 12) & 0x1)) & 0x1ULL) << 0)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x1ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) & 1)) & 0x1ULL) << 0)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x40ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 1) & 0x1)) & 0x1ULL) << 6)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x2ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 2) & 0x1)) & 0x1ULL) << 1)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x80ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 3) & 0x1)) & 0x1ULL) << 7)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x4ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 4) & 0x1)) & 0x1ULL) << 2)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x100ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 5) & 0x1)) & 0x1ULL) << 8)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x8ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 6) & 0x1)) & 0x1ULL) << 3)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x200ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 7) & 0x1)) & 0x1ULL) << 9)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x10ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 8) & 0x1)) & 0x1ULL) << 4)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x400ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 9) & 0x1)) & 0x1ULL) << 10)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x20ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 10) & 0x1)) & 0x1ULL) << 5)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox & ~0x800ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_flipped) >> 11) & 0x1)) & 0x1ULL) << 11)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_state_1_ = (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_0_data_state_sbox) & ((1ULL << 13) - 1);
    }
    /* comb SCC -13: 31 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_state_1_) ^ (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_key_i)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) & 0xF))) < 16u) ? ((const uint64_t[16]){0xCULL,0x5ULL,0x6ULL,0xBULL,0x9ULL,0x0ULL,0xAULL,0xDULL,0x3ULL,0xEULL,0xFULL,0x8ULL,0x4ULL,0x7ULL,0x1ULL,0x2ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) & 0xF)))] : 0)) & 0xFULL) << 0)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xCULL,0x5ULL,0x6ULL,0xBULL,0x9ULL,0x0ULL,0xAULL,0xDULL,0x3ULL,0xEULL,0xFULL,0x8ULL,0x4ULL,0x7ULL,0x1ULL,0x2ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0xF00ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xCULL,0x5ULL,0x6ULL,0xBULL,0x9ULL,0x0ULL,0xAULL,0xDULL,0x3ULL,0xEULL,0xFULL,0x8ULL,0x4ULL,0x7ULL,0x1ULL,0x2ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 8) & 0xF)))] : 0)) & 0xFULL) << 8)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x1000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) & 1)) & 0x1ULL) << 12)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x800ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 1) & 0x1)) & 0x1ULL) << 11)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x400ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 2) & 0x1)) & 0x1ULL) << 10)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x200ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 3) & 0x1)) & 0x1ULL) << 9)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x100ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 4) & 0x1)) & 0x1ULL) << 8)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x80ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 5) & 0x1)) & 0x1ULL) << 7)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x40ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 6) & 0x1)) & 0x1ULL) << 6)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x20ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 7) & 0x1)) & 0x1ULL) << 5)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x10ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 8) & 0x1)) & 0x1ULL) << 4)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x8ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 9) & 0x1)) & 0x1ULL) << 3)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x4ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 10) & 0x1)) & 0x1ULL) << 2)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x2ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 11) & 0x1)) & 0x1ULL) << 1)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped & ~0x1ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) >> 12) & 0x1)) & 0x1ULL) << 0)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x1ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) & 1)) & 0x1ULL) << 0)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x40ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 1) & 0x1)) & 0x1ULL) << 6)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x2ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 2) & 0x1)) & 0x1ULL) << 1)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x80ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 3) & 0x1)) & 0x1ULL) << 7)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x4ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 4) & 0x1)) & 0x1ULL) << 2)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x100ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 5) & 0x1)) & 0x1ULL) << 8)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x8ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 6) & 0x1)) & 0x1ULL) << 3)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x200ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 7) & 0x1)) & 0x1ULL) << 9)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x10ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 8) & 0x1)) & 0x1ULL) << 4)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x400ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 9) & 0x1)) & 0x1ULL) << 10)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x20ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 10) & 0x1)) & 0x1ULL) << 5)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox = ((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox & ~0x800ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_flipped) >> 11) & 0x1)) & 0x1ULL) << 11)) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_state_2_ = (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_gen_round_1_data_state_sbox) & ((1ULL << 13) - 1);
    }
    s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_o = (((s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_state_2_) ^ (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_key_i))) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_i = ((((uint64_t)(s->gen_rom_scramble_enabled_u_rom_prince_addr_i)) << 0) | (((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_seed_anchor_out_o[2]) & 0x7FFFFFFFFFFFFULL))) << 13));
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_i = s->gen_rom_scramble_enabled_u_rom_u_prince_data_i;
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_ = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_i) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_ = (s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k1_d);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_ = s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_;
    /* comb SCC -17: 59 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_0_) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl8_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xFULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) & 0xFF)) & (189)) & 0xF)) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 8) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 13) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF0ULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 4) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 9) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF00ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 5) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 8) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 8) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 1) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 4) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 4) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 12) & 0x7)))))) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF0000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 17) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 20) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 20) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 28) & 0x7)))))) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF00000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 16) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 16) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 24) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 29) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 16) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 20) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 25) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 16) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF0000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 16) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 21) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 24) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 24) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF00000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 33) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 36) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 36) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 44) & 0x7)))))) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 32) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 32) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 40) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 45) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF0000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 32) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 36) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 41) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 32) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF00000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 32) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 37) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 40) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 40) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 48) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 48) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 56) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 61) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF0000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 48) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 52) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 57) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 48) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF00000000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 48) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 53) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 56) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 56) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out & ~0xF000000000000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 49) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 52) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 52) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 60) & 0x7)))))) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl28_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xFULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 16) & 0xF)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF0ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 36) & 0xF)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF00ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 56) & 0xF)) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 12) & 0xF)) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF0000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 32) & 0xF)) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF00000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 52) & 0xF)) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 8) & 0xF)) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF0000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 28) & 0xF)) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF00000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 48) & 0xF)) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 4) & 0xF)) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF0000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 24) & 0xF)) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF00000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 44) & 0xF)) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF000000000000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) & 0xF)) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF0000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 20) & 0xF)) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF00000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 40) & 0xF)) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out & ~0xF000000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) >> 60) & 0xF)) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl9_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_ = (s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_1_data_state_round) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_d) ^ (1376283091369227076ULL);
    /* comb SCC -18: 59 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_1_) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl10_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xFULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) & 0xFF)) & (189)) & 0xF)) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 8) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 13) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF0ULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 4) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 9) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF00ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 5) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 8) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 8) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 1) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 4) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 4) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 12) & 0x7)))))) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF0000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 17) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 20) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 20) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 28) & 0x7)))))) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF00000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 16) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 16) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 24) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 29) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 16) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 20) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 25) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 16) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF0000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 16) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 21) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 24) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 24) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF00000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 33) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 36) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 36) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 44) & 0x7)))))) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 32) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 32) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 40) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 45) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF0000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 32) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 36) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 41) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 32) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF00000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 32) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 37) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 40) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 40) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 48) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 48) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 56) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 61) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF0000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 48) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 52) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 57) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 48) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF00000000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 48) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 53) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 56) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 56) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out & ~0xF000000000000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 49) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 52) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 52) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 60) & 0x7)))))) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl29_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xFULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 16) & 0xF)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF0ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 36) & 0xF)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF00ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 56) & 0xF)) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 12) & 0xF)) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF0000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 32) & 0xF)) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF00000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 52) & 0xF)) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 8) & 0xF)) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF0000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 28) & 0xF)) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF00000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 48) & 0xF)) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 4) & 0xF)) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF0000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 24) & 0xF)) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF00000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 44) & 0xF)) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF000000000000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) & 0xF)) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF0000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 20) & 0xF)) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF00000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 40) & 0xF)) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out & ~0xF000000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) >> 60) & 0xF)) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl11_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_ = (s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_2_data_state_round) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k1_d) ^ (11820040416388919760ULL);
    /* comb SCC -19: 59 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_2_) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl12_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xFULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) & 0xFF)) & (189)) & 0xF)) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 8) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 13) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF0ULL) | (((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 4) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 9) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF00ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)(((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 5) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 8) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 8) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 1) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 4) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 4) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 12) & 0x7)))))) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF0000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 17) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 20) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 20) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 28) & 0x7)))))) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF00000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 16) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 16) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 24) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 29) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 16) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 20) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 25) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 16) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF0000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 16) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 21) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 24) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 24) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF00000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 33) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 36) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 36) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 44) & 0x7)))))) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 32) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 32) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 40) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 45) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF0000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 32) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 36) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 41) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 32) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF00000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 32) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 37) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 40) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 40) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 48) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 48) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 56) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 61) & 0x7))) << 1) | ((uint64_t)(0))))) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF0000000000000ULL) | ((((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 48) & 0xFFFF)) & (56955)) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 52) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 57) & 0x7))) << 1) | ((uint64_t)(0)))) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 48) & 0xFFFF)) & (56955)) >> 12) & 0xF))) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF00000000000000ULL) | ((((((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 48) & 0x7))))) ^ (((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 53) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 56) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 56) & 0xFF)) & (189)) >> 4) & 0xF))) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out & ~0xF000000000000000ULL) | ((((((((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 49) & 0x7))) << 1) | ((uint64_t)(0)))) ^ (((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 52) & 0xFF)) & (189)) & 0xF)) ^ ((((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 52) & 0xFF)) & (189)) >> 4) & 0xF)) ^ (((((uint64_t)(0)) << 3) | ((uint64_t)((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 60) & 0x7)))))) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl30_state_out;
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xFULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 16) & 0xF)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF0ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 36) & 0xF)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF00ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 56) & 0xF)) & 0xFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 12) & 0xF)) & 0xFULL) << 12);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF0000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 32) & 0xF)) & 0xFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF00000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 52) & 0xF)) & 0xFULL) << 20);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 8) & 0xF)) & 0xFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF0000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 28) & 0xF)) & 0xFULL) << 28);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF00000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 48) & 0xF)) & 0xFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 4) & 0xF)) & 0xFULL) << 36);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF0000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 24) & 0xF)) & 0xFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF00000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 44) & 0xF)) & 0xFULL) << 44);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF000000000000ULL) | (((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) & 0xF)) & 0xFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF0000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 20) & 0xF)) & 0xFULL) << 52);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF00000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 40) & 0xF)) & 0xFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out & ~0xF000000000000000ULL) | ((((((s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) >> 60) & 0xF)) & 0xFULL) << 60);
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl13_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_ = (s->gen_rom_scramble_enabled_u_rom_u_prince_gen_fwd_pass_3_data_state_round) ^ (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_d) ^ (589684135938649225ULL);
    /* comb SCC -20: 25 assignment(s), local fixed point */
    for (unsigned _qp_scc = 0; _qp_scc < 8u; ++_qp_scc) {
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl0_state_out & ~0xFULL) | (((((((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)(((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl0_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl0_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 4) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 4) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFFULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl0_state_out) & 0xFFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl1_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 8) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 8) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl1_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl1_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 12) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 12) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFF00ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl1_state_out) & 0xFFULL) << 8);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl2_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 16) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 16) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl2_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl2_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 20) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 20) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFF0000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl2_state_out) & 0xFFULL) << 16);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl3_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 24) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 24) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl3_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl3_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 28) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 28) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFF000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl3_state_out) & 0xFFULL) << 24);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl4_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 32) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 32) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl4_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl4_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 36) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 36) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFF00000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl4_state_out) & 0xFFULL) << 32);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl5_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 40) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 40) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl5_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl5_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 44) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 44) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFF0000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl5_state_out) & 0xFFULL) << 40);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl6_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 48) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 48) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl6_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl6_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 52) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 52) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFF000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl6_state_out) & 0xFFULL) << 48);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl7_state_out & ~0xFULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 56) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 56) & 0xF)))] : 0)) & 0xFULL) << 0);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl7_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl7_state_out & ~0xF0ULL) | (((((((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 60) & 0xF))) < 16u) ? ((const uint64_t[16]){0xBULL,0xFULL,0x3ULL,0x2ULL,0xAULL,0xCULL,0x9ULL,0x1ULL,0x6ULL,0x7ULL,0x8ULL,0x0ULL,0xEULL,0x5ULL,0xDULL,0x4ULL})[((unsigned)((((s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_lo_3_) >> 60) & 0xF)))] : 0)) & 0xFULL) << 4);
    s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out = (s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out & ~0xFF00000000000000ULL) | (((s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_qpinl7_state_out) & 0xFFULL) << 56);
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_d = s->gen_rom_scramble_enabled_u_rom_u_prince_qpinl14_state_out;
    }
    s->gen_rom_scramble_enabled_u_rom_u_rom_addr_i = (s->gen_rom_scramble_enabled_u_rom_u_sp_addr_data_o) & ((1ULL << 13) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_addr_i = (s->gen_rom_scramble_enabled_u_rom_u_rom_addr_i) & ((1ULL << 13) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_rom_req_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_counter_read_req) & ((1ULL << 1) - 1);
    s->u_mux_chk_req_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_rom_req_o) & ((1ULL << 1) - 1);
    s->u_mux_chk_req_i = (s->u_mux_chk_req_i) & ((1ULL << 1) - 1);
    s->u_mux_rom_req_o = (((((s->u_mux_sel_bus_i) == (6))) ? (s->u_mux_bus_req_i) : (s->u_mux_chk_req_i))) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_req_i = (s->u_mux_rom_req_o) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i = (s->gen_rom_scramble_enabled_u_rom_req_i) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i = (s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_req_i = (s->gen_rom_scramble_enabled_u_rom_req_i) & ((1ULL << 1) - 1);
    s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_req_i = (s->gen_rom_scramble_enabled_u_rom_u_rom_req_i) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o = (s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o) & ((1ULL << 1) - 1);
    s->hw2reg_fatal_alert_cause_checker_error_d = ((s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o) | (s->u_mux_alert_o)) & ((1ULL << 1) - 1);
    s->hw2reg_fatal_alert_cause_checker_error_de = ((s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o) | (s->u_mux_alert_o)) & ((1ULL << 1) - 1);
    s->alerts = (((s->u_tl_adapter_rom_intg_error_o) | (s->u_reg_regs_intg_err_o)) | (s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o) | (s->u_mux_alert_o)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_rvalid_i = (((s->u_mux_bus_rvalid_o) & (((((s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o) | (s->u_mux_alert_o))) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_rready_i = (((s->u_tl_adapter_rom_rvalid_i) & (s->u_tl_adapter_rom_reqfifo_rvalid))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_empty_o) ^ (1))) & (s->u_tl_adapter_rom_u_sramreqfifo_rready_i) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i) & (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_oflow = ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_sat = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_oflow) ? (3) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_en = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) ^ (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i)) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) != (3)))) | ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_oflow = ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_sat = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_oflow) ? (3) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_en = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) ^ (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i)) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_) != (3)))) | ((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i) ? (0) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_set_val) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_en) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_sat) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_0_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_1_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i) ? (3) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_set_val) : (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_en) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_sat) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_1_ = (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_1_) : (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_1_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_after_commit_o = (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_wvalid_i = (((s->u_tl_adapter_rom_rvalid_i) & (s->u_tl_adapter_rom_reqfifo_rvalid))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_fifo_incr_wptr = ((s->u_tl_adapter_rom_u_rspfifo_wvalid_i) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_full_o) ^ 1)) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst) ^ 1)))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_rdata_int = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_empty_o) & (s->u_tl_adapter_rom_u_rspfifo_wvalid_i)) ? (s->u_tl_adapter_rom_u_rspfifo_wdata_i) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_rdata))) & ((1ULL << 40) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_fifo_incr_wptr) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i) & (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_wptr_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_incr_wptr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_oflow = ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_sat = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_oflow) ? (3) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_en = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) ^ (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i)) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_incr_en) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) != (3)))) | ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_decr_en_i) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_oflow = ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_sat = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_oflow) ? (3) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_en = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) ^ (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i)) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_incr_en) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_) != (3)))) | ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_incr_en_i) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i) ? (0) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_set_val) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_en) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_cnt_sat) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_0_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_0_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_1_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_clr_i) ? (3) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_set_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_set_val) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_en) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_cnt_sat) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_1_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_commit_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_1_) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_q_1_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_committed_1_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_after_commit_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_cnt_d_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_rvalid_o = (((((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_empty_o) & (((s->u_tl_adapter_rom_u_rspfifo_wvalid_i) ^ (1))))) ^ (1))) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_rspfifo_rvalid = (s->u_tl_adapter_rom_u_rspfifo_rvalid_o) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_d_valid = (((((s->u_tl_adapter_rom_reqfifo_rvalid) && (!(s->u_tl_adapter_rom_reqfifo_rdata_error))) && (s->u_tl_adapter_rom_reqfifo_rdata_is_read)) ? (s->u_tl_adapter_rom_rspfifo_rvalid) : s->u_tl_adapter_rom_d_valid)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_vld_rd_rsp = ((s->u_tl_adapter_rom_d_valid) & (s->u_tl_adapter_rom_rspfifo_rvalid) & (s->u_tl_adapter_rom_reqfifo_rdata_is_read)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_valid = (s->u_tl_adapter_rom_d_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_opcode = (((((uint64_t)(((((s->u_tl_adapter_rom_d_valid) & (((s->u_tl_adapter_rom_reqfifo_rdata_is_read) ^ (1))))) ^ (1)))) << 0) | (((uint64_t)(0)) << 1))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_size = (((s->u_tl_adapter_rom_d_valid) ? (s->u_tl_adapter_rom_reqfifo_rdata_size) : (0))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_source = ((s->u_tl_adapter_rom_d_valid) ? (s->u_tl_adapter_rom_reqfifo_rdata_source) : (0));
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_valid = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_valid = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_valid = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_valid = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_valid = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_opcode = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_opcode = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_opcode = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_qpinl10_payload_opcode = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_opcode = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_opcode = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_size = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_size = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_size = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_qpinl10_payload_size = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_size = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_size = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_source = s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_source;
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_source = s->u_tl_adapter_rom_u_sram_byte_tl_o_d_source;
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_source = s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_source;
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_source = s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_source;
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_source = s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_source;
    s->u_tl_adapter_rom_u_reqfifo_rready_i = (((s->u_tl_adapter_rom_d_valid) & (s->u_tl_adapter_rom_tl_i_int_d_ready))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_empty_o) ^ (1))) & (s->u_tl_adapter_rom_u_reqfifo_rready_i) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst) ^ (1))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i) & (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_rptr_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_oflow = ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_sat = ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_oflow) ? (3) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_en = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) ^ (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i)) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) != (3)))) | ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_oflow = ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_sat = ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & ((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_oflow) ? (3) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_en = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) ^ (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i)) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_) != (3)))) | ((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i) ? (0) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_set_val) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_en) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_sat) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_0_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_1_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i) ? (3) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_set_val) : (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_en) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_sat) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_1_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_1_) : (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_1_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_after_commit_o = (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_rready_i = (((s->u_tl_adapter_rom_reqfifo_rdata_is_read) & (((s->u_tl_adapter_rom_reqfifo_rdata_error) ^ (1))) & (((s->u_tl_adapter_rom_d_valid) & (s->u_tl_adapter_rom_tl_i_int_d_ready))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i = (((((((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_empty_o) & (((s->u_tl_adapter_rom_u_rspfifo_wvalid_i) ^ (1))))) ^ (1))) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst) ^ (1))))) & (s->u_tl_adapter_rom_u_rspfifo_rready_i))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i) & (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_rptr_o))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_incr_rptr_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_oflow = ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_sat = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_oflow) ? (3) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_en = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) ^ (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i)) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_incr_en) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) != (3)))) | ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_decr_en_i) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))) - (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) ? ((((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))) + (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_step_i))))) : (((((uint64_t)(0)) << 2) | ((uint64_t)(s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_)))))))) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_oflow = ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_sat = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) >> 2) & 0x1))) ? (0) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_oflow) ? (3) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_ext_cnt) & 0x3)))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_en = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) ^ (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i)) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_incr_en) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_) != (3)))) | ((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_incr_en_i) & (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_) != (0)))))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i) ? (0) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_set_val) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_en) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_cnt_sat) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_0_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_0_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_1_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_clr_i) ? (3) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_set_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_set_val) : (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_en) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_cnt_sat) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))))))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_1_ = (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_commit_i) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_1_) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_q_1_))) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_committed_1_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_after_commit_o = (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_cnt_d_0_) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_u_rspfifo_rdata_o = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_empty_o) & (((s->u_tl_adapter_rom_u_rspfifo_wvalid_i) ^ (1))))) ? (0) : (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_rdata_int))) & ((1ULL << 40) - 1);
    s->u_tl_adapter_rom_rspfifo_rdata_data = (((s->u_tl_adapter_rom_u_rspfifo_rdata_o) >> 8) & 0xFFFFFFFFULL);
    s->u_tl_adapter_rom_rspfifo_rdata_data_intg = ((((s->u_tl_adapter_rom_u_rspfifo_rdata_o) >> 1) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_rspfifo_rdata_error = (((s->u_tl_adapter_rom_u_rspfifo_rdata_o) & 1)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_d_error = ((((s->u_tl_adapter_rom_reqfifo_rvalid) && (s->u_tl_adapter_rom_reqfifo_rdata_is_read)) ? ((s->u_tl_adapter_rom_rspfifo_rdata_error) | (s->u_tl_adapter_rom_reqfifo_rdata_error)) : s->u_tl_adapter_rom_d_error)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_d_error = ((((s->u_tl_adapter_rom_reqfifo_rvalid) && (!(s->u_tl_adapter_rom_reqfifo_rdata_is_read))) ? (s->u_tl_adapter_rom_reqfifo_rdata_error) : s->u_tl_adapter_rom_d_error)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_d_error = (((!(s->u_tl_adapter_rom_reqfifo_rvalid)) ? (0) : s->u_tl_adapter_rom_d_error)) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_data = ((((s->u_tl_adapter_rom_vld_rd_rsp) & (((s->u_tl_adapter_rom_d_error) ^ (1))))) ? (s->u_tl_adapter_rom_rspfifo_rdata_data) : (s->u_tl_adapter_rom_error_blanking_data));
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_user_data_intg = (((s->u_tl_adapter_rom_reqfifo_rdata_error) ? (s->u_tl_adapter_rom_error_blanking_integ) : (((s->u_tl_adapter_rom_vld_rd_rsp) ? (s->u_tl_adapter_rom_rspfifo_rdata_data_intg) : (42))))) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_error = (((s->u_tl_adapter_rom_d_valid) & (s->u_tl_adapter_rom_d_error))) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_data = s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_data;
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_data = s->u_tl_adapter_rom_u_sram_byte_tl_o_d_data;
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_data = s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_data;
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_data = s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_data;
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_data = s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_data;
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_user_data_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_data_intg = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_data_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_data_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_data_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_data_intg = (s->u_tl_adapter_rom_u_rsp_gen_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_data_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_sram_byte_tl_o_d_error = (s->u_tl_adapter_rom_u_sram_byte_tl_sram_i_d_error) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_error = (s->u_tl_adapter_rom_u_sram_byte_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_error = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_qpinl10_payload_error = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_error = (s->u_tl_adapter_rom_u_rsp_gen_tl_i_d_error) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_i = (((((((uint64_t)(s->u_tl_adapter_rom_u_rsp_gen_qpinl10_payload_error)) << 0) | (((uint64_t)(s->u_tl_adapter_rom_u_rsp_gen_qpinl10_payload_size)) << 1)) | (((uint64_t)(s->u_tl_adapter_rom_u_rsp_gen_qpinl10_payload_opcode)) << 3)) | (((uint64_t)(0)) << 6))) & ((1ULL << 57) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_i = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_i) & ((1ULL << 57) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = ((((uint64_t)(0)) << 57) | ((uint64_t)(s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_i)));
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x200000000000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (73183459585064959ULL)))) & 0x1ULL) << 57);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x400000000000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (106995641195921439ULL)))) & 0x1ULL) << 58);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x800000000000000ULL) | ((((__builtin_parityll((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (125504822018802145ULL)))) & 0x1ULL) << 59);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x1000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 1) & 0xFFFFFFFFFFFFFFULL)) & (67403489212122897ULL))) << 1) | ((uint64_t)(0)))))) & 0x1ULL) << 60);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x2000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 2) & 0x7FFFFFFFFFFFFFULL)) & (34865184827919505ULL))) << 2) | ((uint64_t)(0)))))) & 0x1ULL) << 61);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x4000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 3) & 0x3FFFFFFFFFFFFFULL)) & (17723486863248017ULL))) << 3) | ((uint64_t)(0)))))) & 0x1ULL) << 62);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x8000000000000000ULL) | ((((__builtin_parityll(((((uint64_t)(0)) << 57) | (((uint64_t)(((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 4) & 0x1FFFFFFFFFFFFFULL)) & (8934470268372625ULL))) << 4) | ((uint64_t)(0)))))) & 0x1ULL) << 63);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = (s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) ^ (6052837899185946624ULL);
    s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o = s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o;
    s->u_tl_adapter_rom_u_rsp_gen_rsp_intg = ((((s->u_tl_adapter_rom_u_rsp_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 57) & 0x7F)) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_rsp_intg = (s->u_tl_adapter_rom_u_rsp_gen_rsp_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_rsp_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_error = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_tl_o_d_valid = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->u_tl_adapter_rom_tl_o_d_opcode = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->u_tl_adapter_rom_tl_o_d_size = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_size) & ((1ULL << 2) - 1);
    s->u_tl_adapter_rom_tl_o_d_source = s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_source;
    s->u_tl_adapter_rom_tl_o_d_data = s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_data;
    s->u_tl_adapter_rom_tl_o_d_user_rsp_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_tl_o_d_user_data_intg = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->u_tl_adapter_rom_tl_o_d_error = (s->u_tl_adapter_rom_u_rsp_gen_tl_o_d_error) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_fatal_alert_cause_checker_error_d = (s->hw2reg_fatal_alert_cause_checker_error_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_hw2reg_fatal_alert_cause_checker_error_de = (s->hw2reg_fatal_alert_cause_checker_error_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_de = (s->u_reg_regs_hw2reg_fatal_alert_cause_checker_error_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_d = (s->u_reg_regs_hw2reg_fatal_alert_cause_checker_error_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_de = (s->u_reg_regs_u_fatal_alert_cause_checker_error_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_d = (s->u_reg_regs_u_fatal_alert_cause_checker_error_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_wr_en = (s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_de) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en = (s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_wr_en) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_wr_data = (s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_d) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_data = (s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en_data_arb_wr_data) & ((1ULL << 1) - 1);
    s->u_reg_regs_u_fatal_alert_cause_checker_error_ds = (((s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en) ? (s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_data) : (s->u_reg_regs_u_fatal_alert_cause_checker_error_qs))) & ((1ULL << 1) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_state_d = (((s->gen_fsm_scramble_enabled_u_checker_fsm_alert_o) ? (297) : s->gen_fsm_scramble_enabled_u_checker_fsm_state_d)) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_state_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_state_d) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_state_i) & ((1ULL << 10) - 1);
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_d_i = (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_d_i) & ((1ULL << 10) - 1);
    s->gen_alert_tx_0_u_alert_sender_clk_i = (s->clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_rst_ni = (s->rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_test_i = (s->alert_test) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_req_i = (s->alerts) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ping_p = (s->alert_rx_i_0__ping_p) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ping_n = (s->alert_rx_i_0__ping_n) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ack_p = (s->alert_rx_i_0__ack_p) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ack_n = (s->alert_rx_i_0__ack_n) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_test_trigger = ((s->gen_alert_tx_0_u_alert_sender_alert_test_i) | (s->gen_alert_tx_0_u_alert_sender_alert_test_set_q)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_state_d = (s->gen_alert_tx_0_u_alert_sender_state_q) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_pd = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_nd = (-1) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_ping_clr = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_clr = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_rst_ni = (s->gen_alert_tx_0_u_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_in_i = (((((uint64_t)(s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ping_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ping_n)) << 1))) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_out_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_clk_i = (s->gen_alert_tx_0_u_alert_sender_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni = (s->gen_alert_tx_0_u_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_diff_pi = ((((s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_out_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_diff_ni = ((((s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ping_out_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rise_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_fall_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_diff_pi) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pd = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_p_edge = ((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_level = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pd) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_diff_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nd = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_n_edge = ((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok = ((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d = ((((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rise_o = (((((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_n_edge)))) && (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_level) ? (0) : (1))) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rise_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_fall_o = (((((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_n_edge)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_level) ? (0) : (1)))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_fall_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d = ((((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q) + (1)) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rise_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rise_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_fall_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_fall_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_event_o = (((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rise_o) | (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_fall_o))) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_ping_trigger = ((s->gen_alert_tx_0_u_alert_sender_ping_set_q) | (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_event_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_in_i = (((((uint64_t)(s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ack_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_alert_sender_alert_rx_i_ack_n)) << 1))) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_out_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_clk_i = (s->gen_alert_tx_0_u_alert_sender_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni = (s->gen_alert_tx_0_u_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_diff_pi = ((((s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_out_o) >> 0) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_diff_ni = ((((s->gen_alert_tx_0_u_alert_sender_u_prim_buf_ack_out_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rise_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_fall_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o = (0) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_diff_pi) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pd = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_p_edge = ((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_level = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pd) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_diff_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_q_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nd = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_q_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_n_edge = ((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok = ((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d = ((((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rise_o = (((((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_n_edge)))) && (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_level) ? (0) : (1))) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rise_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_fall_o = (((((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) || (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) || (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) && ((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_p_edge) & (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_n_edge)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_level) ? (0) : (1)))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_fall_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d = ((((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q) + (1)) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o = (((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d)) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_ack_level = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_clr = ((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_alert_sender_ack_level) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_alert_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_ping_clr = ((((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_alert_sender_ack_level) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_ping_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rise_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rise_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_fall_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_fall_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_event_o = (((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rise_o) | (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_fall_o))) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o = (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_sigint_detected = ((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_sigint_o) | (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_sigint_o)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_ping_clr = (((s->gen_alert_tx_0_u_alert_sender_sigint_detected) ? (-1) : s->gen_alert_tx_0_u_alert_sender_ping_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_ping_set_d = ((((s->gen_alert_tx_0_u_alert_sender_ping_clr) ^ 1)) & (s->gen_alert_tx_0_u_alert_sender_ping_trigger)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_clr = (((s->gen_alert_tx_0_u_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_alert_sender_alert_clr)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_test_set_d = ((((s->gen_alert_tx_0_u_alert_sender_alert_clr) ^ 1)) & (s->gen_alert_tx_0_u_alert_sender_alert_test_trigger)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_in_i = (s->gen_alert_tx_0_u_alert_sender_alert_req_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_in_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_out_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_set_d = ((s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_alert_sender_alert_set_q)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_trigger = (((s->gen_alert_tx_0_u_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_alert_sender_alert_set_q)) | (s->gen_alert_tx_0_u_alert_sender_alert_test_trigger)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_state_d = ((((((s->gen_alert_tx_0_u_alert_sender_state_q) == (0))) && ((s->gen_alert_tx_0_u_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_alert_sender_ping_trigger))) ? (((((uint64_t)(0)) << 2) | (((uint64_t)(((s->gen_alert_tx_0_u_alert_sender_alert_trigger) ^ 1))) << 1) | ((uint64_t)(1)))) : s->gen_alert_tx_0_u_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_pd = ((((((s->gen_alert_tx_0_u_alert_sender_state_q) == (0))) && ((s->gen_alert_tx_0_u_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_alert_sender_ping_trigger))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_alert_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_nd = ((((((s->gen_alert_tx_0_u_alert_sender_state_q) == (0))) && ((s->gen_alert_tx_0_u_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_alert_sender_ping_trigger))) ? (0) : s->gen_alert_tx_0_u_alert_sender_alert_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_state_d = (((((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (4))))) || ((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (3)))) || ((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (s->gen_alert_tx_0_u_alert_sender_ack_level))) && (((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (4))))) || (((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (3)))) && (s->gen_alert_tx_0_u_alert_sender_ack_level))) || (((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (1)))) && (s->gen_alert_tx_0_u_alert_sender_ack_level)))) ? (((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (4))))) ? (((((s->gen_alert_tx_0_u_alert_sender_state_q) == (5))) ? (6) : (0))) : ((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (3)))) ? (4) : (2))))) : s->gen_alert_tx_0_u_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_pd = ((((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (3)))) && (!(s->gen_alert_tx_0_u_alert_sender_ack_level))) || (((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_ack_level)))) ? (-1) : s->gen_alert_tx_0_u_alert_sender_alert_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_nd = ((((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (3)))) && (!(s->gen_alert_tx_0_u_alert_sender_ack_level))) || (((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_alert_sender_ack_level)))) ? (0) : s->gen_alert_tx_0_u_alert_sender_alert_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_state_d = ((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_alert_sender_ack_level) ^ 1))) ? (-3) : s->gen_alert_tx_0_u_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_alert_sender_state_d = ((((((((!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (0)))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_alert_sender_ack_level) ^ 1))) ? (-3) : s->gen_alert_tx_0_u_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_alert_sender_state_d = (((s->gen_alert_tx_0_u_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_alert_sender_state_d)) & ((1ULL << 3) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_pd = (((s->gen_alert_tx_0_u_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_alert_sender_alert_pd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_nd = (((s->gen_alert_tx_0_u_alert_sender_sigint_detected) ? (0) : s->gen_alert_tx_0_u_alert_sender_alert_nd)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_clk_i = (s->gen_alert_tx_0_u_alert_sender_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_rst_ni = (s->gen_alert_tx_0_u_alert_sender_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_d_i = (((((uint64_t)(s->gen_alert_tx_0_u_alert_sender_alert_pd)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_alert_sender_alert_nd)) << 1))) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_clk_i = (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_clk_i) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i = (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_d_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni = (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i = (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_q_o = (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o) & ((1ULL << 2) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_p = (((s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_q_o) & 1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_n = ((((s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_q_o) >> 1) & 0x1)) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_ack_o = (((s->gen_alert_tx_0_u_alert_sender_alert_clr) & (s->gen_alert_tx_0_u_alert_sender_alert_set_q))) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_state_o = (s->gen_alert_tx_0_u_alert_sender_alert_set_q) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_p = (s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_p) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_p = (s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_p) & ((1ULL << 1) - 1);
    s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_n = (s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_n) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_n = (s->gen_alert_tx_0_u_alert_sender_alert_tx_o_alert_n) & ((1ULL << 1) - 1);
    s->rom_tl_o_d_valid = (s->u_tl_adapter_rom_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->rom_tl_o_d_opcode = (s->u_tl_adapter_rom_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->rom_tl_o_d_param = (s->u_tl_adapter_rom_tl_o_d_param) & ((1ULL << 3) - 1);
    s->rom_tl_o_d_size = (s->u_tl_adapter_rom_tl_o_d_size) & ((1ULL << 2) - 1);
    s->rom_tl_o_d_source = s->u_tl_adapter_rom_tl_o_d_source;
    s->rom_tl_o_d_sink = (s->u_tl_adapter_rom_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->rom_tl_o_d_data = s->u_tl_adapter_rom_tl_o_d_data;
    s->rom_tl_o_d_user_rsp_intg = (s->u_tl_adapter_rom_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->rom_tl_o_d_user_data_intg = (s->u_tl_adapter_rom_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->rom_tl_o_d_error = (s->u_tl_adapter_rom_tl_o_d_error) & ((1ULL << 1) - 1);
    s->rom_tl_o_a_ready = (s->u_tl_adapter_rom_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->regs_tl_o_d_valid = (s->u_reg_regs_tl_o_d_valid) & ((1ULL << 1) - 1);
    s->regs_tl_o_d_opcode = (s->u_reg_regs_tl_o_d_opcode) & ((1ULL << 3) - 1);
    s->regs_tl_o_d_param = (s->u_reg_regs_tl_o_d_param) & ((1ULL << 3) - 1);
    s->regs_tl_o_d_size = (s->u_reg_regs_tl_o_d_size) & ((1ULL << 2) - 1);
    s->regs_tl_o_d_source = s->u_reg_regs_tl_o_d_source;
    s->regs_tl_o_d_sink = (s->u_reg_regs_tl_o_d_sink) & ((1ULL << 1) - 1);
    s->regs_tl_o_d_data = s->u_reg_regs_tl_o_d_data;
    s->regs_tl_o_d_user_rsp_intg = (s->u_reg_regs_tl_o_d_user_rsp_intg) & ((1ULL << 7) - 1);
    s->regs_tl_o_d_user_data_intg = (s->u_reg_regs_tl_o_d_user_data_intg) & ((1ULL << 7) - 1);
    s->regs_tl_o_d_error = (s->u_reg_regs_tl_o_d_error) & ((1ULL << 1) - 1);
    s->regs_tl_o_a_ready = (s->u_reg_regs_tl_o_a_ready) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_p = (((((uint64_t)(s->alert_tx_o_0__alert_p)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->alert_tx_o_0__alert_n = (((((uint64_t)(s->alert_tx_o_0__alert_n)) & ((1ULL << 1) - 1)))) & ((1ULL << 1) - 1);
    s->pwrmgr_data_o_done = (s->gen_fsm_scramble_enabled_u_checker_fsm_pwrmgr_data_o_done) & ((1ULL << 4) - 1);
    s->pwrmgr_data_o_good = (s->gen_fsm_scramble_enabled_u_checker_fsm_pwrmgr_data_o_good) & ((1ULL << 4) - 1);
    memcpy(s->keymgr_data_o_data, s->gen_fsm_scramble_enabled_u_checker_fsm_keymgr_data_o_data, sizeof(s->keymgr_data_o_data));
    s->keymgr_data_o_valid = (s->gen_fsm_scramble_enabled_u_checker_fsm_keymgr_data_o_valid) & ((1ULL << 1) - 1);
    s->kmac_data_o_valid = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_o) & ((1ULL << 1) - 1);
    s->kmac_data_o_data = ((((uint64_t)(s->u_mux_chk_rdata_o)) << 0) | (((uint64_t)(0)) << 39));
    s->kmac_data_o_strb = 31;
    s->kmac_data_o_last = (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_last_o) & ((1ULL << 1) - 1);
}

/*
 * tick() - Evaluate and atomically commit one sequential edge.
 *
 * Phase 1 computes every next-state value from the same old state.
 * Phase 2 commits all registers together, matching Verilog NBA
 * semantics. The return value reports whether sequential state changed.
 * ACCUMULATE counters are handled by ptimer instead.
 */
static bool tick(rom_ctrl_state *s)
{
    bool _qp_changed = false;
    /* Phase 1: snapshot old state into next-state temporaries. */
    uint8_t _qp_next_u_tl_adapter_rom_intg_error_q = s->u_tl_adapter_rom_intg_error_q;
    uint8_t _qp_next_u_tl_adapter_rom_missed_err_gnt_q = s->u_tl_adapter_rom_missed_err_gnt_q;
    uint8_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst;
    uint16_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_ = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_;
    uint16_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_ = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_;
    uint8_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    uint8_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    uint8_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_ = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_ = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst;
    uint64_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_ = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_;
    uint64_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_ = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_;
    uint8_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    uint8_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    uint8_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    uint8_t _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    uint8_t _qp_next_u_mux_alert_q = s->u_mux_alert_q;
    uint8_t _qp_next_u_mux_u_sel_bus_q_flop_q_o = s->u_mux_u_sel_bus_q_flop_q_o;
    uint8_t _qp_next_u_mux_u_sel_bus_qq_flop_q_o = s->u_mux_u_sel_bus_qq_flop_q_o;
    uint64_t _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k1_q = s->gen_rom_scramble_enabled_u_rom_u_prince_k1_q;
    uint64_t _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q = s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q;
    uint64_t _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q = s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q;
    uint8_t _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q = s->gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q;
    uint64_t _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q = s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q;
    uint8_t _qp_next_u_reg_regs_err_q = s->u_reg_regs_err_q;
    uint8_t _qp_next_u_reg_regs_u_reg_if_outstanding_q = s->u_reg_regs_u_reg_if_outstanding_q;
    uint8_t _qp_next_u_reg_regs_u_reg_if_reqid_q = s->u_reg_regs_u_reg_if_reqid_q;
    uint8_t _qp_next_u_reg_regs_u_reg_if_reqsz_q = s->u_reg_regs_u_reg_if_reqsz_q;
    uint8_t _qp_next_u_reg_regs_u_reg_if_rspop_q = s->u_reg_regs_u_reg_if_rspop_q;
    uint32_t _qp_next_u_reg_regs_u_reg_if_rdata_q = s->u_reg_regs_u_reg_if_rdata_q;
    uint8_t _qp_next_u_reg_regs_u_reg_if_error_q = s->u_reg_regs_u_reg_if_error_q;
    uint8_t _qp_next_u_reg_regs_u_fatal_alert_cause_checker_error_q = s->u_reg_regs_u_fatal_alert_cause_checker_error_q;
    uint8_t _qp_next_u_reg_regs_u_fatal_alert_cause_integrity_error_q = s->u_reg_regs_u_fatal_alert_cause_integrity_error_q;
    uint32_t _qp_next_u_reg_regs_u_digest_0_q = s->u_reg_regs_u_digest_0_q;
    uint32_t _qp_next_u_reg_regs_u_digest_1_q = s->u_reg_regs_u_digest_1_q;
    uint32_t _qp_next_u_reg_regs_u_digest_2_q = s->u_reg_regs_u_digest_2_q;
    uint32_t _qp_next_u_reg_regs_u_digest_3_q = s->u_reg_regs_u_digest_3_q;
    uint32_t _qp_next_u_reg_regs_u_digest_4_q = s->u_reg_regs_u_digest_4_q;
    uint32_t _qp_next_u_reg_regs_u_digest_5_q = s->u_reg_regs_u_digest_5_q;
    uint32_t _qp_next_u_reg_regs_u_digest_6_q = s->u_reg_regs_u_digest_6_q;
    uint32_t _qp_next_u_reg_regs_u_digest_7_q = s->u_reg_regs_u_digest_7_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_0_q = s->u_reg_regs_u_exp_digest_0_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_1_q = s->u_reg_regs_u_exp_digest_1_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_2_q = s->u_reg_regs_u_exp_digest_2_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_3_q = s->u_reg_regs_u_exp_digest_3_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_4_q = s->u_reg_regs_u_exp_digest_4_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_5_q = s->u_reg_regs_u_exp_digest_5_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_6_q = s->u_reg_regs_u_exp_digest_6_q;
    uint32_t _qp_next_u_reg_regs_u_exp_digest_7_q = s->u_reg_regs_u_exp_digest_7_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q = s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q = s->gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q = s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q;
    uint16_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q = s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q = s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q = s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q = s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q = s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o = s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q = s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o = s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o = s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o;
    uint8_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o = s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o;
    uint16_t _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o = s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_state_q = s->gen_alert_tx_0_u_alert_sender_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_alert_set_q = s->gen_alert_tx_0_u_alert_sender_alert_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_alert_test_set_q = s->gen_alert_tx_0_u_alert_sender_alert_test_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_ping_set_q = s->gen_alert_tx_0_u_alert_sender_ping_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;

    /* Evaluate all next-state expressions from pre-edge state. */
    _qp_next_u_tl_adapter_rom_intg_error_q = ((((((!(((s->u_tl_adapter_rom_rst_ni) ^ 1))) && ((s->u_tl_adapter_rom_intg_error) | (s->u_tl_adapter_rom_rsp_fifo_error) | (s->u_tl_adapter_rom_sramreqfifo_error) | (s->u_tl_adapter_rom_reqfifo_error))) || (((s->u_tl_adapter_rom_rst_ni) ^ 1))) && (((!(((s->u_tl_adapter_rom_rst_ni) ^ 1))) && ((s->u_tl_adapter_rom_intg_error) | (s->u_tl_adapter_rom_rsp_fifo_error) | (s->u_tl_adapter_rom_sramreqfifo_error) | (s->u_tl_adapter_rom_reqfifo_error))) || (((s->u_tl_adapter_rom_rst_ni) ^ 1)))) ? ((((!(((s->u_tl_adapter_rom_rst_ni) ^ 1))) && ((s->u_tl_adapter_rom_intg_error) | (s->u_tl_adapter_rom_rsp_fifo_error) | (s->u_tl_adapter_rom_sramreqfifo_error) | (s->u_tl_adapter_rom_reqfifo_error))) ? (1) : (0))) : _qp_next_u_tl_adapter_rom_intg_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_missed_err_gnt_q = (((((s->u_tl_adapter_rom_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_missed_err_gnt_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_missed_err_gnt_q = (((!(((s->u_tl_adapter_rom_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_missed_err_gnt_d) : _qp_next_u_tl_adapter_rom_missed_err_gnt_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst = (((((s->u_tl_adapter_rom_u_reqfifo_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst = ((((!(((s->u_tl_adapter_rom_u_reqfifo_rst_ni) ^ 1))) && (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst)) ? (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst) ^ 1)) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_fifo_incr_wptr) && (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_fifo_wptr) ^ 1))) ? (s->u_tl_adapter_rom_u_reqfifo_wdata_i) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_ = (((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_fifo_incr_wptr) && (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_fifo_wptr)) ? (s->u_tl_adapter_rom_u_reqfifo_wdata_i) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = (((!(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_d) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = (((!(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_d) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst = (((((s->u_tl_adapter_rom_u_sramreqfifo_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst = ((((!(((s->u_tl_adapter_rom_u_sramreqfifo_rst_ni) ^ 1))) && (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst)) ? (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst) ^ 1)) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_ = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_fifo_incr_wptr) && (((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_fifo_wptr) ^ 1))) ? (s->u_tl_adapter_rom_u_sramreqfifo_wdata_i) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_)) & ((1ULL << 5) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_ = ((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_fifo_incr_wptr) && (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_fifo_wptr)) ? (s->u_tl_adapter_rom_u_sramreqfifo_wdata_i) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_)) & ((1ULL << 5) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = (((!(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_d) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = (((!(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_d) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst = (((((s->u_tl_adapter_rom_u_rspfifo_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst = ((((!(((s->u_tl_adapter_rom_u_rspfifo_rst_ni) ^ 1))) && (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst)) ? (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst) ^ 1)) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_ = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_fifo_incr_wptr) && (((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_fifo_wptr) ^ 1))) ? (s->u_tl_adapter_rom_u_rspfifo_wdata_i) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_)) & ((1ULL << 40) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_ = ((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_fifo_incr_wptr) && (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_fifo_wptr)) ? (s->u_tl_adapter_rom_u_rspfifo_wdata_i) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_)) & ((1ULL << 40) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = (((!(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_d) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = (((!(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_d) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1)) ? (0) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (((((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1)) ? (-1) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = (((!(((s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1))) ? (s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_d_i) : _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_u_mux_alert_q = (((((s->u_mux_rst_ni) ^ 1)) ? (0) : _qp_next_u_mux_alert_q)) & ((1ULL << 1) - 1);
    _qp_next_u_mux_alert_q = (((!(((s->u_mux_rst_ni) ^ 1))) ? ((s->u_mux_alert_q) | (s->u_mux_alert_d)) : _qp_next_u_mux_alert_q)) & ((1ULL << 1) - 1);
    _qp_next_u_mux_u_sel_bus_q_flop_q_o = (((((s->u_mux_u_sel_bus_q_flop_rst_ni) ^ 1)) ? (-7) : _qp_next_u_mux_u_sel_bus_q_flop_q_o)) & ((1ULL << 4) - 1);
    _qp_next_u_mux_u_sel_bus_q_flop_q_o = (((!(((s->u_mux_u_sel_bus_q_flop_rst_ni) ^ 1))) ? (s->u_mux_u_sel_bus_q_flop_d_i) : _qp_next_u_mux_u_sel_bus_q_flop_q_o)) & ((1ULL << 4) - 1);
    _qp_next_u_mux_u_sel_bus_qq_flop_q_o = (((((s->u_mux_u_sel_bus_qq_flop_rst_ni) ^ 1)) ? (-7) : _qp_next_u_mux_u_sel_bus_qq_flop_q_o)) & ((1ULL << 4) - 1);
    _qp_next_u_mux_u_sel_bus_qq_flop_q_o = (((!(((s->u_mux_u_sel_bus_qq_flop_rst_ni) ^ 1))) ? (s->u_mux_u_sel_bus_qq_flop_d_i) : _qp_next_u_mux_u_sel_bus_qq_flop_q_o)) & ((1ULL << 4) - 1);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k1_q = ((((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1)) ? (0) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k1_q);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q = ((((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1)) ? (0) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q = ((((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1)) ? (0) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k1_q = (((!(((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1))) && (s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i)) ? (s->gen_rom_scramble_enabled_u_rom_u_prince_k1_d) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k1_q);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q = (((!(((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1))) && (s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i)) ? (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_d) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q = (((!(((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1))) && (s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i)) ? (s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_d) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q = (((((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1)) ? (0) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q = ((((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1)) ? (0) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q = (((!(((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1))) ? (s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q = (((!(((s->gen_rom_scramble_enabled_u_rom_u_prince_rst_ni) ^ 1))) && (s->gen_rom_scramble_enabled_u_rom_u_prince_valid_i)) ? (s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_d) : _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q);
    _qp_next_u_reg_regs_err_q = ((((((!(((s->u_reg_regs_rst_ni) ^ 1))) && ((s->u_reg_regs_intg_err) | (s->u_reg_regs_reg_we_err))) || (((s->u_reg_regs_rst_ni) ^ 1))) && (((!(((s->u_reg_regs_rst_ni) ^ 1))) && ((s->u_reg_regs_intg_err) | (s->u_reg_regs_reg_we_err))) || (((s->u_reg_regs_rst_ni) ^ 1)))) ? ((((!(((s->u_reg_regs_rst_ni) ^ 1))) && ((s->u_reg_regs_intg_err) | (s->u_reg_regs_reg_we_err))) ? (1) : (0))) : _qp_next_u_reg_regs_err_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_reg_if_outstanding_q = ((((((((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (!(s->u_reg_regs_u_reg_if_a_ack))) && (s->u_reg_regs_u_reg_if_d_ack)) || ((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack))) || (((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (((((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (!(s->u_reg_regs_u_reg_if_a_ack))) && (s->u_reg_regs_u_reg_if_d_ack)) || ((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack))) || (((s->u_reg_regs_u_reg_if_rst_ni) ^ 1)))) ? ((((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack)) ? (1) : (0))) : _qp_next_u_reg_regs_u_reg_if_outstanding_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_reg_if_reqid_q = ((((s->u_reg_regs_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_reg_if_reqid_q);
    _qp_next_u_reg_regs_u_reg_if_reqsz_q = (((((s->u_reg_regs_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_reg_if_reqsz_q)) & ((1ULL << 2) - 1);
    _qp_next_u_reg_regs_u_reg_if_rspop_q = (((((s->u_reg_regs_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_reg_if_rspop_q)) & ((1ULL << 3) - 1);
    _qp_next_u_reg_regs_u_reg_if_reqid_q = (((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack)) ? (s->u_reg_regs_u_reg_if_tl_i_a_source) : _qp_next_u_reg_regs_u_reg_if_reqid_q);
    _qp_next_u_reg_regs_u_reg_if_reqsz_q = ((((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack)) ? (s->u_reg_regs_u_reg_if_tl_i_a_size) : _qp_next_u_reg_regs_u_reg_if_reqsz_q)) & ((1ULL << 2) - 1);
    _qp_next_u_reg_regs_u_reg_if_rspop_q = ((((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack)) ? (((((uint64_t)(0)) << 1) | ((uint64_t)(s->u_reg_regs_u_reg_if_rd_req)))) : _qp_next_u_reg_regs_u_reg_if_rspop_q)) & ((1ULL << 3) - 1);
    _qp_next_u_reg_regs_u_reg_if_rdata_q = ((((s->u_reg_regs_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_reg_if_rdata_q);
    _qp_next_u_reg_regs_u_reg_if_error_q = (((((s->u_reg_regs_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_reg_if_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_reg_if_rdata_q = (((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack)) ? ((((s->u_reg_regs_u_reg_if_error_i) | (s->u_reg_regs_u_reg_if_err_internal) | (s->u_reg_regs_u_reg_if_wr_req)) ? (4294967295ULL) : (s->u_reg_regs_u_reg_if_rdata_i))) : _qp_next_u_reg_regs_u_reg_if_rdata_q);
    _qp_next_u_reg_regs_u_reg_if_error_q = ((((!(((s->u_reg_regs_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_regs_u_reg_if_a_ack)) ? ((s->u_reg_regs_u_reg_if_error_i) | (s->u_reg_regs_u_reg_if_err_internal)) : _qp_next_u_reg_regs_u_reg_if_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_fatal_alert_cause_checker_error_q = (((((s->u_reg_regs_u_fatal_alert_cause_checker_error_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_fatal_alert_cause_checker_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_fatal_alert_cause_checker_error_q = ((((!(((s->u_reg_regs_u_fatal_alert_cause_checker_error_rst_ni) ^ 1))) && (s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_en)) ? (s->u_reg_regs_u_fatal_alert_cause_checker_error_wr_data) : _qp_next_u_reg_regs_u_fatal_alert_cause_checker_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_fatal_alert_cause_integrity_error_q = (((((s->u_reg_regs_u_fatal_alert_cause_integrity_error_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_fatal_alert_cause_integrity_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_fatal_alert_cause_integrity_error_q = ((((!(((s->u_reg_regs_u_fatal_alert_cause_integrity_error_rst_ni) ^ 1))) && (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_en)) ? (s->u_reg_regs_u_fatal_alert_cause_integrity_error_wr_data) : _qp_next_u_reg_regs_u_fatal_alert_cause_integrity_error_q)) & ((1ULL << 1) - 1);
    _qp_next_u_reg_regs_u_digest_0_q = ((((s->u_reg_regs_u_digest_0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_0_q);
    _qp_next_u_reg_regs_u_digest_0_q = (((!(((s->u_reg_regs_u_digest_0_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_0_wr_en)) ? (s->u_reg_regs_u_digest_0_wr_data) : _qp_next_u_reg_regs_u_digest_0_q);
    _qp_next_u_reg_regs_u_digest_1_q = ((((s->u_reg_regs_u_digest_1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_1_q);
    _qp_next_u_reg_regs_u_digest_1_q = (((!(((s->u_reg_regs_u_digest_1_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_1_wr_en)) ? (s->u_reg_regs_u_digest_1_wr_data) : _qp_next_u_reg_regs_u_digest_1_q);
    _qp_next_u_reg_regs_u_digest_2_q = ((((s->u_reg_regs_u_digest_2_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_2_q);
    _qp_next_u_reg_regs_u_digest_2_q = (((!(((s->u_reg_regs_u_digest_2_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_2_wr_en)) ? (s->u_reg_regs_u_digest_2_wr_data) : _qp_next_u_reg_regs_u_digest_2_q);
    _qp_next_u_reg_regs_u_digest_3_q = ((((s->u_reg_regs_u_digest_3_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_3_q);
    _qp_next_u_reg_regs_u_digest_3_q = (((!(((s->u_reg_regs_u_digest_3_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_3_wr_en)) ? (s->u_reg_regs_u_digest_3_wr_data) : _qp_next_u_reg_regs_u_digest_3_q);
    _qp_next_u_reg_regs_u_digest_4_q = ((((s->u_reg_regs_u_digest_4_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_4_q);
    _qp_next_u_reg_regs_u_digest_4_q = (((!(((s->u_reg_regs_u_digest_4_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_4_wr_en)) ? (s->u_reg_regs_u_digest_4_wr_data) : _qp_next_u_reg_regs_u_digest_4_q);
    _qp_next_u_reg_regs_u_digest_5_q = ((((s->u_reg_regs_u_digest_5_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_5_q);
    _qp_next_u_reg_regs_u_digest_5_q = (((!(((s->u_reg_regs_u_digest_5_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_5_wr_en)) ? (s->u_reg_regs_u_digest_5_wr_data) : _qp_next_u_reg_regs_u_digest_5_q);
    _qp_next_u_reg_regs_u_digest_6_q = ((((s->u_reg_regs_u_digest_6_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_6_q);
    _qp_next_u_reg_regs_u_digest_6_q = (((!(((s->u_reg_regs_u_digest_6_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_6_wr_en)) ? (s->u_reg_regs_u_digest_6_wr_data) : _qp_next_u_reg_regs_u_digest_6_q);
    _qp_next_u_reg_regs_u_digest_7_q = ((((s->u_reg_regs_u_digest_7_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_digest_7_q);
    _qp_next_u_reg_regs_u_digest_7_q = (((!(((s->u_reg_regs_u_digest_7_rst_ni) ^ 1))) && (s->u_reg_regs_u_digest_7_wr_en)) ? (s->u_reg_regs_u_digest_7_wr_data) : _qp_next_u_reg_regs_u_digest_7_q);
    _qp_next_u_reg_regs_u_exp_digest_0_q = ((((s->u_reg_regs_u_exp_digest_0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_0_q);
    _qp_next_u_reg_regs_u_exp_digest_0_q = (((!(((s->u_reg_regs_u_exp_digest_0_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_0_wr_en)) ? (s->u_reg_regs_u_exp_digest_0_wr_data) : _qp_next_u_reg_regs_u_exp_digest_0_q);
    _qp_next_u_reg_regs_u_exp_digest_1_q = ((((s->u_reg_regs_u_exp_digest_1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_1_q);
    _qp_next_u_reg_regs_u_exp_digest_1_q = (((!(((s->u_reg_regs_u_exp_digest_1_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_1_wr_en)) ? (s->u_reg_regs_u_exp_digest_1_wr_data) : _qp_next_u_reg_regs_u_exp_digest_1_q);
    _qp_next_u_reg_regs_u_exp_digest_2_q = ((((s->u_reg_regs_u_exp_digest_2_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_2_q);
    _qp_next_u_reg_regs_u_exp_digest_2_q = (((!(((s->u_reg_regs_u_exp_digest_2_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_2_wr_en)) ? (s->u_reg_regs_u_exp_digest_2_wr_data) : _qp_next_u_reg_regs_u_exp_digest_2_q);
    _qp_next_u_reg_regs_u_exp_digest_3_q = ((((s->u_reg_regs_u_exp_digest_3_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_3_q);
    _qp_next_u_reg_regs_u_exp_digest_3_q = (((!(((s->u_reg_regs_u_exp_digest_3_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_3_wr_en)) ? (s->u_reg_regs_u_exp_digest_3_wr_data) : _qp_next_u_reg_regs_u_exp_digest_3_q);
    _qp_next_u_reg_regs_u_exp_digest_4_q = ((((s->u_reg_regs_u_exp_digest_4_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_4_q);
    _qp_next_u_reg_regs_u_exp_digest_4_q = (((!(((s->u_reg_regs_u_exp_digest_4_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_4_wr_en)) ? (s->u_reg_regs_u_exp_digest_4_wr_data) : _qp_next_u_reg_regs_u_exp_digest_4_q);
    _qp_next_u_reg_regs_u_exp_digest_5_q = ((((s->u_reg_regs_u_exp_digest_5_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_5_q);
    _qp_next_u_reg_regs_u_exp_digest_5_q = (((!(((s->u_reg_regs_u_exp_digest_5_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_5_wr_en)) ? (s->u_reg_regs_u_exp_digest_5_wr_data) : _qp_next_u_reg_regs_u_exp_digest_5_q);
    _qp_next_u_reg_regs_u_exp_digest_6_q = ((((s->u_reg_regs_u_exp_digest_6_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_6_q);
    _qp_next_u_reg_regs_u_exp_digest_6_q = (((!(((s->u_reg_regs_u_exp_digest_6_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_6_wr_en)) ? (s->u_reg_regs_u_exp_digest_6_wr_data) : _qp_next_u_reg_regs_u_exp_digest_6_q);
    _qp_next_u_reg_regs_u_exp_digest_7_q = ((((s->u_reg_regs_u_exp_digest_7_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_regs_u_exp_digest_7_q);
    _qp_next_u_reg_regs_u_exp_digest_7_q = (((!(((s->u_reg_regs_u_exp_digest_7_rst_ni) ^ 1))) && (s->u_reg_regs_u_exp_digest_7_wr_en)) ? (s->u_reg_regs_u_exp_digest_7_wr_data) : _qp_next_u_reg_regs_u_exp_digest_7_q);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_d) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_rst_ni) ^ 1))) ? ((((s->gen_fsm_scramble_enabled_u_checker_fsm_state_q) != (345))) & (((s->gen_fsm_scramble_enabled_u_checker_fsm_state_d) == (345)))) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_d) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q)) & ((1ULL << 13) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q = ((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1))) && (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_go)) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_d) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q)) & ((1ULL << 13) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q = ((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1))) && (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_go)) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_d) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1))) ? (-1) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q = ((((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_rst_ni) ^ 1))) && (((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_state_q) == (18)))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_d) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_rst_ni) ^ 1)) ? (4) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o)) & ((1ULL << 5) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_d_i) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o)) & ((1ULL << 5) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_d) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1)) ? (0) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 3) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_d_i) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o)) & ((1ULL << 3) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 3) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_d_i) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o)) & ((1ULL << 3) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_rst_ni) ^ 1)) ? (-7) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o)) & ((1ULL << 4) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_d_i) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o)) & ((1ULL << 4) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o = (((((s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_rst_ni) ^ 1)) ? (201) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o)) & ((1ULL << 10) - 1);
    _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o = (((!(((s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_rst_ni) ^ 1))) ? (s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_d_i) : _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o)) & ((1ULL << 10) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_state_q = (((((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_state_q)) & ((1ULL << 3) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_alert_set_q = (((((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_alert_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_alert_test_set_q = (((((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_alert_test_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_ping_set_q = (((((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_ping_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_state_q = (((!(((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_state_d) : _qp_next_gen_alert_tx_0_u_alert_sender_state_q)) & ((1ULL << 3) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_alert_set_q = (((!(((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_alert_set_d) : _qp_next_gen_alert_tx_0_u_alert_sender_alert_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_alert_test_set_q = (((!(((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_alert_test_set_d) : _qp_next_gen_alert_tx_0_u_alert_sender_alert_test_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_ping_set_q = (((!(((s->gen_alert_tx_0_u_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_ping_set_d) : _qp_next_gen_alert_tx_0_u_alert_sender_ping_set_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_d) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pd) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nd) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_d) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_d) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_d) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pd) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nd) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_d) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_d) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o)) & ((1ULL << 1) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = (((((s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1)) ? (-2) : _qp_next_gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o)) & ((1ULL << 2) - 1);
    _qp_next_gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = (((!(((s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i) : _qp_next_gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o)) & ((1ULL << 2) - 1);

    /* Specialized sequential blocks sampled after Phase 1. */
    char _qp_raw_prev[sizeof(*s)];
    memcpy(_qp_raw_prev, s, sizeof(*s));
    { /* prim_rom memory organ: u_prim_rom */
        if (!(s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_rst_ni)) { s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_rvalid_o = 0; }
        else {
            s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_rvalid_o = (s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_req_i) & 1;
            if (s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_req_i) s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_rdata_o = s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_qp_mem[(s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_addr_i) & 8191ULL];
        }
    }
    _qp_changed |= memcmp(_qp_raw_prev, s, sizeof(*s)) != 0;

    /* Detect changes before committing ordinary registers. */
    _qp_changed |= _qp_next_u_tl_adapter_rom_intg_error_q != s->u_tl_adapter_rom_intg_error_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_missed_err_gnt_q != s->u_tl_adapter_rom_missed_err_gnt_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_ != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_ != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_ != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_ != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_ != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_ != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o != s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_u_mux_alert_q != s->u_mux_alert_q;
    _qp_changed |= _qp_next_u_mux_u_sel_bus_q_flop_q_o != s->u_mux_u_sel_bus_q_flop_q_o;
    _qp_changed |= _qp_next_u_mux_u_sel_bus_qq_flop_q_o != s->u_mux_u_sel_bus_qq_flop_q_o;
    _qp_changed |= _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k1_q != s->gen_rom_scramble_enabled_u_rom_u_prince_k1_q;
    _qp_changed |= _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q != s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q;
    _qp_changed |= _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q != s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q;
    _qp_changed |= _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q != s->gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q;
    _qp_changed |= _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q != s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q;
    _qp_changed |= _qp_next_u_reg_regs_err_q != s->u_reg_regs_err_q;
    _qp_changed |= _qp_next_u_reg_regs_u_reg_if_outstanding_q != s->u_reg_regs_u_reg_if_outstanding_q;
    _qp_changed |= _qp_next_u_reg_regs_u_reg_if_reqid_q != s->u_reg_regs_u_reg_if_reqid_q;
    _qp_changed |= _qp_next_u_reg_regs_u_reg_if_reqsz_q != s->u_reg_regs_u_reg_if_reqsz_q;
    _qp_changed |= _qp_next_u_reg_regs_u_reg_if_rspop_q != s->u_reg_regs_u_reg_if_rspop_q;
    _qp_changed |= _qp_next_u_reg_regs_u_reg_if_rdata_q != s->u_reg_regs_u_reg_if_rdata_q;
    _qp_changed |= _qp_next_u_reg_regs_u_reg_if_error_q != s->u_reg_regs_u_reg_if_error_q;
    _qp_changed |= _qp_next_u_reg_regs_u_fatal_alert_cause_checker_error_q != s->u_reg_regs_u_fatal_alert_cause_checker_error_q;
    _qp_changed |= _qp_next_u_reg_regs_u_fatal_alert_cause_integrity_error_q != s->u_reg_regs_u_fatal_alert_cause_integrity_error_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_0_q != s->u_reg_regs_u_digest_0_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_1_q != s->u_reg_regs_u_digest_1_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_2_q != s->u_reg_regs_u_digest_2_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_3_q != s->u_reg_regs_u_digest_3_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_4_q != s->u_reg_regs_u_digest_4_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_5_q != s->u_reg_regs_u_digest_5_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_6_q != s->u_reg_regs_u_digest_6_q;
    _qp_changed |= _qp_next_u_reg_regs_u_digest_7_q != s->u_reg_regs_u_digest_7_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_0_q != s->u_reg_regs_u_exp_digest_0_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_1_q != s->u_reg_regs_u_exp_digest_1_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_2_q != s->u_reg_regs_u_exp_digest_2_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_3_q != s->u_reg_regs_u_exp_digest_3_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_4_q != s->u_reg_regs_u_exp_digest_4_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_5_q != s->u_reg_regs_u_exp_digest_5_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_6_q != s->u_reg_regs_u_exp_digest_6_q;
    _qp_changed |= _qp_next_u_reg_regs_u_exp_digest_7_q != s->u_reg_regs_u_exp_digest_7_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q != s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q != s->gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q != s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q != s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q != s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q != s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q != s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q != s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o != s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q != s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o != s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o != s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o != s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o;
    _qp_changed |= _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o != s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_state_q != s->gen_alert_tx_0_u_alert_sender_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_alert_set_q != s->gen_alert_tx_0_u_alert_sender_alert_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_alert_test_set_q != s->gen_alert_tx_0_u_alert_sender_alert_test_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_ping_set_q != s->gen_alert_tx_0_u_alert_sender_ping_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o != s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o != s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;

    /* Phase 2: commit all ordinary registers simultaneously. */
    s->u_tl_adapter_rom_intg_error_q = _qp_next_u_tl_adapter_rom_intg_error_q;
    s->u_tl_adapter_rom_missed_err_gnt_q = _qp_next_u_tl_adapter_rom_missed_err_gnt_q;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_under_rst;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_ = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_0_;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_ = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_storage_1_;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_reqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_under_rst;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_ = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_0_;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_ = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_storage_1_;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_sramreqfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_under_rst;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_ = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_0_;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_ = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_storage_1_;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_err_q;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_0_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_wptr_gen_cnts_1_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_err_q;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_0_u_cnt_flop_q_o;
    s->u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o = _qp_next_u_tl_adapter_rom_u_rspfifo_gen_normal_fifo_u_fifo_cnt_gen_secure_ptrs_u_rptr_gen_cnts_1_u_cnt_flop_q_o;
    s->u_mux_alert_q = _qp_next_u_mux_alert_q;
    s->u_mux_u_sel_bus_q_flop_q_o = _qp_next_u_mux_u_sel_bus_q_flop_q_o;
    s->u_mux_u_sel_bus_qq_flop_q_o = _qp_next_u_mux_u_sel_bus_qq_flop_q_o;
    s->gen_rom_scramble_enabled_u_rom_u_prince_k1_q = _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k1_q;
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q = _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_prime_q;
    s->gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q = _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_k0_new_q;
    s->gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q = _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_gen_data_reg_valid_q;
    s->gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q = _qp_next_gen_rom_scramble_enabled_u_rom_u_prince_data_state_middle_q;
    s->u_reg_regs_err_q = _qp_next_u_reg_regs_err_q;
    s->u_reg_regs_u_reg_if_outstanding_q = _qp_next_u_reg_regs_u_reg_if_outstanding_q;
    s->u_reg_regs_u_reg_if_reqid_q = _qp_next_u_reg_regs_u_reg_if_reqid_q;
    s->u_reg_regs_u_reg_if_reqsz_q = _qp_next_u_reg_regs_u_reg_if_reqsz_q;
    s->u_reg_regs_u_reg_if_rspop_q = _qp_next_u_reg_regs_u_reg_if_rspop_q;
    s->u_reg_regs_u_reg_if_rdata_q = _qp_next_u_reg_regs_u_reg_if_rdata_q;
    s->u_reg_regs_u_reg_if_error_q = _qp_next_u_reg_regs_u_reg_if_error_q;
    s->u_reg_regs_u_fatal_alert_cause_checker_error_q = _qp_next_u_reg_regs_u_fatal_alert_cause_checker_error_q;
    s->u_reg_regs_u_fatal_alert_cause_integrity_error_q = _qp_next_u_reg_regs_u_fatal_alert_cause_integrity_error_q;
    s->u_reg_regs_u_digest_0_q = _qp_next_u_reg_regs_u_digest_0_q;
    s->u_reg_regs_u_digest_1_q = _qp_next_u_reg_regs_u_digest_1_q;
    s->u_reg_regs_u_digest_2_q = _qp_next_u_reg_regs_u_digest_2_q;
    s->u_reg_regs_u_digest_3_q = _qp_next_u_reg_regs_u_digest_3_q;
    s->u_reg_regs_u_digest_4_q = _qp_next_u_reg_regs_u_digest_4_q;
    s->u_reg_regs_u_digest_5_q = _qp_next_u_reg_regs_u_digest_5_q;
    s->u_reg_regs_u_digest_6_q = _qp_next_u_reg_regs_u_digest_6_q;
    s->u_reg_regs_u_digest_7_q = _qp_next_u_reg_regs_u_digest_7_q;
    s->u_reg_regs_u_exp_digest_0_q = _qp_next_u_reg_regs_u_exp_digest_0_q;
    s->u_reg_regs_u_exp_digest_1_q = _qp_next_u_reg_regs_u_exp_digest_1_q;
    s->u_reg_regs_u_exp_digest_2_q = _qp_next_u_reg_regs_u_exp_digest_2_q;
    s->u_reg_regs_u_exp_digest_3_q = _qp_next_u_reg_regs_u_exp_digest_3_q;
    s->u_reg_regs_u_exp_digest_4_q = _qp_next_u_reg_regs_u_exp_digest_4_q;
    s->u_reg_regs_u_exp_digest_5_q = _qp_next_u_reg_regs_u_exp_digest_5_q;
    s->u_reg_regs_u_exp_digest_6_q = _qp_next_u_reg_regs_u_exp_digest_6_q;
    s->u_reg_regs_u_exp_digest_7_q = _qp_next_u_reg_regs_u_exp_digest_7_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_kmac_rom_vld_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_start_checker_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_done_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_addr_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_last_nontop_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_req_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_counter_vld_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_matches_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_state_regs_u_state_flop_q_o;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_err_q;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_0_u_cnt_flop_q_o;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_prim_count_addr_gen_cnts_1_u_cnt_flop_q_o;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_compare_u_done_sender_gen_flops_u_prim_flop_q_o;
    s->gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o = _qp_next_gen_fsm_scramble_enabled_u_checker_fsm_u_state_regs_u_state_flop_q_o;
    s->gen_alert_tx_0_u_alert_sender_state_q = _qp_next_gen_alert_tx_0_u_alert_sender_state_q;
    s->gen_alert_tx_0_u_alert_sender_alert_set_q = _qp_next_gen_alert_tx_0_u_alert_sender_alert_set_q;
    s->gen_alert_tx_0_u_alert_sender_alert_test_set_q = _qp_next_gen_alert_tx_0_u_alert_sender_alert_test_set_q;
    s->gen_alert_tx_0_u_alert_sender_ping_set_q = _qp_next_gen_alert_tx_0_u_alert_sender_ping_set_q;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_state_q;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_pq;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_diff_nq;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_level_q;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_state_q;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_pq;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_diff_nq;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_level_q;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = _qp_next_gen_alert_tx_0_u_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;

    return _qp_changed;
}

/* Aux TL-UL port 'regs_tl': same transaction recipe as
 * the primary port, response captured from the bridged top-level
 * output leaves. */
uint64_t rom_ctrl_regs_tl_read(void *opaque, hwaddr addr, unsigned size)
{
    rom_ctrl_state *s = opaque;
    s->_qp_access_gen++;
    s->regs_tl_i_a_address = (uint32_t)addr;
    s->regs_tl_i_a_opcode = (uint8_t)4;  /* Get */
    s->regs_tl_i_a_size = (uint8_t)(size >= 4 ? 2 : size == 2 ? 1 : 0);
    s->regs_tl_i_a_mask = (uint8_t)(((1u << (size >= 4 ? 4 : size)) - 1u) << (addr & 3u));
    s->regs_tl_i_a_user_instr_type = (uint8_t)9;  /* MuBi4False */
    s->regs_tl_i_d_ready = 1;
    s->regs_tl_i_a_valid = 1;
    s->_qp_in_request = 1;
    update_state(s);
    qp_tick(s);
    s->regs_tl_i_a_valid = 0;
    s->_qp_in_request = 0;
    s->_qp_rd_cap = 0;
    {
    if (!s->_qp_busy) {
        s->_qp_busy = 1;
        unsigned _qp_ticks = 0;
        update_state(s);
            if (!s->_qp_rd_cap && s->regs_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->regs_tl_o_d_data; }
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
            if (!s->_qp_rd_cap && s->regs_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->regs_tl_o_d_data; }
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
            if (!s->_qp_rd_cap && s->regs_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->regs_tl_o_d_data; }
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

    uint32_t _qp_rv = s->_qp_rd_cap ? s->_qp_rd_capv : s->regs_tl_o_d_data;
    if (size < 4)
        return ((uint64_t)_qp_rv >> (8u * (addr & 3u))) & ((1ULL << (8u * size)) - 1u);
    return (uint64_t)_qp_rv;
}

void rom_ctrl_regs_tl_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    rom_ctrl_state *s = opaque;
    s->_qp_access_gen++;
    s->regs_tl_i_a_address = (uint32_t)addr;
    s->regs_tl_i_a_data = (uint32_t)value;
    s->regs_tl_i_a_opcode = (uint8_t)0;  /* PutFullData */
    s->regs_tl_i_a_size = (uint8_t)(size >= 4 ? 2 : size == 2 ? 1 : 0);
    s->regs_tl_i_a_mask = (uint8_t)(((1u << (size >= 4 ? 4 : size)) - 1u) << (addr & 3u));
    s->regs_tl_i_a_user_instr_type = (uint8_t)9;
    s->regs_tl_i_d_ready = 1;
    s->regs_tl_i_a_valid = 1;
    s->_qp_in_request = 1;
    update_state(s);
    qp_tick(s);
    s->regs_tl_i_a_valid = 0;
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

/* Load an image into backing store <idx>.  Layout: one 64-bit
 * little-endian word per memory word (upper bits beyond the
 * organ width ignored).  Returns words loaded, or -1 on
 * bad idx / oversized image. */
int rom_ctrl_load_backing(rom_ctrl_state *s, unsigned idx, const void *bytes, unsigned long n)
{
    const unsigned long depths[] = {8192UL};
    uint64_t *mems[] = {s->gen_rom_scramble_enabled_u_rom_u_rom_u_prim_rom_qp_mem};
    if (idx >= 1u || n > depths[idx] * 8) return -1;
    memcpy(mems[idx], bytes, n);
    return (int)(n / 8);
}

/* Settle to quiescence (no bus access). */
void rom_ctrl_settle(rom_ctrl_state *s)
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
void rom_ctrl_step(rom_ctrl_state *s)
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
void rom_ctrl_update(rom_ctrl_state *s)
{
    update_state(s);
}

void rom_ctrl_tick(rom_ctrl_state *s)
{
    qp_tick(s);
}

void rom_ctrl_step_many(rom_ctrl_state *s, unsigned count)
{
    update_state(s);
    while (count--) { qp_tick(s); update_state(s); }
}

/* Pulse reset to commit RESVALs into every prim_subreg. */
void rom_ctrl_reset(rom_ctrl_state *s)
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
uint64_t rom_ctrl_read(void *opaque, hwaddr addr, unsigned size)
{
    rom_ctrl_state *s = opaque;
    s->_qp_access_gen++;   /* organs: state snapshots older than this are stale */

    /* Inject QEMU address into the internal address signal */
    s->rom_tl_i_a_address = (uint32_t)addr;

    /* TL-UL: assert request valid + Get opcode + always-ready response acceptor */
    s->rom_tl_i_a_valid = 1;
    s->rom_tl_i_d_ready = 1;
    s->rom_tl_i_a_opcode = (uint8_t)4;  /* Get */
    s->rom_tl_i_a_size = (uint8_t)(size >= 4 ? 2 : size == 2 ? 1 : 0);  /* log2(bytes) */
    s->rom_tl_i_a_mask = (uint8_t)(((1u << (size >= 4 ? 4 : size)) - 1u) << (addr & 3u));  /* byte lanes */
    s->rom_tl_i_a_user_instr_type = (uint8_t)9;  /* MuBi4False: data access */

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
    s->rom_tl_i_a_valid = 0;   /* request lasted one clock */
    s->_qp_in_request = 0;
    s->_qp_rd_cap = 0;
    {
    if (!s->_qp_busy) {
        s->_qp_busy = 1;
        unsigned _qp_ticks = 0;
        update_state(s);
            if (!s->_qp_rd_cap && s->u_tl_adapter_rom_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->u_tl_adapter_rom_tl_o_d_data; }
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
            if (!s->_qp_rd_cap && s->u_tl_adapter_rom_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->u_tl_adapter_rom_tl_o_d_data; }
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
            if (!s->_qp_rd_cap && s->u_tl_adapter_rom_tl_o_d_valid) { s->_qp_rd_cap = 1; s->_qp_rd_capv = s->u_tl_adapter_rom_tl_o_d_data; }
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

    uint32_t _qp_rv = s->_qp_rd_cap ? s->_qp_rd_capv : s->u_tl_adapter_rom_tl_o_d_data;
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
void rom_ctrl_write(void *opaque, hwaddr addr,
                uint64_t value, unsigned size)
{
    rom_ctrl_state *s = opaque;
    s->_qp_access_gen++;   /* organs: state snapshots older than this are stale */

    /* Inject QEMU address into the internal address signal */
    s->rom_tl_i_a_address = (uint32_t)addr;

    /* Inject QEMU write data into the internal wdata signal
     * (sub-word writes: data sits in the addressed byte lanes) */
    s->rom_tl_i_a_data = (uint32_t)(size < 4 ? (value << (8u * (addr & 3u))) : value);

    /* Set write mask (byte-enable bits for the access size, in the
     * addressed lanes: a byte write at +1 -> a_mask = 0x2) */
    s->rom_tl_i_a_mask = (uint8_t)(((1u << (size >= 4 ? 4 : size)) - 1u) << (addr & 3u));

    /* TL-UL: assert request valid + PutFullData opcode + always-ready response acceptor */
    s->rom_tl_i_a_valid = 1;
    s->rom_tl_i_d_ready = 1;
    s->rom_tl_i_a_opcode = (uint8_t)0;  /* PutFullData */
    s->rom_tl_i_a_size = (uint8_t)(size >= 4 ? 2 : size == 2 ? 1 : 0);  /* log2(bytes) */
    s->rom_tl_i_a_user_instr_type = (uint8_t)9;  /* MuBi4False: data access */

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
    s->rom_tl_i_a_valid = 0;   /* request lasted one clock */
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

static const MemoryRegionOps rom_ctrl_ops = {
    .read  = rom_ctrl_read,
    .write = rom_ctrl_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void rom_ctrl_realize(DeviceState *dev, Error **errp)
{
    rom_ctrl_state *s = ROM_CTRL(dev);

    memory_region_init_io(&s->mmio, OBJECT(dev), &rom_ctrl_ops, s,
                          "rom_ctrl", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    /* Initialize state to zero */
    update_state(s);
}

static void rom_ctrl_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = rom_ctrl_realize;
}

static const TypeInfo rom_ctrl_info = {
    .name          = TYPE_ROM_CTRL,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(rom_ctrl_state),
    .class_init    = rom_ctrl_class_init,
};

static void rom_ctrl_register_types(void)
{
    type_register_static(&rom_ctrl_info);
}

type_init(rom_ctrl_register_types)

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
void rom_ctrl_set_rom_cfg_i_test(rom_ctrl_state *s, uint8_t value)
{
    s->rom_cfg_i_test = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_cfg_i_cfg_en(rom_ctrl_state *s, uint8_t value)
{
    s->rom_cfg_i_cfg_en = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_cfg_i_cfg(rom_ctrl_state *s, uint8_t value)
{
    s->rom_cfg_i_cfg = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_valid(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_valid = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_opcode(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_opcode = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_param(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_param = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_size(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_size = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_source(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_source = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_address(rom_ctrl_state *s, uint32_t value)
{
    s->rom_tl_i_a_address = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_mask(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_mask = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_data(rom_ctrl_state *s, uint32_t value)
{
    s->rom_tl_i_a_data = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_user_rsvd(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_user_rsvd = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_user_instr_type(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_user_instr_type = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_user_cmd_intg(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_user_cmd_intg = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_a_user_data_intg(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_a_user_data_intg = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_rom_tl_i_d_ready(rom_ctrl_state *s, uint8_t value)
{
    s->rom_tl_i_d_ready = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_valid(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_valid = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_opcode(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_opcode = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_param(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_param = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_size(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_size = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_source(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_source = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_address(rom_ctrl_state *s, uint32_t value)
{
    s->regs_tl_i_a_address = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_mask(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_mask = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_data(rom_ctrl_state *s, uint32_t value)
{
    s->regs_tl_i_a_data = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_user_rsvd(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_user_rsvd = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_user_instr_type(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_user_instr_type = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_user_cmd_intg(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_user_cmd_intg = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_a_user_data_intg(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_a_user_data_intg = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_regs_tl_i_d_ready(rom_ctrl_state *s, uint8_t value)
{
    s->regs_tl_i_d_ready = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_alert_rx_i_0__ping_p(rom_ctrl_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_p = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_alert_rx_i_0__ping_n(rom_ctrl_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_n = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_alert_rx_i_0__ack_p(rom_ctrl_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_p = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_alert_rx_i_0__ack_n(rom_ctrl_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_n = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_kmac_data_i_ready(rom_ctrl_state *s, uint8_t value)
{
    s->kmac_data_i_ready = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_kmac_data_i_done(rom_ctrl_state *s, uint8_t value)
{
    s->kmac_data_i_done = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_kmac_data_i_digest_share0(rom_ctrl_state *s, const uint64_t value[6])
{
    memcpy(s->kmac_data_i_digest_share0, value, sizeof(s->kmac_data_i_digest_share0));
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_kmac_data_i_digest_share1(rom_ctrl_state *s, const uint64_t value[6])
{
    memcpy(s->kmac_data_i_digest_share1, value, sizeof(s->kmac_data_i_digest_share1));
    update_state(s);
    qp_tick(s);
    update_state(s);
}

void rom_ctrl_set_kmac_data_i_error(rom_ctrl_state *s, uint8_t value)
{
    s->kmac_data_i_error = value;
    update_state(s);
    qp_tick(s);
    update_state(s);
}

