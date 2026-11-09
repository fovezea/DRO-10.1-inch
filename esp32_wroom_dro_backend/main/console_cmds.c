/**
 * @file console_cmds.c
 * @brief Human-friendly setup shell on UART0.
 *
 * The console is the "no tools required" path: every setting the frontend can
 * change over the link can also be changed here, and `status` / `diag` show
 * exactly what the scales are doing while the board is still on the bench.
 */

#include "console_cmds.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "axis_reader.h"
#include "board_pins.h"
#include "config_store.h"
#include "dro_link_protocol.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "link.h"
#include "sdkconfig.h"
#include "transport_espnow.h"
#include "transport_uart.h"

static const char *TAG = "console";

static const char *const AXIS_NAMES[AXIS_READER_COUNT] = {"X", "Y", "Z", "W", "V"};

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */
static int axis_from_string(const char *s)
{
    if (s == NULL || s[0] == '\0') {
        return -1;
    }
    if (s[1] == '\0') {
        if (s[0] >= '0' && s[0] <= '4') {
            return s[0] - '0';
        }
        for (int i = 0; i < AXIS_READER_COUNT; i++) {
            if (toupper((unsigned char)s[0]) == AXIS_NAMES[i][0]) {
                return i;
            }
        }
    }
    return -1;
}

static const char *axis_name(int axis)
{
    return (axis >= 0 && axis < AXIS_READER_COUNT) ? AXIS_NAMES[axis] : "?";
}

static int parse_bool(const char *s)
{
    if (s == NULL) {
        return -1;
    }
    if (!strcmp(s, "1") || !strcasecmp(s, "on") || !strcasecmp(s, "true") ||
        !strcasecmp(s, "yes")) {
        return 1;
    }
    if (!strcmp(s, "0") || !strcasecmp(s, "off") || !strcasecmp(s, "false") ||
        !strcasecmp(s, "no")) {
        return 0;
    }
    return -1;
}

static bool parse_mac(const char *s, uint8_t mac[6])
{
    unsigned int v[6];
    if (s == NULL || sscanf(s, "%2x:%2x:%2x:%2x:%2x:%2x",
                            &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; i++) {
        mac[i] = (uint8_t)v[i];
    }
    return true;
}

static void mac_str(const uint8_t mac[6], char out[18])
{
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             (unsigned)mac[0], (unsigned)mac[1], (unsigned)mac[2],
             (unsigned)mac[3], (unsigned)mac[4], (unsigned)mac[5]);
}

static const char *link_mode_name(uint8_t mode)
{
    switch (mode) {
    case DRO_LINK_MODE_UART:   return "uart";
    case DRO_LINK_MODE_ESPNOW: return "espnow";
    default:                   return "auto";
    }
}

static const char *transport_name(dro_transport_id_t tr)
{
    switch (tr) {
    case DRO_TR_UART:   return "uart";
    case DRO_TR_ESPNOW: return "espnow";
    default:            return "none";
    }
}

static void apply_and_save(dro_cfg_t *cfg, bool save)
{
    config_store_set(cfg);
    dro_link_apply_config();
    if (save) {
        config_store_save();
    }
}

/* ------------------------------------------------------------------------- */
/* Commands                                                                  */
/* ------------------------------------------------------------------------- */
static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    dro_link_status_t st;
    dro_link_get_status(&st);
    char mac[18];
    mac_str(st.own_mac, mac);

    dro_cfg_t cfg;
    config_store_get(&cfg);

    printf("\nESP32-WROOM-32E 5-axis DRO backend (board Rev C)\n");
    printf("  firmware      : 1.0, protocol %d\n", DRO_LINK_VERSION);
    printf("  name          : %s\n", cfg.name);
    printf("  station MAC   : %s\n", mac);
    printf("  axes          : X IO34/35/36  Y IO39/32/33  Z IO25/26/27\n");
    printf("                  W IO14/13/4    V IO16/17/18\n");
    printf("  wired link    : UART%d TX IO%d RX IO%d @ %" PRIu32 " baud\n",
#if CONFIG_DRO_LINK_UART_PORT_0
           0, 1, 3,
#else
           2, CONFIG_DRO_LINK_UART_TX_GPIO, CONFIG_DRO_LINK_UART_RX_GPIO,
#endif
           transport_uart_baud());
    printf("  wireless link : ESP-NOW channel %u (%s)\n", (unsigned)cfg.espnow_channel,
           st.wifi_ready ? "ready" : (st.wifi_error ? "error" : "off"));
    printf("  status LED    : IO%d\n\n", CONFIG_DRO_STATUS_LED_GPIO);
    return 0;
}

static int cmd_status(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    dro_link_status_t st;
    dro_link_get_status(&st);

    dro_cfg_t cfg;
    config_store_get(&cfg);

    if (st.session_valid) {
        char mac[18];
        mac_str(st.session_mac, mac);
        printf("link   : %s %s, last frame %" PRIu32 " ms ago, %u Hz, axes 0x%02x\n",
               transport_name(st.session_transport), mac, st.session_age_ms,
               (unsigned)st.session_hz, (unsigned)st.session_axes_mask);
    } else {
        printf("link   : no session (waiting for HELLO_REQ%s)\n",
               st.wifi_ready ? ", ESP-NOW beaconing" : "");
    }

    printf("frames : rx %" PRIu32 "  tx %" PRIu32 "  crc-errors %" PRIu32
           "  dropped %" PRIu32 "\n",
           st.rx_frames, st.tx_frames, st.crc_errors, st.dropped);
    printf("wifi   : %s, channel %u, mode %s\n",
           st.wifi_ready ? "ready" : (st.wifi_error ? "ERROR" : "off"),
           (unsigned)cfg.espnow_channel, link_mode_name(cfg.link_mode));
    printf("config : enabled 0x%02x  inverted 0x%02x  index 0x%02x%s\n",
           (unsigned)cfg.enabled_mask, (unsigned)cfg.invert_mask,
           (unsigned)cfg.index_mask,
           config_store_dirty() ? "  (unsaved)" : "");
    printf("sim    : %s\n", axis_reader_simulation() ? "on" : "off");

    printf("\naxis  en dir   raw count        position     counts/s  idx  ovf\n");
    for (int a = 0; a < AXIS_READER_COUNT; a++) {
        axis_reader_diag_t d;
        axis_reader_get_diag(a, &d);
        printf(" %-4s %2s %3s   %14" PRId64 "  %12" PRId32 "  %8" PRIu32 "  %3" PRIu32
               "  %3" PRIu32 "\n",
               axis_name(a),
               (cfg.enabled_mask & (1u << a)) ? "on" : "off",
               (cfg.invert_mask & (1u << a)) ? "-" : "+",
               d.raw, d.pos, d.counts_per_s, d.index_count, d.overflow_count);
    }
    printf("\n");
    return 0;
}

static int cmd_diag(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    dro_link_status_t st;
    dro_link_get_status(&st);

    printf("heap       : %" PRIu32 " bytes free\n", (uint32_t)esp_get_free_heap_size());
    printf("sampler    : 1000 Hz\n");
    printf("frames     : rx %" PRIu32 " tx %" PRIu32 " crc %" PRIu32 " drop %" PRIu32 "\n",
           st.rx_frames, st.tx_frames, st.crc_errors, st.dropped);
    printf("selftest   : %s\n", st.self_test_ok ? "PASS" : "not run / failed");

    for (int a = 0; a < AXIS_READER_COUNT; a++) {
        axis_reader_diag_t d;
        axis_reader_get_diag(a, &d);
        printf(" %s: raw=%" PRId64 " pos=%" PRId32 " rate=%" PRIu32
               " cnt/s ovf=%" PRIu32 " idx=%" PRIu32 "\n",
               axis_name(a), d.raw, d.pos, d.counts_per_s, d.overflow_count, d.index_count);
    }
    return 0;
}

static int cmd_zero(int argc, char **argv)
{
    uint8_t mask = 0x1F;

    if (argc >= 2) {
        if (!strcasecmp(argv[1], "all")) {
            mask = 0x1F;
        } else {
            int a = axis_from_string(argv[1]);
            if (a < 0) {
                printf("usage: zero [all|X|Y|Z|W|V]\n");
                return 1;
            }
            mask = (uint8_t)(1u << a);
        }
    }

    axis_reader_zero(mask);
    printf("zeroed mask 0x%02x\n", (unsigned)mask);
    return 0;
}

static int cmd_preset(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: preset <axis> <count>\n");
        return 1;
    }
    int a = axis_from_string(argv[1]);
    if (a < 0) {
        printf("bad axis '%s'\n", argv[1]);
        return 1;
    }
    int32_t value = (int32_t)strtol(argv[2], NULL, 0);
    esp_err_t err = axis_reader_preset(a, value);
    printf("%s\n", err == ESP_OK ? "ok" : esp_err_to_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_invert(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: invert <axis> <0|1>\n");
        return 1;
    }
    int a = axis_from_string(argv[1]);
    int v = parse_bool(argv[2]);
    if (a < 0 || v < 0) {
        printf("bad arguments\n");
        return 1;
    }

    dro_cfg_t cfg;
    config_store_get(&cfg);
    if (v) {
        cfg.invert_mask |= (uint8_t)(1u << a);
    } else {
        cfg.invert_mask &= (uint8_t)~(1u << a);
    }
    apply_and_save(&cfg, true);
    printf("axis %s inverted=%d\n", axis_name(a), v);
    return 0;
}

static int cmd_enable(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: enable <axis> <0|1>\n");
        return 1;
    }
    int a = axis_from_string(argv[1]);
    int v = parse_bool(argv[2]);
    if (a < 0 || v < 0) {
        printf("bad arguments\n");
        return 1;
    }

    dro_cfg_t cfg;
    config_store_get(&cfg);
    if (v) {
        cfg.enabled_mask |= (uint8_t)(1u << a);
    } else {
        cfg.enabled_mask &= (uint8_t)~(1u << a);
    }
    apply_and_save(&cfg, true);
    printf("axis %s enabled=%d\n", axis_name(a), v);
    return 0;
}

static int cmd_index(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: index <axis> <0|1>\n");
        return 1;
    }
    int a = axis_from_string(argv[1]);
    int v = parse_bool(argv[2]);
    if (a < 0 || v < 0) {
        printf("bad arguments\n");
        return 1;
    }

    dro_cfg_t cfg;
    config_store_get(&cfg);
    if (v) {
        cfg.index_mask |= (uint8_t)(1u << a);
    } else {
        cfg.index_mask &= (uint8_t)~(1u << a);
    }
    apply_and_save(&cfg, true);
    printf("axis %s index=%d\n", axis_name(a), v);
    return 0;
}

static int cmd_rate(int argc, char **argv)
{
    if (argc < 2) {
        dro_cfg_t cfg;
        config_store_get(&cfg);
        printf("default telemetry rate: %u Hz\n", cfg.stream_hz);
        return 0;
    }
    int hz = atoi(argv[1]);
    if (hz < 1 || hz > 200) {
        printf("rate must be 1..200 Hz\n");
        return 1;
    }
    dro_cfg_t cfg;
    config_store_get(&cfg);
    cfg.stream_hz = (uint16_t)hz;
    apply_and_save(&cfg, true);
    printf("default telemetry rate: %d Hz\n", hz);
    return 0;
}

static int cmd_sim(int argc, char **argv)
{
    if (argc < 2) {
        printf("simulation: %s\n", axis_reader_simulation() ? "on" : "off");
        return 0;
    }
    int on = parse_bool(argv[1]);
    if (on < 0) {
        printf("usage: sim <on|off> [counts_per_s]\n");
        return 1;
    }
    int32_t rate = (argc >= 3) ? (int32_t)strtol(argv[2], NULL, 0) : 1000;
    axis_reader_set_simulation(on != 0, rate);
    printf("simulation %s at %" PRId32 " counts/s\n", on ? "on" : "off", rate);
    return 0;
}

static int cmd_link(int argc, char **argv)
{
    dro_cfg_t cfg;
    config_store_get(&cfg);

    if (argc < 2 || !strcasecmp(argv[1], "status")) {
        dro_link_status_t st;
        dro_link_get_status(&st);
        printf("mode    : %s\n", link_mode_name(cfg.link_mode));
        printf("espnow  : %s, channel %u\n",
               st.wifi_ready ? "ready" : (st.wifi_error ? "ERROR" : "off"),
               (unsigned)cfg.espnow_channel);
        if (cfg.peer_locked) {
            char mac[18];
            mac_str(cfg.peer_mac, mac);
            printf("locked  : %s\n", mac);
        } else {
            printf("locked  : no\n");
        }
        if (st.session_valid) {
            char mac[18];
            mac_str(st.session_mac, mac);
            printf("session : %s %s @ %u Hz\n", transport_name(st.session_transport),
                   mac, (unsigned)st.session_hz);
        }
        return 0;
    }

    if (!strcasecmp(argv[1], "mode")) {
        if (argc < 3) {
            printf("usage: link mode auto|uart|espnow\n");
            return 1;
        }
        if (!strcasecmp(argv[2], "auto")) {
            cfg.link_mode = DRO_LINK_MODE_AUTO;
        } else if (!strcasecmp(argv[2], "uart")) {
            cfg.link_mode = DRO_LINK_MODE_UART;
        } else if (!strcasecmp(argv[2], "espnow")) {
            cfg.link_mode = DRO_LINK_MODE_ESPNOW;
        } else {
            printf("usage: link mode auto|uart|espnow\n");
            return 1;
        }
        apply_and_save(&cfg, true);
        printf("link mode: %s\n", link_mode_name(cfg.link_mode));
        return 0;
    }

    if (!strcasecmp(argv[1], "channel")) {
        if (argc < 3) {
            printf("channel: %u\n", (unsigned)cfg.espnow_channel);
            return 0;
        }
        int ch = atoi(argv[2]);
        if (ch < 1 || ch > 13) {
            printf("channel must be 1..13\n");
            return 1;
        }
        cfg.espnow_channel = (uint8_t)ch;
        apply_and_save(&cfg, true);
        printf("channel: %d\n", ch);
        return 0;
    }

    if (!strcasecmp(argv[1], "peer")) {
        if (argc < 3) {
            char mac[18];
            mac_str(cfg.peer_mac, mac);
            printf("peer: %s (lock %s)\n", mac, cfg.peer_locked ? "on" : "off");
            return 0;
        }
        if (!strcasecmp(argv[2], "none") || !strcasecmp(argv[2], "clear")) {
            memset(cfg.peer_mac, 0, sizeof(cfg.peer_mac));
            cfg.peer_locked = 0;
            apply_and_save(&cfg, true);
            printf("peer cleared\n");
            return 0;
        }
        uint8_t mac[6];
        if (!parse_mac(argv[2], mac)) {
            printf("usage: link peer aa:bb:cc:dd:ee:ff | none\n");
            return 1;
        }
        memcpy(cfg.peer_mac, mac, sizeof(mac));
        cfg.peer_locked = 1;
        apply_and_save(&cfg, true);
        printf("peer locked to %s\n", argv[2]);
        return 0;
    }

    if (!strcasecmp(argv[1], "lock")) {
        if (argc < 3) {
            printf("lock: %s\n", cfg.peer_locked ? "on" : "off");
            return 0;
        }
        int on = parse_bool(argv[2]);
        if (on < 0) {
            printf("usage: link lock <0|1>\n");
            return 1;
        }
        cfg.peer_locked = (uint8_t)on;
        apply_and_save(&cfg, true);
        printf("peer lock: %s\n", on ? "on" : "off");
        return 0;
    }

    printf("usage: link [status|mode ...|channel N|peer MAC|lock 0|1]\n");
    return 1;
}

static int cmd_uart(int argc, char **argv)
{
    dro_cfg_t cfg;
    config_store_get(&cfg);

    if (argc < 2 || !strcasecmp(argv[1], "status")) {
        printf("link UART baud: %" PRIu32 " (configured %" PRIu32 ")\n",
               transport_uart_baud(), cfg.uart_baud);
        return 0;
    }
    if (!strcasecmp(argv[1], "baud")) {
        if (argc < 3) {
            printf("usage: uart baud <rate>\n");
            return 1;
        }
        uint32_t baud = (uint32_t)strtoul(argv[2], NULL, 0);
        if (baud < 9600 || baud > 3000000) {
            printf("baud must be 9600..3000000\n");
            return 1;
        }
        cfg.uart_baud = baud;
        apply_and_save(&cfg, true);
        printf("link UART baud: %" PRIu32 "\n", baud);
        return 0;
    }
    printf("usage: uart [status|baud N]\n");
    return 1;
}

static int cmd_save(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    esp_err_t err = config_store_save();
    printf("%s\n", err == ESP_OK ? "saved" : esp_err_to_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_reset(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    esp_err_t err = config_store_reset();
    dro_link_apply_config();
    printf("%s\n", err == ESP_OK ? "settings restored to defaults" : esp_err_to_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("rebooting...\n");
    fflush(stdout);
    esp_restart();
    return 0;
}

static int cmd_selftest(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    bool ok = true;

    /* CRC-16/CCITT-FALSE check value for "123456789" is 0x29B1. */
    static const uint8_t check[] = "123456789";
    uint16_t crc = dro_crc16(check, 9);
    printf("crc16    : 0x%04x (expect 0x29b1)\n", crc);
    ok = ok && (crc == 0x29B1);

    /* Fixed wire vector, generated independently by tools/dro_link_test.py:
     * TELEMETRY, seq 42, payload 00 01 .. 1f.  If this matches, the C and the
     * Python encoder agree byte for byte. */
    static const uint8_t k_vector[] = {
        0x04, 0x01, 0x02, 0x2a, 0x02, 0x20, 0x01, 0x22, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
        0x1d, 0x1e, 0x1f, 0x50, 0x56, 0x00,
    };
    uint8_t vp[32];
    for (size_t i = 0; i < sizeof(vp); i++) {
        vp[i] = (uint8_t)i;
    }
    uint8_t vw[64];
    int vn = dro_link_frame_encode(0x02, 42, 0, vp, sizeof(vp), vw, sizeof(vw));
    bool vec_ok = (vn == (int)sizeof(k_vector)) &&
                  (memcmp(vw, k_vector, sizeof(k_vector)) == 0);
    printf("vector   : %s (%d bytes)\n", vec_ok ? "OK" : "FAIL", vn);
    ok = ok && vec_ok;

    /* COBS round trip through a payload full of zeros. */
    uint8_t raw[64];
    for (size_t i = 0; i < sizeof(raw); i++) {
        raw[i] = (uint8_t)((i % 5 == 0) ? 0 : i * 7);
    }
    uint8_t enc[96];
    uint8_t dec[96];
    size_t enc_len = dro_cobs_encode(raw, sizeof(raw), enc, sizeof(enc));
    int dec_len = dro_cobs_decode(enc, enc_len, dec, sizeof(dec));
    printf("cobs     : %u -> %u -> %d\n", (unsigned)sizeof(raw), (unsigned)enc_len, dec_len);
    ok = ok && (dec_len == (int)sizeof(raw)) && (memcmp(raw, dec, sizeof(raw)) == 0);

    /* Full frame round trip. */
    dro_telemetry_t t = {0};
    t.sample_seq = 0x12345678;
    t.uptime_ms = 4242;
    t.axis_count = AXIS_READER_COUNT;
    for (int i = 0; i < AXIS_READER_COUNT; i++) {
        t.pos[i] = (int32_t)(i * 1000 - 2000);
    }

    uint8_t wire[DRO_LINK_MAX_WIRE];
    int wire_len = dro_link_frame_encode(DRO_MSG_TELEMETRY, 42, 0, &t, sizeof(t),
                                         wire, sizeof(wire));
    uint8_t scratch[DRO_LINK_MAX_FRAME];
    dro_frame_t f;
    int rc = dro_link_frame_decode(wire, (size_t)wire_len, scratch, sizeof(scratch), &f);
    bool frame_ok = (rc == 0) && (f.type == DRO_MSG_TELEMETRY) && (f.seq == 42) &&
                    (f.payload_len == sizeof(t)) && (memcmp(f.payload, &t, sizeof(t)) == 0);
    printf("frame    : %d bytes, decode %s\n", wire_len, frame_ok ? "OK" : "FAIL");
    ok = ok && frame_ok;

    /* Single bit flip must be rejected by the CRC. */
    if (wire_len > 6) {
        uint8_t save = wire[wire_len / 2];
        wire[wire_len / 2] ^= 0x01;
        int bad = dro_link_frame_decode(wire, (size_t)wire_len, scratch, sizeof(scratch), &f);
        wire[wire_len / 2] = save;
        printf("corrupt  : %s\n", bad != 0 ? "rejected (OK)" : "ACCEPTED (FAIL)");
        ok = ok && (bad != 0);
    }

    dro_link_mark_self_test(ok);
    printf("selftest : %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}

/* ------------------------------------------------------------------------- */
/* Registration                                                              */
/* ------------------------------------------------------------------------- */
static const esp_console_cmd_t s_commands[] = {
    { .command = "info",     .help = "Board, pin map and firmware information", .func = cmd_info },
    { .command = "status",   .help = "Link state and live counts for all axes", .func = cmd_status },
    { .command = "diag",     .help = "Heap, frame counters, per-axis diagnostics", .func = cmd_diag },
    { .command = "zero",     .help = "zero [all|X|Y|Z|W|V] - set position to 0", .func = cmd_zero },
    { .command = "preset",   .help = "preset <axis> <count> - set position", .func = cmd_preset },
    { .command = "invert",   .help = "invert <axis> <0|1> - flip counting direction", .func = cmd_invert },
    { .command = "enable",   .help = "enable <axis> <0|1> - mark axis present", .func = cmd_enable },
    { .command = "index",    .help = "index <axis> <0|1> - enable Z pulse capture", .func = cmd_index },
    { .command = "rate",     .help = "rate [Hz] - default telemetry rate", .func = cmd_rate },
    { .command = "sim",      .help = "sim <on|off> [counts/s] - synthetic counts", .func = cmd_sim },
    { .command = "link",     .help = "link [status|mode|channel|peer|lock]", .func = cmd_link },
    { .command = "uart",     .help = "uart [status|baud N] - wired link settings", .func = cmd_uart },
    { .command = "save",     .help = "Write the current settings to NVS", .func = cmd_save },
    { .command = "reset",    .help = "Restore factory settings", .func = cmd_reset },
    { .command = "selftest", .help = "Run the protocol codec self-test", .func = cmd_selftest },
    { .command = "reboot",   .help = "Restart the ESP32", .func = cmd_reboot },
};

esp_err_t console_init(void)
{
#if CONFIG_DRO_CONSOLE_ENABLE
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "dro>";
    repl_cfg.max_cmdline_length = 160;

    esp_console_dev_uart_config_t uart_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    esp_err_t err = esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_uart: %s", esp_err_to_name(err));
        return err;
    }

    for (size_t i = 0; i < sizeof(s_commands) / sizeof(s_commands[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&s_commands[i]));
    }
    ESP_ERROR_CHECK(esp_console_register_help_command());

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "console ready - type 'help'");
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
