# Board port: adding the osptek ESP32-P4C5 (JD9366) board to DRO

Status: plan, nothing implemented. Branch `feature/board-port`, cut from `feature/idf6.1`
because no local `dev` existed at the time; rebase onto `dev` once it has been fetched and
merged (see §8).

Goal: one DRO application, two boards, selected at build time — the Guition board it runs on
today, and the osptek ESP32-P4C5 board from the HMI project. Repos stay separate; only the
driver sources and a co-processor firmware image move from `HMI-10.1-esp32-p4c6`.

## 1. Rotation: audited first, because it decides the design

Both projects rotate in **LVGL**, not in the panel driver. `bsp_display_start()` registers the
display at the panel's native 800×1280 with `rotation = {swap_xy: false, mirror_x: false,
mirror_y: false}` and `sw_rotate = true`; `main.c` then calls `lv_display_set_rotation()`.

So the "one board rotated in the driver, one in LVGL" risk does **not** materialise. Something
else does:

| | DRO / Guition (JD9365) | HMI / osptek (JD9366) |
|---|---|---|
| Angle | `LV_DISPLAY_ROTATION_270` | `LV_DISPLAY_ROTATION_90` |
| Where applied | hardcoded in `main.c`, **twice** (lines ~349 and ~428) | hardcoded in `main.c`, once |
| Rotation engine | **CPU** — `CONFIG_LVGL_PORT_ENABLE_PPA` is **not set** | **PPA** — `CONFIG_LVGL_PORT_ENABLE_PPA=y` |
| Touch config | GSL3680, `mirror_x = 1`, x_max/y_max = 800×1280 | JD9366 in-cell, all flags 0 |
| Native panel | 800×1280, rotated to 1280×800 logical | 800×1280, rotated to 1280×800 logical |

Three facts follow:

1. **The two boards sit 180° apart.** 270 vs 90 both yield a 1280×800 logical screen, but the
   content is flipped. Dropping the osptek BSP into DRO with `main.c` still at 270 would show
   the UI upside down — silently, because the resolution matches.
2. **The angle lives in the wrong place.** It is a property of how the panel is mounted on a
   particular board, yet DRO has it in `main.c` (in two independent init paths, which can drift
   apart). Any board port that leaves it there is a landmine.
3. **The rendering engines differ.** The osptek path was verified with PPA; DRO runs CPU
   rotation with PPA off. That may be *why* DRO rotates in CPU: your note that the JD9365 driver
   "did not cooperate" suggests PPA rotation was tried and abandoned there. Keep each board on
   the mechanism that is proven for it rather than forcing one.

Also: touch coordinates are mapped through the display rotation, so **changing the angle
invalidates the touch calibration**. DRO's `mirror_x = 1` was tuned against 270°; the osptek
in-cell driver was verified with all-zero flags at 90°. Each pair (angle, touch flags) must be
validated together on real hardware.

### The rotation contract this port will enforce

- One angle per board, defined in the **BSP**, applied exactly once inside
  `bsp_display_start()`. `main.c` stops calling `lv_display_set_rotation()` at all.
- One rendering mechanism per board, selected by Kconfig: `BSP_DISPLAY_ROTATE_PPA` (osptek,
  proven) or CPU rotation (Guition, current behaviour — do not change what works).
- Touch flags per board, next to the angle, with a comment tying them to it.
- Acceptance for every board: UI upright **and** a dragged slider that tracks the finger.

## 2. Board comparison (corrected)

Both panels are native **800×1280** and both are rotated to 1280×800 landscape; the earlier note
calling the Guition panel "1280×800 native" was wrong.

| | DRO / Guition | HMI / osptek |
|---|---|---|
| P4 silicon | rev < 3.0 (`SELECTS_REV_LESS_V3`, max v1.99) | **rev v3.2** (min v3.1, max v3.99) |
| Radio | ESP32-C6 | ESP32-C5 (2.4 + 5 GHz) |
| Panel | JD9365 | **JD9366** |
| Touch | GSL3680 / GT911, separate IC | **JD9366 in-cell**, same I²C bus |
| I²C | SDA 7 / SCL 8 | SDA 7 / SCL 8 — same |
| I²S | 13/12/10/9/11 | 13/12/10/9/11 — same |
| PA enable | GPIO20 (schematic pin 22 — the "22" on the drawing is the pin number) | GPIO53 |
| LCD RST / BL | 27 / 23 | not wired / hardwired |
| TF card | 39-41… | 39-44 + detect 45 |
| CP SDIO (P4 side) | **14-19** | **14-19** — same |
| Hosted | esp_hosted 3.1.0, C6 CP | 3.1.0, C5 CP |

Shared: SDIO pins, audio, I²C, TF card, LVGL 9.2 (DRO) with the RGB565 compat shim,
esp_lvgl_port 2.9.0, esp_codec_dev 1.6.2. Different: panel driver, touch driver, three pins,
P4 revision family, CP target, rotation angle, rotation engine.

## 3. Design

**Extend the existing BSP with a board choice; do not add a second BSP component.** DRO's
`components/esp32_p4_function_ev_board` already implements the whole surface the app uses
(`bsp_display_start`, `bsp_display_backlight_on`, `bsp_audio_init`,
`bsp_audio_codec_speaker_init`, `bsp_sdcard_mount`, `bsp_i2c_init`) and already has a
`choice BSP_LCD_TYPE`. A second BSP would duplicate all of that, including the parts that were
hard to get right.

**Two selection axes, deliberately separate:**

| Axis | Where | Values |
|---|---|---|
| Board / panel / touch / rotation | Kconfig, this component | `BSP_BOARD_EV_JD9365` (default), `BSP_BOARD_OSPTEK_JD9366` |
| SoC-level profile | `sdkconfig.board.<profile>` overlays + `CMakeLists.txt` | `guition` (P4 rev < 3.0, `SLAVE_IDF_TARGET_ESP32C6`), `osptek` (rev ≥ 3.0, `..._ESP32C5`) |

They must be separate because the P4 revision families are mutually exclusive in IDF and DRO's
`sdkconfig` is committed — a Kconfig-only switch cannot change `ESP32P4_SELECTS_REV_LESS_V3`
without editing a tracked file.

## 4. Work breakdown

**S1 — branch and this document.** `feature/board-port`; rebase onto `dev` after the merge.

**S2 — drivers.** Copy `components/esp_lcd_jd9366/` from HMI into DRO (5 files: panel + in-cell
touch driver). DRO keeps `esp_lcd_jd9365`. *Acceptance:* builds green with the component present
but unreferenced.

**S3 — BSP board choice.** Add `choice BSP_BOARD` to `components/esp32_p4_function_ev_board/Kconfig`;
move the current JD9365 path under `BSP_BOARD_EV_JD9365` unchanged; add `BSP_BOARD_OSPTEK_JD9366`
that instantiates `esp_lcd_jd9366` with the vendor DPI macros (80 MHz, 8/28/40 + 8/20/140 =
63.07 Hz), `BSP_LCD_RST`/`BSP_LCD_BL` = NC, PA = 53, JD9366 in-cell touch on the existing I²C
bus, and the rotation contract from §1. *Acceptance:* Guition build unchanged and still upright;
osptek build upright with correct touch.

**S4 — sdkconfig profiles.** `sdkconfig.board.guition` / `sdkconfig.board.osptek` plus the
`CMakeLists.txt` selection (same mechanism as `HMI-10.1-esp32-p4c6`). *Acceptance:* both
profiles configure; `ESP_REV_MAX_FULL` is 199 / 399 respectively; the C5 profile builds an image
that boots on v3.2.

**S5 — co-processor firmware.** Copy `firmware/ESP32-C5_ESP-Hosted_v3.1.0_merged_0x0.bin` from
HMI into DRO and document the flashing procedure (switch, BOOT+RST, `--before no-reset`, offset
`0x0`). No CP source duplication — if the image ever needs rebuilding, it happens in HMI.
*Acceptance:* link reports `coprocessor=3.1.0 (match)` on the osptek board.

**S6 — verification matrix** (§6) on both boards.

## 5. Risks

| Risk | Mitigation |
|---|---|
| Rotation regression — UI flipped or touch inverted on one board | Rotation contract in §1; per-board angle+flags; S3 acceptance tests both |
| PPA rotation is unusable on the JD9365 board (why it was disabled) | Keep CPU rotation for Guition; do not "unify" the engines |
| Wrong image family flashed → board refuses to boot | Profiles; check `Chip rev:` in the boot log; documented in S4 |
| rev ≥ 3.0 bootloader is ~1.4 KB larger and may not fit DRO's partition layout | Check `bootloader.bin` size against the partition table; fall back to `CONFIG_BOOTLOADER_LOG_LEVEL_WARN` like HMI did |
| LVGL 9.2 (DRO) vs 9.5 (HMI) API drift in the copied drivers | Copy only the driver sources (they use panel/dsi APIs, not LVGL); if `lv_display_set_rotation` semantics differ, S3 catches it |
| DRO's `main.c` calls rotation twice, in two paths | Remove both in S3; the BSP owns it |

## 6. Test matrix

Per board: boot log (`Chip rev:`, panel ready), display upright, touch tracking (dragged
slider), audio playback, TF card mount, Wi-Fi scan (C6: 2.4 GHz only; C5: expect 5 GHz
channels), and one DRO-specific smoke test (axis readout / UI navigation).

## 7. Open questions

1. **Is DRO's panel really JD9365?** DRO instantiates `esp_lcd_jd9365`, which matches your earlier
   statement; if the Guition board turns out to be JD9366 as well, S2/S3 shrink to pins + rotation.
2. **Was 270° chosen arbitrarily or is it the panel's physical mounting?** It must stay 270 for
   this board either way, but knowing why tells us whether the osptek angle is fixed at 90 or
   needs measuring.
3. **Touch on the Guition board** — GSL3680 or GT911? DRO's BSP instantiates GSL3680; confirm on
   hardware before touching that path.
4. **Does the 9365 path still boot and rotate correctly today?** It is the baseline; S3 must not
   disturb it, so run the matrix on it *before* adding the second board.

## 8. Git

```
origin/dev
  └─ merge feature/idf6.1 → dev     (6.1 migration + C6 CP project)
       └─ feature/board-port        (this work)
```

`feature/idf6.1` is in sync with its origin, pushed earlier. `dev` exists only as a stale
`origin/dev` ref locally, so the merge needs a `fetch` first — which needs the network. Nothing
is pushed until you confirm GitHub is reachable; until then this branch lives locally.
