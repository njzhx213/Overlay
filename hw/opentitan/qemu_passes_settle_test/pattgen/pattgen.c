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

static void update_state(pattgen_state *s);
static bool tick(pattgen_state *s);

typedef struct QPSettleFingerprint {
    uint64_t first;
    uint64_t second;
} QPSettleFingerprint;

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
static void update_state(pattgen_state *s)
{
    s->u_reg_clk_i = s->clk_i;
    s->u_reg_rst_ni = s->rst_ni;
    s->u_reg_tl_i_a_valid = s->tl_i_a_valid;
    s->u_reg_tl_i_a_opcode = s->tl_i_a_opcode;
    s->u_reg_tl_i_a_param = s->tl_i_a_param;
    s->u_reg_tl_i_a_size = s->tl_i_a_size;
    s->u_reg_tl_i_a_source = s->tl_i_a_source;
    s->u_reg_tl_i_a_address = s->tl_i_a_address;
    s->u_reg_tl_i_a_mask = s->tl_i_a_mask;
    s->u_reg_tl_i_a_data = s->tl_i_a_data;
    s->u_reg_tl_i_a_user_rsvd = s->tl_i_a_user_rsvd;
    s->u_reg_tl_i_a_user_instr_type = s->tl_i_a_user_instr_type;
    s->u_reg_tl_i_a_user_cmd_intg = s->tl_i_a_user_cmd_intg;
    s->u_reg_tl_i_a_user_data_intg = s->tl_i_a_user_data_intg;
    s->u_reg_tl_i_d_ready = s->tl_i_d_ready;
    s->u_reg_intg_err = 0;
    s->u_reg_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_chk_tl_i_a_valid = s->u_reg_tl_i_a_valid;
    s->u_reg_u_chk_tl_i_a_opcode = s->u_reg_tl_i_a_opcode;
    s->u_reg_u_chk_tl_i_a_param = s->u_reg_tl_i_a_param;
    s->u_reg_u_chk_tl_i_a_size = s->u_reg_tl_i_a_size;
    s->u_reg_u_chk_tl_i_a_source = s->u_reg_tl_i_a_source;
    s->u_reg_u_chk_tl_i_a_address = s->u_reg_tl_i_a_address;
    s->u_reg_u_chk_tl_i_a_mask = s->u_reg_tl_i_a_mask;
    s->u_reg_u_chk_tl_i_a_data = s->u_reg_tl_i_a_data;
    s->u_reg_u_chk_tl_i_a_user_rsvd = s->u_reg_tl_i_a_user_rsvd;
    s->u_reg_u_chk_tl_i_a_user_instr_type = s->u_reg_tl_i_a_user_instr_type;
    s->u_reg_u_chk_tl_i_a_user_cmd_intg = s->u_reg_tl_i_a_user_cmd_intg;
    s->u_reg_u_chk_tl_i_a_user_data_intg = s->u_reg_tl_i_a_user_data_intg;
    s->u_reg_u_chk_tl_i_d_ready = s->u_reg_tl_i_d_ready;
    s->u_reg_u_chk_u_chk_data_i = s->u_reg_u_chk_u_chk_data_i;
    s->u_reg_u_chk_u_chk_data_o = s->u_reg_u_chk_u_chk_data_o;
    s->u_reg_u_chk_u_chk_syndrome_o = s->u_reg_u_chk_u_chk_syndrome_o;
    s->u_reg_u_chk_u_chk_err_o = 0;
    s->u_reg_u_chk_u_tlul_data_integ_dec_data_intg_i = ((((uint64_t)(s->u_reg_u_chk_tl_i_a_data)) << 0) | (((uint64_t)(s->u_reg_u_chk_tl_i_a_user_data_intg)) << 32));
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i = s->u_reg_u_chk_u_tlul_data_integ_dec_data_intg_i;
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i = s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i;
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o;
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o;
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o = s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o;
    s->u_reg_u_chk_u_tlul_data_integ_dec_data_err_o = ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o) != (0));
    s->u_reg_u_chk_err_o = 0;
    s->u_reg_u_prim_reg_we_check_clk_i = s->u_reg_clk_i;
    s->u_reg_u_prim_reg_we_check_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_prim_reg_we_check_oh_i = s->u_reg_reg_we_check;
    s->u_reg_u_prim_reg_we_check_u_prim_buf_in_i = s->u_reg_u_prim_reg_we_check_oh_i;
    s->u_reg_u_prim_reg_we_check_u_prim_buf_out_o = s->u_reg_u_prim_reg_we_check_u_prim_buf_in_i;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_clk_i = s->u_reg_u_prim_reg_we_check_clk_i;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_rst_ni = s->u_reg_u_prim_reg_we_check_rst_ni;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i = s->u_reg_u_prim_reg_we_check_u_prim_buf_out_o;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i = 0;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x1ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 1) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 2) & 0x1))) & 0x1ULL) << 0);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x1ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 3) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 1) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 3) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 2) & 0x1)))) & 0x1ULL) << 0);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x2ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 3) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 4) & 0x1))) & 0x1ULL) << 1);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x2ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 3) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 4) & 0x1)))) & 0x1ULL) << 1);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x4ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 5) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 6) & 0x1))) & 0x1ULL) << 2);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x4ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 5) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 6) & 0x1)))) & 0x1ULL) << 2);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x8ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 7) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 8) & 0x1))) & 0x1ULL) << 3);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x8ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 7) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 8) & 0x1)))) & 0x1ULL) << 3);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x10ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 9) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 10) & 0x1))) & 0x1ULL) << 4);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x10ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 9) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 10) & 0x1)))) & 0x1ULL) << 4);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x20ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 11) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 12) & 0x1))) & 0x1ULL) << 5);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x20ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 11) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 12) & 0x1)))) & 0x1ULL) << 5);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x40ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 13) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 14) & 0x1))) & 0x1ULL) << 6);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x40ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 13) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 14) & 0x1)))) & 0x1ULL) << 6);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x80ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 15) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 16) & 0x1))) & 0x1ULL) << 7);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x80ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 15) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 16) & 0x1)))) & 0x1ULL) << 7);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x100ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 17) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 18) & 0x1))) & 0x1ULL) << 8);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x100ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 17) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 18) & 0x1)))) & 0x1ULL) << 8);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x200ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 19) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 20) & 0x1))) & 0x1ULL) << 9);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x200ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 19) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 20) & 0x1)))) & 0x1ULL) << 9);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x400ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 21) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 22) & 0x1))) & 0x1ULL) << 10);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x400ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 21) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 22) & 0x1)))) & 0x1ULL) << 10);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x800ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 23) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 24) & 0x1))) & 0x1ULL) << 11);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x800ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 23) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 24) & 0x1)))) & 0x1ULL) << 11);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x1000ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 25) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 26) & 0x1))) & 0x1ULL) << 12);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x1000ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 25) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 26) & 0x1)))) & 0x1ULL) << 12);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x2000ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 27) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 28) & 0x1))) & 0x1ULL) << 13);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x2000ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 27) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 28) & 0x1)))) & 0x1ULL) << 13);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x4000ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 29) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 30) & 0x1))) & 0x1ULL) << 14);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x4000ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 29) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 30) & 0x1)))) & 0x1ULL) << 14);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x8000ULL) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) & 1)) & 0x1ULL) << 15);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x8000ULL) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) & 1)) & 0x1ULL) << 15);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x10000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 1) & 0x1)) & 0x1ULL) << 16);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x10000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 1) & 0x1)) & 0x1ULL) << 16);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x20000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 2) & 0x1)) & 0x1ULL) << 17);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x20000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 2) & 0x1)) & 0x1ULL) << 17);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x40000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 3) & 0x1)) & 0x1ULL) << 18);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x40000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 3) & 0x1)) & 0x1ULL) << 18);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x80000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 4) & 0x1)) & 0x1ULL) << 19);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x80000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 4) & 0x1)) & 0x1ULL) << 19);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x100000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 5) & 0x1)) & 0x1ULL) << 20);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x100000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 5) & 0x1)) & 0x1ULL) << 20);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x200000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 6) & 0x1)) & 0x1ULL) << 21);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x200000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 6) & 0x1)) & 0x1ULL) << 21);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x400000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 7) & 0x1)) & 0x1ULL) << 22);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x400000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 7) & 0x1)) & 0x1ULL) << 22);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x800000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 8) & 0x1)) & 0x1ULL) << 23);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x800000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 8) & 0x1)) & 0x1ULL) << 23);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x1000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 9) & 0x1)) & 0x1ULL) << 24);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x1000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 9) & 0x1)) & 0x1ULL) << 24);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x2000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 10) & 0x1)) & 0x1ULL) << 25);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x2000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 10) & 0x1)) & 0x1ULL) << 25);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x4000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 11) & 0x1)) & 0x1ULL) << 26);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x4000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 11) & 0x1)) & 0x1ULL) << 26);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x8000000ULL) | (((0) & 0x1ULL) << 27);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x8000000ULL) | (((0) & 0x1ULL) << 27);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x10000000ULL) | (((0) & 0x1ULL) << 28);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x10000000ULL) | (((0) & 0x1ULL) << 28);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x20000000ULL) | (((0) & 0x1ULL) << 29);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x20000000ULL) | (((0) & 0x1ULL) << 29);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x40000000ULL) | (((0) & 0x1ULL) << 30);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x1ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 2) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 1) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 2) & 0x1))) & 0x1ULL) << 0);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x2ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 3) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 4) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 3) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 4) & 0x1))) & 0x1ULL) << 1);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x4ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 5) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 6) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 5) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 6) & 0x1))) & 0x1ULL) << 2);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x8ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 7) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 8) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 7) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 8) & 0x1))) & 0x1ULL) << 3);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x10ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 9) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 10) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 9) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 10) & 0x1))) & 0x1ULL) << 4);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x20ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 11) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 12) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 11) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 12) & 0x1))) & 0x1ULL) << 5);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x40ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 13) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 14) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 13) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 14) & 0x1))) & 0x1ULL) << 6);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x80ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 15) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 16) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 15) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 16) & 0x1))) & 0x1ULL) << 7);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x100ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 17) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 18) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 17) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 18) & 0x1))) & 0x1ULL) << 8);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x200ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 19) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 20) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 19) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 20) & 0x1))) & 0x1ULL) << 9);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x400ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 21) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 22) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 21) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 22) & 0x1))) & 0x1ULL) << 10);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x800ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 23) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 24) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 23) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 24) & 0x1))) & 0x1ULL) << 11);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x1000ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 25) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 26) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 25) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 26) & 0x1))) & 0x1ULL) << 12);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x2000ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 27) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 28) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 27) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 28) & 0x1))) & 0x1ULL) << 13);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x4000ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 29) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 30) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 29) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 30) & 0x1))) & 0x1ULL) << 14);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x8000ULL) | (((0) & 0x1ULL) << 15);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x10000ULL) | (((0) & 0x1ULL) << 16);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x20000ULL) | (((0) & 0x1ULL) << 17);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x40000ULL) | (((0) & 0x1ULL) << 18);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x80000ULL) | (((0) & 0x1ULL) << 19);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x100000ULL) | (((0) & 0x1ULL) << 20);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x200000ULL) | (((0) & 0x1ULL) << 21);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x400000ULL) | (((0) & 0x1ULL) << 22);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x800000ULL) | (((0) & 0x1ULL) << 23);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x1000000ULL) | (((0) & 0x1ULL) << 24);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x2000000ULL) | (((0) & 0x1ULL) << 25);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x4000000ULL) | (((0) & 0x1ULL) << 26);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x8000000ULL) | (((0) & 0x1ULL) << 27);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x10000000ULL) | (((0) & 0x1ULL) << 28);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x20000000ULL) | (((0) & 0x1ULL) << 29);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x40000000ULL) | (((0) & 0x1ULL) << 30);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x40000000ULL) | (((0) & 0x1ULL) << 30);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i;
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o;
    s->u_reg_u_rsp_intg_gen_rsp_intg = (((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 57) & 0x7F);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o;
    s->u_reg_u_rsp_intg_gen_data_intg = (((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o) >> 32) & 0x7F);
    s->u_reg_u_rsp_intg_gen_tl_o_d_valid = s->u_reg_u_rsp_intg_gen_tl_o_d_valid;
    s->u_reg_u_rsp_intg_gen_tl_o_d_opcode = s->u_reg_u_rsp_intg_gen_tl_o_d_opcode;
    s->u_reg_u_rsp_intg_gen_tl_o_d_param = s->u_reg_u_rsp_intg_gen_tl_o_d_param;
    s->u_reg_u_rsp_intg_gen_tl_o_d_size = s->u_reg_u_rsp_intg_gen_tl_o_d_size;
    s->u_reg_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_rsp_intg_gen_tl_o_d_sink = s->u_reg_u_rsp_intg_gen_tl_o_d_sink;
    s->u_reg_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg;
    s->u_reg_u_rsp_intg_gen_tl_o_d_error = s->u_reg_u_rsp_intg_gen_tl_o_d_error;
    s->u_reg_u_rsp_intg_gen_tl_o_a_ready = s->u_reg_u_rsp_intg_gen_tl_o_a_ready;
    s->u_reg_u_reg_if_clk_i = s->u_reg_clk_i;
    s->u_reg_u_reg_if_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_reg_if_tl_i_a_valid = s->u_reg_tl_i_a_valid;
    s->u_reg_u_reg_if_tl_i_a_opcode = s->u_reg_tl_i_a_opcode;
    s->u_reg_u_reg_if_tl_i_a_param = s->u_reg_tl_i_a_param;
    s->u_reg_u_reg_if_tl_i_a_size = s->u_reg_tl_i_a_size;
    s->u_reg_u_reg_if_tl_i_a_source = s->u_reg_tl_i_a_source;
    s->u_reg_u_reg_if_tl_i_a_address = s->u_reg_tl_i_a_address;
    s->u_reg_u_reg_if_tl_i_a_mask = s->u_reg_tl_i_a_mask;
    s->u_reg_u_reg_if_tl_i_a_data = s->u_reg_tl_i_a_data;
    s->u_reg_u_reg_if_tl_i_a_user_rsvd = s->u_reg_tl_i_a_user_rsvd;
    s->u_reg_u_reg_if_tl_i_a_user_instr_type = s->u_reg_tl_i_a_user_instr_type;
    s->u_reg_u_reg_if_tl_i_a_user_cmd_intg = s->u_reg_tl_i_a_user_cmd_intg;
    s->u_reg_u_reg_if_tl_i_a_user_data_intg = s->u_reg_tl_i_a_user_data_intg;
    s->u_reg_u_reg_if_tl_i_d_ready = s->u_reg_tl_i_d_ready;
    s->u_reg_u_reg_if_en_ifetch_i = 9;
    s->u_reg_u_reg_if_busy_i = 0;
    s->u_reg_u_reg_if_rdata_i = s->u_reg_reg_rdata_next;
    s->u_reg_u_reg_if_rst_ni = s->u_reg_u_reg_if_rst_ni;
    s->u_reg_u_reg_if_tl_i_a_valid = s->u_reg_u_reg_if_tl_i_a_valid;
    s->u_reg_u_reg_if_tl_i_a_opcode = s->u_reg_u_reg_if_tl_i_a_opcode;
    s->u_reg_u_reg_if_tl_i_a_param = s->u_reg_u_reg_if_tl_i_a_param;
    s->u_reg_u_reg_if_tl_i_a_size = s->u_reg_u_reg_if_tl_i_a_size;
    s->u_reg_u_reg_if_tl_i_a_source = s->u_reg_u_reg_if_tl_i_a_source;
    s->u_reg_u_reg_if_tl_i_a_address = s->u_reg_u_reg_if_tl_i_a_address;
    s->u_reg_u_reg_if_tl_i_a_mask = s->u_reg_u_reg_if_tl_i_a_mask;
    s->u_reg_u_reg_if_tl_i_a_data = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_tl_i_a_user_rsvd = s->u_reg_u_reg_if_tl_i_a_user_rsvd;
    s->u_reg_u_reg_if_tl_i_a_user_instr_type = s->u_reg_u_reg_if_tl_i_a_user_instr_type;
    s->u_reg_u_reg_if_tl_i_a_user_cmd_intg = s->u_reg_u_reg_if_tl_i_a_user_cmd_intg;
    s->u_reg_u_reg_if_tl_i_a_user_data_intg = s->u_reg_u_reg_if_tl_i_a_user_data_intg;
    s->u_reg_u_reg_if_tl_i_d_ready = s->u_reg_u_reg_if_tl_i_d_ready;
    s->u_reg_u_reg_if_rdata_i = s->u_reg_u_reg_if_rdata_i;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid = s->u_reg_u_reg_if_outstanding_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode = s->u_reg_u_reg_if_rspop_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param = 0;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size = s->u_reg_u_reg_if_reqsz_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_reg_if_reqid_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink = 0;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_reg_if_rdata_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg = 0;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg = 0;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error = s->u_reg_u_reg_if_error_q;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready = ((((s->u_reg_u_reg_if_outstanding_q) | (s->u_reg_u_reg_if_busy_i))) ^ (1));
    s->u_reg_u_reg_if_u_rsp_intg_gen_rsp_intg = 0;
    s->u_reg_u_reg_if_u_rsp_intg_gen_data_intg = 0;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid;
    s->u_reg_u_reg_if_d_ack = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & (s->u_reg_u_reg_if_tl_i_d_ready);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready;
    s->u_reg_u_reg_if_a_ack = (s->u_reg_u_reg_if_tl_i_a_valid) & (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready);
    s->u_reg_u_reg_if_wr_req = (s->u_reg_u_reg_if_a_ack) & ((((s->u_reg_u_reg_if_tl_i_a_opcode) == (0))) | (((s->u_reg_u_reg_if_tl_i_a_opcode) == (1))));
    s->u_reg_u_reg_if_rd_req = (s->u_reg_u_reg_if_a_ack) & (((s->u_reg_u_reg_if_tl_i_a_opcode) == (4)));
    s->u_reg_u_reg_if_u_err_clk_i = s->u_reg_u_reg_if_clk_i;
    s->u_reg_u_reg_if_u_err_rst_ni = s->u_reg_u_reg_if_rst_ni;
    s->u_reg_u_reg_if_u_err_tl_i_a_valid = s->u_reg_u_reg_if_tl_i_a_valid;
    s->u_reg_u_reg_if_u_err_tl_i_a_opcode = s->u_reg_u_reg_if_tl_i_a_opcode;
    s->u_reg_u_reg_if_u_err_tl_i_a_param = s->u_reg_u_reg_if_tl_i_a_param;
    s->u_reg_u_reg_if_u_err_tl_i_a_size = s->u_reg_u_reg_if_tl_i_a_size;
    s->u_reg_u_reg_if_u_err_tl_i_a_source = s->u_reg_u_reg_if_tl_i_a_source;
    s->u_reg_u_reg_if_u_err_tl_i_a_address = s->u_reg_u_reg_if_tl_i_a_address;
    s->u_reg_u_reg_if_u_err_tl_i_a_mask = s->u_reg_u_reg_if_tl_i_a_mask;
    s->u_reg_u_reg_if_u_err_tl_i_a_data = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd = s->u_reg_u_reg_if_tl_i_a_user_rsvd;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type = s->u_reg_u_reg_if_tl_i_a_user_instr_type;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg = s->u_reg_u_reg_if_tl_i_a_user_cmd_intg;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg = s->u_reg_u_reg_if_tl_i_a_user_data_intg;
    s->u_reg_u_reg_if_u_err_tl_i_d_ready = s->u_reg_u_reg_if_tl_i_d_ready;
    s->u_reg_u_reg_if_u_err_tl_i_a_valid = s->u_reg_u_reg_if_u_err_tl_i_a_valid;
    s->u_reg_u_reg_if_u_err_tl_i_a_opcode = s->u_reg_u_reg_if_u_err_tl_i_a_opcode;
    s->u_reg_u_reg_if_u_err_tl_i_a_param = s->u_reg_u_reg_if_u_err_tl_i_a_param;
    s->u_reg_u_reg_if_u_err_tl_i_a_size = s->u_reg_u_reg_if_u_err_tl_i_a_size;
    s->u_reg_u_reg_if_u_err_tl_i_a_source = s->u_reg_u_reg_if_u_err_tl_i_a_source;
    s->u_reg_u_reg_if_u_err_tl_i_a_address = s->u_reg_u_reg_if_u_err_tl_i_a_address;
    s->u_reg_u_reg_if_u_err_mask = (1) << ((((0) << 2) | (((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 0x3))));
    s->u_reg_u_reg_if_u_err_tl_i_a_mask = s->u_reg_u_reg_if_u_err_tl_i_a_mask;
    s->u_reg_u_reg_if_u_err_tl_i_a_data = s->u_reg_u_reg_if_u_err_tl_i_a_data;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd = s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type = s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg = s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg = s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg;
    s->u_reg_u_reg_if_u_err_tl_i_d_ready = s->u_reg_u_reg_if_u_err_tl_i_d_ready;
    s->u_reg_u_reg_if_tl_o_d_valid = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid;
    s->u_reg_u_rsp_intg_gen_tl_i_d_valid = s->u_reg_u_reg_if_tl_o_d_valid;
    s->u_reg_u_rsp_intg_gen_tl_i_d_valid = s->u_reg_u_rsp_intg_gen_tl_i_d_valid;
    s->u_reg_u_reg_if_tl_o_d_opcode = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode;
    s->u_reg_u_rsp_intg_gen_tl_i_d_opcode = s->u_reg_u_reg_if_tl_o_d_opcode;
    s->u_reg_u_rsp_intg_gen_tl_i_d_opcode = s->u_reg_u_rsp_intg_gen_tl_i_d_opcode;
    s->u_reg_u_reg_if_tl_o_d_param = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param;
    s->u_reg_u_rsp_intg_gen_tl_i_d_param = s->u_reg_u_reg_if_tl_o_d_param;
    s->u_reg_u_rsp_intg_gen_tl_i_d_param = s->u_reg_u_rsp_intg_gen_tl_i_d_param;
    s->u_reg_u_reg_if_tl_o_d_size = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size;
    s->u_reg_u_rsp_intg_gen_tl_i_d_size = s->u_reg_u_reg_if_tl_o_d_size;
    s->u_reg_u_rsp_intg_gen_tl_i_d_size = s->u_reg_u_rsp_intg_gen_tl_i_d_size;
    s->u_reg_u_reg_if_tl_o_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_reg_if_tl_o_d_source;
    s->u_reg_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_u_reg_if_tl_o_d_sink = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink;
    s->u_reg_u_rsp_intg_gen_tl_i_d_sink = s->u_reg_u_reg_if_tl_o_d_sink;
    s->u_reg_u_rsp_intg_gen_tl_i_d_sink = s->u_reg_u_rsp_intg_gen_tl_i_d_sink;
    s->u_reg_u_reg_if_tl_o_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_reg_if_tl_o_d_data;
    s->u_reg_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i = s->u_reg_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i;
    s->u_reg_u_reg_if_tl_o_d_user_rsp_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg = s->u_reg_u_reg_if_tl_o_d_user_rsp_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg = s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg;
    s->u_reg_u_reg_if_tl_o_d_user_data_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg = s->u_reg_u_reg_if_tl_o_d_user_data_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg = s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg;
    s->u_reg_u_reg_if_tl_o_d_error = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error;
    s->u_reg_u_rsp_intg_gen_tl_i_d_error = s->u_reg_u_reg_if_tl_o_d_error;
    s->u_reg_u_rsp_intg_gen_tl_i_d_error = s->u_reg_u_rsp_intg_gen_tl_i_d_error;
    s->u_reg_u_reg_if_tl_o_a_ready = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready;
    s->u_reg_u_rsp_intg_gen_tl_i_a_ready = s->u_reg_u_reg_if_tl_o_a_ready;
    s->u_reg_u_rsp_intg_gen_tl_i_a_ready = s->u_reg_u_rsp_intg_gen_tl_i_a_ready;
    s->u_reg_u_reg_if_intg_error_o = 0;
    s->u_reg_u_reg_if_re_o = ((s->u_reg_u_reg_if_rd_req) & (((s->u_reg_u_reg_if_err_internal) ^ (1))));
    s->u_reg_u_reg_if_we_o = ((s->u_reg_u_reg_if_wr_req) & (((s->u_reg_u_reg_if_err_internal) ^ (1))));
    s->u_reg_reg_we = s->u_reg_u_reg_if_we_o;
    s->u_reg_intr_state_we = (((s->u_reg_addr_hit) & 1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_intr_enable_we = ((((s->u_reg_addr_hit) >> 1) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_intr_test_we = ((((s->u_reg_addr_hit) >> 2) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_alert_test_we = ((((s->u_reg_addr_hit) >> 3) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_ctrl_we = ((((s->u_reg_addr_hit) >> 4) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_prediv_ch0_we = ((((s->u_reg_addr_hit) >> 5) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_prediv_ch1_we = ((((s->u_reg_addr_hit) >> 6) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_data_ch0_0_we = ((((s->u_reg_addr_hit) >> 7) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_data_ch0_1_we = ((((s->u_reg_addr_hit) >> 8) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_data_ch1_0_we = ((((s->u_reg_addr_hit) >> 9) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_data_ch1_1_we = ((((s->u_reg_addr_hit) >> 10) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_size_we = ((((s->u_reg_addr_hit) >> 11) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((s->u_reg_addr_hit) == (0)))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_u_prim_reg_we_check_en_i = ((s->u_reg_reg_we) & (((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((s->u_reg_addr_hit) == (0))))) ^ (1))));
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_en_i = s->u_reg_u_prim_reg_we_check_en_i;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_o = (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 0) & 0x1)) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_en_i) ^ (1))) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 0) & 0x1)))));
    s->u_reg_u_prim_reg_we_check_err_o = s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_o;
    s->u_reg_reg_we_err = s->u_reg_u_prim_reg_we_check_err_o;
    s->u_reg_u_reg_if_error_i = ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((s->u_reg_addr_hit) == (0))))) | (s->u_reg_wr_err) | (s->u_reg_intg_err));
    s->u_reg_u_reg_if_error_i = s->u_reg_u_reg_if_error_i;
    s->u_reg_u_reg_if_addr_o = ((((uint64_t)(0)) << 0) | (((uint64_t)((((s->u_reg_u_reg_if_tl_i_a_address) >> 2) & 0xF))) << 2));
    s->u_reg_reg_addr = s->u_reg_u_reg_if_addr_o;
    s->u_reg_u_reg_if_wdata_o = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_be_o = s->u_reg_u_reg_if_tl_i_a_mask;
    s->u_reg_reg_be = s->u_reg_u_reg_if_be_o;
    s->u_reg_u_intr_state_done_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_intr_state_done_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_intr_state_done_ch0_we = s->u_reg_intr_state_we;
    s->u_reg_u_intr_state_done_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_intr_state_done_ch0_rst_ni = s->u_reg_u_intr_state_done_ch0_rst_ni;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_we = s->u_reg_u_intr_state_done_ch0_we;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd = s->u_reg_u_intr_state_done_ch0_wd;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd = s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd;
    s->u_reg_u_intr_state_done_ch0_qe = s->u_reg_u_intr_state_done_ch0_we;
    s->u_reg_u_intr_state_done_ch0_q = s->u_reg_u_intr_state_done_ch0_q;
    s->u_reg_reg2hw_intr_state_done_ch0_q = s->u_reg_u_intr_state_done_ch0_q;
    s->u_reg_u_intr_state_done_ch0_qs = s->u_reg_u_intr_state_done_ch0_q;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q = s->u_reg_u_intr_state_done_ch0_q;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q = s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q;
    s->u_reg_u_intr_state_done_ch0_qs = s->u_reg_u_intr_state_done_ch0_qs;
    s->u_reg_intr_state_done_ch0_qs = s->u_reg_u_intr_state_done_ch0_qs;
    s->u_reg_u_intr_state_done_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_intr_state_done_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_intr_state_done_ch1_we = s->u_reg_intr_state_we;
    s->u_reg_u_intr_state_done_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1);
    s->u_reg_u_intr_state_done_ch1_rst_ni = s->u_reg_u_intr_state_done_ch1_rst_ni;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_we = s->u_reg_u_intr_state_done_ch1_we;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd = s->u_reg_u_intr_state_done_ch1_wd;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd = s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd;
    s->u_reg_u_intr_state_done_ch1_qe = s->u_reg_u_intr_state_done_ch1_we;
    s->u_reg_u_intr_state_done_ch1_q = s->u_reg_u_intr_state_done_ch1_q;
    s->u_reg_reg2hw_intr_state_done_ch1_q = s->u_reg_u_intr_state_done_ch1_q;
    s->u_reg_u_intr_state_done_ch1_qs = s->u_reg_u_intr_state_done_ch1_q;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q = s->u_reg_u_intr_state_done_ch1_q;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q = s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q;
    s->u_reg_u_intr_state_done_ch1_qs = s->u_reg_u_intr_state_done_ch1_qs;
    s->u_reg_intr_state_done_ch1_qs = s->u_reg_u_intr_state_done_ch1_qs;
    s->u_reg_u_intr_enable_done_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_intr_enable_done_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_intr_enable_done_ch0_we = s->u_reg_intr_enable_we;
    s->u_reg_u_intr_enable_done_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_intr_enable_done_ch0_de = 0;
    s->u_reg_u_intr_enable_done_ch0_d = 0;
    s->u_reg_u_intr_enable_done_ch0_rst_ni = s->u_reg_u_intr_enable_done_ch0_rst_ni;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_we = s->u_reg_u_intr_enable_done_ch0_we;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd = s->u_reg_u_intr_enable_done_ch0_wd;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_de = s->u_reg_u_intr_enable_done_ch0_de;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d = s->u_reg_u_intr_enable_done_ch0_d;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd = s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d = s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_we) | (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_de));
    s->u_reg_u_intr_enable_done_ch0_wr_en = s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_we) ? (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wd) : (s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_d));
    s->u_reg_u_intr_enable_done_ch0_wr_data = s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_intr_enable_done_ch0_qe = s->u_reg_u_intr_enable_done_ch0_we;
    s->u_reg_u_intr_enable_done_ch0_q = s->u_reg_u_intr_enable_done_ch0_q;
    s->u_reg_reg2hw_intr_enable_done_ch0_q = s->u_reg_u_intr_enable_done_ch0_q;
    s->u_reg_u_intr_enable_done_ch0_qs = s->u_reg_u_intr_enable_done_ch0_q;
    s->u_reg_u_intr_enable_done_ch0_wr_en_data_arb_q = s->u_reg_u_intr_enable_done_ch0_q;
    s->u_reg_u_intr_enable_done_ch0_qs = s->u_reg_u_intr_enable_done_ch0_qs;
    s->u_reg_intr_enable_done_ch0_qs = s->u_reg_u_intr_enable_done_ch0_qs;
    s->u_reg_u_intr_enable_done_ch0_ds = ((s->u_reg_u_intr_enable_done_ch0_wr_en) ? (s->u_reg_u_intr_enable_done_ch0_wr_data) : (s->u_reg_u_intr_enable_done_ch0_qs));
    s->u_reg_u_intr_enable_done_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_intr_enable_done_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_intr_enable_done_ch1_we = s->u_reg_intr_enable_we;
    s->u_reg_u_intr_enable_done_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1);
    s->u_reg_u_intr_enable_done_ch1_de = 0;
    s->u_reg_u_intr_enable_done_ch1_d = 0;
    s->u_reg_u_intr_enable_done_ch1_rst_ni = s->u_reg_u_intr_enable_done_ch1_rst_ni;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_we = s->u_reg_u_intr_enable_done_ch1_we;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd = s->u_reg_u_intr_enable_done_ch1_wd;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_de = s->u_reg_u_intr_enable_done_ch1_de;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d = s->u_reg_u_intr_enable_done_ch1_d;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd = s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d = s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_we) | (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_de));
    s->u_reg_u_intr_enable_done_ch1_wr_en = s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_we) ? (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wd) : (s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_d));
    s->u_reg_u_intr_enable_done_ch1_wr_data = s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_intr_enable_done_ch1_qe = s->u_reg_u_intr_enable_done_ch1_we;
    s->u_reg_u_intr_enable_done_ch1_q = s->u_reg_u_intr_enable_done_ch1_q;
    s->u_reg_reg2hw_intr_enable_done_ch1_q = s->u_reg_u_intr_enable_done_ch1_q;
    s->u_reg_u_intr_enable_done_ch1_qs = s->u_reg_u_intr_enable_done_ch1_q;
    s->u_reg_u_intr_enable_done_ch1_wr_en_data_arb_q = s->u_reg_u_intr_enable_done_ch1_q;
    s->u_reg_u_intr_enable_done_ch1_qs = s->u_reg_u_intr_enable_done_ch1_qs;
    s->u_reg_intr_enable_done_ch1_qs = s->u_reg_u_intr_enable_done_ch1_qs;
    s->u_reg_u_intr_enable_done_ch1_ds = ((s->u_reg_u_intr_enable_done_ch1_wr_en) ? (s->u_reg_u_intr_enable_done_ch1_wr_data) : (s->u_reg_u_intr_enable_done_ch1_qs));
    s->u_reg_u_intr_test_done_ch0_re = 0;
    s->u_reg_u_intr_test_done_ch0_we = s->u_reg_intr_test_we;
    s->u_reg_u_intr_test_done_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_intr_test_done_ch0_d = 0;
    s->u_reg_u_intr_test_done_ch0_qe = s->u_reg_u_intr_test_done_ch0_we;
    s->u_reg_intr_test_flds_we = (s->u_reg_intr_test_flds_we & ~0x1ULL) | (((s->u_reg_u_intr_test_done_ch0_qe) & 0x1ULL) << 0);
    s->u_reg_u_intr_test_done_ch0_qre = s->u_reg_u_intr_test_done_ch0_re;
    s->u_reg_u_intr_test_done_ch0_q = s->u_reg_u_intr_test_done_ch0_wd;
    s->u_reg_reg2hw_intr_test_done_ch0_q = s->u_reg_u_intr_test_done_ch0_q;
    s->u_reg_u_intr_test_done_ch0_ds = s->u_reg_u_intr_test_done_ch0_d;
    s->u_reg_u_intr_test_done_ch0_qs = s->u_reg_u_intr_test_done_ch0_d;
    s->u_reg_u_intr_test_done_ch1_re = 0;
    s->u_reg_u_intr_test_done_ch1_we = s->u_reg_intr_test_we;
    s->u_reg_u_intr_test_done_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1);
    s->u_reg_u_intr_test_done_ch1_d = 0;
    s->u_reg_u_intr_test_done_ch1_qe = s->u_reg_u_intr_test_done_ch1_we;
    s->u_reg_intr_test_flds_we = (s->u_reg_intr_test_flds_we & ~0x2ULL) | (((s->u_reg_u_intr_test_done_ch1_qe) & 0x1ULL) << 1);
    s->u_reg_reg2hw_intr_test_done_ch0_qe = ((s->u_reg_intr_test_flds_we) == (3));
    s->u_reg_reg2hw_intr_test_done_ch1_qe = ((s->u_reg_intr_test_flds_we) == (3));
    s->u_reg_u_intr_test_done_ch1_qre = s->u_reg_u_intr_test_done_ch1_re;
    s->u_reg_u_intr_test_done_ch1_q = s->u_reg_u_intr_test_done_ch1_wd;
    s->u_reg_reg2hw_intr_test_done_ch1_q = s->u_reg_u_intr_test_done_ch1_q;
    s->u_reg_u_intr_test_done_ch1_ds = s->u_reg_u_intr_test_done_ch1_d;
    s->u_reg_u_intr_test_done_ch1_qs = s->u_reg_u_intr_test_done_ch1_d;
    s->u_reg_u_alert_test_re = 0;
    s->u_reg_u_alert_test_we = s->u_reg_alert_test_we;
    s->u_reg_u_alert_test_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_alert_test_d = 0;
    s->u_reg_u_alert_test_qe = s->u_reg_u_alert_test_we;
    s->u_reg_alert_test_flds_we = s->u_reg_u_alert_test_qe;
    s->u_reg_reg2hw_alert_test_qe = s->u_reg_alert_test_flds_we;
    s->u_reg_u_alert_test_qre = s->u_reg_u_alert_test_re;
    s->u_reg_u_alert_test_q = s->u_reg_u_alert_test_wd;
    s->u_reg_reg2hw_alert_test_q = s->u_reg_u_alert_test_q;
    s->u_reg_u_alert_test_ds = s->u_reg_u_alert_test_d;
    s->u_reg_u_alert_test_qs = s->u_reg_u_alert_test_d;
    s->u_reg_u_ctrl_enable_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_enable_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_enable_ch0_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_enable_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_ctrl_enable_ch0_de = 0;
    s->u_reg_u_ctrl_enable_ch0_d = 0;
    s->u_reg_u_ctrl_enable_ch0_rst_ni = s->u_reg_u_ctrl_enable_ch0_rst_ni;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_we = s->u_reg_u_ctrl_enable_ch0_we;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_enable_ch0_wd;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_de = s->u_reg_u_ctrl_enable_ch0_de;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_enable_ch0_d;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_de));
    s->u_reg_u_ctrl_enable_ch0_wr_en = s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_d));
    s->u_reg_u_ctrl_enable_ch0_wr_data = s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_enable_ch0_qe = s->u_reg_u_ctrl_enable_ch0_we;
    s->u_reg_u_ctrl_enable_ch0_q = s->u_reg_u_ctrl_enable_ch0_q;
    s->u_reg_reg2hw_ctrl_enable_ch0_q = s->u_reg_u_ctrl_enable_ch0_q;
    s->u_reg_u_ctrl_enable_ch0_qs = s->u_reg_u_ctrl_enable_ch0_q;
    s->u_reg_u_ctrl_enable_ch0_wr_en_data_arb_q = s->u_reg_u_ctrl_enable_ch0_q;
    s->u_reg_u_ctrl_enable_ch0_qs = s->u_reg_u_ctrl_enable_ch0_qs;
    s->u_reg_ctrl_enable_ch0_qs = s->u_reg_u_ctrl_enable_ch0_qs;
    s->u_reg_u_ctrl_enable_ch0_ds = ((s->u_reg_u_ctrl_enable_ch0_wr_en) ? (s->u_reg_u_ctrl_enable_ch0_wr_data) : (s->u_reg_u_ctrl_enable_ch0_qs));
    s->u_reg_u_ctrl_enable_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_enable_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_enable_ch1_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_enable_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 1) & 0x1);
    s->u_reg_u_ctrl_enable_ch1_de = 0;
    s->u_reg_u_ctrl_enable_ch1_d = 0;
    s->u_reg_u_ctrl_enable_ch1_rst_ni = s->u_reg_u_ctrl_enable_ch1_rst_ni;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_we = s->u_reg_u_ctrl_enable_ch1_we;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_enable_ch1_wd;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_de = s->u_reg_u_ctrl_enable_ch1_de;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_enable_ch1_d;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_de));
    s->u_reg_u_ctrl_enable_ch1_wr_en = s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_d));
    s->u_reg_u_ctrl_enable_ch1_wr_data = s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_enable_ch1_qe = s->u_reg_u_ctrl_enable_ch1_we;
    s->u_reg_u_ctrl_enable_ch1_q = s->u_reg_u_ctrl_enable_ch1_q;
    s->u_reg_reg2hw_ctrl_enable_ch1_q = s->u_reg_u_ctrl_enable_ch1_q;
    s->u_reg_u_ctrl_enable_ch1_qs = s->u_reg_u_ctrl_enable_ch1_q;
    s->u_reg_u_ctrl_enable_ch1_wr_en_data_arb_q = s->u_reg_u_ctrl_enable_ch1_q;
    s->u_reg_u_ctrl_enable_ch1_qs = s->u_reg_u_ctrl_enable_ch1_qs;
    s->u_reg_ctrl_enable_ch1_qs = s->u_reg_u_ctrl_enable_ch1_qs;
    s->u_reg_u_ctrl_enable_ch1_ds = ((s->u_reg_u_ctrl_enable_ch1_wr_en) ? (s->u_reg_u_ctrl_enable_ch1_wr_data) : (s->u_reg_u_ctrl_enable_ch1_qs));
    s->u_reg_u_ctrl_polarity_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_polarity_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_polarity_ch0_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_polarity_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 2) & 0x1);
    s->u_reg_u_ctrl_polarity_ch0_de = 0;
    s->u_reg_u_ctrl_polarity_ch0_d = 0;
    s->u_reg_u_ctrl_polarity_ch0_rst_ni = s->u_reg_u_ctrl_polarity_ch0_rst_ni;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_we = s->u_reg_u_ctrl_polarity_ch0_we;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_polarity_ch0_wd;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_de = s->u_reg_u_ctrl_polarity_ch0_de;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_polarity_ch0_d;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_de));
    s->u_reg_u_ctrl_polarity_ch0_wr_en = s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_d));
    s->u_reg_u_ctrl_polarity_ch0_wr_data = s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_polarity_ch0_qe = s->u_reg_u_ctrl_polarity_ch0_we;
    s->u_reg_u_ctrl_polarity_ch0_q = s->u_reg_u_ctrl_polarity_ch0_q;
    s->u_reg_reg2hw_ctrl_polarity_ch0_q = s->u_reg_u_ctrl_polarity_ch0_q;
    s->u_reg_u_ctrl_polarity_ch0_qs = s->u_reg_u_ctrl_polarity_ch0_q;
    s->u_reg_u_ctrl_polarity_ch0_wr_en_data_arb_q = s->u_reg_u_ctrl_polarity_ch0_q;
    s->u_reg_u_ctrl_polarity_ch0_qs = s->u_reg_u_ctrl_polarity_ch0_qs;
    s->u_reg_ctrl_polarity_ch0_qs = s->u_reg_u_ctrl_polarity_ch0_qs;
    s->u_reg_u_ctrl_polarity_ch0_ds = ((s->u_reg_u_ctrl_polarity_ch0_wr_en) ? (s->u_reg_u_ctrl_polarity_ch0_wr_data) : (s->u_reg_u_ctrl_polarity_ch0_qs));
    s->u_reg_u_ctrl_polarity_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_polarity_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_polarity_ch1_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_polarity_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 3) & 0x1);
    s->u_reg_u_ctrl_polarity_ch1_de = 0;
    s->u_reg_u_ctrl_polarity_ch1_d = 0;
    s->u_reg_u_ctrl_polarity_ch1_rst_ni = s->u_reg_u_ctrl_polarity_ch1_rst_ni;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_we = s->u_reg_u_ctrl_polarity_ch1_we;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_polarity_ch1_wd;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_de = s->u_reg_u_ctrl_polarity_ch1_de;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_polarity_ch1_d;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_de));
    s->u_reg_u_ctrl_polarity_ch1_wr_en = s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_d));
    s->u_reg_u_ctrl_polarity_ch1_wr_data = s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_polarity_ch1_qe = s->u_reg_u_ctrl_polarity_ch1_we;
    s->u_reg_u_ctrl_polarity_ch1_q = s->u_reg_u_ctrl_polarity_ch1_q;
    s->u_reg_reg2hw_ctrl_polarity_ch1_q = s->u_reg_u_ctrl_polarity_ch1_q;
    s->u_reg_u_ctrl_polarity_ch1_qs = s->u_reg_u_ctrl_polarity_ch1_q;
    s->u_reg_u_ctrl_polarity_ch1_wr_en_data_arb_q = s->u_reg_u_ctrl_polarity_ch1_q;
    s->u_reg_u_ctrl_polarity_ch1_qs = s->u_reg_u_ctrl_polarity_ch1_qs;
    s->u_reg_ctrl_polarity_ch1_qs = s->u_reg_u_ctrl_polarity_ch1_qs;
    s->u_reg_u_ctrl_polarity_ch1_ds = ((s->u_reg_u_ctrl_polarity_ch1_wr_en) ? (s->u_reg_u_ctrl_polarity_ch1_wr_data) : (s->u_reg_u_ctrl_polarity_ch1_qs));
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 4) & 0x1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_de = 0;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_d = 0;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni = s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_we = s->u_reg_u_ctrl_inactive_level_pcl_ch0_we;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pcl_ch0_wd;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_de = s->u_reg_u_ctrl_inactive_level_pcl_ch0_de;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pcl_ch0_d;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_de));
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en = s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_d));
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_data = s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_qe = s->u_reg_u_ctrl_inactive_level_pcl_ch0_we;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_q = s->u_reg_u_ctrl_inactive_level_pcl_ch0_q;
    s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch0_q = s->u_reg_u_ctrl_inactive_level_pcl_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs = s->u_reg_u_ctrl_inactive_level_pcl_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en_data_arb_q = s->u_reg_u_ctrl_inactive_level_pcl_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs = s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs;
    s->u_reg_ctrl_inactive_level_pcl_ch0_qs = s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs;
    s->u_reg_u_ctrl_inactive_level_pcl_ch0_ds = ((s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_data) : (s->u_reg_u_ctrl_inactive_level_pcl_ch0_qs));
    s->u_reg_u_ctrl_inactive_level_pda_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 5) & 0x1);
    s->u_reg_u_ctrl_inactive_level_pda_ch0_de = 0;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_d = 0;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni = s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_we = s->u_reg_u_ctrl_inactive_level_pda_ch0_we;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pda_ch0_wd;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_de = s->u_reg_u_ctrl_inactive_level_pda_ch0_de;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pda_ch0_d;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_de));
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en = s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_d));
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_data = s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_qe = s->u_reg_u_ctrl_inactive_level_pda_ch0_we;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_q = s->u_reg_u_ctrl_inactive_level_pda_ch0_q;
    s->u_reg_reg2hw_ctrl_inactive_level_pda_ch0_q = s->u_reg_u_ctrl_inactive_level_pda_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_qs = s->u_reg_u_ctrl_inactive_level_pda_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en_data_arb_q = s->u_reg_u_ctrl_inactive_level_pda_ch0_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_qs = s->u_reg_u_ctrl_inactive_level_pda_ch0_qs;
    s->u_reg_ctrl_inactive_level_pda_ch0_qs = s->u_reg_u_ctrl_inactive_level_pda_ch0_qs;
    s->u_reg_u_ctrl_inactive_level_pda_ch0_ds = ((s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_data) : (s->u_reg_u_ctrl_inactive_level_pda_ch0_qs));
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 6) & 0x1);
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_de = 0;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_d = 0;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni = s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_we = s->u_reg_u_ctrl_inactive_level_pcl_ch1_we;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pcl_ch1_wd;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_de = s->u_reg_u_ctrl_inactive_level_pcl_ch1_de;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pcl_ch1_d;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_de));
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en = s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_d));
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_data = s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_qe = s->u_reg_u_ctrl_inactive_level_pcl_ch1_we;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_q = s->u_reg_u_ctrl_inactive_level_pcl_ch1_q;
    s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch1_q = s->u_reg_u_ctrl_inactive_level_pcl_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs = s->u_reg_u_ctrl_inactive_level_pcl_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en_data_arb_q = s->u_reg_u_ctrl_inactive_level_pcl_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs = s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs;
    s->u_reg_ctrl_inactive_level_pcl_ch1_qs = s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs;
    s->u_reg_u_ctrl_inactive_level_pcl_ch1_ds = ((s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_data) : (s->u_reg_u_ctrl_inactive_level_pcl_ch1_qs));
    s->u_reg_u_ctrl_inactive_level_pda_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 7) & 0x1);
    s->u_reg_u_ctrl_inactive_level_pda_ch1_de = 0;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_d = 0;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni = s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_we = s->u_reg_u_ctrl_inactive_level_pda_ch1_we;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pda_ch1_wd;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_de = s->u_reg_u_ctrl_inactive_level_pda_ch1_de;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pda_ch1_d;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd = s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d = s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_we) | (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_de));
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en = s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_d));
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_data = s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_qe = s->u_reg_u_ctrl_inactive_level_pda_ch1_we;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_q = s->u_reg_u_ctrl_inactive_level_pda_ch1_q;
    s->u_reg_reg2hw_ctrl_inactive_level_pda_ch1_q = s->u_reg_u_ctrl_inactive_level_pda_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_qs = s->u_reg_u_ctrl_inactive_level_pda_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en_data_arb_q = s->u_reg_u_ctrl_inactive_level_pda_ch1_q;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_qs = s->u_reg_u_ctrl_inactive_level_pda_ch1_qs;
    s->u_reg_ctrl_inactive_level_pda_ch1_qs = s->u_reg_u_ctrl_inactive_level_pda_ch1_qs;
    s->u_reg_u_ctrl_inactive_level_pda_ch1_ds = ((s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en) ? (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_data) : (s->u_reg_u_ctrl_inactive_level_pda_ch1_qs));
    s->u_reg_u_prediv_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_prediv_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_prediv_ch0_we = s->u_reg_prediv_ch0_we;
    s->u_reg_u_prediv_ch0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_prediv_ch0_de = 0;
    s->u_reg_u_prediv_ch0_d = 0;
    s->u_reg_u_prediv_ch0_rst_ni = s->u_reg_u_prediv_ch0_rst_ni;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_we = s->u_reg_u_prediv_ch0_we;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wd = s->u_reg_u_prediv_ch0_wd;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_de = s->u_reg_u_prediv_ch0_de;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_d = s->u_reg_u_prediv_ch0_d;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wd = s->u_reg_u_prediv_ch0_wr_en_data_arb_wd;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_d = s->u_reg_u_prediv_ch0_wr_en_data_arb_d;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_prediv_ch0_wr_en_data_arb_we) | (s->u_reg_u_prediv_ch0_wr_en_data_arb_de));
    s->u_reg_u_prediv_ch0_wr_en = s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_prediv_ch0_wr_en_data_arb_we) ? (s->u_reg_u_prediv_ch0_wr_en_data_arb_wd) : (s->u_reg_u_prediv_ch0_wr_en_data_arb_d));
    s->u_reg_u_prediv_ch0_wr_data = s->u_reg_u_prediv_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_prediv_ch0_qe = s->u_reg_u_prediv_ch0_we;
    s->u_reg_u_prediv_ch0_q = s->u_reg_u_prediv_ch0_q;
    s->u_reg_reg2hw_prediv_ch0_q = s->u_reg_u_prediv_ch0_q;
    s->u_reg_u_prediv_ch0_qs = s->u_reg_u_prediv_ch0_q;
    s->u_reg_u_prediv_ch0_wr_en_data_arb_q = s->u_reg_u_prediv_ch0_q;
    s->u_reg_u_prediv_ch0_qs = s->u_reg_u_prediv_ch0_qs;
    s->u_reg_prediv_ch0_qs = s->u_reg_u_prediv_ch0_qs;
    s->u_reg_u_prediv_ch0_ds = ((s->u_reg_u_prediv_ch0_wr_en) ? (s->u_reg_u_prediv_ch0_wr_data) : (s->u_reg_u_prediv_ch0_qs));
    s->u_reg_u_prediv_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_prediv_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_prediv_ch1_we = s->u_reg_prediv_ch1_we;
    s->u_reg_u_prediv_ch1_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_prediv_ch1_de = 0;
    s->u_reg_u_prediv_ch1_d = 0;
    s->u_reg_u_prediv_ch1_rst_ni = s->u_reg_u_prediv_ch1_rst_ni;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_we = s->u_reg_u_prediv_ch1_we;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wd = s->u_reg_u_prediv_ch1_wd;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_de = s->u_reg_u_prediv_ch1_de;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_d = s->u_reg_u_prediv_ch1_d;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wd = s->u_reg_u_prediv_ch1_wr_en_data_arb_wd;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_d = s->u_reg_u_prediv_ch1_wr_en_data_arb_d;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_prediv_ch1_wr_en_data_arb_we) | (s->u_reg_u_prediv_ch1_wr_en_data_arb_de));
    s->u_reg_u_prediv_ch1_wr_en = s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_prediv_ch1_wr_en_data_arb_we) ? (s->u_reg_u_prediv_ch1_wr_en_data_arb_wd) : (s->u_reg_u_prediv_ch1_wr_en_data_arb_d));
    s->u_reg_u_prediv_ch1_wr_data = s->u_reg_u_prediv_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_prediv_ch1_qe = s->u_reg_u_prediv_ch1_we;
    s->u_reg_u_prediv_ch1_q = s->u_reg_u_prediv_ch1_q;
    s->u_reg_reg2hw_prediv_ch1_q = s->u_reg_u_prediv_ch1_q;
    s->u_reg_u_prediv_ch1_qs = s->u_reg_u_prediv_ch1_q;
    s->u_reg_u_prediv_ch1_wr_en_data_arb_q = s->u_reg_u_prediv_ch1_q;
    s->u_reg_u_prediv_ch1_qs = s->u_reg_u_prediv_ch1_qs;
    s->u_reg_prediv_ch1_qs = s->u_reg_u_prediv_ch1_qs;
    s->u_reg_u_prediv_ch1_ds = ((s->u_reg_u_prediv_ch1_wr_en) ? (s->u_reg_u_prediv_ch1_wr_data) : (s->u_reg_u_prediv_ch1_qs));
    s->u_reg_u_data_ch0_0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_data_ch0_0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_data_ch0_0_we = s->u_reg_data_ch0_0_we;
    s->u_reg_u_data_ch0_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch0_0_de = 0;
    s->u_reg_u_data_ch0_0_d = 0;
    s->u_reg_u_data_ch0_0_rst_ni = s->u_reg_u_data_ch0_0_rst_ni;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_we = s->u_reg_u_data_ch0_0_we;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wd = s->u_reg_u_data_ch0_0_wd;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_de = s->u_reg_u_data_ch0_0_de;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_d = s->u_reg_u_data_ch0_0_d;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wd = s->u_reg_u_data_ch0_0_wr_en_data_arb_wd;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_d = s->u_reg_u_data_ch0_0_wr_en_data_arb_d;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_en = ((s->u_reg_u_data_ch0_0_wr_en_data_arb_we) | (s->u_reg_u_data_ch0_0_wr_en_data_arb_de));
    s->u_reg_u_data_ch0_0_wr_en = s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_en;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch0_0_wr_en_data_arb_we) ? (s->u_reg_u_data_ch0_0_wr_en_data_arb_wd) : (s->u_reg_u_data_ch0_0_wr_en_data_arb_d));
    s->u_reg_u_data_ch0_0_wr_data = s->u_reg_u_data_ch0_0_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch0_0_qe = s->u_reg_u_data_ch0_0_we;
    s->u_reg_u_data_ch0_0_q = s->u_reg_u_data_ch0_0_q;
    s->u_reg_reg2hw_data_ch0_0__q = s->u_reg_u_data_ch0_0_q;
    s->u_reg_u_data_ch0_0_qs = s->u_reg_u_data_ch0_0_q;
    s->u_reg_u_data_ch0_0_wr_en_data_arb_q = s->u_reg_u_data_ch0_0_q;
    s->u_reg_u_data_ch0_0_qs = s->u_reg_u_data_ch0_0_qs;
    s->u_reg_data_ch0_0_qs = s->u_reg_u_data_ch0_0_qs;
    s->u_reg_u_data_ch0_0_ds = ((s->u_reg_u_data_ch0_0_wr_en) ? (s->u_reg_u_data_ch0_0_wr_data) : (s->u_reg_u_data_ch0_0_qs));
    s->u_reg_u_data_ch0_1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_data_ch0_1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_data_ch0_1_we = s->u_reg_data_ch0_1_we;
    s->u_reg_u_data_ch0_1_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch0_1_de = 0;
    s->u_reg_u_data_ch0_1_d = 0;
    s->u_reg_u_data_ch0_1_rst_ni = s->u_reg_u_data_ch0_1_rst_ni;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_we = s->u_reg_u_data_ch0_1_we;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wd = s->u_reg_u_data_ch0_1_wd;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_de = s->u_reg_u_data_ch0_1_de;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_d = s->u_reg_u_data_ch0_1_d;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wd = s->u_reg_u_data_ch0_1_wr_en_data_arb_wd;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_d = s->u_reg_u_data_ch0_1_wr_en_data_arb_d;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_en = ((s->u_reg_u_data_ch0_1_wr_en_data_arb_we) | (s->u_reg_u_data_ch0_1_wr_en_data_arb_de));
    s->u_reg_u_data_ch0_1_wr_en = s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_en;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch0_1_wr_en_data_arb_we) ? (s->u_reg_u_data_ch0_1_wr_en_data_arb_wd) : (s->u_reg_u_data_ch0_1_wr_en_data_arb_d));
    s->u_reg_u_data_ch0_1_wr_data = s->u_reg_u_data_ch0_1_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch0_1_qe = s->u_reg_u_data_ch0_1_we;
    s->u_reg_u_data_ch0_1_q = s->u_reg_u_data_ch0_1_q;
    s->u_reg_reg2hw_data_ch0_1__q = s->u_reg_u_data_ch0_1_q;
    s->u_reg_u_data_ch0_1_qs = s->u_reg_u_data_ch0_1_q;
    s->u_reg_u_data_ch0_1_wr_en_data_arb_q = s->u_reg_u_data_ch0_1_q;
    s->u_reg_u_data_ch0_1_qs = s->u_reg_u_data_ch0_1_qs;
    s->u_reg_data_ch0_1_qs = s->u_reg_u_data_ch0_1_qs;
    s->u_reg_u_data_ch0_1_ds = ((s->u_reg_u_data_ch0_1_wr_en) ? (s->u_reg_u_data_ch0_1_wr_data) : (s->u_reg_u_data_ch0_1_qs));
    s->u_reg_u_data_ch1_0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_data_ch1_0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_data_ch1_0_we = s->u_reg_data_ch1_0_we;
    s->u_reg_u_data_ch1_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch1_0_de = 0;
    s->u_reg_u_data_ch1_0_d = 0;
    s->u_reg_u_data_ch1_0_rst_ni = s->u_reg_u_data_ch1_0_rst_ni;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_we = s->u_reg_u_data_ch1_0_we;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wd = s->u_reg_u_data_ch1_0_wd;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_de = s->u_reg_u_data_ch1_0_de;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_d = s->u_reg_u_data_ch1_0_d;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wd = s->u_reg_u_data_ch1_0_wr_en_data_arb_wd;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_d = s->u_reg_u_data_ch1_0_wr_en_data_arb_d;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_en = ((s->u_reg_u_data_ch1_0_wr_en_data_arb_we) | (s->u_reg_u_data_ch1_0_wr_en_data_arb_de));
    s->u_reg_u_data_ch1_0_wr_en = s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_en;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch1_0_wr_en_data_arb_we) ? (s->u_reg_u_data_ch1_0_wr_en_data_arb_wd) : (s->u_reg_u_data_ch1_0_wr_en_data_arb_d));
    s->u_reg_u_data_ch1_0_wr_data = s->u_reg_u_data_ch1_0_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch1_0_qe = s->u_reg_u_data_ch1_0_we;
    s->u_reg_u_data_ch1_0_q = s->u_reg_u_data_ch1_0_q;
    s->u_reg_reg2hw_data_ch1_0__q = s->u_reg_u_data_ch1_0_q;
    s->u_reg_u_data_ch1_0_qs = s->u_reg_u_data_ch1_0_q;
    s->u_reg_u_data_ch1_0_wr_en_data_arb_q = s->u_reg_u_data_ch1_0_q;
    s->u_reg_u_data_ch1_0_qs = s->u_reg_u_data_ch1_0_qs;
    s->u_reg_data_ch1_0_qs = s->u_reg_u_data_ch1_0_qs;
    s->u_reg_u_data_ch1_0_ds = ((s->u_reg_u_data_ch1_0_wr_en) ? (s->u_reg_u_data_ch1_0_wr_data) : (s->u_reg_u_data_ch1_0_qs));
    s->u_reg_u_data_ch1_1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_data_ch1_1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_data_ch1_1_we = s->u_reg_data_ch1_1_we;
    s->u_reg_u_data_ch1_1_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_data_ch1_1_de = 0;
    s->u_reg_u_data_ch1_1_d = 0;
    s->u_reg_u_data_ch1_1_rst_ni = s->u_reg_u_data_ch1_1_rst_ni;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_we = s->u_reg_u_data_ch1_1_we;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wd = s->u_reg_u_data_ch1_1_wd;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_de = s->u_reg_u_data_ch1_1_de;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_d = s->u_reg_u_data_ch1_1_d;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wd = s->u_reg_u_data_ch1_1_wr_en_data_arb_wd;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_d = s->u_reg_u_data_ch1_1_wr_en_data_arb_d;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_en = ((s->u_reg_u_data_ch1_1_wr_en_data_arb_we) | (s->u_reg_u_data_ch1_1_wr_en_data_arb_de));
    s->u_reg_u_data_ch1_1_wr_en = s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_en;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_data = ((s->u_reg_u_data_ch1_1_wr_en_data_arb_we) ? (s->u_reg_u_data_ch1_1_wr_en_data_arb_wd) : (s->u_reg_u_data_ch1_1_wr_en_data_arb_d));
    s->u_reg_u_data_ch1_1_wr_data = s->u_reg_u_data_ch1_1_wr_en_data_arb_wr_data;
    s->u_reg_u_data_ch1_1_qe = s->u_reg_u_data_ch1_1_we;
    s->u_reg_u_data_ch1_1_q = s->u_reg_u_data_ch1_1_q;
    s->u_reg_reg2hw_data_ch1_1__q = s->u_reg_u_data_ch1_1_q;
    s->u_reg_u_data_ch1_1_qs = s->u_reg_u_data_ch1_1_q;
    s->u_reg_u_data_ch1_1_wr_en_data_arb_q = s->u_reg_u_data_ch1_1_q;
    s->u_reg_u_data_ch1_1_qs = s->u_reg_u_data_ch1_1_qs;
    s->u_reg_data_ch1_1_qs = s->u_reg_u_data_ch1_1_qs;
    s->u_reg_u_data_ch1_1_ds = ((s->u_reg_u_data_ch1_1_wr_en) ? (s->u_reg_u_data_ch1_1_wr_data) : (s->u_reg_u_data_ch1_1_qs));
    s->u_reg_u_size_len_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_size_len_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_size_len_ch0_we = s->u_reg_size_we;
    s->u_reg_u_size_len_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x3F);
    s->u_reg_u_size_len_ch0_de = 0;
    s->u_reg_u_size_len_ch0_d = 0;
    s->u_reg_u_size_len_ch0_rst_ni = s->u_reg_u_size_len_ch0_rst_ni;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_we = s->u_reg_u_size_len_ch0_we;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wd = s->u_reg_u_size_len_ch0_wd;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_de = s->u_reg_u_size_len_ch0_de;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_d = s->u_reg_u_size_len_ch0_d;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wd = s->u_reg_u_size_len_ch0_wr_en_data_arb_wd;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_d = s->u_reg_u_size_len_ch0_wr_en_data_arb_d;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_size_len_ch0_wr_en_data_arb_we) | (s->u_reg_u_size_len_ch0_wr_en_data_arb_de));
    s->u_reg_u_size_len_ch0_wr_en = s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_size_len_ch0_wr_en_data_arb_we) ? (s->u_reg_u_size_len_ch0_wr_en_data_arb_wd) : (s->u_reg_u_size_len_ch0_wr_en_data_arb_d));
    s->u_reg_u_size_len_ch0_wr_data = s->u_reg_u_size_len_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_size_len_ch0_qe = s->u_reg_u_size_len_ch0_we;
    s->u_reg_u_size_len_ch0_q = s->u_reg_u_size_len_ch0_q;
    s->u_reg_reg2hw_size_len_ch0_q = s->u_reg_u_size_len_ch0_q;
    s->u_reg_u_size_len_ch0_qs = s->u_reg_u_size_len_ch0_q;
    s->u_reg_u_size_len_ch0_wr_en_data_arb_q = s->u_reg_u_size_len_ch0_q;
    s->u_reg_u_size_len_ch0_qs = s->u_reg_u_size_len_ch0_qs;
    s->u_reg_size_len_ch0_qs = s->u_reg_u_size_len_ch0_qs;
    s->u_reg_u_size_len_ch0_ds = ((s->u_reg_u_size_len_ch0_wr_en) ? (s->u_reg_u_size_len_ch0_wr_data) : (s->u_reg_u_size_len_ch0_qs));
    s->u_reg_u_size_reps_ch0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_size_reps_ch0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_size_reps_ch0_we = s->u_reg_size_we;
    s->u_reg_u_size_reps_ch0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 6) & 0x3FF);
    s->u_reg_u_size_reps_ch0_de = 0;
    s->u_reg_u_size_reps_ch0_d = 0;
    s->u_reg_u_size_reps_ch0_rst_ni = s->u_reg_u_size_reps_ch0_rst_ni;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_we = s->u_reg_u_size_reps_ch0_we;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd = s->u_reg_u_size_reps_ch0_wd;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_de = s->u_reg_u_size_reps_ch0_de;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_d = s->u_reg_u_size_reps_ch0_d;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd = s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_d = s->u_reg_u_size_reps_ch0_wr_en_data_arb_d;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_size_reps_ch0_wr_en_data_arb_we) | (s->u_reg_u_size_reps_ch0_wr_en_data_arb_de));
    s->u_reg_u_size_reps_ch0_wr_en = s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_data = ((s->u_reg_u_size_reps_ch0_wr_en_data_arb_we) ? (s->u_reg_u_size_reps_ch0_wr_en_data_arb_wd) : (s->u_reg_u_size_reps_ch0_wr_en_data_arb_d));
    s->u_reg_u_size_reps_ch0_wr_data = s->u_reg_u_size_reps_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_size_reps_ch0_qe = s->u_reg_u_size_reps_ch0_we;
    s->u_reg_u_size_reps_ch0_q = s->u_reg_u_size_reps_ch0_q;
    s->u_reg_reg2hw_size_reps_ch0_q = s->u_reg_u_size_reps_ch0_q;
    s->u_reg_u_size_reps_ch0_qs = s->u_reg_u_size_reps_ch0_q;
    s->u_reg_u_size_reps_ch0_wr_en_data_arb_q = s->u_reg_u_size_reps_ch0_q;
    s->u_reg_u_size_reps_ch0_qs = s->u_reg_u_size_reps_ch0_qs;
    s->u_reg_size_reps_ch0_qs = s->u_reg_u_size_reps_ch0_qs;
    s->u_reg_u_size_reps_ch0_ds = ((s->u_reg_u_size_reps_ch0_wr_en) ? (s->u_reg_u_size_reps_ch0_wr_data) : (s->u_reg_u_size_reps_ch0_qs));
    s->u_reg_u_size_len_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_size_len_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_size_len_ch1_we = s->u_reg_size_we;
    s->u_reg_u_size_len_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 16) & 0x3F);
    s->u_reg_u_size_len_ch1_de = 0;
    s->u_reg_u_size_len_ch1_d = 0;
    s->u_reg_u_size_len_ch1_rst_ni = s->u_reg_u_size_len_ch1_rst_ni;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_we = s->u_reg_u_size_len_ch1_we;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wd = s->u_reg_u_size_len_ch1_wd;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_de = s->u_reg_u_size_len_ch1_de;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_d = s->u_reg_u_size_len_ch1_d;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wd = s->u_reg_u_size_len_ch1_wr_en_data_arb_wd;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_d = s->u_reg_u_size_len_ch1_wr_en_data_arb_d;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_size_len_ch1_wr_en_data_arb_we) | (s->u_reg_u_size_len_ch1_wr_en_data_arb_de));
    s->u_reg_u_size_len_ch1_wr_en = s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_size_len_ch1_wr_en_data_arb_we) ? (s->u_reg_u_size_len_ch1_wr_en_data_arb_wd) : (s->u_reg_u_size_len_ch1_wr_en_data_arb_d));
    s->u_reg_u_size_len_ch1_wr_data = s->u_reg_u_size_len_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_size_len_ch1_qe = s->u_reg_u_size_len_ch1_we;
    s->u_reg_u_size_len_ch1_q = s->u_reg_u_size_len_ch1_q;
    s->u_reg_reg2hw_size_len_ch1_q = s->u_reg_u_size_len_ch1_q;
    s->u_reg_u_size_len_ch1_qs = s->u_reg_u_size_len_ch1_q;
    s->u_reg_u_size_len_ch1_wr_en_data_arb_q = s->u_reg_u_size_len_ch1_q;
    s->u_reg_u_size_len_ch1_qs = s->u_reg_u_size_len_ch1_qs;
    s->u_reg_size_len_ch1_qs = s->u_reg_u_size_len_ch1_qs;
    s->u_reg_u_size_len_ch1_ds = ((s->u_reg_u_size_len_ch1_wr_en) ? (s->u_reg_u_size_len_ch1_wr_data) : (s->u_reg_u_size_len_ch1_qs));
    s->u_reg_u_size_reps_ch1_clk_i = s->u_reg_clk_i;
    s->u_reg_u_size_reps_ch1_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_size_reps_ch1_we = s->u_reg_size_we;
    s->u_reg_u_size_reps_ch1_wd = (((s->u_reg_u_reg_if_wdata_o) >> 22) & 0x3FF);
    s->u_reg_u_size_reps_ch1_de = 0;
    s->u_reg_u_size_reps_ch1_d = 0;
    s->u_reg_u_size_reps_ch1_rst_ni = s->u_reg_u_size_reps_ch1_rst_ni;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_we = s->u_reg_u_size_reps_ch1_we;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd = s->u_reg_u_size_reps_ch1_wd;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_de = s->u_reg_u_size_reps_ch1_de;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_d = s->u_reg_u_size_reps_ch1_d;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd = s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_d = s->u_reg_u_size_reps_ch1_wr_en_data_arb_d;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_size_reps_ch1_wr_en_data_arb_we) | (s->u_reg_u_size_reps_ch1_wr_en_data_arb_de));
    s->u_reg_u_size_reps_ch1_wr_en = s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_data = ((s->u_reg_u_size_reps_ch1_wr_en_data_arb_we) ? (s->u_reg_u_size_reps_ch1_wr_en_data_arb_wd) : (s->u_reg_u_size_reps_ch1_wr_en_data_arb_d));
    s->u_reg_u_size_reps_ch1_wr_data = s->u_reg_u_size_reps_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_size_reps_ch1_qe = s->u_reg_u_size_reps_ch1_we;
    s->u_reg_u_size_reps_ch1_q = s->u_reg_u_size_reps_ch1_q;
    s->u_reg_reg2hw_size_reps_ch1_q = s->u_reg_u_size_reps_ch1_q;
    s->u_reg_u_size_reps_ch1_qs = s->u_reg_u_size_reps_ch1_q;
    s->u_reg_u_size_reps_ch1_wr_en_data_arb_q = s->u_reg_u_size_reps_ch1_q;
    s->u_reg_u_size_reps_ch1_qs = s->u_reg_u_size_reps_ch1_qs;
    s->u_reg_size_reps_ch1_qs = s->u_reg_u_size_reps_ch1_qs;
    s->u_reg_u_size_reps_ch1_ds = ((s->u_reg_u_size_reps_ch1_wr_en) ? (s->u_reg_u_size_reps_ch1_wr_data) : (s->u_reg_u_size_reps_ch1_qs));
    s->u_reg_tl_o_d_valid = s->u_reg_u_rsp_intg_gen_tl_o_d_valid;
    s->u_reg_tl_o_d_opcode = s->u_reg_u_rsp_intg_gen_tl_o_d_opcode;
    s->u_reg_tl_o_d_param = s->u_reg_u_rsp_intg_gen_tl_o_d_param;
    s->u_reg_tl_o_d_size = s->u_reg_u_rsp_intg_gen_tl_o_d_size;
    s->u_reg_tl_o_d_source = s->u_reg_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_tl_o_d_sink = s->u_reg_u_rsp_intg_gen_tl_o_d_sink;
    s->u_reg_tl_o_d_data = s->u_reg_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_tl_o_d_user_rsp_intg = s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    s->u_reg_tl_o_d_user_data_intg = s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg;
    s->u_reg_tl_o_d_error = s->u_reg_u_rsp_intg_gen_tl_o_d_error;
    s->u_reg_tl_o_a_ready = s->u_reg_u_rsp_intg_gen_tl_o_a_ready;
    s->u_reg_intg_err_o = ((s->u_reg_err_q) | (s->u_reg_intg_err) | (s->u_reg_reg_we_err));
    s->alerts = s->u_reg_intg_err_o;
    s->gen_alert_tx_0_u_prim_alert_sender_clk_i = s->clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_rst_ni = s->rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_i = ((s->u_reg_reg2hw_alert_test_q) & (s->u_reg_reg2hw_alert_test_qe));
    s->gen_alert_tx_0_u_prim_alert_sender_alert_req_i = s->alerts;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_p = s->alert_rx_i_0__ping_p;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_n = s->alert_rx_i_0__ping_n;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_p = s->alert_rx_i_0__ack_p;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_n = s->alert_rx_i_0__ack_n;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger = (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_i) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_d = (((s->gen_alert_tx_0_u_prim_alert_sender_alert_clr) ^ 1)) & (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger);
    s->gen_alert_tx_0_u_prim_alert_sender_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_in_i = ((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_n)) << 1));
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_pi = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o) >> 0) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_ni = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o) >> 1) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_pi;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_p_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_n_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_event_o = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o));
    s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger = (s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_event_o);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_set_d = (((s->gen_alert_tx_0_u_prim_alert_sender_ping_clr) ^ 1)) & (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_in_i = ((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_n)) << 1));
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_pi = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o) >> 0) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_ni = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o) >> 1) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_pi;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_p_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_n_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d;
    s->gen_alert_tx_0_u_prim_alert_sender_ack_level = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_event_o = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o));
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o;
    s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_in_i = s->gen_alert_tx_0_u_prim_alert_sender_alert_req_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_set_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger = ((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q)) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_d_i = ((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_pd)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_nd)) << 1));
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p = ((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o) & 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o) >> 1) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_ack_o = ((s->gen_alert_tx_0_u_prim_alert_sender_alert_clr) & (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q));
    s->gen_alert_tx_0_u_prim_alert_sender_alert_state_o = s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p;
    s->alert_tx_o_0__alert_p = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n;
    s->alert_tx_o_0__alert_n = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n;
    s->u_pattgen_core_clk_i = s->clk_i;
    s->u_pattgen_core_rst_ni = s->rst_ni;
    s->u_pattgen_core_reg2hw_intr_state_done_ch1_q = s->u_reg_reg2hw_intr_state_done_ch1_q;
    s->u_pattgen_core_reg2hw_intr_state_done_ch0_q = s->u_reg_reg2hw_intr_state_done_ch0_q;
    s->u_pattgen_core_reg2hw_intr_enable_done_ch1_q = s->u_reg_reg2hw_intr_enable_done_ch1_q;
    s->u_pattgen_core_reg2hw_intr_enable_done_ch0_q = s->u_reg_reg2hw_intr_enable_done_ch0_q;
    s->u_pattgen_core_reg2hw_intr_test_done_ch1_q = s->u_reg_reg2hw_intr_test_done_ch1_q;
    s->u_pattgen_core_reg2hw_intr_test_done_ch1_qe = s->u_reg_reg2hw_intr_test_done_ch1_qe;
    s->u_pattgen_core_reg2hw_intr_test_done_ch0_q = s->u_reg_reg2hw_intr_test_done_ch0_q;
    s->u_pattgen_core_reg2hw_intr_test_done_ch0_qe = s->u_reg_reg2hw_intr_test_done_ch0_qe;
    s->u_pattgen_core_reg2hw_alert_test_q = s->u_reg_reg2hw_alert_test_q;
    s->u_pattgen_core_reg2hw_alert_test_qe = s->u_reg_reg2hw_alert_test_qe;
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch1_q = s->u_reg_reg2hw_ctrl_inactive_level_pda_ch1_q;
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch1_q = s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch1_q;
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch0_q = s->u_reg_reg2hw_ctrl_inactive_level_pda_ch0_q;
    s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch0_q = s->u_reg_reg2hw_ctrl_inactive_level_pcl_ch0_q;
    s->u_pattgen_core_reg2hw_ctrl_polarity_ch1_q = s->u_reg_reg2hw_ctrl_polarity_ch1_q;
    s->u_pattgen_core_reg2hw_ctrl_polarity_ch0_q = s->u_reg_reg2hw_ctrl_polarity_ch0_q;
    s->u_pattgen_core_reg2hw_ctrl_enable_ch1_q = s->u_reg_reg2hw_ctrl_enable_ch1_q;
    s->u_pattgen_core_reg2hw_ctrl_enable_ch0_q = s->u_reg_reg2hw_ctrl_enable_ch0_q;
    s->u_pattgen_core_reg2hw_prediv_ch0_q = s->u_reg_reg2hw_prediv_ch0_q;
    s->u_pattgen_core_reg2hw_prediv_ch1_q = s->u_reg_reg2hw_prediv_ch1_q;
    s->u_pattgen_core_reg2hw_data_ch0_0__q = s->u_reg_reg2hw_data_ch0_0__q;
    s->u_pattgen_core_reg2hw_data_ch0_1__q = s->u_reg_reg2hw_data_ch0_1__q;
    s->u_pattgen_core_reg2hw_data_ch1_0__q = s->u_reg_reg2hw_data_ch1_0__q;
    s->u_pattgen_core_reg2hw_data_ch1_1__q = s->u_reg_reg2hw_data_ch1_1__q;
    s->u_pattgen_core_reg2hw_size_reps_ch1_q = s->u_reg_reg2hw_size_reps_ch1_q;
    s->u_pattgen_core_reg2hw_size_len_ch1_q = s->u_reg_reg2hw_size_len_ch1_q;
    s->u_pattgen_core_reg2hw_size_reps_ch0_q = s->u_reg_reg2hw_size_reps_ch0_q;
    s->u_pattgen_core_reg2hw_size_len_ch0_q = s->u_reg_reg2hw_size_len_ch0_q;
    s->u_pattgen_core_ch0_ctrl_enable = s->u_pattgen_core_reg2hw_ctrl_enable_ch0_q;
    s->u_pattgen_core_ch0_ctrl_polarity = s->u_pattgen_core_reg2hw_ctrl_polarity_ch0_q;
    s->u_pattgen_core_ch0_ctrl_inactive_level_pcl = s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch0_q;
    s->u_pattgen_core_ch0_ctrl_inactive_level_pda = s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch0_q;
    s->u_pattgen_core_ch0_ctrl_data = (s->u_pattgen_core_ch0_ctrl_data & ~0xFFFFFFFF00000000ULL) | (((s->u_pattgen_core_reg2hw_data_ch0_1__q) & 0xFFFFFFFFULL) << 32);
    s->u_pattgen_core_ch0_ctrl_data = (s->u_pattgen_core_ch0_ctrl_data & ~0xFFFFFFFFULL) | (((s->u_pattgen_core_reg2hw_data_ch0_0__q) & 0xFFFFFFFFULL) << 0);
    s->u_pattgen_core_ch0_ctrl_prediv = s->u_pattgen_core_reg2hw_prediv_ch0_q;
    s->u_pattgen_core_ch0_ctrl_len = s->u_pattgen_core_reg2hw_size_len_ch0_q;
    s->u_pattgen_core_ch0_ctrl_reps = s->u_pattgen_core_reg2hw_size_reps_ch0_q;
    s->u_pattgen_core_ch1_ctrl_enable = s->u_pattgen_core_reg2hw_ctrl_enable_ch1_q;
    s->u_pattgen_core_ch1_ctrl_polarity = s->u_pattgen_core_reg2hw_ctrl_polarity_ch1_q;
    s->u_pattgen_core_ch1_ctrl_inactive_level_pcl = s->u_pattgen_core_reg2hw_ctrl_inactive_level_pcl_ch1_q;
    s->u_pattgen_core_ch1_ctrl_inactive_level_pda = s->u_pattgen_core_reg2hw_ctrl_inactive_level_pda_ch1_q;
    s->u_pattgen_core_ch1_ctrl_data = (s->u_pattgen_core_ch1_ctrl_data & ~0xFFFFFFFF00000000ULL) | (((s->u_pattgen_core_reg2hw_data_ch1_1__q) & 0xFFFFFFFFULL) << 32);
    s->u_pattgen_core_ch1_ctrl_data = (s->u_pattgen_core_ch1_ctrl_data & ~0xFFFFFFFFULL) | (((s->u_pattgen_core_reg2hw_data_ch1_0__q) & 0xFFFFFFFFULL) << 0);
    s->u_pattgen_core_ch1_ctrl_prediv = s->u_pattgen_core_reg2hw_prediv_ch1_q;
    s->u_pattgen_core_ch1_ctrl_len = s->u_pattgen_core_reg2hw_size_len_ch1_q;
    s->u_pattgen_core_ch1_ctrl_reps = s->u_pattgen_core_reg2hw_size_reps_ch1_q;
    s->u_pattgen_core_chan0_clk_i = s->u_pattgen_core_clk_i;
    s->u_pattgen_core_chan0_rst_ni = s->u_pattgen_core_rst_ni;
    s->u_pattgen_core_chan0_ctrl_i_enable = s->u_pattgen_core_ch0_ctrl_enable;
    s->u_pattgen_core_chan0_ctrl_i_polarity = s->u_pattgen_core_ch0_ctrl_polarity;
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl = s->u_pattgen_core_ch0_ctrl_inactive_level_pcl;
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda = s->u_pattgen_core_ch0_ctrl_inactive_level_pda;
    s->u_pattgen_core_chan0_ctrl_i_prediv = s->u_pattgen_core_ch0_ctrl_prediv;
    s->u_pattgen_core_chan0_ctrl_i_data = s->u_pattgen_core_ch0_ctrl_data;
    s->u_pattgen_core_chan0_ctrl_i_len = s->u_pattgen_core_ch0_ctrl_len;
    s->u_pattgen_core_chan0_ctrl_i_reps = s->u_pattgen_core_ch0_ctrl_reps;
    s->u_pattgen_core_chan0_clk_en = ((s->u_pattgen_core_chan0_complete_q) ^ 1);
    s->u_pattgen_core_chan0_prediv_clk_rollover = ((s->u_pattgen_core_chan0_clk_cnt_q) == (s->u_pattgen_core_chan0_prediv_q));
    s->u_pattgen_core_chan0_rst_ni = s->u_pattgen_core_chan0_rst_ni;
    s->u_pattgen_core_chan0_ctrl_i_enable = s->u_pattgen_core_chan0_ctrl_i_enable;
    s->u_pattgen_core_chan0_enable = s->u_pattgen_core_chan0_ctrl_i_enable;
    s->u_pattgen_core_chan0_clk_cnt_d = (((((s->u_pattgen_core_chan0_enable) ^ 1)) | (s->u_pattgen_core_chan0_prediv_clk_rollover)) ? (0) : ((s->u_pattgen_core_chan0_clk_cnt_q) + (1)));
    s->u_pattgen_core_chan0_pcl_int_d = (s->u_pattgen_core_chan0_enable) & ((s->u_pattgen_core_chan0_prediv_clk_rollover) ^ (s->u_pattgen_core_chan0_pcl_int_q));
    s->u_pattgen_core_chan0_bit_cnt_en = ((s->u_pattgen_core_chan0_pcl_int_q) & (s->u_pattgen_core_chan0_prediv_clk_rollover)) | (((s->u_pattgen_core_chan0_enable) ^ 1));
    s->u_pattgen_core_chan0_bit_cnt_d = (((((s->u_pattgen_core_chan0_enable) ^ 1)) | (((s->u_pattgen_core_chan0_bit_cnt_q) == (s->u_pattgen_core_chan0_len_q)))) ? (0) : ((s->u_pattgen_core_chan0_bit_cnt_q) + (1)));
    s->u_pattgen_core_chan0_rep_cnt_en = ((s->u_pattgen_core_chan0_bit_cnt_en) & (((s->u_pattgen_core_chan0_bit_cnt_q) == (s->u_pattgen_core_chan0_len_q)))) | (((s->u_pattgen_core_chan0_enable) ^ 1));
    s->u_pattgen_core_chan0_rep_cnt_d = (((((s->u_pattgen_core_chan0_enable) ^ 1)) | (((s->u_pattgen_core_chan0_rep_cnt_q) == (s->u_pattgen_core_chan0_reps_q)))) ? (0) : ((s->u_pattgen_core_chan0_rep_cnt_q) + (1)));
    s->u_pattgen_core_chan0_complete_en = ((s->u_pattgen_core_chan0_rep_cnt_en) & (((s->u_pattgen_core_chan0_rep_cnt_q) == (s->u_pattgen_core_chan0_reps_q)))) | (((s->u_pattgen_core_chan0_enable) ^ 1));
    s->u_pattgen_core_chan0_complete_d = s->u_pattgen_core_chan0_enable;
    s->u_pattgen_core_chan0_active_d = (((s->u_pattgen_core_chan0_complete_q) ^ 1)) & ((s->u_pattgen_core_chan0_enable) | (s->u_pattgen_core_chan0_active_q));
    s->u_pattgen_core_chan0_active = ((s->u_pattgen_core_chan0_enable) ? (s->u_pattgen_core_chan0_active_d) : (s->u_pattgen_core_chan0_active_q));
    s->u_pattgen_core_chan0_ctrl_i_polarity = s->u_pattgen_core_chan0_ctrl_i_polarity;
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl = s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl;
    s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda = s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda;
    s->u_pattgen_core_chan0_ctrl_i_prediv = s->u_pattgen_core_chan0_ctrl_i_prediv;
    s->u_pattgen_core_chan0_ctrl_i_data = s->u_pattgen_core_chan0_ctrl_i_data;
    s->u_pattgen_core_chan0_ctrl_i_len = s->u_pattgen_core_chan0_ctrl_i_len;
    s->u_pattgen_core_chan0_ctrl_i_reps = s->u_pattgen_core_chan0_ctrl_i_reps;
    s->u_pattgen_core_chan0_pda_o = s->u_pattgen_core_chan0_pda_o;
    s->u_pattgen_core_chan0_pcl_o = s->u_pattgen_core_chan0_pcl_o;
    s->u_pattgen_core_chan0_event_done_o = ((s->u_pattgen_core_chan0_complete_q) & (((s->u_pattgen_core_chan0_complete_q2) ^ (1))));
    s->u_pattgen_core_chan1_clk_i = s->u_pattgen_core_clk_i;
    s->u_pattgen_core_chan1_rst_ni = s->u_pattgen_core_rst_ni;
    s->u_pattgen_core_chan1_ctrl_i_enable = s->u_pattgen_core_ch1_ctrl_enable;
    s->u_pattgen_core_chan1_ctrl_i_polarity = s->u_pattgen_core_ch1_ctrl_polarity;
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl = s->u_pattgen_core_ch1_ctrl_inactive_level_pcl;
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda = s->u_pattgen_core_ch1_ctrl_inactive_level_pda;
    s->u_pattgen_core_chan1_ctrl_i_prediv = s->u_pattgen_core_ch1_ctrl_prediv;
    s->u_pattgen_core_chan1_ctrl_i_data = s->u_pattgen_core_ch1_ctrl_data;
    s->u_pattgen_core_chan1_ctrl_i_len = s->u_pattgen_core_ch1_ctrl_len;
    s->u_pattgen_core_chan1_ctrl_i_reps = s->u_pattgen_core_ch1_ctrl_reps;
    s->u_pattgen_core_chan1_clk_en = ((s->u_pattgen_core_chan1_complete_q) ^ 1);
    s->u_pattgen_core_chan1_prediv_clk_rollover = ((s->u_pattgen_core_chan1_clk_cnt_q) == (s->u_pattgen_core_chan1_prediv_q));
    s->u_pattgen_core_chan1_rst_ni = s->u_pattgen_core_chan1_rst_ni;
    s->u_pattgen_core_chan1_ctrl_i_enable = s->u_pattgen_core_chan1_ctrl_i_enable;
    s->u_pattgen_core_chan1_enable = s->u_pattgen_core_chan1_ctrl_i_enable;
    s->u_pattgen_core_chan1_clk_cnt_d = (((((s->u_pattgen_core_chan1_enable) ^ 1)) | (s->u_pattgen_core_chan1_prediv_clk_rollover)) ? (0) : ((s->u_pattgen_core_chan1_clk_cnt_q) + (1)));
    s->u_pattgen_core_chan1_pcl_int_d = (s->u_pattgen_core_chan1_enable) & ((s->u_pattgen_core_chan1_prediv_clk_rollover) ^ (s->u_pattgen_core_chan1_pcl_int_q));
    s->u_pattgen_core_chan1_bit_cnt_en = ((s->u_pattgen_core_chan1_pcl_int_q) & (s->u_pattgen_core_chan1_prediv_clk_rollover)) | (((s->u_pattgen_core_chan1_enable) ^ 1));
    s->u_pattgen_core_chan1_bit_cnt_d = (((((s->u_pattgen_core_chan1_enable) ^ 1)) | (((s->u_pattgen_core_chan1_bit_cnt_q) == (s->u_pattgen_core_chan1_len_q)))) ? (0) : ((s->u_pattgen_core_chan1_bit_cnt_q) + (1)));
    s->u_pattgen_core_chan1_rep_cnt_en = ((s->u_pattgen_core_chan1_bit_cnt_en) & (((s->u_pattgen_core_chan1_bit_cnt_q) == (s->u_pattgen_core_chan1_len_q)))) | (((s->u_pattgen_core_chan1_enable) ^ 1));
    s->u_pattgen_core_chan1_rep_cnt_d = (((((s->u_pattgen_core_chan1_enable) ^ 1)) | (((s->u_pattgen_core_chan1_rep_cnt_q) == (s->u_pattgen_core_chan1_reps_q)))) ? (0) : ((s->u_pattgen_core_chan1_rep_cnt_q) + (1)));
    s->u_pattgen_core_chan1_complete_en = ((s->u_pattgen_core_chan1_rep_cnt_en) & (((s->u_pattgen_core_chan1_rep_cnt_q) == (s->u_pattgen_core_chan1_reps_q)))) | (((s->u_pattgen_core_chan1_enable) ^ 1));
    s->u_pattgen_core_chan1_complete_d = s->u_pattgen_core_chan1_enable;
    s->u_pattgen_core_chan1_active_d = (((s->u_pattgen_core_chan1_complete_q) ^ 1)) & ((s->u_pattgen_core_chan1_enable) | (s->u_pattgen_core_chan1_active_q));
    s->u_pattgen_core_chan1_active = ((s->u_pattgen_core_chan1_enable) ? (s->u_pattgen_core_chan1_active_d) : (s->u_pattgen_core_chan1_active_q));
    s->u_pattgen_core_chan1_ctrl_i_polarity = s->u_pattgen_core_chan1_ctrl_i_polarity;
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl = s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl;
    s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda = s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda;
    s->u_pattgen_core_chan1_ctrl_i_prediv = s->u_pattgen_core_chan1_ctrl_i_prediv;
    s->u_pattgen_core_chan1_ctrl_i_data = s->u_pattgen_core_chan1_ctrl_i_data;
    s->u_pattgen_core_chan1_ctrl_i_len = s->u_pattgen_core_chan1_ctrl_i_len;
    s->u_pattgen_core_chan1_ctrl_i_reps = s->u_pattgen_core_chan1_ctrl_i_reps;
    s->u_pattgen_core_chan1_pda_o = s->u_pattgen_core_chan1_pda_o;
    s->u_pattgen_core_chan1_pcl_o = s->u_pattgen_core_chan1_pcl_o;
    s->u_pattgen_core_chan1_event_done_o = ((s->u_pattgen_core_chan1_complete_q) & (((s->u_pattgen_core_chan1_complete_q2) ^ (1))));
    s->u_pattgen_core_intr_hw_done_ch0_clk_i = s->u_pattgen_core_clk_i;
    s->u_pattgen_core_intr_hw_done_ch0_rst_ni = s->u_pattgen_core_rst_ni;
    s->u_pattgen_core_intr_hw_done_ch0_event_intr_i = s->u_pattgen_core_chan0_event_done_o;
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i = s->u_pattgen_core_reg2hw_intr_enable_done_ch0_q;
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_q_i = s->u_pattgen_core_reg2hw_intr_test_done_ch0_q;
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_qe_i = s->u_pattgen_core_reg2hw_intr_test_done_ch0_qe;
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_state_q_i = s->u_pattgen_core_reg2hw_intr_state_done_ch0_q;
    s->u_pattgen_core_intr_hw_done_ch0_status = s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_state_q_i;
    s->u_pattgen_core_intr_hw_done_ch0_rst_ni = s->u_pattgen_core_intr_hw_done_ch0_rst_ni;
    s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i = s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i;
    s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_de_o = ((((s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch0_event_intr_i));
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_de = s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_de_o;
    s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_d_o = ((((((s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch0_event_intr_i))) | (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_state_q_i));
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_d = s->u_pattgen_core_intr_hw_done_ch0_hw2reg_intr_state_d_o;
    s->u_pattgen_core_intr_hw_done_ch0_intr_o = s->u_pattgen_core_intr_hw_done_ch0_intr_o;
    s->u_pattgen_core_intr_hw_done_ch1_clk_i = s->u_pattgen_core_clk_i;
    s->u_pattgen_core_intr_hw_done_ch1_rst_ni = s->u_pattgen_core_rst_ni;
    s->u_pattgen_core_intr_hw_done_ch1_event_intr_i = s->u_pattgen_core_chan1_event_done_o;
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i = s->u_pattgen_core_reg2hw_intr_enable_done_ch1_q;
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_q_i = s->u_pattgen_core_reg2hw_intr_test_done_ch1_q;
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_qe_i = s->u_pattgen_core_reg2hw_intr_test_done_ch1_qe;
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_state_q_i = s->u_pattgen_core_reg2hw_intr_state_done_ch1_q;
    s->u_pattgen_core_intr_hw_done_ch1_status = s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_state_q_i;
    s->u_pattgen_core_intr_hw_done_ch1_rst_ni = s->u_pattgen_core_intr_hw_done_ch1_rst_ni;
    s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i = s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i;
    s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_de_o = ((((s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch1_event_intr_i));
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_de = s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_de_o;
    s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_d_o = ((((((s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_qe_i) & (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_test_q_i))) | (s->u_pattgen_core_intr_hw_done_ch1_event_intr_i))) | (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_state_q_i));
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_d = s->u_pattgen_core_intr_hw_done_ch1_hw2reg_intr_state_d_o;
    s->u_pattgen_core_intr_hw_done_ch1_intr_o = s->u_pattgen_core_intr_hw_done_ch1_intr_o;
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_d = s->u_pattgen_core_hw2reg_intr_state_done_ch1_d;
    s->u_reg_hw2reg_intr_state_done_ch1_d = s->u_pattgen_core_hw2reg_intr_state_done_ch1_d;
    s->u_reg_u_intr_state_done_ch1_d = s->u_reg_hw2reg_intr_state_done_ch1_d;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d = s->u_reg_u_intr_state_done_ch1_d;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d = s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d;
    s->u_pattgen_core_hw2reg_intr_state_done_ch1_de = s->u_pattgen_core_hw2reg_intr_state_done_ch1_de;
    s->u_reg_hw2reg_intr_state_done_ch1_de = s->u_pattgen_core_hw2reg_intr_state_done_ch1_de;
    s->u_reg_u_intr_state_done_ch1_de = s->u_reg_hw2reg_intr_state_done_ch1_de;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_de = s->u_reg_u_intr_state_done_ch1_de;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_en = ((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_we) | (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_de));
    s->u_reg_u_intr_state_done_ch1_wr_en = s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_en;
    s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_data = ((((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_de) ? (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_d) : (s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_q))) & (((((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_we) ^ (1))) | (((s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wd) ^ (1))))));
    s->u_reg_u_intr_state_done_ch1_wr_data = s->u_reg_u_intr_state_done_ch1_wr_en_data_arb_wr_data;
    s->u_reg_u_intr_state_done_ch1_ds = ((s->u_reg_u_intr_state_done_ch1_wr_en) ? (s->u_reg_u_intr_state_done_ch1_wr_data) : (s->u_reg_u_intr_state_done_ch1_qs));
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_d = s->u_pattgen_core_hw2reg_intr_state_done_ch0_d;
    s->u_reg_hw2reg_intr_state_done_ch0_d = s->u_pattgen_core_hw2reg_intr_state_done_ch0_d;
    s->u_reg_u_intr_state_done_ch0_d = s->u_reg_hw2reg_intr_state_done_ch0_d;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d = s->u_reg_u_intr_state_done_ch0_d;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d = s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d;
    s->u_pattgen_core_hw2reg_intr_state_done_ch0_de = s->u_pattgen_core_hw2reg_intr_state_done_ch0_de;
    s->u_reg_hw2reg_intr_state_done_ch0_de = s->u_pattgen_core_hw2reg_intr_state_done_ch0_de;
    s->u_reg_u_intr_state_done_ch0_de = s->u_reg_hw2reg_intr_state_done_ch0_de;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_de = s->u_reg_u_intr_state_done_ch0_de;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_en = ((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_we) | (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_de));
    s->u_reg_u_intr_state_done_ch0_wr_en = s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_en;
    s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_data = ((((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_de) ? (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_d) : (s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_q))) & (((((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_we) ^ (1))) | (((s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wd) ^ (1))))));
    s->u_reg_u_intr_state_done_ch0_wr_data = s->u_reg_u_intr_state_done_ch0_wr_en_data_arb_wr_data;
    s->u_reg_u_intr_state_done_ch0_ds = ((s->u_reg_u_intr_state_done_ch0_wr_en) ? (s->u_reg_u_intr_state_done_ch0_wr_data) : (s->u_reg_u_intr_state_done_ch0_qs));
    s->u_pattgen_core_pda0_tx_o = s->u_pattgen_core_chan0_pda_o;
    s->u_pattgen_core_pcl0_tx_o = s->u_pattgen_core_chan0_pcl_o;
    s->u_pattgen_core_pda1_tx_o = s->u_pattgen_core_chan1_pda_o;
    s->u_pattgen_core_pcl1_tx_o = s->u_pattgen_core_chan1_pcl_o;
    s->u_pattgen_core_intr_done_ch0_o = s->u_pattgen_core_intr_hw_done_ch0_intr_o;
    s->u_pattgen_core_intr_done_ch1_o = s->u_pattgen_core_intr_hw_done_ch1_intr_o;
    s->cio_pda0_tx_o = s->u_pattgen_core_pda0_tx_o;
    s->cio_pcl0_tx_o = s->u_pattgen_core_pcl0_tx_o;
    s->cio_pda1_tx_o = s->u_pattgen_core_pda1_tx_o;
    s->cio_pcl1_tx_o = s->u_pattgen_core_pcl1_tx_o;
    s->cio_pda0_tx_en_o = 1;
    s->cio_pcl0_tx_en_o = 1;
    s->cio_pda1_tx_en_o = 1;
    s->cio_pcl1_tx_en_o = 1;
    s->intr_done_ch0_o = s->u_pattgen_core_intr_done_ch0_o;
    s->intr_done_ch1_o = s->u_pattgen_core_intr_done_ch1_o;
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
    uint16_t _qp_next_u_reg_addr_hit = s->u_reg_addr_hit;
    uint8_t _qp_next_u_reg_wr_err = s->u_reg_wr_err;
    uint16_t _qp_next_u_reg_reg_we_check = s->u_reg_reg_we_check;
    uint32_t _qp_next_u_reg_reg_rdata_next = s->u_reg_reg_rdata_next;
    uint8_t _qp_next_u_reg_u_chk_u_chk_syndrome_o = s->u_reg_u_chk_u_chk_syndrome_o;
    uint64_t _qp_next_u_reg_u_chk_u_chk_data_o = s->u_reg_u_chk_u_chk_data_o;
    uint8_t _qp_next_u_reg_u_chk_u_chk_err_o = s->u_reg_u_chk_u_chk_err_o;
    uint8_t _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o;
    uint32_t _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o;
    uint8_t _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o = s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_valid = s->u_reg_u_rsp_intg_gen_tl_o_d_valid;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_opcode = s->u_reg_u_rsp_intg_gen_tl_o_d_opcode;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_param = s->u_reg_u_rsp_intg_gen_tl_o_d_param;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_size = s->u_reg_u_rsp_intg_gen_tl_o_d_size;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_rsp_intg_gen_tl_o_d_source;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_sink = s->u_reg_u_rsp_intg_gen_tl_o_d_sink;
    uint32_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_rsp_intg_gen_tl_o_d_data;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_error = s->u_reg_u_rsp_intg_gen_tl_o_d_error;
    uint8_t _qp_next_u_reg_u_rsp_intg_gen_tl_o_a_ready = s->u_reg_u_rsp_intg_gen_tl_o_a_ready;
    uint64_t _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o;
    uint64_t _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o;
    uint8_t _qp_next_u_reg_u_reg_if_outstanding_q = s->u_reg_u_reg_if_outstanding_q;
    uint8_t _qp_next_u_reg_u_reg_if_reqid_q = s->u_reg_u_reg_if_reqid_q;
    uint8_t _qp_next_u_reg_u_reg_if_reqsz_q = s->u_reg_u_reg_if_reqsz_q;
    uint8_t _qp_next_u_reg_u_reg_if_rspop_q = s->u_reg_u_reg_if_rspop_q;
    uint32_t _qp_next_u_reg_u_reg_if_rdata_q = s->u_reg_u_reg_if_rdata_q;
    uint8_t _qp_next_u_reg_u_reg_if_error_q = s->u_reg_u_reg_if_error_q;
    uint8_t _qp_next_u_reg_u_reg_if_addr_align_err = s->u_reg_u_reg_if_addr_align_err;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink;
    uint32_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error;
    uint8_t _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready;
    uint8_t _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk = s->u_reg_u_reg_if_u_err_addr_sz_chk;
    uint8_t _qp_next_u_reg_u_reg_if_u_err_mask_chk = s->u_reg_u_reg_if_u_err_mask_chk;
    uint8_t _qp_next_u_reg_u_reg_if_u_err_fulldata_chk = s->u_reg_u_reg_if_u_err_fulldata_chk;
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
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d = s->gen_alert_tx_0_u_prim_alert_sender_state_d;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd = s->gen_alert_tx_0_u_prim_alert_sender_alert_pd;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd = s->gen_alert_tx_0_u_prim_alert_sender_alert_nd;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr = s->gen_alert_tx_0_u_prim_alert_sender_ping_clr;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr = s->gen_alert_tx_0_u_prim_alert_sender_alert_clr;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q = s->gen_alert_tx_0_u_prim_alert_sender_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q = s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q = s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    uint8_t _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o;
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
    _qp_next_u_reg_err_q = ((((!(((s->u_reg_rst_ni) ^ 1))) && ((s->u_reg_intg_err) | (s->u_reg_reg_we_err))) || (((s->u_reg_rst_ni) ^ 1))) ? ((((!(((s->u_reg_rst_ni) ^ 1))) && ((s->u_reg_intg_err) | (s->u_reg_reg_we_err))) ? (1) : (0))) : _qp_next_u_reg_err_q);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x1ULL) | (((((s->u_reg_reg_addr) == (0))) & 0x1ULL) << 0)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x2ULL) | (((((s->u_reg_reg_addr) == (4))) & 0x1ULL) << 1)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x4ULL) | (((((s->u_reg_reg_addr) == (8))) & 0x1ULL) << 2)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x8ULL) | (((((s->u_reg_reg_addr) == (12))) & 0x1ULL) << 3)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x10ULL) | (((((s->u_reg_reg_addr) == (16))) & 0x1ULL) << 4)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x20ULL) | (((((s->u_reg_reg_addr) == (20))) & 0x1ULL) << 5)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x40ULL) | (((((s->u_reg_reg_addr) == (24))) & 0x1ULL) << 6)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x80ULL) | (((((s->u_reg_reg_addr) == (28))) & 0x1ULL) << 7)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x100ULL) | (((((s->u_reg_reg_addr) == (32))) & 0x1ULL) << 8)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x200ULL) | (((((s->u_reg_reg_addr) == (36))) & 0x1ULL) << 9)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x400ULL) | (((((s->u_reg_reg_addr) == (40))) & 0x1ULL) << 10)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_addr_hit = (((1) || (1)) ? ((_qp_next_u_reg_addr_hit & ~0x800ULL) | (((((s->u_reg_reg_addr) == (44))) & 0x1ULL) << 11)) : _qp_next_u_reg_addr_hit);
    _qp_next_u_reg_wr_err = (((1) || (1)) ? ((s->u_reg_reg_we) & (((((s->u_reg_addr_hit) & 1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 1) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 2) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 3) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 4) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_addr_hit) >> 5) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 6) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 7) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 8) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 9) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 10) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_addr_hit) >> 11) & 0x1)) & (((s->u_reg_reg_be) != (15)))))) : _qp_next_u_reg_wr_err);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x1ULL) | (((s->u_reg_intr_state_we) & 0x1ULL) << 0)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x2ULL) | (((s->u_reg_intr_enable_we) & 0x1ULL) << 1)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x4ULL) | (((s->u_reg_intr_test_we) & 0x1ULL) << 2)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x8ULL) | (((s->u_reg_alert_test_we) & 0x1ULL) << 3)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x10ULL) | (((s->u_reg_ctrl_we) & 0x1ULL) << 4)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x20ULL) | (((s->u_reg_prediv_ch0_we) & 0x1ULL) << 5)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x40ULL) | (((s->u_reg_prediv_ch1_we) & 0x1ULL) << 6)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x80ULL) | (((s->u_reg_data_ch0_0_we) & 0x1ULL) << 7)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x100ULL) | (((s->u_reg_data_ch0_1_we) & 0x1ULL) << 8)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x200ULL) | (((s->u_reg_data_ch1_0_we) & 0x1ULL) << 9)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x400ULL) | (((s->u_reg_data_ch1_1_we) & 0x1ULL) << 10)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_we_check = (((1) || (1)) ? ((_qp_next_u_reg_reg_we_check & ~0x800ULL) | (((s->u_reg_size_we) & 0x1ULL) << 11)) : _qp_next_u_reg_reg_we_check);
    _qp_next_u_reg_reg_rdata_next = (((1) || (1)) ? (0) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) || (((((s->u_reg_addr_hit) & 1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x1ULL) | ((((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) ? (s->u_reg_intr_enable_done_ch0_qs) : (s->u_reg_intr_state_done_ch0_qs))) & 0x1ULL) << 0)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) || (((((s->u_reg_addr_hit) & 1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x2ULL) | ((((((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))) ? (s->u_reg_intr_enable_done_ch1_qs) : (s->u_reg_intr_state_done_ch1_qs))) & 0x1ULL) << 1)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x1ULL) | (((0) & 0x1ULL) << 0)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x2ULL) | (((0) & 0x1ULL) << 1)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = (((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x1ULL) | (((0) & 0x1ULL) << 0)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x1ULL) | (((s->u_reg_ctrl_enable_ch0_qs) & 0x1ULL) << 0)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x2ULL) | (((s->u_reg_ctrl_enable_ch1_qs) & 0x1ULL) << 1)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x4ULL) | (((s->u_reg_ctrl_polarity_ch0_qs) & 0x1ULL) << 2)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x8ULL) | (((s->u_reg_ctrl_polarity_ch1_qs) & 0x1ULL) << 3)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x10ULL) | (((s->u_reg_ctrl_inactive_level_pcl_ch0_qs) & 0x1ULL) << 4)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x20ULL) | (((s->u_reg_ctrl_inactive_level_pda_ch0_qs) & 0x1ULL) << 5)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x40ULL) | (((s->u_reg_ctrl_inactive_level_pcl_ch1_qs) & 0x1ULL) << 6)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x80ULL) | (((s->u_reg_ctrl_inactive_level_pda_ch1_qs) & 0x1ULL) << 7)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = ((((((((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))) || ((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1)))) || ((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1)))) ? ((((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))) ? (s->u_reg_data_ch1_1_qs) : ((((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))) ? (s->u_reg_data_ch1_0_qs) : ((((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))) ? (s->u_reg_data_ch0_1_qs) : ((((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))) ? (s->u_reg_data_ch0_0_qs) : ((((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))) ? (s->u_reg_prediv_ch1_qs) : (s->u_reg_prediv_ch0_qs))))))))))) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = (((((((((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x3FULL) | (((s->u_reg_size_len_ch0_qs) & 0x3FULL) << 0)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = (((((((((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0xFFC0ULL) | (((s->u_reg_size_reps_ch0_qs) & 0x3FFULL) << 6)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = (((((((((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0x3F0000ULL) | (((s->u_reg_size_len_ch1_qs) & 0x3FULL) << 16)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = (((((((((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && ((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1)))) ? ((_qp_next_u_reg_reg_rdata_next & ~0xFFC00000ULL) | (((s->u_reg_size_reps_ch1_qs) & 0x3FFULL) << 22)) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_reg_rdata_next = (((((((((((((((1) || (1)) && (!(((((s->u_reg_addr_hit) & 1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 9) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 10) & 0x1)) == (1))))) && (!((((((s->u_reg_addr_hit) >> 11) & 0x1)) == (1))))) ? (-1) : _qp_next_u_reg_reg_rdata_next);
    _qp_next_u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_syndrome_o & ~0x1ULL) | ((((__builtin_parityll((((0) << 58) | ((((s->u_reg_u_chk_u_chk_data_i) & 0x3FFFFFFFFFFFFFFULL)) & (217298647660920831)))))) & 0x1ULL) << 0)) : _qp_next_u_reg_u_chk_u_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_syndrome_o & ~0x2ULL) | ((((__builtin_parityll((((0) << 59) | (((((s->u_reg_u_chk_u_chk_data_i) & 0x7FFFFFFFFFFFFFFULL)) ^ (288230376151711744)) & (395226017347633183)))))) & 0x1ULL) << 1)) : _qp_next_u_reg_u_chk_u_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_syndrome_o & ~0x4ULL) | ((((__builtin_parityll((((0) << 60) | (((((s->u_reg_u_chk_u_chk_data_i) & 0xFFFFFFFFFFFFFFFULL)) ^ (288230376151711744)) & (701965574322225633)))))) & 0x1ULL) << 2)) : _qp_next_u_reg_u_chk_u_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_syndrome_o & ~0x8ULL) | ((((__builtin_parityll((((0) << 61) | (((((((s->u_reg_u_chk_u_chk_data_i) >> 1) & 0xFFFFFFFFFFFFFFFULL)) ^ (720575940379279360)) & (643864241515546385)) << 1) | (0))))) & 0x1ULL) << 3)) : _qp_next_u_reg_u_chk_u_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_syndrome_o & ~0x10ULL) | ((((__builtin_parityll((((0) << 62) | (((((((s->u_reg_u_chk_u_chk_data_i) >> 2) & 0xFFFFFFFFFFFFFFFULL)) ^ (360287970189639680)) & (611325937131342993)) << 2) | (0))))) & 0x1ULL) << 4)) : _qp_next_u_reg_u_chk_u_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_syndrome_o & ~0x20ULL) | ((((__builtin_parityll((((0) << 63) | (((((((s->u_reg_u_chk_u_chk_data_i) >> 3) & 0xFFFFFFFFFFFFFFFULL)) ^ (756604737398243328)) & (594184239166671505)) << 3) | (0))))) & 0x1ULL) << 5)) : _qp_next_u_reg_u_chk_u_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_syndrome_o & ~0x40ULL) | ((((__builtin_parityll(((((((((s->u_reg_u_chk_u_chk_data_i) >> 4) & 0xFFFFFFFFFFFFFFFULL)) ^ (378302368699121664)) & (585395222571796113)) << 4) | (0))))) & 0x1ULL) << 6)) : _qp_next_u_reg_u_chk_u_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x1ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (7))) ^ (((s->u_reg_u_chk_u_chk_data_i) & 1))) & 0x1ULL) << 0)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x2ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (11))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 1) & 0x1))) & 0x1ULL) << 1)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x4ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (19))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 2) & 0x1))) & 0x1ULL) << 2)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x8ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (35))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 3) & 0x1))) & 0x1ULL) << 3)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x10ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (67))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 4) & 0x1))) & 0x1ULL) << 4)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x20ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (13))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 5) & 0x1))) & 0x1ULL) << 5)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x40ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (21))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 6) & 0x1))) & 0x1ULL) << 6)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x80ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (37))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 7) & 0x1))) & 0x1ULL) << 7)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x100ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (69))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 8) & 0x1))) & 0x1ULL) << 8)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x200ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (25))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 9) & 0x1))) & 0x1ULL) << 9)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x400ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (41))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 10) & 0x1))) & 0x1ULL) << 10)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x800ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (73))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 11) & 0x1))) & 0x1ULL) << 11)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x1000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (49))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 12) & 0x1))) & 0x1ULL) << 12)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x2000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (81))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 13) & 0x1))) & 0x1ULL) << 13)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x4000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (97))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 14) & 0x1))) & 0x1ULL) << 14)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x8000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (14))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 15) & 0x1))) & 0x1ULL) << 15)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x10000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (22))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 16) & 0x1))) & 0x1ULL) << 16)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x20000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (38))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 17) & 0x1))) & 0x1ULL) << 17)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x40000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (70))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 18) & 0x1))) & 0x1ULL) << 18)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x80000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (26))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 19) & 0x1))) & 0x1ULL) << 19)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x100000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (42))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 20) & 0x1))) & 0x1ULL) << 20)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x200000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (74))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 21) & 0x1))) & 0x1ULL) << 21)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x400000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (50))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 22) & 0x1))) & 0x1ULL) << 22)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x800000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (82))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 23) & 0x1))) & 0x1ULL) << 23)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x1000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (98))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 24) & 0x1))) & 0x1ULL) << 24)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x2000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (28))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 25) & 0x1))) & 0x1ULL) << 25)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x4000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (44))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 26) & 0x1))) & 0x1ULL) << 26)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x8000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (76))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 27) & 0x1))) & 0x1ULL) << 27)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x10000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (52))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 28) & 0x1))) & 0x1ULL) << 28)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x20000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (84))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 29) & 0x1))) & 0x1ULL) << 29)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x40000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (100))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 30) & 0x1))) & 0x1ULL) << 30)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x80000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (56))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 31) & 0x1))) & 0x1ULL) << 31)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x100000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (88))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 32) & 0x1))) & 0x1ULL) << 32)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x200000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (104))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 33) & 0x1))) & 0x1ULL) << 33)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x400000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (112))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 34) & 0x1))) & 0x1ULL) << 34)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x800000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (31))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 35) & 0x1))) & 0x1ULL) << 35)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x1000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (47))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 36) & 0x1))) & 0x1ULL) << 36)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x2000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (79))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 37) & 0x1))) & 0x1ULL) << 37)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x4000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (55))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 38) & 0x1))) & 0x1ULL) << 38)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x8000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (87))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 39) & 0x1))) & 0x1ULL) << 39)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x10000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (103))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 40) & 0x1))) & 0x1ULL) << 40)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x20000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (59))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 41) & 0x1))) & 0x1ULL) << 41)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x40000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (91))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 42) & 0x1))) & 0x1ULL) << 42)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x80000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (107))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 43) & 0x1))) & 0x1ULL) << 43)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x100000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (115))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 44) & 0x1))) & 0x1ULL) << 44)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x200000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (61))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 45) & 0x1))) & 0x1ULL) << 45)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x400000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (93))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 46) & 0x1))) & 0x1ULL) << 46)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x800000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (109))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 47) & 0x1))) & 0x1ULL) << 47)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x1000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (117))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 48) & 0x1))) & 0x1ULL) << 48)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x2000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (121))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 49) & 0x1))) & 0x1ULL) << 49)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x4000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (62))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 50) & 0x1))) & 0x1ULL) << 50)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x8000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (94))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 51) & 0x1))) & 0x1ULL) << 51)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x10000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (110))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 52) & 0x1))) & 0x1ULL) << 52)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x20000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (118))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 53) & 0x1))) & 0x1ULL) << 53)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x40000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (122))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 54) & 0x1))) & 0x1ULL) << 54)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x80000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (124))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 55) & 0x1))) & 0x1ULL) << 55)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_data_o & ~0x100000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (127))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 56) & 0x1))) & 0x1ULL) << 56)) : _qp_next_u_reg_u_chk_u_chk_data_o);
    _qp_next_u_reg_u_chk_u_chk_err_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_err_o & ~0x1ULL) | ((((__builtin_parityll(s->u_reg_u_chk_u_chk_syndrome_o))) & 0x1ULL) << 0)) : _qp_next_u_reg_u_chk_u_chk_err_o);
    _qp_next_u_reg_u_chk_u_chk_err_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_chk_err_o & ~0x2ULL) | ((((((((_qp_next_u_reg_u_chk_u_chk_err_o) & 1)) ^ 1)) & (((s->u_reg_u_chk_u_chk_syndrome_o) != (0)))) & 0x1ULL) << 1)) : _qp_next_u_reg_u_chk_u_chk_err_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x1ULL) | ((((__builtin_parityll((((0) << 33) | ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 0x1FFFFFFFFULL)) & (4932943141)))))) & 0x1ULL) << 0)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x2ULL) | ((((__builtin_parityll((((0) << 34) | (((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 4) & 0x3FFFFFFFULL)) ^ (536870912)) & (770418693)) << 4) | (0))))) & 0x1ULL) << 1)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x4ULL) | ((((__builtin_parityll((((0) << 35) | (((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 1) & 0x3FFFFFFFFULL)) ^ (4294967296)) & (9137210581)) << 1) | (0))))) & 0x1ULL) << 2)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x8ULL) | ((((__builtin_parityll((((0) << 36) | (((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 0xFFFFFFFFFULL)) ^ (42949672960)) & (35184135889)))))) & 0x1ULL) << 3)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x10ULL) | ((((__builtin_parityll((((0) << 37) | (((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 0x1FFFFFFFFFULL)) ^ (42949672960)) & (71986917947)))))) & 0x1ULL) << 4)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x20ULL) | ((((__builtin_parityll((((0) << 38) | (((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 2) & 0xFFFFFFFFFULL)) ^ (45097156608)) & (34551830675)) << 2) | (0))))) & 0x1ULL) << 5)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x40ULL) | ((((__builtin_parityll(((((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 1) & 0x3FFFFFFFFFULL)) ^ (90194313216)) & (138716654275)) << 1) | (0))))) & 0x1ULL) << 6)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x1ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (25))) ^ (((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 1))) & 0x1ULL) << 0)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x2ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (84))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 1) & 0x1))) & 0x1ULL) << 1)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x4ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (97))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 2) & 0x1))) & 0x1ULL) << 2)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x8ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (52))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 3) & 0x1))) & 0x1ULL) << 3)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x10ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (26))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 4) & 0x1))) & 0x1ULL) << 4)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x20ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (21))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 5) & 0x1))) & 0x1ULL) << 5)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x40ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (42))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 6) & 0x1))) & 0x1ULL) << 6)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x80ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (76))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 7) & 0x1))) & 0x1ULL) << 7)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x100ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (69))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 8) & 0x1))) & 0x1ULL) << 8)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x200ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (56))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 9) & 0x1))) & 0x1ULL) << 9)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x400ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (73))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 10) & 0x1))) & 0x1ULL) << 10)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x800ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (13))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 11) & 0x1))) & 0x1ULL) << 11)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x1000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (81))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 12) & 0x1))) & 0x1ULL) << 12)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x2000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (49))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 13) & 0x1))) & 0x1ULL) << 13)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x4000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (104))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 14) & 0x1))) & 0x1ULL) << 14)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x8000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (7))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 15) & 0x1))) & 0x1ULL) << 15)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x10000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (28))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 16) & 0x1))) & 0x1ULL) << 16)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x20000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (11))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 17) & 0x1))) & 0x1ULL) << 17)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x40000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (37))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 18) & 0x1))) & 0x1ULL) << 18)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x80000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (38))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 19) & 0x1))) & 0x1ULL) << 19)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x100000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (70))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 20) & 0x1))) & 0x1ULL) << 20)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x200000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (14))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 21) & 0x1))) & 0x1ULL) << 21)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x400000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (112))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 22) & 0x1))) & 0x1ULL) << 22)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x800000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (50))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 23) & 0x1))) & 0x1ULL) << 23)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x1000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (44))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 24) & 0x1))) & 0x1ULL) << 24)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x2000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (19))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 25) & 0x1))) & 0x1ULL) << 25)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x4000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (35))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 26) & 0x1))) & 0x1ULL) << 26)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x8000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (98))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 27) & 0x1))) & 0x1ULL) << 27)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x10000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (74))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 28) & 0x1))) & 0x1ULL) << 28)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x20000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (41))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 29) & 0x1))) & 0x1ULL) << 29)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x40000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (22))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 30) & 0x1))) & 0x1ULL) << 30)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x80000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (82))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 31) & 0x1))) & 0x1ULL) << 31)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o & ~0x1ULL) | ((((__builtin_parityll(s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o))) & 0x1ULL) << 0)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o);
    _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o = (((1) || (1)) ? ((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o & ~0x2ULL) | ((((((((_qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o) & 1)) ^ 1)) & (((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) != (0)))) & 0x1ULL) << 1)) : _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_valid = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_valid) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_valid);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_opcode = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_opcode) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_opcode);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_param = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_param) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_param);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_size = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_size) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_size);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_source = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_source) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_source);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_sink = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_sink) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_sink);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_data = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_data) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_data);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_error = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_error) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_error);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_a_ready = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_a_ready) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_a_ready);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_rsp_intg) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_data_intg) : _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((((0) << 57) | (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i))) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x200000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (73183459585064959)))))) & 0x1ULL) << 57)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x400000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (106995641195921439)))))) & 0x1ULL) << 58)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x800000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (125504822018802145)))))) & 0x1ULL) << 59)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x1000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 1) & 0xFFFFFFFFFFFFFFULL)) & (67403489212122897)) << 1) | (0))))) & 0x1ULL) << 60)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x2000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 2) & 0x7FFFFFFFFFFFFFULL)) & (34865184827919505)) << 2) | (0))))) & 0x1ULL) << 61)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x4000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 3) & 0x3FFFFFFFFFFFFFULL)) & (17723486863248017)) << 3) | (0))))) & 0x1ULL) << 62)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x8000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 4) & 0x1FFFFFFFFFFFFFULL)) & (8934470268372625)) << 4) | (0))))) & 0x1ULL) << 63)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) ^ (6052837899185946624)) : _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((((0) << 32) | (s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i))) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x100000000ULL) | ((((__builtin_parityll((((0) << 30) | ((((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (637975845)))))) & 0x1ULL) << 32)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x200000000ULL) | ((((__builtin_parityll((((0) << 32) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 4) & 0xFFFFFFFULL)) & (233547781)) << 4) | (0))))) & 0x1ULL) << 33)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x400000000ULL) | ((((__builtin_parityll((((0) << 31) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x3FFFFFFFULL)) & (547275989)) << 1) | (0))))) & 0x1ULL) << 34)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x800000000ULL) | ((((__builtin_parityll((((0) << 30) | ((((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (824397521)))))) & 0x1ULL) << 35)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x1000000000ULL) | ((((__builtin_parityll((((0) << 32) | ((((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0xFFFFFFFFULL)) & (3267441211)))))) & 0x1ULL) << 36)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x2000000000ULL) | ((((__builtin_parityll((((0) << 30) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 2) & 0xFFFFFFFULL)) & (192092307)) << 2) | (0))))) & 0x1ULL) << 37)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x4000000000ULL) | ((((__builtin_parityll((((0) << 32) | ((((((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x7FFFFFFFULL)) & (1277700803)) << 1) | (0))))) & 0x1ULL) << 38)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((_qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) ^ (180388626432)) : _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    _qp_next_u_reg_u_reg_if_outstanding_q = ((((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (!(s->u_reg_u_reg_if_a_ack))) && (s->u_reg_u_reg_if_d_ack)) || ((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack))) || (((s->u_reg_u_reg_if_rst_ni) ^ 1))) ? (((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (!(s->u_reg_u_reg_if_a_ack))) && (s->u_reg_u_reg_if_d_ack)) ? (0) : ((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (1) : (0))))) : _qp_next_u_reg_u_reg_if_outstanding_q);
    _qp_next_u_reg_u_reg_if_reqid_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_reqid_q);
    _qp_next_u_reg_u_reg_if_reqsz_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_reqsz_q);
    _qp_next_u_reg_u_reg_if_rspop_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_rspop_q);
    _qp_next_u_reg_u_reg_if_reqid_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (s->u_reg_u_reg_if_tl_i_a_source) : _qp_next_u_reg_u_reg_if_reqid_q);
    _qp_next_u_reg_u_reg_if_reqsz_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (s->u_reg_u_reg_if_tl_i_a_size) : _qp_next_u_reg_u_reg_if_reqsz_q);
    _qp_next_u_reg_u_reg_if_rspop_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((((0) << 1) | (s->u_reg_u_reg_if_rd_req))) : _qp_next_u_reg_u_reg_if_rspop_q);
    _qp_next_u_reg_u_reg_if_rdata_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_rdata_q);
    _qp_next_u_reg_u_reg_if_error_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_reg_if_error_q);
    _qp_next_u_reg_u_reg_if_rdata_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((((s->u_reg_u_reg_if_error_i) | (s->u_reg_u_reg_if_err_internal) | (s->u_reg_u_reg_if_wr_req)) ? (4294967295) : (s->u_reg_u_reg_if_rdata_i))) : _qp_next_u_reg_u_reg_if_rdata_q);
    _qp_next_u_reg_u_reg_if_error_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((s->u_reg_u_reg_if_error_i) | (s->u_reg_u_reg_if_err_internal)) : _qp_next_u_reg_u_reg_if_error_q);
    _qp_next_u_reg_u_reg_if_addr_align_err = ((((1) || (1)) && (s->u_reg_u_reg_if_wr_req)) ? (((((s->u_reg_u_reg_if_tl_i_a_address) & 0x3)) != (0))) : _qp_next_u_reg_u_reg_if_addr_align_err);
    _qp_next_u_reg_u_reg_if_addr_align_err = ((((1) || (1)) && (!(s->u_reg_u_reg_if_wr_req))) ? (0) : _qp_next_u_reg_u_reg_if_addr_align_err);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_rsp_intg) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_data_intg) : _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg);
    _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk = (((1) || (1)) ? (0) : _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk);
    _qp_next_u_reg_u_reg_if_u_err_mask_chk = (((1) || (1)) ? (0) : _qp_next_u_reg_u_reg_if_u_err_mask_chk);
    _qp_next_u_reg_u_reg_if_u_err_fulldata_chk = (((1) || (1)) ? (0) : _qp_next_u_reg_u_reg_if_u_err_fulldata_chk);
    _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk = (((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0)))) ? (-1) : _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk);
    _qp_next_u_reg_u_reg_if_u_err_mask_chk = (((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0)))) ? ((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & (~(s->u_reg_u_reg_if_u_err_mask))) == (0))) : _qp_next_u_reg_u_reg_if_u_err_mask_chk);
    _qp_next_u_reg_u_reg_if_u_err_fulldata_chk = (((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0)))) ? ((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & (s->u_reg_u_reg_if_u_err_mask)) != (0))) : _qp_next_u_reg_u_reg_if_u_err_fulldata_chk);
    _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk = ((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1)))) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 1)) ^ 1)) : _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk);
    _qp_next_u_reg_u_reg_if_u_err_mask_chk = ((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1)))) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (0))) : ((((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (0))))) : _qp_next_u_reg_u_reg_if_u_err_mask_chk);
    _qp_next_u_reg_u_reg_if_u_err_fulldata_chk = ((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1)))) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (3))) : (((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (3))))) : _qp_next_u_reg_u_reg_if_u_err_fulldata_chk);
    _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2)))) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 0x3)) == (0))) : _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk);
    _qp_next_u_reg_u_reg_if_u_err_mask_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2)))) ? (-1) : _qp_next_u_reg_u_reg_if_u_err_mask_chk);
    _qp_next_u_reg_u_reg_if_u_err_fulldata_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2)))) ? (((s->u_reg_u_reg_if_u_err_tl_i_a_mask) == (15))) : _qp_next_u_reg_u_reg_if_u_err_fulldata_chk);
    _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2))))) ? (0) : _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk);
    _qp_next_u_reg_u_reg_if_u_err_mask_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2))))) ? (0) : _qp_next_u_reg_u_reg_if_u_err_mask_chk);
    _qp_next_u_reg_u_reg_if_u_err_fulldata_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2))))) ? (0) : _qp_next_u_reg_u_reg_if_u_err_fulldata_chk);
    _qp_next_u_reg_u_intr_state_done_ch0_q = ((((s->u_reg_u_intr_state_done_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_state_done_ch0_q);
    _qp_next_u_reg_u_intr_state_done_ch0_q = (((!(((s->u_reg_u_intr_state_done_ch0_rst_ni) ^ 1))) && (s->u_reg_u_intr_state_done_ch0_wr_en)) ? (s->u_reg_u_intr_state_done_ch0_wr_data) : _qp_next_u_reg_u_intr_state_done_ch0_q);
    _qp_next_u_reg_u_intr_state_done_ch1_q = ((((s->u_reg_u_intr_state_done_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_state_done_ch1_q);
    _qp_next_u_reg_u_intr_state_done_ch1_q = (((!(((s->u_reg_u_intr_state_done_ch1_rst_ni) ^ 1))) && (s->u_reg_u_intr_state_done_ch1_wr_en)) ? (s->u_reg_u_intr_state_done_ch1_wr_data) : _qp_next_u_reg_u_intr_state_done_ch1_q);
    _qp_next_u_reg_u_intr_enable_done_ch0_q = ((((s->u_reg_u_intr_enable_done_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_enable_done_ch0_q);
    _qp_next_u_reg_u_intr_enable_done_ch0_q = (((!(((s->u_reg_u_intr_enable_done_ch0_rst_ni) ^ 1))) && (s->u_reg_u_intr_enable_done_ch0_wr_en)) ? (s->u_reg_u_intr_enable_done_ch0_wr_data) : _qp_next_u_reg_u_intr_enable_done_ch0_q);
    _qp_next_u_reg_u_intr_enable_done_ch1_q = ((((s->u_reg_u_intr_enable_done_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_intr_enable_done_ch1_q);
    _qp_next_u_reg_u_intr_enable_done_ch1_q = (((!(((s->u_reg_u_intr_enable_done_ch1_rst_ni) ^ 1))) && (s->u_reg_u_intr_enable_done_ch1_wr_en)) ? (s->u_reg_u_intr_enable_done_ch1_wr_data) : _qp_next_u_reg_u_intr_enable_done_ch1_q);
    _qp_next_u_reg_u_ctrl_enable_ch0_q = ((((s->u_reg_u_ctrl_enable_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_enable_ch0_q);
    _qp_next_u_reg_u_ctrl_enable_ch0_q = (((!(((s->u_reg_u_ctrl_enable_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_enable_ch0_wr_en)) ? (s->u_reg_u_ctrl_enable_ch0_wr_data) : _qp_next_u_reg_u_ctrl_enable_ch0_q);
    _qp_next_u_reg_u_ctrl_enable_ch1_q = ((((s->u_reg_u_ctrl_enable_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_enable_ch1_q);
    _qp_next_u_reg_u_ctrl_enable_ch1_q = (((!(((s->u_reg_u_ctrl_enable_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_enable_ch1_wr_en)) ? (s->u_reg_u_ctrl_enable_ch1_wr_data) : _qp_next_u_reg_u_ctrl_enable_ch1_q);
    _qp_next_u_reg_u_ctrl_polarity_ch0_q = ((((s->u_reg_u_ctrl_polarity_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_polarity_ch0_q);
    _qp_next_u_reg_u_ctrl_polarity_ch0_q = (((!(((s->u_reg_u_ctrl_polarity_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_polarity_ch0_wr_en)) ? (s->u_reg_u_ctrl_polarity_ch0_wr_data) : _qp_next_u_reg_u_ctrl_polarity_ch0_q);
    _qp_next_u_reg_u_ctrl_polarity_ch1_q = ((((s->u_reg_u_ctrl_polarity_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_polarity_ch1_q);
    _qp_next_u_reg_u_ctrl_polarity_ch1_q = (((!(((s->u_reg_u_ctrl_polarity_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_polarity_ch1_wr_en)) ? (s->u_reg_u_ctrl_polarity_ch1_wr_data) : _qp_next_u_reg_u_ctrl_polarity_ch1_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q = ((((s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q = (((!(((s->u_reg_u_ctrl_inactive_level_pcl_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch0_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch0_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q = ((((s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q = (((!(((s->u_reg_u_ctrl_inactive_level_pda_ch0_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pda_ch0_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch0_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q = ((((s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q = (((!(((s->u_reg_u_ctrl_inactive_level_pcl_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pcl_ch1_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pcl_ch1_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q = ((((s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q);
    _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q = (((!(((s->u_reg_u_ctrl_inactive_level_pda_ch1_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_en)) ? (s->u_reg_u_ctrl_inactive_level_pda_ch1_wr_data) : _qp_next_u_reg_u_ctrl_inactive_level_pda_ch1_q);
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
    _qp_next_u_reg_u_size_len_ch0_q = ((((s->u_reg_u_size_len_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_len_ch0_q);
    _qp_next_u_reg_u_size_len_ch0_q = (((!(((s->u_reg_u_size_len_ch0_rst_ni) ^ 1))) && (s->u_reg_u_size_len_ch0_wr_en)) ? (s->u_reg_u_size_len_ch0_wr_data) : _qp_next_u_reg_u_size_len_ch0_q);
    _qp_next_u_reg_u_size_reps_ch0_q = ((((s->u_reg_u_size_reps_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_reps_ch0_q);
    _qp_next_u_reg_u_size_reps_ch0_q = (((!(((s->u_reg_u_size_reps_ch0_rst_ni) ^ 1))) && (s->u_reg_u_size_reps_ch0_wr_en)) ? (s->u_reg_u_size_reps_ch0_wr_data) : _qp_next_u_reg_u_size_reps_ch0_q);
    _qp_next_u_reg_u_size_len_ch1_q = ((((s->u_reg_u_size_len_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_len_ch1_q);
    _qp_next_u_reg_u_size_len_ch1_q = (((!(((s->u_reg_u_size_len_ch1_rst_ni) ^ 1))) && (s->u_reg_u_size_len_ch1_wr_en)) ? (s->u_reg_u_size_len_ch1_wr_data) : _qp_next_u_reg_u_size_len_ch1_q);
    _qp_next_u_reg_u_size_reps_ch1_q = ((((s->u_reg_u_size_reps_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_reg_u_size_reps_ch1_q);
    _qp_next_u_reg_u_size_reps_ch1_q = (((!(((s->u_reg_u_size_reps_ch1_rst_ni) ^ 1))) && (s->u_reg_u_size_reps_ch1_wr_en)) ? (s->u_reg_u_size_reps_ch1_wr_data) : _qp_next_u_reg_u_size_reps_ch1_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_state_q) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd = (((1) || (1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? ((((0) << 2) | ((((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) ^ 1)) << 1) | (1))) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) ? (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4))))) ? (((((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (5))) ? (6) : (0))) : ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) ? (4) : (2))))) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-3) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-3) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_state_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_ping_set_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed = -1;
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) + (1)) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (((1) || (1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed = -1;
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) + (1)) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1)) ? (-2) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o);
    _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i) : _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o);
    _qp_next_u_pattgen_core_chan0_polarity_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_polarity_q);
    _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q);
    _qp_next_u_pattgen_core_chan0_inactive_level_pda_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_inactive_level_pda_q);
    _qp_next_u_pattgen_core_chan0_prediv_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_prediv_q);
    _qp_next_u_pattgen_core_chan0_data_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_data_q);
    _qp_next_u_pattgen_core_chan0_len_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_len_q);
    _qp_next_u_pattgen_core_chan0_reps_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_reps_q);
    _qp_next_u_pattgen_core_chan0_polarity_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (_qp_next_u_pattgen_core_chan0_polarity_q) : (s->u_pattgen_core_chan0_ctrl_i_polarity))) : _qp_next_u_pattgen_core_chan0_polarity_q);
    _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (_qp_next_u_pattgen_core_chan0_inactive_level_pcl_q) : (s->u_pattgen_core_chan0_ctrl_i_inactive_level_pcl))) : _qp_next_u_pattgen_core_chan0_inactive_level_pcl_q);
    _qp_next_u_pattgen_core_chan0_inactive_level_pda_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (_qp_next_u_pattgen_core_chan0_inactive_level_pda_q) : (s->u_pattgen_core_chan0_ctrl_i_inactive_level_pda))) : _qp_next_u_pattgen_core_chan0_inactive_level_pda_q);
    _qp_next_u_pattgen_core_chan0_prediv_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (_qp_next_u_pattgen_core_chan0_prediv_q) : (s->u_pattgen_core_chan0_ctrl_i_prediv))) : _qp_next_u_pattgen_core_chan0_prediv_q);
    _qp_next_u_pattgen_core_chan0_data_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (_qp_next_u_pattgen_core_chan0_data_q) : (s->u_pattgen_core_chan0_ctrl_i_data))) : _qp_next_u_pattgen_core_chan0_data_q);
    _qp_next_u_pattgen_core_chan0_len_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (_qp_next_u_pattgen_core_chan0_len_q) : (s->u_pattgen_core_chan0_ctrl_i_len))) : _qp_next_u_pattgen_core_chan0_len_q);
    _qp_next_u_pattgen_core_chan0_reps_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_enable) ? (_qp_next_u_pattgen_core_chan0_reps_q) : (s->u_pattgen_core_chan0_ctrl_i_reps))) : _qp_next_u_pattgen_core_chan0_reps_q);
    _qp_next_u_pattgen_core_chan0_pcl_o = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_pcl_o);
    _qp_next_u_pattgen_core_chan0_pda_o = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_pda_o);
    _qp_next_u_pattgen_core_chan0_pcl_o = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_active) ? ((s->u_pattgen_core_chan0_polarity_q) ^ (s->u_pattgen_core_chan0_pcl_int_q)) : (s->u_pattgen_core_chan0_inactive_level_pcl_q))) : _qp_next_u_pattgen_core_chan0_pcl_o);
    _qp_next_u_pattgen_core_chan0_pda_o = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_active) ? ((((s->u_pattgen_core_chan0_data_q) >> ((((0) << 6) | (s->u_pattgen_core_chan0_bit_cnt_q)))) & 1)) : (s->u_pattgen_core_chan0_inactive_level_pda_q))) : _qp_next_u_pattgen_core_chan0_pda_o);
    _qp_next_u_pattgen_core_chan0_clk_cnt_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_clk_cnt_q);
    _qp_next_u_pattgen_core_chan0_clk_cnt_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_clk_en) ? (s->u_pattgen_core_chan0_clk_cnt_d) : (_qp_next_u_pattgen_core_chan0_clk_cnt_q))) : _qp_next_u_pattgen_core_chan0_clk_cnt_q);
    _qp_next_u_pattgen_core_chan0_pcl_int_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_pcl_int_q);
    _qp_next_u_pattgen_core_chan0_pcl_int_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_clk_en) ? (s->u_pattgen_core_chan0_pcl_int_d) : (_qp_next_u_pattgen_core_chan0_pcl_int_q))) : _qp_next_u_pattgen_core_chan0_pcl_int_q);
    _qp_next_u_pattgen_core_chan0_bit_cnt_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_bit_cnt_q);
    _qp_next_u_pattgen_core_chan0_bit_cnt_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_bit_cnt_en) ? (s->u_pattgen_core_chan0_bit_cnt_d) : (_qp_next_u_pattgen_core_chan0_bit_cnt_q))) : _qp_next_u_pattgen_core_chan0_bit_cnt_q);
    _qp_next_u_pattgen_core_chan0_rep_cnt_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_rep_cnt_q);
    _qp_next_u_pattgen_core_chan0_rep_cnt_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_rep_cnt_en) ? (s->u_pattgen_core_chan0_rep_cnt_d) : (_qp_next_u_pattgen_core_chan0_rep_cnt_q))) : _qp_next_u_pattgen_core_chan0_rep_cnt_q);
    _qp_next_u_pattgen_core_chan0_complete_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_complete_q);
    _qp_next_u_pattgen_core_chan0_complete_q2 = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_complete_q2);
    _qp_next_u_pattgen_core_chan0_complete_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan0_complete_en) ? (s->u_pattgen_core_chan0_complete_d) : (_qp_next_u_pattgen_core_chan0_complete_q))) : _qp_next_u_pattgen_core_chan0_complete_q);
    _qp_next_u_pattgen_core_chan0_complete_q2 = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan0_complete_q) : _qp_next_u_pattgen_core_chan0_complete_q2);
    _qp_next_u_pattgen_core_chan0_active_q = ((((s->u_pattgen_core_chan0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan0_active_q);
    _qp_next_u_pattgen_core_chan0_active_q = ((!(((s->u_pattgen_core_chan0_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan0_active_d) : _qp_next_u_pattgen_core_chan0_active_q);
    _qp_next_u_pattgen_core_chan1_polarity_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_polarity_q);
    _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q);
    _qp_next_u_pattgen_core_chan1_inactive_level_pda_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_inactive_level_pda_q);
    _qp_next_u_pattgen_core_chan1_prediv_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_prediv_q);
    _qp_next_u_pattgen_core_chan1_data_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_data_q);
    _qp_next_u_pattgen_core_chan1_len_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_len_q);
    _qp_next_u_pattgen_core_chan1_reps_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_reps_q);
    _qp_next_u_pattgen_core_chan1_polarity_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (_qp_next_u_pattgen_core_chan1_polarity_q) : (s->u_pattgen_core_chan1_ctrl_i_polarity))) : _qp_next_u_pattgen_core_chan1_polarity_q);
    _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (_qp_next_u_pattgen_core_chan1_inactive_level_pcl_q) : (s->u_pattgen_core_chan1_ctrl_i_inactive_level_pcl))) : _qp_next_u_pattgen_core_chan1_inactive_level_pcl_q);
    _qp_next_u_pattgen_core_chan1_inactive_level_pda_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (_qp_next_u_pattgen_core_chan1_inactive_level_pda_q) : (s->u_pattgen_core_chan1_ctrl_i_inactive_level_pda))) : _qp_next_u_pattgen_core_chan1_inactive_level_pda_q);
    _qp_next_u_pattgen_core_chan1_prediv_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (_qp_next_u_pattgen_core_chan1_prediv_q) : (s->u_pattgen_core_chan1_ctrl_i_prediv))) : _qp_next_u_pattgen_core_chan1_prediv_q);
    _qp_next_u_pattgen_core_chan1_data_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (_qp_next_u_pattgen_core_chan1_data_q) : (s->u_pattgen_core_chan1_ctrl_i_data))) : _qp_next_u_pattgen_core_chan1_data_q);
    _qp_next_u_pattgen_core_chan1_len_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (_qp_next_u_pattgen_core_chan1_len_q) : (s->u_pattgen_core_chan1_ctrl_i_len))) : _qp_next_u_pattgen_core_chan1_len_q);
    _qp_next_u_pattgen_core_chan1_reps_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_enable) ? (_qp_next_u_pattgen_core_chan1_reps_q) : (s->u_pattgen_core_chan1_ctrl_i_reps))) : _qp_next_u_pattgen_core_chan1_reps_q);
    _qp_next_u_pattgen_core_chan1_pcl_o = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_pcl_o);
    _qp_next_u_pattgen_core_chan1_pda_o = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_pda_o);
    _qp_next_u_pattgen_core_chan1_pcl_o = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_active) ? ((s->u_pattgen_core_chan1_polarity_q) ^ (s->u_pattgen_core_chan1_pcl_int_q)) : (s->u_pattgen_core_chan1_inactive_level_pcl_q))) : _qp_next_u_pattgen_core_chan1_pcl_o);
    _qp_next_u_pattgen_core_chan1_pda_o = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_active) ? ((((s->u_pattgen_core_chan1_data_q) >> ((((0) << 6) | (s->u_pattgen_core_chan1_bit_cnt_q)))) & 1)) : (s->u_pattgen_core_chan1_inactive_level_pda_q))) : _qp_next_u_pattgen_core_chan1_pda_o);
    _qp_next_u_pattgen_core_chan1_clk_cnt_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_clk_cnt_q);
    _qp_next_u_pattgen_core_chan1_clk_cnt_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_clk_en) ? (s->u_pattgen_core_chan1_clk_cnt_d) : (_qp_next_u_pattgen_core_chan1_clk_cnt_q))) : _qp_next_u_pattgen_core_chan1_clk_cnt_q);
    _qp_next_u_pattgen_core_chan1_pcl_int_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_pcl_int_q);
    _qp_next_u_pattgen_core_chan1_pcl_int_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_clk_en) ? (s->u_pattgen_core_chan1_pcl_int_d) : (_qp_next_u_pattgen_core_chan1_pcl_int_q))) : _qp_next_u_pattgen_core_chan1_pcl_int_q);
    _qp_next_u_pattgen_core_chan1_bit_cnt_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_bit_cnt_q);
    _qp_next_u_pattgen_core_chan1_bit_cnt_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_bit_cnt_en) ? (s->u_pattgen_core_chan1_bit_cnt_d) : (_qp_next_u_pattgen_core_chan1_bit_cnt_q))) : _qp_next_u_pattgen_core_chan1_bit_cnt_q);
    _qp_next_u_pattgen_core_chan1_rep_cnt_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_rep_cnt_q);
    _qp_next_u_pattgen_core_chan1_rep_cnt_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_rep_cnt_en) ? (s->u_pattgen_core_chan1_rep_cnt_d) : (_qp_next_u_pattgen_core_chan1_rep_cnt_q))) : _qp_next_u_pattgen_core_chan1_rep_cnt_q);
    _qp_next_u_pattgen_core_chan1_complete_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_complete_q);
    _qp_next_u_pattgen_core_chan1_complete_q2 = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_complete_q2);
    _qp_next_u_pattgen_core_chan1_complete_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (((s->u_pattgen_core_chan1_complete_en) ? (s->u_pattgen_core_chan1_complete_d) : (_qp_next_u_pattgen_core_chan1_complete_q))) : _qp_next_u_pattgen_core_chan1_complete_q);
    _qp_next_u_pattgen_core_chan1_complete_q2 = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan1_complete_q) : _qp_next_u_pattgen_core_chan1_complete_q2);
    _qp_next_u_pattgen_core_chan1_active_q = ((((s->u_pattgen_core_chan1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_chan1_active_q);
    _qp_next_u_pattgen_core_chan1_active_q = ((!(((s->u_pattgen_core_chan1_rst_ni) ^ 1))) ? (s->u_pattgen_core_chan1_active_d) : _qp_next_u_pattgen_core_chan1_active_q);
    _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o = ((((s->u_pattgen_core_intr_hw_done_ch0_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o);
    _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o = ((!(((s->u_pattgen_core_intr_hw_done_ch0_rst_ni) ^ 1))) ? ((s->u_pattgen_core_intr_hw_done_ch0_status) & (s->u_pattgen_core_intr_hw_done_ch0_reg2hw_intr_enable_q_i)) : _qp_next_u_pattgen_core_intr_hw_done_ch0_intr_o);
    _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o = ((((s->u_pattgen_core_intr_hw_done_ch1_rst_ni) ^ 1)) ? (0) : _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o);
    _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o = ((!(((s->u_pattgen_core_intr_hw_done_ch1_rst_ni) ^ 1))) ? ((s->u_pattgen_core_intr_hw_done_ch1_status) & (s->u_pattgen_core_intr_hw_done_ch1_reg2hw_intr_enable_q_i)) : _qp_next_u_pattgen_core_intr_hw_done_ch1_intr_o);

    /* Detect changes before committing ordinary registers. */
    _qp_changed |= _qp_next_u_reg_err_q != s->u_reg_err_q;
    _qp_changed |= _qp_next_u_reg_addr_hit != s->u_reg_addr_hit;
    _qp_changed |= _qp_next_u_reg_wr_err != s->u_reg_wr_err;
    _qp_changed |= _qp_next_u_reg_reg_we_check != s->u_reg_reg_we_check;
    _qp_changed |= _qp_next_u_reg_reg_rdata_next != s->u_reg_reg_rdata_next;
    _qp_changed |= _qp_next_u_reg_u_chk_u_chk_syndrome_o != s->u_reg_u_chk_u_chk_syndrome_o;
    _qp_changed |= _qp_next_u_reg_u_chk_u_chk_data_o != s->u_reg_u_chk_u_chk_data_o;
    _qp_changed |= _qp_next_u_reg_u_chk_u_chk_err_o != s->u_reg_u_chk_u_chk_err_o;
    _qp_changed |= _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o != s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o;
    _qp_changed |= _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o != s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o;
    _qp_changed |= _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o != s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_valid != s->u_reg_u_rsp_intg_gen_tl_o_d_valid;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_opcode != s->u_reg_u_rsp_intg_gen_tl_o_d_opcode;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_param != s->u_reg_u_rsp_intg_gen_tl_o_d_param;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_size != s->u_reg_u_rsp_intg_gen_tl_o_d_size;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_source != s->u_reg_u_rsp_intg_gen_tl_o_d_source;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_sink != s->u_reg_u_rsp_intg_gen_tl_o_d_sink;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_data != s->u_reg_u_rsp_intg_gen_tl_o_d_data;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg != s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg != s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_error != s->u_reg_u_rsp_intg_gen_tl_o_d_error;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_tl_o_a_ready != s->u_reg_u_rsp_intg_gen_tl_o_a_ready;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o != s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o;
    _qp_changed |= _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o != s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o;
    _qp_changed |= _qp_next_u_reg_u_reg_if_outstanding_q != s->u_reg_u_reg_if_outstanding_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_reqid_q != s->u_reg_u_reg_if_reqid_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_reqsz_q != s->u_reg_u_reg_if_reqsz_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_rspop_q != s->u_reg_u_reg_if_rspop_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_rdata_q != s->u_reg_u_reg_if_rdata_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_error_q != s->u_reg_u_reg_if_error_q;
    _qp_changed |= _qp_next_u_reg_u_reg_if_addr_align_err != s->u_reg_u_reg_if_addr_align_err;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready != s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk != s->u_reg_u_reg_if_u_err_addr_sz_chk;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_err_mask_chk != s->u_reg_u_reg_if_u_err_mask_chk;
    _qp_changed |= _qp_next_u_reg_u_reg_if_u_err_fulldata_chk != s->u_reg_u_reg_if_u_err_fulldata_chk;
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
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d != s->gen_alert_tx_0_u_prim_alert_sender_state_d;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd != s->gen_alert_tx_0_u_prim_alert_sender_alert_pd;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd != s->gen_alert_tx_0_u_prim_alert_sender_alert_nd;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr != s->gen_alert_tx_0_u_prim_alert_sender_ping_clr;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr != s->gen_alert_tx_0_u_prim_alert_sender_alert_clr;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q != s->gen_alert_tx_0_u_prim_alert_sender_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q != s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q != s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q != s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    _qp_changed |= _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o != s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o;
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
    s->u_reg_addr_hit = _qp_next_u_reg_addr_hit;
    s->u_reg_wr_err = _qp_next_u_reg_wr_err;
    s->u_reg_reg_we_check = _qp_next_u_reg_reg_we_check;
    s->u_reg_reg_rdata_next = _qp_next_u_reg_reg_rdata_next;
    s->u_reg_u_chk_u_chk_syndrome_o = _qp_next_u_reg_u_chk_u_chk_syndrome_o;
    s->u_reg_u_chk_u_chk_data_o = _qp_next_u_reg_u_chk_u_chk_data_o;
    s->u_reg_u_chk_u_chk_err_o = _qp_next_u_reg_u_chk_u_chk_err_o;
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o;
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o;
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o = _qp_next_u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o;
    s->u_reg_u_rsp_intg_gen_tl_o_d_valid = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_valid;
    s->u_reg_u_rsp_intg_gen_tl_o_d_opcode = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_opcode;
    s->u_reg_u_rsp_intg_gen_tl_o_d_param = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_param;
    s->u_reg_u_rsp_intg_gen_tl_o_d_size = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_size;
    s->u_reg_u_rsp_intg_gen_tl_o_d_source = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_rsp_intg_gen_tl_o_d_sink = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_sink;
    s->u_reg_u_rsp_intg_gen_tl_o_d_data = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg;
    s->u_reg_u_rsp_intg_gen_tl_o_d_error = _qp_next_u_reg_u_rsp_intg_gen_tl_o_d_error;
    s->u_reg_u_rsp_intg_gen_tl_o_a_ready = _qp_next_u_reg_u_rsp_intg_gen_tl_o_a_ready;
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = _qp_next_u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = _qp_next_u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o;
    s->u_reg_u_reg_if_outstanding_q = _qp_next_u_reg_u_reg_if_outstanding_q;
    s->u_reg_u_reg_if_reqid_q = _qp_next_u_reg_u_reg_if_reqid_q;
    s->u_reg_u_reg_if_reqsz_q = _qp_next_u_reg_u_reg_if_reqsz_q;
    s->u_reg_u_reg_if_rspop_q = _qp_next_u_reg_u_reg_if_rspop_q;
    s->u_reg_u_reg_if_rdata_q = _qp_next_u_reg_u_reg_if_rdata_q;
    s->u_reg_u_reg_if_error_q = _qp_next_u_reg_u_reg_if_error_q;
    s->u_reg_u_reg_if_addr_align_err = _qp_next_u_reg_u_reg_if_addr_align_err;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error;
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = _qp_next_u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready;
    s->u_reg_u_reg_if_u_err_addr_sz_chk = _qp_next_u_reg_u_reg_if_u_err_addr_sz_chk;
    s->u_reg_u_reg_if_u_err_mask_chk = _qp_next_u_reg_u_reg_if_u_err_mask_chk;
    s->u_reg_u_reg_if_u_err_fulldata_chk = _qp_next_u_reg_u_reg_if_u_err_fulldata_chk;
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
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_d;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_pd;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_nd;
    s->gen_alert_tx_0_u_prim_alert_sender_ping_clr = _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_clr;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_clr = _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_clr;
    s->gen_alert_tx_0_u_prim_alert_sender_state_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_state_q;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_ping_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o = _qp_next_gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o;
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

/* Pulse reset to commit RESVALs into every prim_subreg. */
void pattgen_reset(pattgen_state *s)
{
    s->rst_ni = 0;
    update_state(s);
    tick(s);
    update_state(s);
    s->rst_ni = 1;
    update_state(s);
    tick(s);
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

    /* Inject QEMU address into the internal address signal */
    s->tl_i_a_address = (uint32_t)addr;

    /* TL-UL: assert request valid + Get opcode + always-ready response acceptor */
    s->tl_i_a_valid = 1;
    s->tl_i_d_ready = 1;
    s->tl_i_a_opcode = (uint8_t)4;  /* Get */

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
    {
        unsigned _qp_required_ticks = 0;
        unsigned _qp_seen_count = 0;
        QPSettleFingerprint _qp_seen[257];
        update_state(s);
        _qp_seen[_qp_seen_count++] = qp_settle_fingerprint(s);
        while (_qp_required_ticks < 256) {
            if (!tick(s))
                break;  /* sequential fixed point reached */
            ++_qp_required_ticks;
            update_state(s);

            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            bool _qp_repeated = false;
            for (unsigned _qp_i = 0; _qp_i < _qp_seen_count; ++_qp_i) {
                if (_qp_now.first == _qp_seen[_qp_i].first &&
                    _qp_now.second == _qp_seen[_qp_i].second) {
                    _qp_repeated = true;
                    break;
                }
            }
            if (_qp_repeated)
                break;  /* periodic state: no fixed point exists */
            _qp_seen[_qp_seen_count++] = _qp_now;
        }
    }
    update_state(s);

    /* De-assert TL-UL valid after the bus cycle settled */
    s->tl_i_a_valid = 0;

    return (uint64_t)s->u_reg_tl_o_d_data;
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

    /* Inject QEMU address into the internal address signal */
    s->tl_i_a_address = (uint32_t)addr;

    /* Inject QEMU write data into the internal wdata signal */
    s->tl_i_a_data = (uint32_t)value;

    /* Set write mask (byte-enable bits for the access size) */
    s->tl_i_a_mask = (1ULL << size) - 1;

    /* TL-UL: assert request valid + PutFullData opcode + always-ready response acceptor */
    s->tl_i_a_valid = 1;
    s->tl_i_d_ready = 1;
    s->tl_i_a_opcode = (uint8_t)0;  /* PutFullData */

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
    {
        unsigned _qp_required_ticks = 0;
        unsigned _qp_seen_count = 0;
        QPSettleFingerprint _qp_seen[257];
        update_state(s);
        _qp_seen[_qp_seen_count++] = qp_settle_fingerprint(s);
        while (_qp_required_ticks < 256) {
            if (!tick(s))
                break;  /* sequential fixed point reached */
            ++_qp_required_ticks;
            update_state(s);

            QPSettleFingerprint _qp_now = qp_settle_fingerprint(s);
            bool _qp_repeated = false;
            for (unsigned _qp_i = 0; _qp_i < _qp_seen_count; ++_qp_i) {
                if (_qp_now.first == _qp_seen[_qp_i].first &&
                    _qp_now.second == _qp_seen[_qp_i].second) {
                    _qp_repeated = true;
                    break;
                }
            }
            if (_qp_repeated)
                break;  /* periodic state: no fixed point exists */
            _qp_seen[_qp_seen_count++] = _qp_now;
        }
    }
    update_state(s);

    /* De-assert TL-UL valid after the bus cycle settled */
    s->tl_i_a_valid = 0;

}

static const MemoryRegionOps pattgen_ops = {
    .read  = pattgen_read,
    .write = pattgen_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 4,
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
    tick(s);
    update_state(s);
}

void pattgen_set_alert_rx_i_0__ping_n(pattgen_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_n = value;
    update_state(s);
    tick(s);
    update_state(s);
}

void pattgen_set_alert_rx_i_0__ack_p(pattgen_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_p = value;
    update_state(s);
    tick(s);
    update_state(s);
}

void pattgen_set_alert_rx_i_0__ack_n(pattgen_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_n = value;
    update_state(s);
    tick(s);
    update_state(s);
}

