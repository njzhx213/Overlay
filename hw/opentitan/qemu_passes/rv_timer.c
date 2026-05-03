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
#include "hw/ptimer.h"
#include "qemu/main-loop.h"
#include "rv_timer.h"

#pragma GCC diagnostic ignored "-Wbool-operation"
#pragma GCC diagnostic ignored "-Wshift-count-overflow"

/* Placeholder tick rate for ACCUMULATE counters.  Real frequency
 * recovery requires info that is stripped by clock-drv-removal;
 * this gives a best-effort "one decrement per virtual tick" rate. */
#define RV_TIMER_PTIMER_FREQ  1000000U

static void update_state(rv_timer_state *s);
static void tick(rv_timer_state *s);

/*
 * ptimer expiration callback for counter 'tick_count'.
 * Fires once when the counter reaches zero (or wraps, depending
 * on ptimer policy).  We raise the associated IRQ line.
 */
static void rv_timer_tick_gen_harts_0_u_core_tick_count(void *opaque)
{
    rv_timer_state *s = opaque;
    qemu_irq_raise(s->irq_gen_harts_0_u_core_tick_count);
}

/*
 * update_state() - Re-evaluate all combinational logic.
 *
 * Called after every MMIO write or read (for read-triggered outputs).
 * This is the "single-cycle evaluation" model: we assume all
 * combinational paths settle instantaneously (no multi-cycle handshake).
 */
static void update_state(rv_timer_state *s)
{
    s->active = s->u_reg_reg2hw_ctrl_0__q;
    s->hw2reg_timer_v_upper0_de = s->tick;
    s->hw2reg_timer_v_lower0_de = s->tick;
    s->hw2reg_timer_v_upper0_d = (((s->mtime_d_0_) >> 32) & 0xFFFFFFFFULL);
    s->hw2reg_timer_v_lower0_d = ((s->mtime_d_0_) & 0xFFFFFFFFULL);
    s->mtime_0_ = (((s->u_reg_reg2hw_timer_v_upper0_q) << 32) | (s->u_reg_reg2hw_timer_v_lower0_q));
    s->mtimecmp_update_0__0_ = (s->u_reg_reg2hw_compare_upper0_0_qe) | (s->u_reg_reg2hw_compare_lower0_0_qe);
    s->hw2reg_intr_state0_0__de = (s->gen_harts_0_u_intr_hw_hw2reg_intr_state_de_o) | (s->mtimecmp_update_0__0_);
    s->hw2reg_intr_state0_0__d = (s->intr_timer_state_d) & (((s->mtimecmp_update_0__0_) ^ 1));
    s->intr_timer_state_d = s->gen_harts_0_u_intr_hw_hw2reg_intr_state_d_o;
    s->intr_out = s->gen_harts_0_u_intr_hw_intr_o;
    s->tick = s->gen_harts_0_u_core_tick;
    s->mtime_d_0_ = s->gen_harts_0_u_core_mtime_d;
    s->intr_timer_set = s->gen_harts_0_u_core_intr;
    s->alerts = s->u_reg_intg_err_o;
    s->alert_tx_o_0__alert_p = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p;
    s->alert_tx_o_0__alert_n = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n;
    s->gen_harts_0_u_intr_hw_clk_i = s->clk_i;
    s->gen_harts_0_u_intr_hw_rst_ni = s->rst_ni;
    s->gen_harts_0_u_intr_hw_event_intr_i = s->intr_timer_set;
    s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i = s->u_reg_reg2hw_intr_enable0_0__q;
    s->gen_harts_0_u_intr_hw_reg2hw_intr_test_q_i = s->u_reg_reg2hw_intr_test0_0__q;
    s->gen_harts_0_u_intr_hw_reg2hw_intr_test_qe_i = s->u_reg_reg2hw_intr_test0_0__qe;
    s->gen_harts_0_u_intr_hw_reg2hw_intr_state_q_i = s->u_reg_reg2hw_intr_state0_0__q;
    s->gen_harts_0_u_intr_hw_status = s->gen_harts_0_u_intr_hw_reg2hw_intr_state_q_i;
    s->gen_harts_0_u_intr_hw_rst_ni = s->gen_harts_0_u_intr_hw_rst_ni;
    s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i = s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i;
    s->gen_harts_0_u_intr_hw_hw2reg_intr_state_de_o = ((((s->gen_harts_0_u_intr_hw_reg2hw_intr_test_qe_i) & (s->gen_harts_0_u_intr_hw_reg2hw_intr_test_q_i))) | (s->gen_harts_0_u_intr_hw_event_intr_i));
    s->gen_harts_0_u_intr_hw_hw2reg_intr_state_d_o = ((((((s->gen_harts_0_u_intr_hw_reg2hw_intr_test_qe_i) & (s->gen_harts_0_u_intr_hw_reg2hw_intr_test_q_i))) | (s->gen_harts_0_u_intr_hw_event_intr_i))) | (s->gen_harts_0_u_intr_hw_reg2hw_intr_state_q_i));
    s->gen_harts_0_u_intr_hw_intr_o = s->gen_harts_0_u_intr_hw_intr_o;
    s->gen_harts_0_u_core_clk_i = s->clk_i;
    s->gen_harts_0_u_core_rst_ni = s->rst_ni;
    s->gen_harts_0_u_core_active = s->active;
    s->gen_harts_0_u_core_prescaler = s->u_reg_reg2hw_cfg0_prescale_q;
    s->gen_harts_0_u_core_step = s->u_reg_reg2hw_cfg0_step_q;
    s->gen_harts_0_u_core_mtime = s->mtime_0_;
    s->gen_harts_0_u_core_intr = (s->gen_harts_0_u_core_active) & (((s->gen_harts_0_u_core_mtime) >= (s->gen_harts_0_u_core_mtimecmp_0_)));
    s->gen_harts_0_u_core_rst_ni = s->gen_harts_0_u_core_rst_ni;
    s->gen_harts_0_u_core_active = s->gen_harts_0_u_core_active;
    s->gen_harts_0_u_core_prescaler = s->gen_harts_0_u_core_prescaler;
    s->gen_harts_0_u_core_tick = ((s->gen_harts_0_u_core_active) & (((s->gen_harts_0_u_core_tick_count) >= (s->gen_harts_0_u_core_prescaler))));
    s->gen_harts_0_u_core_mtime_d = ((s->gen_harts_0_u_core_mtime) + (((((uint64_t)(s->gen_harts_0_u_core_step)) << 0) | (((uint64_t)(0)) << 8))));
    s->gen_harts_0_u_core_intr = s->gen_harts_0_u_core_intr;
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
    s->u_reg_hw2reg_intr_state0_0__d = s->hw2reg_intr_state0_0__d;
    s->u_reg_hw2reg_intr_state0_0__de = s->hw2reg_intr_state0_0__de;
    s->u_reg_hw2reg_timer_v_lower0_d = s->hw2reg_timer_v_lower0_d;
    s->u_reg_hw2reg_timer_v_lower0_de = s->hw2reg_timer_v_lower0_de;
    s->u_reg_hw2reg_timer_v_upper0_d = s->hw2reg_timer_v_upper0_d;
    s->u_reg_hw2reg_timer_v_upper0_de = s->hw2reg_timer_v_upper0_de;
    s->u_reg_racl_policies_i_0__write_perm = s->racl_policies_i__0__write_perm;
    s->u_reg_racl_policies_i_0__read_perm = s->racl_policies_i__0__read_perm;
    s->u_reg_intg_err = 0;
    s->u_reg_reg_we_err = s->u_reg_u_prim_reg_we_check_err_o;
    s->u_reg_reg_we = s->u_reg_u_reg_if_we_o;
    s->u_reg_reg_addr = s->u_reg_u_reg_if_addr_o;
    s->u_reg_reg_be = s->u_reg_u_reg_if_be_o;
    s->u_reg_alert_test_flds_we = s->u_reg_u_alert_test_qe;
    s->u_reg_reg2hw_alert_test_q = s->u_reg_u_alert_test_q;
    s->u_reg_reg2hw_alert_test_qe = s->u_reg_alert_test_flds_we;
    s->u_reg_reg2hw_ctrl_0__q = s->u_reg_u_ctrl_q;
    s->u_reg_ctrl_qs = s->u_reg_u_ctrl_qs;
    s->u_reg_reg2hw_intr_enable0_0__q = s->u_reg_u_intr_enable0_q;
    s->u_reg_intr_enable0_qs = s->u_reg_u_intr_enable0_qs;
    s->u_reg_reg2hw_intr_state0_0__q = s->u_reg_u_intr_state0_q;
    s->u_reg_intr_state0_qs = s->u_reg_u_intr_state0_qs;
    s->u_reg_intr_test0_flds_we = s->u_reg_u_intr_test0_qe;
    s->u_reg_reg2hw_intr_test0_0__q = s->u_reg_u_intr_test0_q;
    s->u_reg_reg2hw_intr_test0_0__qe = s->u_reg_intr_test0_flds_we;
    s->u_reg_reg2hw_cfg0_prescale_q = s->u_reg_u_cfg0_prescale_q;
    s->u_reg_cfg0_prescale_qs = s->u_reg_u_cfg0_prescale_qs;
    s->u_reg_reg2hw_cfg0_step_q = s->u_reg_u_cfg0_step_q;
    s->u_reg_cfg0_step_qs = s->u_reg_u_cfg0_step_qs;
    s->u_reg_reg2hw_timer_v_lower0_q = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_timer_v_lower0_qs = s->u_reg_u_timer_v_lower0_qs;
    s->u_reg_reg2hw_timer_v_upper0_q = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_timer_v_upper0_qs = s->u_reg_u_timer_v_upper0_qs;
    s->u_reg_compare_lower0_0_flds_we = s->u_reg_u_compare_lower0_0_qe;
    s->u_reg_reg2hw_compare_lower0_0_q = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_compare_lower0_0_qs = s->u_reg_u_compare_lower0_0_qs;
    s->u_reg_reg2hw_compare_lower0_0_qe = s->u_reg_u_compare_lower0_00_qe_q_o;
    s->u_reg_compare_upper0_0_flds_we = s->u_reg_u_compare_upper0_0_qe;
    s->u_reg_reg2hw_compare_upper0_0_q = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_compare_upper0_0_qs = s->u_reg_u_compare_upper0_0_qs;
    s->u_reg_reg2hw_compare_upper0_0_qe = s->u_reg_u_compare_upper0_00_qe_q_o;
    s->u_reg_racl_role_vec = 0;
    s->u_reg_racl_error_o_valid = (((s->u_reg_addr_hit) != (0))) & (((s->u_reg_u_reg_if_re_o) & (((s->u_reg_racl_addr_hit_read) == (0)))) | ((s->u_reg_reg_we) & (((s->u_reg_racl_addr_hit_write) == (0)))));
    s->u_reg_racl_error_o_request_address = (((0) << 9) | (s->u_reg_reg_addr));
    s->u_reg_racl_error_o_racl_role = 0;
    s->u_reg_racl_error_o_overflow = 0;
    s->u_reg_racl_error_o_ctn_uid = 0;
    s->u_reg_racl_error_o_read_access = 0;
    s->u_reg_alert_test_we = (((s->u_reg_racl_addr_hit_write) & 1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_ctrl_we = ((((s->u_reg_racl_addr_hit_write) >> 1) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_intr_enable0_we = ((((s->u_reg_racl_addr_hit_write) >> 2) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_intr_state0_we = ((((s->u_reg_racl_addr_hit_write) >> 3) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_intr_test0_we = ((((s->u_reg_racl_addr_hit_write) >> 4) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_cfg0_we = ((((s->u_reg_racl_addr_hit_write) >> 5) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_timer_v_lower0_we = ((((s->u_reg_racl_addr_hit_write) >> 6) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_timer_v_upper0_we = ((((s->u_reg_racl_addr_hit_write) >> 7) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_compare_lower0_0_we = ((((s->u_reg_racl_addr_hit_write) >> 8) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_compare_upper0_0_we = ((((s->u_reg_racl_addr_hit_write) >> 9) & 0x1)) & (s->u_reg_reg_we) & ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we)) & (((((s->u_reg_addr_hit) != (0))) ^ 1))) | (s->u_reg_wr_err) | (s->u_reg_intg_err)) ^ 1));
    s->u_reg_rst_ni = s->u_reg_rst_ni;
    s->u_reg_racl_policies_i_0__write_perm = s->u_reg_racl_policies_i_0__write_perm;
    s->u_reg_racl_policies_i_0__read_perm = s->u_reg_racl_policies_i_0__read_perm;
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
    s->u_reg_u_prim_reg_we_check_en_i = ((s->u_reg_reg_we) & (((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((((s->u_reg_addr_hit) != (0))) ^ (1))))) ^ (1))));
    s->u_reg_u_prim_reg_we_check_u_prim_buf_in_i = s->u_reg_u_prim_reg_we_check_oh_i;
    s->u_reg_u_prim_reg_we_check_u_prim_buf_out_o = s->u_reg_u_prim_reg_we_check_u_prim_buf_in_i;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_clk_i = s->u_reg_u_prim_reg_we_check_clk_i;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_rst_ni = s->u_reg_u_prim_reg_we_check_rst_ni;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i = s->u_reg_u_prim_reg_we_check_u_prim_buf_out_o;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i = 0;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_en_i = s->u_reg_u_prim_reg_we_check_en_i;
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x1ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 1) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 2) & 0x1))) & 0x1ULL) << 0);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x1ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 3) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 1) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 3) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 2) & 0x1)))) & 0x1ULL) << 0);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x1ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 2) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 1) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 2) & 0x1))) & 0x1ULL) << 0);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x2ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 3) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 4) & 0x1))) & 0x1ULL) << 1);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x2ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 3) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 4) & 0x1)))) & 0x1ULL) << 1);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x2ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 3) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 4) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 3) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 4) & 0x1))) & 0x1ULL) << 1);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x4ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 5) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 6) & 0x1))) & 0x1ULL) << 2);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x4ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 5) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 2) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 6) & 0x1)))) & 0x1ULL) << 2);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x4ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 5) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 6) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 5) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 6) & 0x1))) & 0x1ULL) << 2);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x8ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 7) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 8) & 0x1))) & 0x1ULL) << 3);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x8ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 7) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 8) & 0x1)))) & 0x1ULL) << 3);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x8ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 7) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 8) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 7) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 8) & 0x1))) & 0x1ULL) << 3);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x10ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 9) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 10) & 0x1))) & 0x1ULL) << 4);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x10ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 9) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 10) & 0x1)))) & 0x1ULL) << 4);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x10ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 9) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 10) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 9) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 10) & 0x1))) & 0x1ULL) << 4);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x20ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 11) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 12) & 0x1))) & 0x1ULL) << 5);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x20ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 11) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 12) & 0x1)))) & 0x1ULL) << 5);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x20ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 11) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 12) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 11) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 12) & 0x1))) & 0x1ULL) << 5);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x40ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 13) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 14) & 0x1))) & 0x1ULL) << 6);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x40ULL) | ((((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 13) & 0x1))) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) >> 1) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 14) & 0x1)))) & 0x1ULL) << 6);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x40ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 13) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 14) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 13) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 14) & 0x1))) & 0x1ULL) << 6);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x80ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 15) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 16) & 0x1))) & 0x1ULL) << 7);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x80ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 15) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 16) & 0x1)))) & 0x1ULL) << 7);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x80ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 15) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 16) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 15) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 16) & 0x1))) & 0x1ULL) << 7);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x100ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 17) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 18) & 0x1))) & 0x1ULL) << 8);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x100ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 17) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 18) & 0x1)))) & 0x1ULL) << 8);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x100ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 17) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 18) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 17) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 18) & 0x1))) & 0x1ULL) << 8);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x200ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 19) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 20) & 0x1))) & 0x1ULL) << 9);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x200ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 19) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 20) & 0x1)))) & 0x1ULL) << 9);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x200ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 19) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 20) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 19) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 20) & 0x1))) & 0x1ULL) << 9);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x400ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 21) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 22) & 0x1))) & 0x1ULL) << 10);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x400ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 21) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 22) & 0x1)))) & 0x1ULL) << 10);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x400ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 21) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 22) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 21) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 22) & 0x1))) & 0x1ULL) << 10);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x800ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 23) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 24) & 0x1))) & 0x1ULL) << 11);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x800ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 23) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 24) & 0x1)))) & 0x1ULL) << 11);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x800ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 23) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 24) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 23) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 24) & 0x1))) & 0x1ULL) << 11);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x1000ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 25) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 26) & 0x1))) & 0x1ULL) << 12);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x1000ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 25) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 26) & 0x1)))) & 0x1ULL) << 12);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x1000ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 25) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 26) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 25) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 26) & 0x1))) & 0x1ULL) << 12);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x2000ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 27) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 28) & 0x1))) & 0x1ULL) << 13);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x2000ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 27) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 28) & 0x1)))) & 0x1ULL) << 13);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x2000ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 27) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 28) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 27) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 28) & 0x1))) & 0x1ULL) << 13);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x4000ULL) | (((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 29) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 30) & 0x1))) & 0x1ULL) << 14);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x4000ULL) | (((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) ^ 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 29) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_addr_i) & 1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree) >> 30) & 0x1)))) & 0x1ULL) << 14);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x4000ULL) | ((((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 29) & 0x1)) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 30) & 0x1))) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 29) & 0x1)) | ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 30) & 0x1))) & 0x1ULL) << 14);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x8000ULL) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) & 1)) & 0x1ULL) << 15);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x8000ULL) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) & 1)) & 0x1ULL) << 15);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x8000ULL) | (((0) & 0x1ULL) << 15);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x10000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 1) & 0x1)) & 0x1ULL) << 16);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x10000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 1) & 0x1)) & 0x1ULL) << 16);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x10000ULL) | (((0) & 0x1ULL) << 16);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x20000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 2) & 0x1)) & 0x1ULL) << 17);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x20000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 2) & 0x1)) & 0x1ULL) << 17);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x20000ULL) | (((0) & 0x1ULL) << 17);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x40000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 3) & 0x1)) & 0x1ULL) << 18);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x40000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 3) & 0x1)) & 0x1ULL) << 18);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x40000ULL) | (((0) & 0x1ULL) << 18);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x80000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 4) & 0x1)) & 0x1ULL) << 19);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x80000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 4) & 0x1)) & 0x1ULL) << 19);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x80000ULL) | (((0) & 0x1ULL) << 19);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x100000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 5) & 0x1)) & 0x1ULL) << 20);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x100000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 5) & 0x1)) & 0x1ULL) << 20);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x100000ULL) | (((0) & 0x1ULL) << 20);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x200000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 6) & 0x1)) & 0x1ULL) << 21);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x200000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 6) & 0x1)) & 0x1ULL) << 21);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x200000ULL) | (((0) & 0x1ULL) << 21);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x400000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 7) & 0x1)) & 0x1ULL) << 22);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x400000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 7) & 0x1)) & 0x1ULL) << 22);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x400000ULL) | (((0) & 0x1ULL) << 22);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x800000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 8) & 0x1)) & 0x1ULL) << 23);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x800000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 8) & 0x1)) & 0x1ULL) << 23);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x800000ULL) | (((0) & 0x1ULL) << 23);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x1000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 9) & 0x1)) & 0x1ULL) << 24);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x1000000ULL) | ((((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_oh_i) >> 9) & 0x1)) & 0x1ULL) << 24);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x1000000ULL) | (((0) & 0x1ULL) << 24);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x2000000ULL) | (((0) & 0x1ULL) << 25);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x2000000ULL) | (((0) & 0x1ULL) << 25);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x2000000ULL) | (((0) & 0x1ULL) << 25);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x4000000ULL) | (((0) & 0x1ULL) << 26);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x4000000ULL) | (((0) & 0x1ULL) << 26);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x4000000ULL) | (((0) & 0x1ULL) << 26);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x8000000ULL) | (((0) & 0x1ULL) << 27);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x8000000ULL) | (((0) & 0x1ULL) << 27);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x8000000ULL) | (((0) & 0x1ULL) << 27);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x10000000ULL) | (((0) & 0x1ULL) << 28);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x10000000ULL) | (((0) & 0x1ULL) << 28);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x10000000ULL) | (((0) & 0x1ULL) << 28);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x20000000ULL) | (((0) & 0x1ULL) << 29);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x20000000ULL) | (((0) & 0x1ULL) << 29);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x20000000ULL) | (((0) & 0x1ULL) << 29);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree & ~0x40000000ULL) | (((0) & 0x1ULL) << 30);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_and_tree & ~0x40000000ULL) | (((0) & 0x1ULL) << 30);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree = (s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree & ~0x40000000ULL) | (((0) & 0x1ULL) << 30);
    s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_o = (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_tree) >> 0) & 0x1)) | (((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_en_i) ^ (1))) & ((((s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_or_tree) >> 0) & 0x1)))));
    s->u_reg_u_prim_reg_we_check_err_o = s->u_reg_u_prim_reg_we_check_u_prim_onehot_check_err_o;
    s->u_reg_u_rsp_intg_gen_tl_i_d_valid = s->u_reg_u_reg_if_tl_o_d_valid;
    s->u_reg_u_rsp_intg_gen_tl_i_d_opcode = s->u_reg_u_reg_if_tl_o_d_opcode;
    s->u_reg_u_rsp_intg_gen_tl_i_d_param = s->u_reg_u_reg_if_tl_o_d_param;
    s->u_reg_u_rsp_intg_gen_tl_i_d_size = s->u_reg_u_reg_if_tl_o_d_size;
    s->u_reg_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_reg_if_tl_o_d_source;
    s->u_reg_u_rsp_intg_gen_tl_i_d_sink = s->u_reg_u_reg_if_tl_o_d_sink;
    s->u_reg_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_reg_if_tl_o_d_data;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg = s->u_reg_u_reg_if_tl_o_d_user_rsp_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg = s->u_reg_u_reg_if_tl_o_d_user_data_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_error = s->u_reg_u_reg_if_tl_o_d_error;
    s->u_reg_u_rsp_intg_gen_tl_i_a_ready = s->u_reg_u_reg_if_tl_o_a_ready;
    s->u_reg_u_rsp_intg_gen_rsp_intg = (((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 57) & 0x7F);
    s->u_reg_u_rsp_intg_gen_data_intg = (((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o) >> 32) & 0x7F);
    s->u_reg_u_rsp_intg_gen_tl_i_d_valid = s->u_reg_u_rsp_intg_gen_tl_i_d_valid;
    s->u_reg_u_rsp_intg_gen_tl_i_d_opcode = s->u_reg_u_rsp_intg_gen_tl_i_d_opcode;
    s->u_reg_u_rsp_intg_gen_tl_i_d_param = s->u_reg_u_rsp_intg_gen_tl_i_d_param;
    s->u_reg_u_rsp_intg_gen_tl_i_d_size = s->u_reg_u_rsp_intg_gen_tl_i_d_size;
    s->u_reg_u_rsp_intg_gen_tl_i_d_source = s->u_reg_u_rsp_intg_gen_tl_i_d_source;
    s->u_reg_u_rsp_intg_gen_tl_i_d_sink = s->u_reg_u_rsp_intg_gen_tl_i_d_sink;
    s->u_reg_u_rsp_intg_gen_tl_i_d_data = s->u_reg_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg = s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg = s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg;
    s->u_reg_u_rsp_intg_gen_tl_i_d_error = s->u_reg_u_rsp_intg_gen_tl_i_d_error;
    s->u_reg_u_rsp_intg_gen_tl_i_a_ready = s->u_reg_u_rsp_intg_gen_tl_i_a_ready;
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i;
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i = s->u_reg_u_rsp_intg_gen_tl_i_d_data;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_i;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o;
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_data_intg_o = s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o;
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
    s->u_reg_u_reg_if_error_i = ((((((s->u_reg_u_reg_if_re_o) | (s->u_reg_reg_we))) & (((((s->u_reg_addr_hit) != (0))) ^ (1))))) | (s->u_reg_wr_err) | (s->u_reg_intg_err));
    s->u_reg_u_reg_if_a_ack = (s->u_reg_u_reg_if_tl_i_a_valid) & (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready);
    s->u_reg_u_reg_if_d_ack = (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid) & (s->u_reg_u_reg_if_tl_i_d_ready);
    s->u_reg_u_reg_if_wr_req = (s->u_reg_u_reg_if_a_ack) & ((((s->u_reg_u_reg_if_tl_i_a_opcode) == (0))) | (((s->u_reg_u_reg_if_tl_i_a_opcode) == (1))));
    s->u_reg_u_reg_if_rd_req = (s->u_reg_u_reg_if_a_ack) & (((s->u_reg_u_reg_if_tl_i_a_opcode) == (4)));
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
    s->u_reg_u_reg_if_error_i = s->u_reg_u_reg_if_error_i;
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
    s->u_reg_u_reg_if_u_err_mask = (1) << ((((0) << 2) | (((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 0x3))));
    s->u_reg_u_reg_if_u_err_tl_i_a_valid = s->u_reg_u_reg_if_u_err_tl_i_a_valid;
    s->u_reg_u_reg_if_u_err_tl_i_a_opcode = s->u_reg_u_reg_if_u_err_tl_i_a_opcode;
    s->u_reg_u_reg_if_u_err_tl_i_a_param = s->u_reg_u_reg_if_u_err_tl_i_a_param;
    s->u_reg_u_reg_if_u_err_tl_i_a_size = s->u_reg_u_reg_if_u_err_tl_i_a_size;
    s->u_reg_u_reg_if_u_err_tl_i_a_source = s->u_reg_u_reg_if_u_err_tl_i_a_source;
    s->u_reg_u_reg_if_u_err_tl_i_a_address = s->u_reg_u_reg_if_u_err_tl_i_a_address;
    s->u_reg_u_reg_if_u_err_tl_i_a_mask = s->u_reg_u_reg_if_u_err_tl_i_a_mask;
    s->u_reg_u_reg_if_u_err_tl_i_a_data = s->u_reg_u_reg_if_u_err_tl_i_a_data;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd = s->u_reg_u_reg_if_u_err_tl_i_a_user_rsvd;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type = s->u_reg_u_reg_if_u_err_tl_i_a_user_instr_type;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg = s->u_reg_u_reg_if_u_err_tl_i_a_user_cmd_intg;
    s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg = s->u_reg_u_reg_if_u_err_tl_i_a_user_data_intg;
    s->u_reg_u_reg_if_u_err_tl_i_d_ready = s->u_reg_u_reg_if_u_err_tl_i_d_ready;
    s->u_reg_u_reg_if_tl_o_d_valid = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid;
    s->u_reg_u_reg_if_tl_o_d_opcode = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode;
    s->u_reg_u_reg_if_tl_o_d_param = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param;
    s->u_reg_u_reg_if_tl_o_d_size = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size;
    s->u_reg_u_reg_if_tl_o_d_source = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source;
    s->u_reg_u_reg_if_tl_o_d_sink = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink;
    s->u_reg_u_reg_if_tl_o_d_data = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data;
    s->u_reg_u_reg_if_tl_o_d_user_rsp_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg;
    s->u_reg_u_reg_if_tl_o_d_user_data_intg = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg;
    s->u_reg_u_reg_if_tl_o_d_error = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error;
    s->u_reg_u_reg_if_tl_o_a_ready = s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready;
    s->u_reg_u_reg_if_intg_error_o = 0;
    s->u_reg_u_reg_if_re_o = ((s->u_reg_u_reg_if_rd_req) & (((s->u_reg_u_reg_if_err_internal) ^ (1))));
    s->u_reg_u_reg_if_we_o = ((s->u_reg_u_reg_if_wr_req) & (((s->u_reg_u_reg_if_err_internal) ^ (1))));
    s->u_reg_u_reg_if_addr_o = ((((uint64_t)(0)) << 0) | (((uint64_t)((((s->u_reg_u_reg_if_tl_i_a_address) >> 2) & 0x7F))) << 2));
    s->u_reg_u_reg_if_wdata_o = s->u_reg_u_reg_if_tl_i_a_data;
    s->u_reg_u_reg_if_be_o = s->u_reg_u_reg_if_tl_i_a_mask;
    s->u_reg_u_alert_test_re = 0;
    s->u_reg_u_alert_test_we = s->u_reg_alert_test_we;
    s->u_reg_u_alert_test_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_alert_test_d = 0;
    s->u_reg_u_alert_test_qe = s->u_reg_u_alert_test_we;
    s->u_reg_u_alert_test_qre = s->u_reg_u_alert_test_re;
    s->u_reg_u_alert_test_q = s->u_reg_u_alert_test_wd;
    s->u_reg_u_alert_test_ds = s->u_reg_u_alert_test_d;
    s->u_reg_u_alert_test_qs = s->u_reg_u_alert_test_d;
    s->u_reg_u_ctrl_clk_i = s->u_reg_clk_i;
    s->u_reg_u_ctrl_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_ctrl_we = s->u_reg_ctrl_we;
    s->u_reg_u_ctrl_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_ctrl_de = 0;
    s->u_reg_u_ctrl_d = 0;
    s->u_reg_u_ctrl_wr_en = s->u_reg_u_ctrl_wr_en_data_arb_wr_en;
    s->u_reg_u_ctrl_wr_data = s->u_reg_u_ctrl_wr_en_data_arb_wr_data;
    s->u_reg_u_ctrl_qs = s->u_reg_u_ctrl_q;
    s->u_reg_u_ctrl_rst_ni = s->u_reg_u_ctrl_rst_ni;
    s->u_reg_u_ctrl_wr_en_data_arb_we = s->u_reg_u_ctrl_we;
    s->u_reg_u_ctrl_wr_en_data_arb_wd = s->u_reg_u_ctrl_wd;
    s->u_reg_u_ctrl_wr_en_data_arb_de = s->u_reg_u_ctrl_de;
    s->u_reg_u_ctrl_wr_en_data_arb_d = s->u_reg_u_ctrl_d;
    s->u_reg_u_ctrl_wr_en_data_arb_q = s->u_reg_u_ctrl_q;
    s->u_reg_u_ctrl_wr_en_data_arb_wd = s->u_reg_u_ctrl_wr_en_data_arb_wd;
    s->u_reg_u_ctrl_wr_en_data_arb_d = s->u_reg_u_ctrl_wr_en_data_arb_d;
    s->u_reg_u_ctrl_wr_en_data_arb_wr_en = ((s->u_reg_u_ctrl_wr_en_data_arb_we) | (s->u_reg_u_ctrl_wr_en_data_arb_de));
    s->u_reg_u_ctrl_wr_en_data_arb_wr_data = ((s->u_reg_u_ctrl_wr_en_data_arb_we) ? (s->u_reg_u_ctrl_wr_en_data_arb_wd) : (s->u_reg_u_ctrl_wr_en_data_arb_d));
    s->u_reg_u_ctrl_qe = s->u_reg_u_ctrl_we;
    s->u_reg_u_ctrl_q = s->u_reg_u_ctrl_q;
    s->u_reg_u_ctrl_ds = ((s->u_reg_u_ctrl_wr_en) ? (s->u_reg_u_ctrl_wr_data) : (s->u_reg_u_ctrl_qs));
    s->u_reg_u_ctrl_qs = s->u_reg_u_ctrl_qs;
    s->u_reg_u_intr_enable0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_intr_enable0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_intr_enable0_we = s->u_reg_intr_enable0_we;
    s->u_reg_u_intr_enable0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_intr_enable0_de = 0;
    s->u_reg_u_intr_enable0_d = 0;
    s->u_reg_u_intr_enable0_wr_en = s->u_reg_u_intr_enable0_wr_en_data_arb_wr_en;
    s->u_reg_u_intr_enable0_wr_data = s->u_reg_u_intr_enable0_wr_en_data_arb_wr_data;
    s->u_reg_u_intr_enable0_qs = s->u_reg_u_intr_enable0_q;
    s->u_reg_u_intr_enable0_rst_ni = s->u_reg_u_intr_enable0_rst_ni;
    s->u_reg_u_intr_enable0_wr_en_data_arb_we = s->u_reg_u_intr_enable0_we;
    s->u_reg_u_intr_enable0_wr_en_data_arb_wd = s->u_reg_u_intr_enable0_wd;
    s->u_reg_u_intr_enable0_wr_en_data_arb_de = s->u_reg_u_intr_enable0_de;
    s->u_reg_u_intr_enable0_wr_en_data_arb_d = s->u_reg_u_intr_enable0_d;
    s->u_reg_u_intr_enable0_wr_en_data_arb_q = s->u_reg_u_intr_enable0_q;
    s->u_reg_u_intr_enable0_wr_en_data_arb_wd = s->u_reg_u_intr_enable0_wr_en_data_arb_wd;
    s->u_reg_u_intr_enable0_wr_en_data_arb_d = s->u_reg_u_intr_enable0_wr_en_data_arb_d;
    s->u_reg_u_intr_enable0_wr_en_data_arb_wr_en = ((s->u_reg_u_intr_enable0_wr_en_data_arb_we) | (s->u_reg_u_intr_enable0_wr_en_data_arb_de));
    s->u_reg_u_intr_enable0_wr_en_data_arb_wr_data = ((s->u_reg_u_intr_enable0_wr_en_data_arb_we) ? (s->u_reg_u_intr_enable0_wr_en_data_arb_wd) : (s->u_reg_u_intr_enable0_wr_en_data_arb_d));
    s->u_reg_u_intr_enable0_qe = s->u_reg_u_intr_enable0_we;
    s->u_reg_u_intr_enable0_q = s->u_reg_u_intr_enable0_q;
    s->u_reg_u_intr_enable0_ds = ((s->u_reg_u_intr_enable0_wr_en) ? (s->u_reg_u_intr_enable0_wr_data) : (s->u_reg_u_intr_enable0_qs));
    s->u_reg_u_intr_enable0_qs = s->u_reg_u_intr_enable0_qs;
    s->u_reg_u_intr_state0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_intr_state0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_intr_state0_we = s->u_reg_intr_state0_we;
    s->u_reg_u_intr_state0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_intr_state0_de = s->u_reg_hw2reg_intr_state0_0__de;
    s->u_reg_u_intr_state0_d = s->u_reg_hw2reg_intr_state0_0__d;
    s->u_reg_u_intr_state0_wr_en = s->u_reg_u_intr_state0_wr_en_data_arb_wr_en;
    s->u_reg_u_intr_state0_wr_data = s->u_reg_u_intr_state0_wr_en_data_arb_wr_data;
    s->u_reg_u_intr_state0_qs = s->u_reg_u_intr_state0_q;
    s->u_reg_u_intr_state0_rst_ni = s->u_reg_u_intr_state0_rst_ni;
    s->u_reg_u_intr_state0_wr_en_data_arb_we = s->u_reg_u_intr_state0_we;
    s->u_reg_u_intr_state0_wr_en_data_arb_wd = s->u_reg_u_intr_state0_wd;
    s->u_reg_u_intr_state0_wr_en_data_arb_de = s->u_reg_u_intr_state0_de;
    s->u_reg_u_intr_state0_wr_en_data_arb_d = s->u_reg_u_intr_state0_d;
    s->u_reg_u_intr_state0_wr_en_data_arb_q = s->u_reg_u_intr_state0_q;
    s->u_reg_u_intr_state0_wr_en_data_arb_wd = s->u_reg_u_intr_state0_wr_en_data_arb_wd;
    s->u_reg_u_intr_state0_wr_en_data_arb_d = s->u_reg_u_intr_state0_wr_en_data_arb_d;
    s->u_reg_u_intr_state0_wr_en_data_arb_q = s->u_reg_u_intr_state0_wr_en_data_arb_q;
    s->u_reg_u_intr_state0_wr_en_data_arb_wr_en = ((s->u_reg_u_intr_state0_wr_en_data_arb_we) | (s->u_reg_u_intr_state0_wr_en_data_arb_de));
    s->u_reg_u_intr_state0_wr_en_data_arb_wr_data = ((((s->u_reg_u_intr_state0_wr_en_data_arb_de) ? (s->u_reg_u_intr_state0_wr_en_data_arb_d) : (s->u_reg_u_intr_state0_wr_en_data_arb_q))) & (((((s->u_reg_u_intr_state0_wr_en_data_arb_we) ^ (1))) | (((s->u_reg_u_intr_state0_wr_en_data_arb_wd) ^ (1))))));
    s->u_reg_u_intr_state0_qe = s->u_reg_u_intr_state0_we;
    s->u_reg_u_intr_state0_q = s->u_reg_u_intr_state0_q;
    s->u_reg_u_intr_state0_ds = ((s->u_reg_u_intr_state0_wr_en) ? (s->u_reg_u_intr_state0_wr_data) : (s->u_reg_u_intr_state0_qs));
    s->u_reg_u_intr_state0_qs = s->u_reg_u_intr_state0_qs;
    s->u_reg_u_intr_test0_re = 0;
    s->u_reg_u_intr_test0_we = s->u_reg_intr_test0_we;
    s->u_reg_u_intr_test0_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0x1);
    s->u_reg_u_intr_test0_d = 0;
    s->u_reg_u_intr_test0_qe = s->u_reg_u_intr_test0_we;
    s->u_reg_u_intr_test0_qre = s->u_reg_u_intr_test0_re;
    s->u_reg_u_intr_test0_q = s->u_reg_u_intr_test0_wd;
    s->u_reg_u_intr_test0_ds = s->u_reg_u_intr_test0_d;
    s->u_reg_u_intr_test0_qs = s->u_reg_u_intr_test0_d;
    s->u_reg_u_cfg0_prescale_clk_i = s->u_reg_clk_i;
    s->u_reg_u_cfg0_prescale_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_cfg0_prescale_we = s->u_reg_cfg0_we;
    s->u_reg_u_cfg0_prescale_wd = (((s->u_reg_u_reg_if_wdata_o) >> 0) & 0xFFF);
    s->u_reg_u_cfg0_prescale_de = 0;
    s->u_reg_u_cfg0_prescale_d = 0;
    s->u_reg_u_cfg0_prescale_wr_en = s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_en;
    s->u_reg_u_cfg0_prescale_wr_data = s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_data;
    s->u_reg_u_cfg0_prescale_qs = s->u_reg_u_cfg0_prescale_q;
    s->u_reg_u_cfg0_prescale_rst_ni = s->u_reg_u_cfg0_prescale_rst_ni;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_we = s->u_reg_u_cfg0_prescale_we;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd = s->u_reg_u_cfg0_prescale_wd;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_de = s->u_reg_u_cfg0_prescale_de;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_d = s->u_reg_u_cfg0_prescale_d;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_q = s->u_reg_u_cfg0_prescale_q;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd = s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_d = s->u_reg_u_cfg0_prescale_wr_en_data_arb_d;
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_en = ((s->u_reg_u_cfg0_prescale_wr_en_data_arb_we) | (s->u_reg_u_cfg0_prescale_wr_en_data_arb_de));
    s->u_reg_u_cfg0_prescale_wr_en_data_arb_wr_data = ((s->u_reg_u_cfg0_prescale_wr_en_data_arb_we) ? (s->u_reg_u_cfg0_prescale_wr_en_data_arb_wd) : (s->u_reg_u_cfg0_prescale_wr_en_data_arb_d));
    s->u_reg_u_cfg0_prescale_qe = s->u_reg_u_cfg0_prescale_we;
    s->u_reg_u_cfg0_prescale_q = s->u_reg_u_cfg0_prescale_q;
    s->u_reg_u_cfg0_prescale_ds = ((s->u_reg_u_cfg0_prescale_wr_en) ? (s->u_reg_u_cfg0_prescale_wr_data) : (s->u_reg_u_cfg0_prescale_qs));
    s->u_reg_u_cfg0_prescale_qs = s->u_reg_u_cfg0_prescale_qs;
    s->u_reg_u_cfg0_step_clk_i = s->u_reg_clk_i;
    s->u_reg_u_cfg0_step_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_cfg0_step_we = s->u_reg_cfg0_we;
    s->u_reg_u_cfg0_step_wd = (((s->u_reg_u_reg_if_wdata_o) >> 16) & 0xFF);
    s->u_reg_u_cfg0_step_de = 0;
    s->u_reg_u_cfg0_step_d = 0;
    s->u_reg_u_cfg0_step_wr_en = s->u_reg_u_cfg0_step_wr_en_data_arb_wr_en;
    s->u_reg_u_cfg0_step_wr_data = s->u_reg_u_cfg0_step_wr_en_data_arb_wr_data;
    s->u_reg_u_cfg0_step_qs = s->u_reg_u_cfg0_step_q;
    s->u_reg_u_cfg0_step_rst_ni = s->u_reg_u_cfg0_step_rst_ni;
    s->u_reg_u_cfg0_step_wr_en_data_arb_we = s->u_reg_u_cfg0_step_we;
    s->u_reg_u_cfg0_step_wr_en_data_arb_wd = s->u_reg_u_cfg0_step_wd;
    s->u_reg_u_cfg0_step_wr_en_data_arb_de = s->u_reg_u_cfg0_step_de;
    s->u_reg_u_cfg0_step_wr_en_data_arb_d = s->u_reg_u_cfg0_step_d;
    s->u_reg_u_cfg0_step_wr_en_data_arb_q = s->u_reg_u_cfg0_step_q;
    s->u_reg_u_cfg0_step_wr_en_data_arb_wd = s->u_reg_u_cfg0_step_wr_en_data_arb_wd;
    s->u_reg_u_cfg0_step_wr_en_data_arb_d = s->u_reg_u_cfg0_step_wr_en_data_arb_d;
    s->u_reg_u_cfg0_step_wr_en_data_arb_wr_en = ((s->u_reg_u_cfg0_step_wr_en_data_arb_we) | (s->u_reg_u_cfg0_step_wr_en_data_arb_de));
    s->u_reg_u_cfg0_step_wr_en_data_arb_wr_data = ((s->u_reg_u_cfg0_step_wr_en_data_arb_we) ? (s->u_reg_u_cfg0_step_wr_en_data_arb_wd) : (s->u_reg_u_cfg0_step_wr_en_data_arb_d));
    s->u_reg_u_cfg0_step_qe = s->u_reg_u_cfg0_step_we;
    s->u_reg_u_cfg0_step_q = s->u_reg_u_cfg0_step_q;
    s->u_reg_u_cfg0_step_ds = ((s->u_reg_u_cfg0_step_wr_en) ? (s->u_reg_u_cfg0_step_wr_data) : (s->u_reg_u_cfg0_step_qs));
    s->u_reg_u_cfg0_step_qs = s->u_reg_u_cfg0_step_qs;
    s->u_reg_u_timer_v_lower0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_timer_v_lower0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_timer_v_lower0_we = s->u_reg_timer_v_lower0_we;
    s->u_reg_u_timer_v_lower0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_timer_v_lower0_de = s->u_reg_hw2reg_timer_v_lower0_de;
    s->u_reg_u_timer_v_lower0_d = s->u_reg_hw2reg_timer_v_lower0_d;
    s->u_reg_u_timer_v_lower0_wr_en = s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_en;
    s->u_reg_u_timer_v_lower0_wr_data = s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_data;
    s->u_reg_u_timer_v_lower0_qs = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_u_timer_v_lower0_rst_ni = s->u_reg_u_timer_v_lower0_rst_ni;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_we = s->u_reg_u_timer_v_lower0_we;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd = s->u_reg_u_timer_v_lower0_wd;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_de = s->u_reg_u_timer_v_lower0_de;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_d = s->u_reg_u_timer_v_lower0_d;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_q = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd = s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_d = s->u_reg_u_timer_v_lower0_wr_en_data_arb_d;
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_en = ((s->u_reg_u_timer_v_lower0_wr_en_data_arb_we) | (s->u_reg_u_timer_v_lower0_wr_en_data_arb_de));
    s->u_reg_u_timer_v_lower0_wr_en_data_arb_wr_data = ((s->u_reg_u_timer_v_lower0_wr_en_data_arb_we) ? (s->u_reg_u_timer_v_lower0_wr_en_data_arb_wd) : (s->u_reg_u_timer_v_lower0_wr_en_data_arb_d));
    s->u_reg_u_timer_v_lower0_qe = s->u_reg_u_timer_v_lower0_we;
    s->u_reg_u_timer_v_lower0_q = s->u_reg_u_timer_v_lower0_q;
    s->u_reg_u_timer_v_lower0_ds = ((s->u_reg_u_timer_v_lower0_wr_en) ? (s->u_reg_u_timer_v_lower0_wr_data) : (s->u_reg_u_timer_v_lower0_qs));
    s->u_reg_u_timer_v_lower0_qs = s->u_reg_u_timer_v_lower0_qs;
    s->u_reg_u_timer_v_upper0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_timer_v_upper0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_timer_v_upper0_we = s->u_reg_timer_v_upper0_we;
    s->u_reg_u_timer_v_upper0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_timer_v_upper0_de = s->u_reg_hw2reg_timer_v_upper0_de;
    s->u_reg_u_timer_v_upper0_d = s->u_reg_hw2reg_timer_v_upper0_d;
    s->u_reg_u_timer_v_upper0_wr_en = s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_en;
    s->u_reg_u_timer_v_upper0_wr_data = s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_data;
    s->u_reg_u_timer_v_upper0_qs = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_u_timer_v_upper0_rst_ni = s->u_reg_u_timer_v_upper0_rst_ni;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_we = s->u_reg_u_timer_v_upper0_we;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd = s->u_reg_u_timer_v_upper0_wd;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_de = s->u_reg_u_timer_v_upper0_de;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_d = s->u_reg_u_timer_v_upper0_d;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_q = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd = s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_d = s->u_reg_u_timer_v_upper0_wr_en_data_arb_d;
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_en = ((s->u_reg_u_timer_v_upper0_wr_en_data_arb_we) | (s->u_reg_u_timer_v_upper0_wr_en_data_arb_de));
    s->u_reg_u_timer_v_upper0_wr_en_data_arb_wr_data = ((s->u_reg_u_timer_v_upper0_wr_en_data_arb_we) ? (s->u_reg_u_timer_v_upper0_wr_en_data_arb_wd) : (s->u_reg_u_timer_v_upper0_wr_en_data_arb_d));
    s->u_reg_u_timer_v_upper0_qe = s->u_reg_u_timer_v_upper0_we;
    s->u_reg_u_timer_v_upper0_q = s->u_reg_u_timer_v_upper0_q;
    s->u_reg_u_timer_v_upper0_ds = ((s->u_reg_u_timer_v_upper0_wr_en) ? (s->u_reg_u_timer_v_upper0_wr_data) : (s->u_reg_u_timer_v_upper0_qs));
    s->u_reg_u_timer_v_upper0_qs = s->u_reg_u_timer_v_upper0_qs;
    s->u_reg_u_compare_lower0_00_qe_clk_i = s->u_reg_clk_i;
    s->u_reg_u_compare_lower0_00_qe_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_compare_lower0_00_qe_d_i = s->u_reg_compare_lower0_0_flds_we;
    s->u_reg_u_compare_lower0_00_qe_rst_ni = s->u_reg_u_compare_lower0_00_qe_rst_ni;
    s->u_reg_u_compare_lower0_00_qe_d_i = s->u_reg_u_compare_lower0_00_qe_d_i;
    s->u_reg_u_compare_lower0_00_qe_q_o = s->u_reg_u_compare_lower0_00_qe_q_o;
    s->u_reg_u_compare_lower0_0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_compare_lower0_0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_compare_lower0_0_we = s->u_reg_compare_lower0_0_we;
    s->u_reg_u_compare_lower0_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_compare_lower0_0_de = 0;
    s->u_reg_u_compare_lower0_0_d = 0;
    s->u_reg_u_compare_lower0_0_wr_en = s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_en;
    s->u_reg_u_compare_lower0_0_wr_data = s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_data;
    s->u_reg_u_compare_lower0_0_qs = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_u_compare_lower0_0_rst_ni = s->u_reg_u_compare_lower0_0_rst_ni;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_we = s->u_reg_u_compare_lower0_0_we;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd = s->u_reg_u_compare_lower0_0_wd;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_de = s->u_reg_u_compare_lower0_0_de;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_d = s->u_reg_u_compare_lower0_0_d;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_q = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd = s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_d = s->u_reg_u_compare_lower0_0_wr_en_data_arb_d;
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_en = ((s->u_reg_u_compare_lower0_0_wr_en_data_arb_we) | (s->u_reg_u_compare_lower0_0_wr_en_data_arb_de));
    s->u_reg_u_compare_lower0_0_wr_en_data_arb_wr_data = ((s->u_reg_u_compare_lower0_0_wr_en_data_arb_we) ? (s->u_reg_u_compare_lower0_0_wr_en_data_arb_wd) : (s->u_reg_u_compare_lower0_0_wr_en_data_arb_d));
    s->u_reg_u_compare_lower0_0_qe = s->u_reg_u_compare_lower0_0_we;
    s->u_reg_u_compare_lower0_0_q = s->u_reg_u_compare_lower0_0_q;
    s->u_reg_u_compare_lower0_0_ds = ((s->u_reg_u_compare_lower0_0_wr_en) ? (s->u_reg_u_compare_lower0_0_wr_data) : (s->u_reg_u_compare_lower0_0_qs));
    s->u_reg_u_compare_lower0_0_qs = s->u_reg_u_compare_lower0_0_qs;
    s->u_reg_u_compare_upper0_00_qe_clk_i = s->u_reg_clk_i;
    s->u_reg_u_compare_upper0_00_qe_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_compare_upper0_00_qe_d_i = s->u_reg_compare_upper0_0_flds_we;
    s->u_reg_u_compare_upper0_00_qe_rst_ni = s->u_reg_u_compare_upper0_00_qe_rst_ni;
    s->u_reg_u_compare_upper0_00_qe_d_i = s->u_reg_u_compare_upper0_00_qe_d_i;
    s->u_reg_u_compare_upper0_00_qe_q_o = s->u_reg_u_compare_upper0_00_qe_q_o;
    s->u_reg_u_compare_upper0_0_clk_i = s->u_reg_clk_i;
    s->u_reg_u_compare_upper0_0_rst_ni = s->u_reg_rst_ni;
    s->u_reg_u_compare_upper0_0_we = s->u_reg_compare_upper0_0_we;
    s->u_reg_u_compare_upper0_0_wd = s->u_reg_u_reg_if_wdata_o;
    s->u_reg_u_compare_upper0_0_de = 0;
    s->u_reg_u_compare_upper0_0_d = 0;
    s->u_reg_u_compare_upper0_0_wr_en = s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_en;
    s->u_reg_u_compare_upper0_0_wr_data = s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_data;
    s->u_reg_u_compare_upper0_0_qs = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_u_compare_upper0_0_rst_ni = s->u_reg_u_compare_upper0_0_rst_ni;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_we = s->u_reg_u_compare_upper0_0_we;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd = s->u_reg_u_compare_upper0_0_wd;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_de = s->u_reg_u_compare_upper0_0_de;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_d = s->u_reg_u_compare_upper0_0_d;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_q = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd = s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_d = s->u_reg_u_compare_upper0_0_wr_en_data_arb_d;
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_en = ((s->u_reg_u_compare_upper0_0_wr_en_data_arb_we) | (s->u_reg_u_compare_upper0_0_wr_en_data_arb_de));
    s->u_reg_u_compare_upper0_0_wr_en_data_arb_wr_data = ((s->u_reg_u_compare_upper0_0_wr_en_data_arb_we) ? (s->u_reg_u_compare_upper0_0_wr_en_data_arb_wd) : (s->u_reg_u_compare_upper0_0_wr_en_data_arb_d));
    s->u_reg_u_compare_upper0_0_qe = s->u_reg_u_compare_upper0_0_we;
    s->u_reg_u_compare_upper0_0_q = s->u_reg_u_compare_upper0_0_q;
    s->u_reg_u_compare_upper0_0_ds = ((s->u_reg_u_compare_upper0_0_wr_en) ? (s->u_reg_u_compare_upper0_0_wr_data) : (s->u_reg_u_compare_upper0_0_qs));
    s->u_reg_u_compare_upper0_0_qs = s->u_reg_u_compare_upper0_0_qs;
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
    s->u_reg_racl_error_o_valid = s->u_reg_racl_error_o_valid;
    s->u_reg_racl_error_o_overflow = s->u_reg_racl_error_o_overflow;
    s->u_reg_racl_error_o_racl_role = s->u_reg_racl_error_o_racl_role;
    s->u_reg_racl_error_o_ctn_uid = s->u_reg_racl_error_o_ctn_uid;
    s->u_reg_racl_error_o_read_access = s->u_reg_racl_error_o_read_access;
    s->u_reg_racl_error_o_request_address = s->u_reg_racl_error_o_request_address;
    s->u_reg_intg_err_o = ((s->u_reg_err_q) | (s->u_reg_intg_err) | (s->u_reg_reg_we_err));
    s->gen_alert_tx_0_u_prim_alert_sender_clk_i = s->clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_rst_ni = s->rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_i = ((s->u_reg_reg2hw_alert_test_q) & (s->u_reg_reg2hw_alert_test_qe));
    s->gen_alert_tx_0_u_prim_alert_sender_alert_req_i = s->alerts;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_p = s->alert_rx_i_0__ping_p;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_n = s->alert_rx_i_0__ping_n;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_p = s->alert_rx_i_0__ack_p;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_n = s->alert_rx_i_0__ack_n;
    s->gen_alert_tx_0_u_prim_alert_sender_ack_level = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_o;
    s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p = ((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o) & 1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_q_o) >> 1) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_set_d = (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger = (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_i) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_d = (((s->gen_alert_tx_0_u_prim_alert_sender_alert_clr) ^ 1)) & (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger = ((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q)) | (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_trigger);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger = (s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_event_o);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_set_d = (((s->gen_alert_tx_0_u_prim_alert_sender_ping_clr) ^ 1)) & (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger);
    s->gen_alert_tx_0_u_prim_alert_sender_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_in_i = ((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ping_n)) << 1));
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_u_secure_anchor_buf_out_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_pi = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o) >> 0) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_diff_ni = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ping_out_o) >> 1) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_p_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_n_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd;
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
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_event_o = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o));
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_in_i = ((((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_p)) << 0) | (((uint64_t)(s->gen_alert_tx_0_u_prim_alert_sender_alert_rx_i_ack_n)) << 1));
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_u_secure_anchor_buf_out_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_clk_i = s->gen_alert_tx_0_u_prim_alert_sender_clk_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni = s->gen_alert_tx_0_u_prim_alert_sender_rst_ni;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_pi = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o) >> 0) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_diff_ni = (((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_ack_out_o) >> 1) & 0x1);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_q_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_p_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_n_edge = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok = (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd) ^ (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd;
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
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_event_o = ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o) | (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o));
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_in_i = s->gen_alert_tx_0_u_prim_alert_sender_alert_req_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_in_i;
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_out_o = s->gen_alert_tx_0_u_prim_alert_sender_u_prim_buf_in_req_u_secure_anchor_buf_out_o;
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
    s->gen_alert_tx_0_u_prim_alert_sender_alert_ack_o = ((s->gen_alert_tx_0_u_prim_alert_sender_alert_clr) & (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q));
    s->gen_alert_tx_0_u_prim_alert_sender_alert_state_o = s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_p;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n = s->gen_alert_tx_0_u_prim_alert_sender_alert_tx_o_alert_n;
}

/*
 * tick() - Commit one cycle of sequential register state.
 *
 * Sequential drvs (originally inside llhd.process) translate
 * to assignments here; ACCUMULATE counters are handled by
 * ptimer instead and are excluded from this body.
 */
static void tick(rv_timer_state *s)
{
    s->gen_harts_0_u_intr_hw_intr_o = ((((s->gen_harts_0_u_intr_hw_rst_ni) ^ 1)) ? (0) : s->gen_harts_0_u_intr_hw_intr_o);
    s->gen_harts_0_u_intr_hw_intr_o = ((!(((s->gen_harts_0_u_intr_hw_rst_ni) ^ 1))) ? ((s->gen_harts_0_u_intr_hw_status) & (s->gen_harts_0_u_intr_hw_reg2hw_intr_enable_q_i)) : s->gen_harts_0_u_intr_hw_intr_o);
    s->gen_harts_0_u_core_tick_count = ((((((!(((s->gen_harts_0_u_core_rst_ni) ^ 1))) && (!(((s->gen_harts_0_u_core_active) ^ 1)))) && (((s->gen_harts_0_u_core_tick_count) == (s->gen_harts_0_u_core_prescaler)))) || ((!(((s->gen_harts_0_u_core_rst_ni) ^ 1))) && (((s->gen_harts_0_u_core_active) ^ 1)))) || (((s->gen_harts_0_u_core_rst_ni) ^ 1))) ? (0) : s->gen_harts_0_u_core_tick_count);
    s->u_reg_err_q = ((((!(((s->u_reg_rst_ni) ^ 1))) && ((s->u_reg_intg_err) | (s->u_reg_reg_we_err))) || (((s->u_reg_rst_ni) ^ 1))) ? (s->u_reg__unknown_arg0) : s->u_reg_err_q);
    s->u_reg_racl_addr_hit_read = (((1) || (1)) ? (0) : s->u_reg_racl_addr_hit_read);
    s->u_reg_racl_addr_hit_write = (((1) || (1)) ? (0) : s->u_reg_racl_addr_hit_write);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x1ULL) | (((((s->u_reg_reg_addr) == (0))) & 0x1ULL) << 0)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x2ULL) | (((((s->u_reg_reg_addr) == (4))) & 0x1ULL) << 1)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x4ULL) | (((((s->u_reg_reg_addr) == (256))) & 0x1ULL) << 2)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x8ULL) | (((((s->u_reg_reg_addr) == (260))) & 0x1ULL) << 3)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x10ULL) | (((((s->u_reg_reg_addr) == (264))) & 0x1ULL) << 4)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x20ULL) | (((((s->u_reg_reg_addr) == (268))) & 0x1ULL) << 5)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x40ULL) | (((((s->u_reg_reg_addr) == (272))) & 0x1ULL) << 6)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x80ULL) | (((((s->u_reg_reg_addr) == (276))) & 0x1ULL) << 7)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x100ULL) | (((((s->u_reg_reg_addr) == (280))) & 0x1ULL) << 8)) : s->u_reg_addr_hit);
    s->u_reg_addr_hit = (((1) || (1)) ? ((s->u_reg_addr_hit & ~0x200ULL) | (((((s->u_reg_reg_addr) == (284))) & 0x1ULL) << 9)) : s->u_reg_addr_hit);
    s->u_reg_racl_addr_hit_read = (((1) || (1)) ? (s->u_reg_addr_hit) : s->u_reg_racl_addr_hit_read);
    s->u_reg_racl_addr_hit_write = (((1) || (1)) ? (s->u_reg_addr_hit) : s->u_reg_racl_addr_hit_write);
    s->u_reg_wr_err = (((1) || (1)) ? ((s->u_reg_reg_we) & (((((s->u_reg_racl_addr_hit_write) & 1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 1) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 2) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 3) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 4) & 0x1)) & (((((s->u_reg_reg_be) & 1)) ^ 1))) | (((((s->u_reg_racl_addr_hit_write) >> 5) & 0x1)) & (((((s->u_reg_reg_be) & 0x7)) != (7)))) | (((((s->u_reg_racl_addr_hit_write) >> 6) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_racl_addr_hit_write) >> 7) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_racl_addr_hit_write) >> 8) & 0x1)) & (((s->u_reg_reg_be) != (15)))) | (((((s->u_reg_racl_addr_hit_write) >> 9) & 0x1)) & (((s->u_reg_reg_be) != (15)))))) : s->u_reg_wr_err);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x1ULL) | (((s->u_reg_alert_test_we) & 0x1ULL) << 0)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x2ULL) | (((s->u_reg_ctrl_we) & 0x1ULL) << 1)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x4ULL) | (((s->u_reg_intr_enable0_we) & 0x1ULL) << 2)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x8ULL) | (((s->u_reg_intr_state0_we) & 0x1ULL) << 3)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x10ULL) | (((s->u_reg_intr_test0_we) & 0x1ULL) << 4)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x20ULL) | (((s->u_reg_cfg0_we) & 0x1ULL) << 5)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x40ULL) | (((s->u_reg_timer_v_lower0_we) & 0x1ULL) << 6)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x80ULL) | (((s->u_reg_timer_v_upper0_we) & 0x1ULL) << 7)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x100ULL) | (((s->u_reg_compare_lower0_0_we) & 0x1ULL) << 8)) : s->u_reg_reg_we_check);
    s->u_reg_reg_we_check = (((1) || (1)) ? ((s->u_reg_reg_we_check & ~0x200ULL) | (((s->u_reg_compare_upper0_0_we) & 0x1ULL) << 9)) : s->u_reg_reg_we_check);
    s->u_reg_reg_rdata_next = (((1) || (1)) ? (0) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((((((1) || (1)) && (!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1)))) || (((1) || (1)) && (((((s->u_reg_racl_addr_hit_read) & 1)) == (1))))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | (((0) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))) || ((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1)))) || ((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0x1ULL) | ((((((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))) ? (s->u_reg_intr_state0_qs) : ((((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))) ? (s->u_reg_intr_enable0_qs) : (s->u_reg_ctrl_qs))))) & 0x1ULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((((((1) || (1)) && (!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0xFFFULL) | (((s->u_reg_cfg0_prescale_qs) & 0xFFFULL) << 0)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((((((1) || (1)) && (!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && ((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1)))) ? ((s->u_reg_reg_rdata_next & ~0xFF0000ULL) | (((s->u_reg_cfg0_step_qs) & 0xFFULL) << 16)) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = ((((((((((s->u_reg_racl_addr_hit_read) >> 9) & 0x1)) == (1))) || ((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1)))) || ((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1)))) || ((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1)))) ? ((((((((s->u_reg_racl_addr_hit_read) >> 9) & 0x1)) == (1))) ? (s->u_reg_compare_upper0_0_qs) : ((((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1))) ? (s->u_reg_compare_lower0_0_qs) : ((((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1))) ? (s->u_reg_timer_v_upper0_qs) : (s->u_reg_timer_v_lower0_qs))))))) : s->u_reg_reg_rdata_next);
    s->u_reg_reg_rdata_next = (((((((((((((1) || (1)) && (!(((((s->u_reg_racl_addr_hit_read) & 1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 1) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 2) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 3) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 4) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 5) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 6) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 7) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 8) & 0x1)) == (1))))) && (!((((((s->u_reg_racl_addr_hit_read) >> 9) & 0x1)) == (1))))) ? (-1) : s->u_reg_reg_rdata_next);
    s->u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_syndrome_o & ~0x1ULL) | ((((__builtin_parityll((((0) << 58) | ((((s->u_reg_u_chk_u_chk_data_i) & 0x3FFFFFFFFFFFFFFULL)) & (217298647660920831)))))) & 0x1ULL) << 0)) : s->u_reg_u_chk_u_chk_syndrome_o);
    s->u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_syndrome_o & ~0x2ULL) | ((((__builtin_parityll((((0) << 59) | (((((s->u_reg_u_chk_u_chk_data_i) & 0x7FFFFFFFFFFFFFFULL)) ^ (288230376151711744)) & (395226017347633183)))))) & 0x1ULL) << 1)) : s->u_reg_u_chk_u_chk_syndrome_o);
    s->u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_syndrome_o & ~0x4ULL) | ((((__builtin_parityll((((0) << 60) | (((((s->u_reg_u_chk_u_chk_data_i) & 0xFFFFFFFFFFFFFFFULL)) ^ (288230376151711744)) & (701965574322225633)))))) & 0x1ULL) << 2)) : s->u_reg_u_chk_u_chk_syndrome_o);
    s->u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_syndrome_o & ~0x8ULL) | ((((__builtin_parityll((((0) << 61) | (((((((s->u_reg_u_chk_u_chk_data_i) >> 1) & 0xFFFFFFFFFFFFFFFULL)) ^ (720575940379279360)) & (643864241515546385)) << 1) | (0))))) & 0x1ULL) << 3)) : s->u_reg_u_chk_u_chk_syndrome_o);
    s->u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_syndrome_o & ~0x10ULL) | ((((__builtin_parityll((((0) << 62) | (((((((s->u_reg_u_chk_u_chk_data_i) >> 2) & 0xFFFFFFFFFFFFFFFULL)) ^ (360287970189639680)) & (611325937131342993)) << 2) | (0))))) & 0x1ULL) << 4)) : s->u_reg_u_chk_u_chk_syndrome_o);
    s->u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_syndrome_o & ~0x20ULL) | ((((__builtin_parityll((((0) << 63) | (((((((s->u_reg_u_chk_u_chk_data_i) >> 3) & 0xFFFFFFFFFFFFFFFULL)) ^ (756604737398243328)) & (594184239166671505)) << 3) | (0))))) & 0x1ULL) << 5)) : s->u_reg_u_chk_u_chk_syndrome_o);
    s->u_reg_u_chk_u_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_syndrome_o & ~0x40ULL) | ((((__builtin_parityll(((((((((s->u_reg_u_chk_u_chk_data_i) >> 4) & 0xFFFFFFFFFFFFFFFULL)) ^ (378302368699121664)) & (585395222571796113)) << 4) | (0))))) & 0x1ULL) << 6)) : s->u_reg_u_chk_u_chk_syndrome_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x1ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (7))) ^ (((s->u_reg_u_chk_u_chk_data_i) & 1))) & 0x1ULL) << 0)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x2ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (11))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 1) & 0x1))) & 0x1ULL) << 1)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x4ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (19))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 2) & 0x1))) & 0x1ULL) << 2)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x8ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (35))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 3) & 0x1))) & 0x1ULL) << 3)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x10ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (67))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 4) & 0x1))) & 0x1ULL) << 4)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x20ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (13))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 5) & 0x1))) & 0x1ULL) << 5)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x40ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (21))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 6) & 0x1))) & 0x1ULL) << 6)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x80ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (37))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 7) & 0x1))) & 0x1ULL) << 7)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x100ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (69))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 8) & 0x1))) & 0x1ULL) << 8)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x200ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (25))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 9) & 0x1))) & 0x1ULL) << 9)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x400ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (41))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 10) & 0x1))) & 0x1ULL) << 10)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x800ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (73))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 11) & 0x1))) & 0x1ULL) << 11)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x1000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (49))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 12) & 0x1))) & 0x1ULL) << 12)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x2000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (81))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 13) & 0x1))) & 0x1ULL) << 13)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x4000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (97))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 14) & 0x1))) & 0x1ULL) << 14)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x8000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (14))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 15) & 0x1))) & 0x1ULL) << 15)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x10000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (22))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 16) & 0x1))) & 0x1ULL) << 16)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x20000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (38))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 17) & 0x1))) & 0x1ULL) << 17)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x40000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (70))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 18) & 0x1))) & 0x1ULL) << 18)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x80000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (26))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 19) & 0x1))) & 0x1ULL) << 19)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x100000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (42))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 20) & 0x1))) & 0x1ULL) << 20)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x200000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (74))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 21) & 0x1))) & 0x1ULL) << 21)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x400000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (50))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 22) & 0x1))) & 0x1ULL) << 22)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x800000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (82))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 23) & 0x1))) & 0x1ULL) << 23)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x1000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (98))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 24) & 0x1))) & 0x1ULL) << 24)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x2000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (28))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 25) & 0x1))) & 0x1ULL) << 25)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x4000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (44))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 26) & 0x1))) & 0x1ULL) << 26)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x8000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (76))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 27) & 0x1))) & 0x1ULL) << 27)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x10000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (52))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 28) & 0x1))) & 0x1ULL) << 28)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x20000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (84))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 29) & 0x1))) & 0x1ULL) << 29)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x40000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (100))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 30) & 0x1))) & 0x1ULL) << 30)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x80000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (56))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 31) & 0x1))) & 0x1ULL) << 31)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x100000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (88))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 32) & 0x1))) & 0x1ULL) << 32)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x200000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (104))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 33) & 0x1))) & 0x1ULL) << 33)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x400000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (112))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 34) & 0x1))) & 0x1ULL) << 34)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x800000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (31))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 35) & 0x1))) & 0x1ULL) << 35)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x1000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (47))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 36) & 0x1))) & 0x1ULL) << 36)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x2000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (79))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 37) & 0x1))) & 0x1ULL) << 37)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x4000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (55))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 38) & 0x1))) & 0x1ULL) << 38)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x8000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (87))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 39) & 0x1))) & 0x1ULL) << 39)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x10000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (103))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 40) & 0x1))) & 0x1ULL) << 40)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x20000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (59))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 41) & 0x1))) & 0x1ULL) << 41)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x40000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (91))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 42) & 0x1))) & 0x1ULL) << 42)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x80000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (107))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 43) & 0x1))) & 0x1ULL) << 43)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x100000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (115))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 44) & 0x1))) & 0x1ULL) << 44)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x200000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (61))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 45) & 0x1))) & 0x1ULL) << 45)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x400000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (93))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 46) & 0x1))) & 0x1ULL) << 46)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x800000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (109))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 47) & 0x1))) & 0x1ULL) << 47)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x1000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (117))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 48) & 0x1))) & 0x1ULL) << 48)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x2000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (121))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 49) & 0x1))) & 0x1ULL) << 49)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x4000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (62))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 50) & 0x1))) & 0x1ULL) << 50)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x8000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (94))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 51) & 0x1))) & 0x1ULL) << 51)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x10000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (110))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 52) & 0x1))) & 0x1ULL) << 52)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x20000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (118))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 53) & 0x1))) & 0x1ULL) << 53)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x40000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (122))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 54) & 0x1))) & 0x1ULL) << 54)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x80000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (124))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 55) & 0x1))) & 0x1ULL) << 55)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_data_o & ~0x100000000000000ULL) | ((((((s->u_reg_u_chk_u_chk_syndrome_o) == (127))) ^ ((((s->u_reg_u_chk_u_chk_data_i) >> 56) & 0x1))) & 0x1ULL) << 56)) : s->u_reg_u_chk_u_chk_data_o);
    s->u_reg_u_chk_u_chk_err_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_err_o & ~0x1ULL) | ((((__builtin_parityll(s->u_reg_u_chk_u_chk_syndrome_o))) & 0x1ULL) << 0)) : s->u_reg_u_chk_u_chk_err_o);
    s->u_reg_u_chk_u_chk_err_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_chk_err_o & ~0x2ULL) | ((((((((s->u_reg_u_chk_u_chk_err_o) & 1)) ^ 1)) & (((s->u_reg_u_chk_u_chk_syndrome_o) != (0)))) & 0x1ULL) << 1)) : s->u_reg_u_chk_u_chk_err_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x1ULL) | ((((__builtin_parityll((((0) << 33) | ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 0x1FFFFFFFFULL)) & (4932943141)))))) & 0x1ULL) << 0)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x2ULL) | ((((__builtin_parityll((((0) << 34) | (((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 4) & 0x3FFFFFFFULL)) ^ (536870912)) & (770418693)) << 4) | (0))))) & 0x1ULL) << 1)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x4ULL) | ((((__builtin_parityll((((0) << 35) | (((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 1) & 0x3FFFFFFFFULL)) ^ (4294967296)) & (9137210581)) << 1) | (0))))) & 0x1ULL) << 2)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x8ULL) | ((((__builtin_parityll((((0) << 36) | (((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 0xFFFFFFFFFULL)) ^ (42949672960)) & (35184135889)))))) & 0x1ULL) << 3)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x10ULL) | ((((__builtin_parityll((((0) << 37) | (((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 0x1FFFFFFFFFULL)) ^ (42949672960)) & (71986917947)))))) & 0x1ULL) << 4)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x20ULL) | ((((__builtin_parityll((((0) << 38) | (((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 2) & 0xFFFFFFFFFULL)) ^ (45097156608)) & (34551830675)) << 2) | (0))))) & 0x1ULL) << 5)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o & ~0x40ULL) | ((((__builtin_parityll(((((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 1) & 0x3FFFFFFFFFULL)) ^ (90194313216)) & (138716654275)) << 1) | (0))))) & 0x1ULL) << 6)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x1ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (25))) ^ (((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) & 1))) & 0x1ULL) << 0)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x2ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (84))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 1) & 0x1))) & 0x1ULL) << 1)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x4ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (97))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 2) & 0x1))) & 0x1ULL) << 2)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x8ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (52))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 3) & 0x1))) & 0x1ULL) << 3)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x10ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (26))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 4) & 0x1))) & 0x1ULL) << 4)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x20ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (21))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 5) & 0x1))) & 0x1ULL) << 5)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x40ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (42))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 6) & 0x1))) & 0x1ULL) << 6)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x80ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (76))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 7) & 0x1))) & 0x1ULL) << 7)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x100ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (69))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 8) & 0x1))) & 0x1ULL) << 8)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x200ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (56))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 9) & 0x1))) & 0x1ULL) << 9)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x400ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (73))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 10) & 0x1))) & 0x1ULL) << 10)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x800ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (13))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 11) & 0x1))) & 0x1ULL) << 11)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x1000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (81))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 12) & 0x1))) & 0x1ULL) << 12)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x2000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (49))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 13) & 0x1))) & 0x1ULL) << 13)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x4000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (104))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 14) & 0x1))) & 0x1ULL) << 14)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x8000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (7))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 15) & 0x1))) & 0x1ULL) << 15)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x10000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (28))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 16) & 0x1))) & 0x1ULL) << 16)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x20000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (11))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 17) & 0x1))) & 0x1ULL) << 17)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x40000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (37))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 18) & 0x1))) & 0x1ULL) << 18)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x80000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (38))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 19) & 0x1))) & 0x1ULL) << 19)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x100000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (70))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 20) & 0x1))) & 0x1ULL) << 20)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x200000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (14))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 21) & 0x1))) & 0x1ULL) << 21)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x400000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (112))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 22) & 0x1))) & 0x1ULL) << 22)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x800000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (50))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 23) & 0x1))) & 0x1ULL) << 23)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x1000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (44))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 24) & 0x1))) & 0x1ULL) << 24)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x2000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (19))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 25) & 0x1))) & 0x1ULL) << 25)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x4000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (35))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 26) & 0x1))) & 0x1ULL) << 26)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x8000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (98))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 27) & 0x1))) & 0x1ULL) << 27)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x10000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (74))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 28) & 0x1))) & 0x1ULL) << 28)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x20000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (41))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 29) & 0x1))) & 0x1ULL) << 29)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x40000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (22))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 30) & 0x1))) & 0x1ULL) << 30)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o & ~0x80000000ULL) | ((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) == (82))) ^ ((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_i) >> 31) & 0x1))) & 0x1ULL) << 31)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_data_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o & ~0x1ULL) | ((((__builtin_parityll(s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o))) & 0x1ULL) << 0)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o);
    s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o = (((1) || (1)) ? ((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o & ~0x2ULL) | ((((((((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o) & 1)) ^ 1)) & (((s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_syndrome_o) != (0)))) & 0x1ULL) << 1)) : s->u_reg_u_chk_u_tlul_data_integ_dec_u_data_chk_err_o);
    s->u_reg_u_rsp_intg_gen_tl_o_d_valid = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_valid) : s->u_reg_u_rsp_intg_gen_tl_o_d_valid);
    s->u_reg_u_rsp_intg_gen_tl_o_d_opcode = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_opcode) : s->u_reg_u_rsp_intg_gen_tl_o_d_opcode);
    s->u_reg_u_rsp_intg_gen_tl_o_d_param = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_param) : s->u_reg_u_rsp_intg_gen_tl_o_d_param);
    s->u_reg_u_rsp_intg_gen_tl_o_d_size = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_size) : s->u_reg_u_rsp_intg_gen_tl_o_d_size);
    s->u_reg_u_rsp_intg_gen_tl_o_d_source = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_source) : s->u_reg_u_rsp_intg_gen_tl_o_d_source);
    s->u_reg_u_rsp_intg_gen_tl_o_d_sink = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_sink) : s->u_reg_u_rsp_intg_gen_tl_o_d_sink);
    s->u_reg_u_rsp_intg_gen_tl_o_d_data = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_data) : s->u_reg_u_rsp_intg_gen_tl_o_d_data);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_user_rsp_intg) : s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_user_data_intg) : s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg);
    s->u_reg_u_rsp_intg_gen_tl_o_d_error = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_d_error) : s->u_reg_u_rsp_intg_gen_tl_o_d_error);
    s->u_reg_u_rsp_intg_gen_tl_o_a_ready = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_tl_i_a_ready) : s->u_reg_u_rsp_intg_gen_tl_o_a_ready);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_rsp_intg) : s->u_reg_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_rsp_intg_gen_data_intg) : s->u_reg_u_rsp_intg_gen_tl_o_d_user_data_intg);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((((0) << 57) | (s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_i))) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x200000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (73183459585064959)))))) & 0x1ULL) << 57)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x400000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (106995641195921439)))))) & 0x1ULL) << 58)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x800000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) & 0x1FFFFFFFFFFFFFFULL)) & (125504822018802145)))))) & 0x1ULL) << 59)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x1000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 1) & 0xFFFFFFFFFFFFFFULL)) & (67403489212122897)) << 1) | (0))))) & 0x1ULL) << 60)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x2000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 2) & 0x7FFFFFFFFFFFFFULL)) & (34865184827919505)) << 2) | (0))))) & 0x1ULL) << 61)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x4000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 3) & 0x3FFFFFFFFFFFFFULL)) & (17723486863248017)) << 3) | (0))))) & 0x1ULL) << 62)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o & ~0x8000000000000000ULL) | ((((__builtin_parityll((((0) << 57) | ((((((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) >> 4) & 0x1FFFFFFFFFFFFFULL)) & (8934470268372625)) << 4) | (0))))) & 0x1ULL) << 63)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o) ^ (6052837899185946624)) : s->u_reg_u_rsp_intg_gen_gen_rsp_intg_u_rsp_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((((0) << 32) | (s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_i))) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x100000000ULL) | ((((__builtin_parityll((((0) << 30) | ((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (637975845)))))) & 0x1ULL) << 32)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x200000000ULL) | ((((__builtin_parityll((((0) << 32) | ((((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 4) & 0xFFFFFFFULL)) & (233547781)) << 4) | (0))))) & 0x1ULL) << 33)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x400000000ULL) | ((((__builtin_parityll((((0) << 31) | ((((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x3FFFFFFFULL)) & (547275989)) << 1) | (0))))) & 0x1ULL) << 34)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x800000000ULL) | ((((__builtin_parityll((((0) << 30) | ((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0x3FFFFFFFULL)) & (824397521)))))) & 0x1ULL) << 35)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x1000000000ULL) | ((((__builtin_parityll((((0) << 32) | ((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) & 0xFFFFFFFFULL)) & (3267441211)))))) & 0x1ULL) << 36)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x2000000000ULL) | ((((__builtin_parityll((((0) << 30) | ((((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 2) & 0xFFFFFFFULL)) & (192092307)) << 2) | (0))))) & 0x1ULL) << 37)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o & ~0x4000000000ULL) | ((((__builtin_parityll((((0) << 32) | ((((((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) >> 1) & 0x7FFFFFFFULL)) & (1277700803)) << 1) | (0))))) & 0x1ULL) << 38)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o = (((1) || (1)) ? ((s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o) ^ (180388626432)) : s->u_reg_u_rsp_intg_gen_gen_data_intg_u_tlul_data_integ_enc_u_data_gen_data_o);
    s->u_reg_u_reg_if_outstanding_q = ((((((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (!(s->u_reg_u_reg_if_a_ack))) && (s->u_reg_u_reg_if_d_ack)) || ((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack))) || (((s->u_reg_u_reg_if_rst_ni) ^ 1))) ? (s->u_reg_u_reg_if__unknown_arg0) : s->u_reg_u_reg_if_outstanding_q);
    s->u_reg_u_reg_if_reqid_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : s->u_reg_u_reg_if_reqid_q);
    s->u_reg_u_reg_if_reqsz_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : s->u_reg_u_reg_if_reqsz_q);
    s->u_reg_u_reg_if_rspop_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : s->u_reg_u_reg_if_rspop_q);
    s->u_reg_u_reg_if_reqid_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (s->u_reg_u_reg_if_tl_i_a_source) : s->u_reg_u_reg_if_reqid_q);
    s->u_reg_u_reg_if_reqsz_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? (s->u_reg_u_reg_if_tl_i_a_size) : s->u_reg_u_reg_if_reqsz_q);
    s->u_reg_u_reg_if_rspop_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((((0) << 1) | (s->u_reg_u_reg_if_rd_req))) : s->u_reg_u_reg_if_rspop_q);
    s->u_reg_u_reg_if_rdata_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : s->u_reg_u_reg_if_rdata_q);
    s->u_reg_u_reg_if_error_q = ((((s->u_reg_u_reg_if_rst_ni) ^ 1)) ? (0) : s->u_reg_u_reg_if_error_q);
    s->u_reg_u_reg_if_rdata_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((((s->u_reg_u_reg_if_error_i) | (s->u_reg_u_reg_if_err_internal) | (s->u_reg_u_reg_if_wr_req)) ? (4294967295) : (s->u_reg_u_reg_if_rdata_i))) : s->u_reg_u_reg_if_rdata_q);
    s->u_reg_u_reg_if_error_q = (((!(((s->u_reg_u_reg_if_rst_ni) ^ 1))) && (s->u_reg_u_reg_if_a_ack)) ? ((s->u_reg_u_reg_if_error_i) | (s->u_reg_u_reg_if_err_internal)) : s->u_reg_u_reg_if_error_q);
    s->u_reg_u_reg_if_addr_align_err = ((((1) || (1)) && (s->u_reg_u_reg_if_wr_req)) ? (((((s->u_reg_u_reg_if_tl_i_a_address) & 0x3)) != (0))) : s->u_reg_u_reg_if_addr_align_err);
    s->u_reg_u_reg_if_addr_align_err = ((((1) || (1)) && (!(s->u_reg_u_reg_if_wr_req))) ? (0) : s->u_reg_u_reg_if_addr_align_err);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_valid) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_valid);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_opcode) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_opcode);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_param) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_param);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_size) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_size);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_source) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_source);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_sink) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_sink);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_data) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_data);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_rsp_intg) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_user_data_intg) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_d_error) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_error);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_tl_i_a_ready) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_a_ready);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_rsp_intg) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_rsp_intg);
    s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg = (((1) || (1)) ? (s->u_reg_u_reg_if_u_rsp_intg_gen_data_intg) : s->u_reg_u_reg_if_u_rsp_intg_gen_tl_o_d_user_data_intg);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = (((1) || (1)) ? (0) : s->u_reg_u_reg_if_u_err_addr_sz_chk);
    s->u_reg_u_reg_if_u_err_mask_chk = (((1) || (1)) ? (0) : s->u_reg_u_reg_if_u_err_mask_chk);
    s->u_reg_u_reg_if_u_err_fulldata_chk = (((1) || (1)) ? (0) : s->u_reg_u_reg_if_u_err_fulldata_chk);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = (((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0)))) ? (-1) : s->u_reg_u_reg_if_u_err_addr_sz_chk);
    s->u_reg_u_reg_if_u_err_mask_chk = (((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0)))) ? ((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & (~(s->u_reg_u_reg_if_u_err_mask))) == (0))) : s->u_reg_u_reg_if_u_err_mask_chk);
    s->u_reg_u_reg_if_u_err_fulldata_chk = (((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0)))) ? ((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & (s->u_reg_u_reg_if_u_err_mask)) != (0))) : s->u_reg_u_reg_if_u_err_fulldata_chk);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = ((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1)))) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 1)) ^ 1)) : s->u_reg_u_reg_if_u_err_addr_sz_chk);
    s->u_reg_u_reg_if_u_err_mask_chk = ((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1)))) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (0))) : ((((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (0))))) : s->u_reg_u_reg_if_u_err_mask_chk);
    s->u_reg_u_reg_if_u_err_fulldata_chk = ((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1)))) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_address) >> 1) & 0x1)) ? ((((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) >> 2) & 0x3)) == (3))) : (((((s->u_reg_u_reg_if_u_err_tl_i_a_mask) & 0x3)) == (3))))) : s->u_reg_u_reg_if_u_err_fulldata_chk);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2)))) ? (((((s->u_reg_u_reg_if_u_err_tl_i_a_address) & 0x3)) == (0))) : s->u_reg_u_reg_if_u_err_addr_sz_chk);
    s->u_reg_u_reg_if_u_err_mask_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2)))) ? (-1) : s->u_reg_u_reg_if_u_err_mask_chk);
    s->u_reg_u_reg_if_u_err_fulldata_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && ((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2)))) ? (((s->u_reg_u_reg_if_u_err_tl_i_a_mask) == (15))) : s->u_reg_u_reg_if_u_err_fulldata_chk);
    s->u_reg_u_reg_if_u_err_addr_sz_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2))))) ? (0) : s->u_reg_u_reg_if_u_err_addr_sz_chk);
    s->u_reg_u_reg_if_u_err_mask_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2))))) ? (0) : s->u_reg_u_reg_if_u_err_mask_chk);
    s->u_reg_u_reg_if_u_err_fulldata_chk = (((((((1) || (1)) && (s->u_reg_u_reg_if_u_err_tl_i_a_valid)) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (0))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (1))))) && (!((((((0) << 2) | (s->u_reg_u_reg_if_u_err_tl_i_a_size))) == (2))))) ? (0) : s->u_reg_u_reg_if_u_err_fulldata_chk);
    s->u_reg_u_ctrl_q = ((((s->u_reg_u_ctrl_rst_ni) ^ 1)) ? (0) : s->u_reg_u_ctrl_q);
    s->u_reg_u_ctrl_q = (((!(((s->u_reg_u_ctrl_rst_ni) ^ 1))) && (s->u_reg_u_ctrl_wr_en)) ? (s->u_reg_u_ctrl_wr_data) : s->u_reg_u_ctrl_q);
    s->u_reg_u_intr_enable0_q = ((((s->u_reg_u_intr_enable0_rst_ni) ^ 1)) ? (0) : s->u_reg_u_intr_enable0_q);
    s->u_reg_u_intr_enable0_q = (((!(((s->u_reg_u_intr_enable0_rst_ni) ^ 1))) && (s->u_reg_u_intr_enable0_wr_en)) ? (s->u_reg_u_intr_enable0_wr_data) : s->u_reg_u_intr_enable0_q);
    s->u_reg_u_intr_state0_q = ((((s->u_reg_u_intr_state0_rst_ni) ^ 1)) ? (0) : s->u_reg_u_intr_state0_q);
    s->u_reg_u_intr_state0_q = (((!(((s->u_reg_u_intr_state0_rst_ni) ^ 1))) && (s->u_reg_u_intr_state0_wr_en)) ? (s->u_reg_u_intr_state0_wr_data) : s->u_reg_u_intr_state0_q);
    s->u_reg_u_cfg0_prescale_q = ((((s->u_reg_u_cfg0_prescale_rst_ni) ^ 1)) ? (0) : s->u_reg_u_cfg0_prescale_q);
    s->u_reg_u_cfg0_prescale_q = (((!(((s->u_reg_u_cfg0_prescale_rst_ni) ^ 1))) && (s->u_reg_u_cfg0_prescale_wr_en)) ? (s->u_reg_u_cfg0_prescale_wr_data) : s->u_reg_u_cfg0_prescale_q);
    s->u_reg_u_cfg0_step_q = ((((s->u_reg_u_cfg0_step_rst_ni) ^ 1)) ? (1) : s->u_reg_u_cfg0_step_q);
    s->u_reg_u_cfg0_step_q = (((!(((s->u_reg_u_cfg0_step_rst_ni) ^ 1))) && (s->u_reg_u_cfg0_step_wr_en)) ? (s->u_reg_u_cfg0_step_wr_data) : s->u_reg_u_cfg0_step_q);
    s->u_reg_u_timer_v_lower0_q = ((((s->u_reg_u_timer_v_lower0_rst_ni) ^ 1)) ? (0) : s->u_reg_u_timer_v_lower0_q);
    s->u_reg_u_timer_v_lower0_q = (((!(((s->u_reg_u_timer_v_lower0_rst_ni) ^ 1))) && (s->u_reg_u_timer_v_lower0_wr_en)) ? (s->u_reg_u_timer_v_lower0_wr_data) : s->u_reg_u_timer_v_lower0_q);
    s->u_reg_u_timer_v_upper0_q = ((((s->u_reg_u_timer_v_upper0_rst_ni) ^ 1)) ? (0) : s->u_reg_u_timer_v_upper0_q);
    s->u_reg_u_timer_v_upper0_q = (((!(((s->u_reg_u_timer_v_upper0_rst_ni) ^ 1))) && (s->u_reg_u_timer_v_upper0_wr_en)) ? (s->u_reg_u_timer_v_upper0_wr_data) : s->u_reg_u_timer_v_upper0_q);
    s->u_reg_u_compare_lower0_00_qe_q_o = ((((s->u_reg_u_compare_lower0_00_qe_rst_ni) ^ 1)) ? (0) : s->u_reg_u_compare_lower0_00_qe_q_o);
    s->u_reg_u_compare_lower0_00_qe_q_o = ((!(((s->u_reg_u_compare_lower0_00_qe_rst_ni) ^ 1))) ? (s->u_reg_u_compare_lower0_00_qe_d_i) : s->u_reg_u_compare_lower0_00_qe_q_o);
    s->u_reg_u_compare_lower0_0_q = ((((s->u_reg_u_compare_lower0_0_rst_ni) ^ 1)) ? (-1) : s->u_reg_u_compare_lower0_0_q);
    s->u_reg_u_compare_lower0_0_q = (((!(((s->u_reg_u_compare_lower0_0_rst_ni) ^ 1))) && (s->u_reg_u_compare_lower0_0_wr_en)) ? (s->u_reg_u_compare_lower0_0_wr_data) : s->u_reg_u_compare_lower0_0_q);
    s->u_reg_u_compare_upper0_00_qe_q_o = ((((s->u_reg_u_compare_upper0_00_qe_rst_ni) ^ 1)) ? (0) : s->u_reg_u_compare_upper0_00_qe_q_o);
    s->u_reg_u_compare_upper0_00_qe_q_o = ((!(((s->u_reg_u_compare_upper0_00_qe_rst_ni) ^ 1))) ? (s->u_reg_u_compare_upper0_00_qe_d_i) : s->u_reg_u_compare_upper0_00_qe_q_o);
    s->u_reg_u_compare_upper0_0_q = ((((s->u_reg_u_compare_upper0_0_rst_ni) ^ 1)) ? (-1) : s->u_reg_u_compare_upper0_0_q);
    s->u_reg_u_compare_upper0_0_q = (((!(((s->u_reg_u_compare_upper0_0_rst_ni) ^ 1))) && (s->u_reg_u_compare_upper0_0_wr_en)) ? (s->u_reg_u_compare_upper0_0_wr_data) : s->u_reg_u_compare_upper0_0_q);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_state_q) : s->gen_alert_tx_0_u_prim_alert_sender_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = (((1) || (1)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_clr = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_ping_clr);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_clr = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_clr);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? ((((0) << 2) | ((((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) ^ 1)) << 1) | (1))) : s->gen_alert_tx_0_u_prim_alert_sender_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = s->gen_alert_tx_0_u_prim_alert_sender__unknown_arg0;
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-3) : s->gen_alert_tx_0_u_prim_alert_sender_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_clr = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_alert_clr);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_clr = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_ping_clr);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = (((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) ? (-3) : s->gen_alert_tx_0_u_prim_alert_sender_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_state_d = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_pd = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_pd);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_nd = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_nd);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_clr = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_ping_clr);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_clr = ((((((((((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (4)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1)))) || ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2)))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_ack_level) ^ 1))))) || ((((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (2))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (3)))) || ((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (1))))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_ack_level)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && ((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger)))) || ((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_state_q) == (0)))) && (!((s->gen_alert_tx_0_u_prim_alert_sender_alert_trigger) | (s->gen_alert_tx_0_u_prim_alert_sender_ping_trigger))))) && (s->gen_alert_tx_0_u_prim_alert_sender_sigint_detected)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_clr);
    s->gen_alert_tx_0_u_prim_alert_sender_state_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_state_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_state_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_state_d) : s->gen_alert_tx_0_u_prim_alert_sender_state_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_alert_set_d) : s->gen_alert_tx_0_u_prim_alert_sender_alert_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_d) : s->gen_alert_tx_0_u_prim_alert_sender_alert_test_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_ping_set_d) : s->gen_alert_tx_0_u_prim_alert_sender_ping_set_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rise_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_fall_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_unnamed = -1;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) + (1)) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q) ^ 1)))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_d) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_state_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pd) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_pq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nd) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_diff_nq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_d) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_level_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_d) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_skew_cnt_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_d_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_p_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_d_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ping_gen_async_i_sync_n_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rise_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_fall_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (((1) || (1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_unnamed = -1;
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((1) || (1)) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1))) ? ((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) + (1)) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-2) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1)))) && (!(s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q) ^ 1)))) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = ((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_sigint_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d = (((((((1) || (1)) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (0))))) && (!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (1))))) && (((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q) == (2)))) && (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_check_ok)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_level) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_d) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_state_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pd) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_pq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nd) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_diff_nq);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_d) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_level_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_d) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_skew_cnt_q);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_d_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1)) ? (0) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_p_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o = (((1) || (1)) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_d_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_1_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1)) ? (-1) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_decode_ack_gen_async_i_sync_n_u_sync_2_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = ((((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1)) ? (-2) : s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o);
    s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o = ((!(((s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_rst_ni) ^ 1))) ? (s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_d_i) : s->gen_alert_tx_0_u_prim_alert_sender_u_prim_flop_alert_u_secure_anchor_flop_q_o);
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

    /* Inject QEMU address into the internal address signal */
    s->tl_i_a_address = (uint32_t)addr;

    /* De-assert write enable — this is a read operation */
    s->reg_we = 0;

    /* TL-UL: assert request valid + Get opcode for one cycle */
    s->tl_i_a_valid = 1;
    s->tl_i_a_opcode = (uint8_t)4;  /* Get */

    /* Sync live ptimer counts into state before address decode.
     * NULL-guarded so a shim that doesn't call uart_realize
     * (frontend-only embedding) doesn't NULL-deref. */
    if (s->ptimer_gen_harts_0_u_core_tick_count) {
        s->gen_harts_0_u_core_tick_count = (uint16_t)ptimer_get_count(s->ptimer_gen_harts_0_u_core_tick_count);
    }

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
    for (int _qp_settle = 0; _qp_settle < 16; ++_qp_settle) {
        update_state(s);
        tick(s);
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
void rv_timer_write(void *opaque, hwaddr addr,
                uint64_t value, unsigned size)
{
    rv_timer_state *s = opaque;

    /* Inject QEMU address into the internal address signal */
    s->tl_i_a_address = (uint32_t)addr;

    /* Inject QEMU write data into the internal wdata signal */
    s->tl_i_a_data = (uint32_t)value;

    /* Set write mask (byte-enable bits for the access size) */
    s->tl_i_a_mask = (1ULL << size) - 1;

    /* Assert the bus enable (write cycle) */
    s->reg_we = 1;

    /* TL-UL: assert request valid + PutFullData opcode */
    s->tl_i_a_valid = 1;
    s->tl_i_a_opcode = (uint8_t)0;  /* PutFullData */

    /* Snapshot ACCUMULATE counter values before dispatch */
    uint16_t prev_gen_harts_0_u_core_tick_count = s->gen_harts_0_u_core_tick_count;

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
     * 4 rounds is a safe upper bound for the OT register chain.
     * For simpler peripherals it just hits the fixed point
     * earlier and the extra iterations are no-ops.  Tradeoff:
     * any non-ptimer register that's modeled as a free-running
     * counter advances by ~4 ticks per MMIO (was 1).  Our model
     * is functional, not cycle-accurate, so this is OK.
     */
    for (int _qp_settle = 0; _qp_settle < 16; ++_qp_settle) {
        update_state(s);
        tick(s);
    }
    update_state(s);

    /* De-assert enable after evaluation */
    s->reg_we = 0;

    /* De-assert TL-UL valid after the bus cycle settled */
    s->tl_i_a_valid = 0;

    /* Reload ptimers for counters whose value changed.
     * NULL-guarded for frontend-only embedding (see uart_read). */
    if (s->ptimer_gen_harts_0_u_core_tick_count && s->gen_harts_0_u_core_tick_count != prev_gen_harts_0_u_core_tick_count) {
        ptimer_transaction_begin(s->ptimer_gen_harts_0_u_core_tick_count);
        ptimer_set_count(s->ptimer_gen_harts_0_u_core_tick_count, s->gen_harts_0_u_core_tick_count);
        ptimer_run(s->ptimer_gen_harts_0_u_core_tick_count, 1);
        ptimer_transaction_commit(s->ptimer_gen_harts_0_u_core_tick_count);
    }

}

static const MemoryRegionOps rv_timer_ops = {
    .read  = rv_timer_read,
    .write = rv_timer_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void rv_timer_realize(DeviceState *dev, Error **errp)
{
    rv_timer_state *s = RV_TIMER(dev);

    memory_region_init_io(&s->mmio, OBJECT(dev), &rv_timer_ops, s,
                          "rv_timer", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    /* Initialize ACCUMULATE counter ptimers and IRQ lines */
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq_gen_harts_0_u_core_tick_count);
    s->ptimer_gen_harts_0_u_core_tick_count = ptimer_init(rv_timer_tick_gen_harts_0_u_core_tick_count, s, PTIMER_POLICY_LEGACY);
    ptimer_transaction_begin(s->ptimer_gen_harts_0_u_core_tick_count);
    ptimer_set_freq(s->ptimer_gen_harts_0_u_core_tick_count, RV_TIMER_PTIMER_FREQ);
    ptimer_transaction_commit(s->ptimer_gen_harts_0_u_core_tick_count);

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
    tick(s);
    update_state(s);
}

void rv_timer_set_alert_rx_i_0__ping_n(rv_timer_state *s, uint8_t value)
{
    s->alert_rx_i_0__ping_n = value;
    update_state(s);
    tick(s);
    update_state(s);
}

void rv_timer_set_alert_rx_i_0__ack_p(rv_timer_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_p = value;
    update_state(s);
    tick(s);
    update_state(s);
}

void rv_timer_set_alert_rx_i_0__ack_n(rv_timer_state *s, uint8_t value)
{
    s->alert_rx_i_0__ack_n = value;
    update_state(s);
    tick(s);
    update_state(s);
}

