/*
 * QEMU OpenTitan ROM_CTRL heart-swap boot front-end (qemu-passes).
 *
 * The HEART SWAP: the machine's boot verdict comes from the GENERATED
 * rom_ctrl model (hw/opentitan/qemu_passes/rom_ctrl.c) checked against
 * the GENERATED kmac (app channel 2, cSHAKE-256 "ROM_CTRL") — the
 * native ot-rom_ctrl no longer decides whether the CPU boots.
 *
 * This device is SoC-integration glue, not a model: it owns the
 * cleartext rom_device at the CPU fetch window (0x8000), loads the
 * same image formats the native loader accepts (ELF / binary /
 * plain VMEM / scrambled VMEM+ECC / scrambled HEX+ECC, via the same
 * -object ot-rom_img), injects the SCRAMBLED image into the generated
 * model's prim_rom backing store, co-steps the two generated models to
 * the checker's completion, and raises the pwrmgr rom done/good lines
 * from the model's own pwrmgr_data_o mubi verdict.
 *
 * Cleartext-image dev mode (native "local_gen" replication): the image
 * carries no digest trailer, so the check runs twice — pass 1 lets the
 * generated kmac compute the authoritative digest, the trailer words
 * are patched with it, and pass 2 re-runs the whole model check over
 * the final image.  good is ALWAYS the model's verdict over the image
 * it serves — never forced in C.
 *
 * Scrambler: C port of OT's scramble_image.py at the model's baked RTL
 * parameters (key=0, nonce=0, PRINCE half-rounds=3, addr subst-perm
 * rounds=2, 8192x39, top-8 digest words raw).  Verified value-exact
 * against the pure-python reference (host_kat/rom_ctrl/gen_image.py)
 * and against the generated model itself (gate target rom-ctrl-kat).
 */

#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "elf.h"
#include "hw/core/rust_demangle.h"
#include "hw/loader.h"
#include "hw/opentitan/ot_common.h"
#include "hw/opentitan/ot_rom_ctrl.h"
#include "hw/opentitan/ot_rom_ctrl_img.h"
#include "hw/qdev-properties.h"
#include "hw/riscv/ibex_common.h"
#include "hw/riscv/ibex_irq.h"
#include "hw/sysbus.h"

#include "qemu_passes/rom_ctrl.h"
#include "qemu_passes/kmac.h"
#include "hw/opentitan/rom_ctrl_qp_shim.h"
#include "hw/opentitan/kmac_qp_shim.h"
#include "hw/opentitan/rom_ctrl_qp_boot.h"

/* ---------------------------------------------------------------- */
/* PRINCE + addr subst-perm scrambler (romscr.c, python-parity)      */
/* ---------------------------------------------------------------- */

#define QPB_AW   13u /* address bits: 8192 words */
#define QPB_W    39u /* data+ECC bits per word */
#define QPB_HALF 3u  /* PRINCE half-rounds */
#define QPB_SPR  2u  /* subst-perm rounds */
#define QPB_DEPTH (1u << QPB_AW)
#define QPB_TOP  8u  /* trailing digest words */

static const uint8_t qpb_prince_sbox4[16] = { 0xb, 0xf, 3,   2,   0xa, 0xc,
                                              9,   1,   6,   7,   8,   0,
                                              0xe, 5,   0xd, 4 };
static const uint8_t qpb_prince_sbox4_inv[16] = { 0xb, 7,   3,   2, 0xf, 0xd,
                                                  8,   9,   0xa, 6, 4,   0,
                                                  5,   0xe, 0xc, 1 };
static const uint8_t qpb_sr64[16] = { 4,   9, 0xe, 3,   8, 0xd, 2,   7,
                                      0xc, 1, 6,   0xb, 0, 5,   0xa, 0xf };
static const uint8_t qpb_sr64i[16] = { 0xc, 9,   6,   3, 0,   0xd, 0xa, 7,
                                       4,   1,   0xe, 0xb, 8, 5,   2,   0xf };
static const uint16_t qpb_src[4] = { 0x7bde, 0xbde7, 0xde7b, 0xe7bd };
static const uint64_t qpb_rc[12] = {
    0,
    0x13198a2e03707344ull,
    0xa4093822299f31d0ull,
    0x082efa98ec4e6c89ull,
    0x452821e638d01377ull,
    0xbe5466cf34e90c6cull,
    0x7ef84f78fd955cb1ull,
    0x85840851f1ac43aaull,
    0xc882d32f25323c54ull,
    0x64a51195e0e3610dull,
    0xd3b5a399ca0c2399ull,
    0xc0ac29b7c97c50ddull,
};

static uint64_t qpb_sbox64(uint64_t d, const uint8_t *c)
{
    uint64_t r = 0;
    for (int i = 0; i < 16; i++) {
        r |= (uint64_t)c[(d >> (4 * i)) & 0xf] << (4 * i);
    }
    return r;
}

static uint8_t qpb_red16(uint16_t d)
{
    return ((d >> 0) & 0xf) ^ ((d >> 4) & 0xf) ^ ((d >> 8) & 0xf) ^
           ((d >> 12) & 0xf);
}

static uint64_t qpb_mult_prime(uint64_t d)
{
    uint64_t r = 0;
    for (int b = 0; b < 4; b++) {
        uint16_t hw = (d >> (16 * b)) & 0xffff;
        int st = (b == 0 || b == 3) ? 0 : 1;
        for (int n = 0; n < 4; n++) {
            int sr = (st + 3 - n) & 3;
            r |= (uint64_t)qpb_red16(hw & qpb_src[sr]) << (16 * b + 4 * n);
        }
    }
    return r;
}

static uint64_t qpb_shiftrows(uint64_t d, int inv)
{
    const uint8_t *sr = inv ? qpb_sr64i : qpb_sr64;
    uint64_t r = 0;
    for (int i = 0; i < 16; i++) {
        r |= ((d >> (4 * sr[i])) & 0xf) << (4 * i);
    }
    return r;
}

static uint64_t qpb_fwd_round(uint64_t rc, uint64_t k, uint64_t d)
{
    d = qpb_sbox64(d, qpb_prince_sbox4);
    d = qpb_mult_prime(d);
    d = qpb_shiftrows(d, 0);
    return d ^ rc ^ k;
}

static uint64_t qpb_inv_round(uint64_t rc, uint64_t k, uint64_t d)
{
    d ^= k;
    d ^= rc;
    d = qpb_shiftrows(d, 1);
    d = qpb_mult_prime(d);
    return qpb_sbox64(d, qpb_prince_sbox4_inv);
}

/* PRINCE with key=0 (the model's baked RTL default) */
static uint64_t qpb_prince(uint64_t data, unsigned nrh)
{
    const uint64_t k0 = 0, k1 = 0, k0p = 0;
    data ^= k0;
    data ^= k1;
    data ^= qpb_rc[0];
    for (unsigned h = 0; h < nrh; h++) {
        unsigned ri = 1 + h;
        data = qpb_fwd_round(qpb_rc[ri], (ri & 1) ? k0 : k1, data);
    }
    data = qpb_sbox64(data, qpb_prince_sbox4);
    data = qpb_mult_prime(data);
    data = qpb_sbox64(data, qpb_prince_sbox4_inv);
    for (unsigned h = 0; h < nrh; h++) {
        unsigned ri = 11 - nrh + h;
        data = qpb_inv_round(qpb_rc[ri], (ri & 1) ? k1 : k0, data);
    }
    data ^= qpb_rc[11];
    data ^= k1;
    data ^= k0p;
    return data;
}

/* address subst-perm (PRESENT sbox), nonce=0 */
static const uint8_t qpb_ps[16] = { 0xc, 5, 6,   0xb, 9,   0, 0xa, 0xd,
                                    3,   0xe, 0xf, 8,   4, 7, 1,   2 };

static uint32_t qpb_addr_sp_enc(uint32_t data)
{
    const uint32_t key = 0;
    const unsigned w = QPB_AW;
    uint32_t full = (1u << w) - 1, bf = (1u << (2 * (w / 2))) - 1;
    for (unsigned r = 0; r < QPB_SPR; r++) {
        uint32_t dx = data ^ key;
        uint32_t ds = dx & (full & ~((1u << (4 * (w / 4))) - 1));
        for (unsigned i = 0; i < w / 4; i++) {
            ds |= (uint32_t)qpb_ps[(dx >> (4 * i)) & 0xf] << (4 * i);
        }
        uint32_t dr = 0;
        for (unsigned i = 0; i < w; i++) {
            dr |= ((ds >> i) & 1) << (w - 1 - i);
        }
        uint32_t db = dr & (full & ~bf);
        for (unsigned i = 0; i < w / 2; i++) {
            db |= ((dr >> (2 * i)) & 1) << i;
            db |= ((dr >> (2 * i + 1)) & 1) << (w / 2 + i);
        }
        data = db;
    }
    return data ^ key;
}

static uint64_t qpb_keystream(uint32_t log_addr)
{
    /* data_nonce=0 -> the PRINCE input is just the logical address */
    return qpb_prince(log_addr, QPB_HALF) & ((1ull << QPB_W) - 1ull);
}

/* ---------------------------------------------------------------- */
/* Device                                                            */
/* ---------------------------------------------------------------- */

struct OtRomCtrlQpBootState {
    SysBusDevice parent_obj;

    MemoryRegion mem; /* cleartext CPU-fetch ROM window (rom_device) */
    IbexIRQ pwrmgr_good;
    IbexIRQ pwrmgr_done;

    DeviceState *rom_ctrl; /* the generated rom_ctrl (primary mount) */
    DeviceState *kmac; /* the generated kmac */

    char *ot_id;
    char *img_id; /* -object ot-rom_img id to consume (native: "rom") */
    uint32_t size;

    bool sealed; /* image loaded + checked; ROM window read-only */
    bool done_cached;
    bool good_cached;
};

OBJECT_DEFINE_SIMPLE_TYPE(OtRomCtrlQpBootState, ot_rom_ctrl_qp_boot,
                          OT_ROM_CTRL_QP_BOOT, SYS_BUS_DEVICE)

/* ---------------------------------------------------------------- */
/* ROM window ops (mirror the native loader-phase semantics)         */
/* ---------------------------------------------------------------- */

static void ot_rom_ctrl_qp_boot_mem_write(void *opaque, hwaddr addr,
                                          uint64_t value, unsigned size)
{
    OtRomCtrlQpBootState *s = opaque;
    uint8_t *rom_ptr = (uint8_t *)memory_region_get_ram_ptr(&s->mem);

    if (!s->sealed && (addr + size) <= s->size) {
        stn_le_p(&rom_ptr[addr], (int)size, value);
    } else {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: %s: Bad offset 0x%02x\n", __func__,
                      s->ot_id, (uint32_t)addr);
    }
}

static bool ot_rom_ctrl_qp_boot_mem_accepts(void *opaque, hwaddr addr,
                                            unsigned size, bool is_write,
                                            MemTxAttrs attrs)
{
    OtRomCtrlQpBootState *s = opaque;
    (void)attrs;

    if (!is_write) {
        /* pre-seal reads only (post-seal the region is in ROMD mode
         * and this callback is no longer consulted) */
        return !s->sealed;
    }

    return !s->sealed && (addr + size) <= s->size;
}

static const MemoryRegionOps ot_rom_ctrl_qp_boot_mem_ops = {
    .write = &ot_rom_ctrl_qp_boot_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 1u,
    .impl.max_access_size = 4u,
    .valid.accepts = &ot_rom_ctrl_qp_boot_mem_accepts,
};

/* ---------------------------------------------------------------- */
/* Generated-model co-step bridge (rom_ctrl x kmac app channel 2)    */
/* ---------------------------------------------------------------- */

/* Re-arm the generated core exactly the way its shim init does: ties
 * BEFORE the reset pulse (sparse security FSMs sample them on the
 * reset-release tick), reset, ties again, settle. */
static void qpb_core_rearm(rom_ctrl_state *r)
{
    rom_ctrl_set_rom_cfg_i_cfg(r, 0x0u);
    rom_ctrl_set_rom_cfg_i_cfg_en(r, 0x0u);
    rom_ctrl_set_rom_cfg_i_test(r, 0x0u);
    rom_ctrl_reset(r);
    rom_ctrl_set_rom_cfg_i_cfg(r, 0x0u);
    rom_ctrl_set_rom_cfg_i_cfg_en(r, 0x0u);
    rom_ctrl_set_rom_cfg_i_test(r, 0x0u);
    rom_ctrl_settle(r);
}

/* Co-step the checker to completion.  ACCUMULATE counters only advance
 * on pump ticks; every co-step is a real clock, so hold both pumps
 * high.  When dig0/dig1 are given, capture the kmac digest shares at
 * the app-done pulse (pass 1 of the cleartext dev-mode flow). */
static bool qpb_bridge(rom_ctrl_state *r, kmac_state *k, uint64_t *dig0,
                       uint64_t *dig1, bool *have_dig)
{
    uint8_t rp = r->_qp_pump, kp = k->_qp_pump;
    r->_qp_pump = 1;
    k->_qp_pump = 1;

    for (unsigned t = 0; t < 200000u && r->pwrmgr_data_o_done != 0x6u; t++) {
        r->kmac_data_i_ready = k->app_o_2__ready;
        r->kmac_data_i_done = k->app_o_2__done;
        r->kmac_data_i_error = k->app_o_2__error;
        for (unsigned w = 0; w < 6u; w++) {
            r->kmac_data_i_digest_share0[w] = k->app_o_2__digest_share0[w];
            r->kmac_data_i_digest_share1[w] = k->app_o_2__digest_share1[w];
        }
        if (k->app_o_2__done && dig0) {
            for (unsigned w = 0; w < 6u; w++) {
                dig0[w] = k->app_o_2__digest_share0[w];
                dig1[w] = k->app_o_2__digest_share1[w];
            }
            *have_dig = true;
        }
        k->app_i_2__valid = r->kmac_data_o_valid;
        k->app_i_2__data = r->kmac_data_o_data;
        k->app_i_2__strb = r->kmac_data_o_strb;
        k->app_i_2__last = r->kmac_data_o_last;
        rom_ctrl_step(r);
        kmac_step(k);
    }
    r->_qp_pump = rp;
    k->_qp_pump = kp;

    return r->pwrmgr_data_o_done == 0x6u;
}

/* ---------------------------------------------------------------- */
/* Image loaders (native ot_rom_ctrl.c parity)                       */
/* ---------------------------------------------------------------- */

static void ot_rom_ctrl_qp_boot_rust_demangle_fn(const char *st_name,
                                                 int st_info, uint64_t st_value,
                                                 uint64_t st_size)
{
    (void)st_info;
    (void)st_value;
    if (!st_size) {
        return;
    }
    rust_demangle_replace((char *)st_name);
}

static void qpb_load_elf(OtRomCtrlQpBootState *s, const OtRomImg *ri)
{
    AddressSpace *as = ot_common_get_local_address_space(DEVICE(s));
    hwaddr minaddr = s->mem.addr;
    hwaddr maxaddr = s->mem.addr + (hwaddr)memory_region_size(&s->mem);
    uint64_t loaddr;

    if (load_elf_ram_sym_nosz(ri->filename, NULL, NULL, NULL, NULL, &loaddr,
                              NULL, NULL, 0, EM_RISCV, 1, 0, as, false,
                              &ot_rom_ctrl_qp_boot_rust_demangle_fn,
                              true) <= 0) {
        error_setg(&error_fatal, "%s: %s: ROM image '%s', ELF loading failed",
                   __func__, s->ot_id, ri->filename);
        return;
    }
    if ((loaddr < minaddr) || (loaddr > maxaddr)) {
        error_setg(&error_fatal, "%s: %s: ELF cannot fit into ROM", __func__,
                   s->ot_id);
    }
}

static void qpb_load_binary(OtRomCtrlQpBootState *s, const OtRomImg *ri)
{
    if (ri->raw_size > s->size) {
        error_setg(&error_fatal, "%s: %s: cannot fit into ROM", __func__,
                   s->ot_id);
        return;
    }

    /* NOLINTNEXTLINE(misc-redundant-expression) */
    int fd = open(ri->filename, O_RDONLY | O_BINARY | O_CLOEXEC);
    if (fd == -1) {
        error_setg(&error_fatal, "%s: %s: could not open ROM '%s': %s",
                   __func__, s->ot_id, ri->filename, strerror(errno));
        return;
    }

    uint8_t *data = g_malloc0(ri->raw_size);
    ssize_t rc = read(fd, data, ri->raw_size);
    close(fd);

    if (rc != (ssize_t)ri->raw_size) {
        g_free(data);
        error_setg(&error_fatal,
                   "%s: %s: file %s: read error: rc=%zd (expected %u)",
                   __func__, s->ot_id, ri->filename, rc, ri->raw_size);
        return;
    }

    uint8_t *rom_ptr = (uint8_t *)memory_region_get_ram_ptr(&s->mem);
    memcpy(rom_ptr, data, ri->raw_size);
    g_free(data);
}

static char *qpb_read_text_file(OtRomCtrlQpBootState *s, const OtRomImg *ri)
{
    /* NOLINTNEXTLINE(misc-redundant-expression) */
    int fd = open(ri->filename, O_RDONLY | O_BINARY | O_CLOEXEC);
    if (fd == -1) {
        error_setg(&error_fatal, "%s: %s: could not open ROM '%s': %s",
                   __func__, s->ot_id, ri->filename, strerror(errno));
        return NULL;
    }

    char *buffer = g_malloc0(ri->raw_size + 1u);
    ssize_t rc = read(fd, buffer, ri->raw_size);
    close(fd);

    if (rc != (ssize_t)ri->raw_size) {
        g_free(buffer);
        error_setg(&error_fatal,
                   "%s: %s: file %s: read error: rc=%zd (expected %u)",
                   __func__, s->ot_id, ri->filename, rc, ri->raw_size);
        return NULL;
    }

    return buffer;
}

/* Parse a VMEM file.  Plain mode: 32-bit words straight into the
 * cleartext window.  Scrambled mode: 39-bit words (stored as u64, in
 * PHYSICAL order, the native on-disk convention) into phys[DEPTH]. */
static void qpb_load_vmem(OtRomCtrlQpBootState *s, const OtRomImg *ri,
                          bool scrambled, uint64_t *phys)
{
    char *buffer = qpb_read_text_file(s, ri);
    if (!buffer) {
        return;
    }

    uint8_t *rom_ptr = (uint8_t *)memory_region_get_ram_ptr(&s->mem);
    unsigned exp_addr = 0u;
    unsigned limit = scrambled ? QPB_DEPTH : (s->size / sizeof(uint32_t));
    const char *sep = "\r\n";
    char *brks;
    char *line;
    for (line = strtok_r(buffer, sep, &brks); line;
         line = strtok_r(NULL, sep, &brks)) {
        if (strlen(line) == 0u) {
            continue;
        }
        gchar **items = g_strsplit_set(line, " ", 0);
        if (items[0u][0u] != '@') {
            g_strfreev(items);
            continue;
        }
        unsigned blk_addr = (unsigned)g_ascii_strtoull(&items[0u][1], NULL, 16);
        if (blk_addr < exp_addr) {
            g_strfreev(items);
            g_free(buffer);
            error_setg(&error_fatal,
                       "%s: %s: address discrepancy in VMEM file '%s'",
                       __func__, s->ot_id, ri->filename);
            return;
        }
        exp_addr = blk_addr;
        for (unsigned blk = 0u; items[1u + blk]; blk++) {
            if (exp_addr >= limit) {
                g_strfreev(items);
                g_free(buffer);
                error_setg(&error_fatal, "%s: %s: VMEM file '%s' too large",
                           __func__, s->ot_id, ri->filename);
                return;
            }
            uint64_t value = g_ascii_strtoull(items[1u + blk], NULL, 16);
            if (scrambled) {
                phys[exp_addr] = value;
            } else {
                stl_le_p(&rom_ptr[exp_addr * sizeof(uint32_t)],
                         (uint32_t)value);
            }
            exp_addr++;
        }
        g_strfreev(items);
    }
    g_free(buffer);
}

static void qpb_load_hex(OtRomCtrlQpBootState *s, const OtRomImg *ri,
                         uint64_t *phys)
{
    char *buffer = qpb_read_text_file(s, ri);
    if (!buffer) {
        return;
    }

    unsigned pos = 0u;
    const char *sep = "\r\n";
    char *brks;
    char *line;
    for (line = strtok_r(buffer, sep, &brks); line;
         line = strtok_r(NULL, sep, &brks)) {
        if (strlen(line) == 0u) {
            continue;
        }
        if (pos >= QPB_DEPTH) {
            g_free(buffer);
            error_setg(&error_fatal, "%s: %s: HEX file '%s' too large",
                       __func__, s->ot_id, ri->filename);
            return;
        }
        char *end;
        uint64_t value = g_ascii_strtoull(line, &end, 16);
        if (((uintptr_t)end - (uintptr_t)line) != 10u) {
            g_free(buffer);
            error_setg(&error_fatal, "%s: %s: invalid line in HEX file '%s'",
                       __func__, s->ot_id, ri->filename);
            return;
        }
        phys[pos++] = value;
    }
    g_free(buffer);
}

/* ---------------------------------------------------------------- */
/* Scramble / descramble between the cleartext window and the         */
/* generated model's prim_rom backing store (physical order)          */
/* ---------------------------------------------------------------- */

static void qpb_scramble_window(OtRomCtrlQpBootState *s, uint64_t *phys)
{
    const uint8_t *rom_ptr = (const uint8_t *)memory_region_get_ram_ptr(&s->mem);

    /* data words: keystream xor, ECC bits zero (gen_image.py parity —
     * the digest covers the words exactly as stored) */
    for (uint32_t log = 0; log < QPB_DEPTH - QPB_TOP; log++) {
        uint64_t clr = ldl_le_p(&rom_ptr[log * sizeof(uint32_t)]);
        phys[qpb_addr_sp_enc(log)] = qpb_keystream(log) ^ clr;
    }
    /* trailer (digest) words stay as the caller left them */
}

static void qpb_descramble_window(OtRomCtrlQpBootState *s,
                                  const uint64_t *phys)
{
    uint8_t *rom_ptr = (uint8_t *)memory_region_get_ram_ptr(&s->mem);

    /* low 32 bits of keystream^word = data bits, ECC bits discarded */
    for (uint32_t log = 0; log < QPB_DEPTH - QPB_TOP; log++) {
        uint64_t clr = qpb_keystream(log) ^ phys[qpb_addr_sp_enc(log)];
        stl_le_p(&rom_ptr[log * sizeof(uint32_t)], (uint32_t)clr);
    }
}

/* ---------------------------------------------------------------- */
/* The boot flow                                                     */
/* ---------------------------------------------------------------- */

static void qpb_notify(OtRomCtrlQpBootState *s)
{
    /* force a fresh edge: the pwrmgr's event bits are cleared on every
     * machine reset and its handlers latch on level only */
    ibex_irq_set(&s->pwrmgr_good, false);
    ibex_irq_set(&s->pwrmgr_done, false);
    ibex_irq_set(&s->pwrmgr_good, s->good_cached);
    ibex_irq_set(&s->pwrmgr_done, s->done_cached);
}

void ot_rom_ctrl_qp_boot_run(DeviceState *dev)
{
    OtRomCtrlQpBootState *s = OT_ROM_CTRL_QP_BOOT(dev);

    if (s->sealed) {
        /* subsequent resets: keep the digest verdict, re-notify */
        qpb_notify(s);
        return;
    }

    rom_ctrl_state *r = ot_rom_ctrl_qp_core(s->rom_ctrl);
    kmac_state *k = ot_kmac_qp_core(s->kmac);

    /* find the ROM image object (-object ot-rom_img,id=<img-id>) */
    Object *obj =
        object_resolve_path_component(object_get_objects_root(), s->img_id);

    if (!obj) {
        /* native parity: no ROM image is a QEMU-only shortcut — boot
         * with an empty (all-zero) window and a pass verdict */
        s->sealed = true;
        s->done_cached = true;
        s->good_cached = true;
        memory_region_rom_device_set_romd(&s->mem, true);
        qpb_notify(s);
        return;
    }

    OtRomImg *ri = (OtRomImg *)object_dynamic_cast(obj, TYPE_OT_ROM_IMG);
    if (!ri) {
        error_setg(&error_fatal, "%s: %s: Object is not a ROM Image", __func__,
                   s->ot_id);
        return;
    }

    uint64_t *phys = g_new0(uint64_t, QPB_DEPTH);
    bool cleartext;

    switch (ri->format) {
    case OT_ROM_IMG_FORMAT_ELF:
        qpb_load_elf(s, ri);
        cleartext = true;
        break;
    case OT_ROM_IMG_FORMAT_BINARY:
        qpb_load_binary(s, ri);
        cleartext = true;
        break;
    case OT_ROM_IMG_FORMAT_VMEM_PLAIN:
        qpb_load_vmem(s, ri, false, NULL);
        cleartext = true;
        break;
    case OT_ROM_IMG_FORMAT_VMEM_SCRAMBLED_ECC:
        qpb_load_vmem(s, ri, true, phys);
        cleartext = false;
        break;
    case OT_ROM_IMG_FORMAT_HEX_SCRAMBLED_ECC:
        qpb_load_hex(s, ri, phys);
        cleartext = false;
        break;
    case OT_ROM_IMG_FORMAT_NONE:
    default:
        error_setg(&error_fatal, "%s: %s: unable to read binary file '%s'",
                   __func__, s->ot_id, ri->filename);
        g_free(phys);
        return;
    }

    bool done;
    if (cleartext) {
        /* dev mode (native local_gen parity): scramble the window,
         * pass 1 computes the digest, the trailer gets it, pass 2
         * re-checks the final image with the model's real logic */
        qpb_scramble_window(s, phys);
        rom_ctrl_load_backing(r, 0, phys, QPB_DEPTH * sizeof(uint64_t));
        qpb_core_rearm(r);

        uint64_t dig0[6] = { 0 }, dig1[6] = { 0 };
        bool have_dig = false;
        done = qpb_bridge(r, k, dig0, dig1, &have_dig);
        if (!done || !have_dig) {
            warn_report("%s: digest pass did not complete (done=%x)", s->ot_id,
                        r->pwrmgr_data_o_done);
        } else {
            for (unsigned i = 0; i < QPB_TOP; i++) {
                uint32_t dword =
                    (uint32_t)((dig0[i / 2u] ^ dig1[i / 2u]) >>
                               (32u * (i % 2u)));
                phys[qpb_addr_sp_enc(QPB_DEPTH - QPB_TOP + i)] = dword;
            }
            rom_ctrl_load_backing(r, 0, phys, QPB_DEPTH * sizeof(uint64_t));
            qpb_core_rearm(r);
            done = qpb_bridge(r, k, NULL, NULL, NULL);
        }
    } else {
        /* scrambled image: single pass, the trailer is the image's own
         * claim and the model's verdict is final */
        qpb_descramble_window(s, phys);
        rom_ctrl_load_backing(r, 0, phys, QPB_DEPTH * sizeof(uint64_t));
        qpb_core_rearm(r);
        done = qpb_bridge(r, k, NULL, NULL, NULL);
    }
    g_free(phys);

    bool good = done && (r->pwrmgr_data_o_good == 0x6u);

    s->sealed = true;
    s->done_cached = done;
    s->good_cached = good;

    memory_region_set_dirty(&s->mem, 0, s->size);
    memory_region_rom_device_set_romd(&s->mem, good);

    if (!good) {
        warn_report("%s: generated rom_ctrl REFUSED the ROM image "
                    "(done=%x good=%x) — CPU stays held",
                    s->ot_id, r->pwrmgr_data_o_done, r->pwrmgr_data_o_good);
    }

    qpb_notify(s);
}

/* ---------------------------------------------------------------- */
/* QOM plumbing                                                      */
/* ---------------------------------------------------------------- */

static const Property ot_rom_ctrl_qp_boot_properties[] = {
    DEFINE_PROP_STRING(OT_COMMON_DEV_ID, OtRomCtrlQpBootState, ot_id),
    DEFINE_PROP_STRING("img-id", OtRomCtrlQpBootState, img_id),
    DEFINE_PROP_UINT32("size", OtRomCtrlQpBootState, size, 0x8000u),
    DEFINE_PROP_LINK("rom-ctrl-qp", OtRomCtrlQpBootState, rom_ctrl,
                     TYPE_OT_ROM_CTRL_QP, DeviceState *),
    DEFINE_PROP_LINK("kmac-qp", OtRomCtrlQpBootState, kmac, TYPE_OT_KMAC_QP,
                     DeviceState *),
};

static void ot_rom_ctrl_qp_boot_realize(DeviceState *dev, Error **errp)
{
    OtRomCtrlQpBootState *s = OT_ROM_CTRL_QP_BOOT(dev);

    g_assert(s->ot_id);
    g_assert(s->size);
    g_assert(s->rom_ctrl);
    g_assert(s->kmac);
    g_assert(s->size == (QPB_DEPTH - QPB_TOP) * sizeof(uint32_t) + 32u);

    memory_region_init_rom_device_nomigrate(&s->mem, OBJECT(dev),
                                            &ot_rom_ctrl_qp_boot_mem_ops, s,
                                            TYPE_OT_ROM_CTRL_QP_BOOT ".mem",
                                            s->size, errp);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->mem);

    /* reads disabled until the generated model's verdict is in */
    memory_region_rom_device_set_romd(&s->mem, false);
}

static void ot_rom_ctrl_qp_boot_init(Object *obj)
{
    OtRomCtrlQpBootState *s = OT_ROM_CTRL_QP_BOOT(obj);

    ibex_qdev_init_irq(obj, &s->pwrmgr_good, OT_ROM_CTRL_GOOD);
    ibex_qdev_init_irq(obj, &s->pwrmgr_done, OT_ROM_CTRL_DONE);
}

static void ot_rom_ctrl_qp_boot_finalize(Object *obj)
{
    (void)obj;
}

static void ot_rom_ctrl_qp_boot_class_init(ObjectClass *klass,
                                           const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    (void)data;

    dc->realize = &ot_rom_ctrl_qp_boot_realize;
    device_class_set_props(dc, ot_rom_ctrl_qp_boot_properties);
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}
