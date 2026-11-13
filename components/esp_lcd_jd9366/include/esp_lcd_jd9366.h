/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD panel initialization command structure.
 */
typedef struct {
    int cmd;                /*!< The specific LCD command */
    const void *data;       /*!< Buffer holding command-specific data */
    size_t data_bytes;      /*!< Size of `data` in bytes */
    unsigned int delay_ms;  /*!< Delay in milliseconds after this command */
} jd9366_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *
 * @note  Pass this structure via the `vendor_config` field of
 *        `esp_lcd_panel_dev_config_t`.
 */
typedef struct {
    const jd9366_lcd_init_cmd_t *init_cmds;       /*!< Pointer to initialization commands array.
                                                    *   Set to NULL to use the built-in default sequence.
                                                    *   The array should be declared as `static const`. */
    uint16_t init_cmds_size;                       /*!< Number of commands in the array above */
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus;              /*!< MIPI-DSI bus handle */
        const esp_lcd_dpi_panel_config_t *dpi_config;  /*!< MIPI-DPI panel configuration */
    } mipi_config;
} jd9366_vendor_config_t;

/**
 * @brief Create LCD panel for model JD9366
 *
 * @note  Vendor-specific initialization may differ between manufacturers.
 *        Consult your LCD supplier for the correct initialization sequence.
 *
 * @param[in]  io               LCD panel IO handle
 * @param[in]  panel_dev_config General panel device configuration
 * @param[out] ret_panel        Returned LCD panel handle
 * @return
 *      - ESP_ERR_INVALID_ARG   if a parameter is invalid
 *      - ESP_OK                on success
 *      - Other error codes     on failure
 */
esp_err_t esp_lcd_new_panel_jd9366(const esp_lcd_panel_io_handle_t io,
                                   const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief MIPI-DSI bus configuration macro (2-lane, 1000 Mbps/lane)
 *
 * phy_clk_src uses MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT (XTAL), not the
 * backward-compat MIPI_DSI_PHY_CLK_SRC_DEFAULT. The legacy macro still expands to
 * PLL_F20M, which is a pre-v3.0-only source: on a rev>=3.0 build
 * _mipi_dsi_ll_set_phy_pllref_clock_source() has no case for it and calls
 * abort() -- an "abort() was called at PC ..." panic right after
 * "MIPI DSI PHY powered on".
 */
#define JD9366_PANEL_BUS_DSI_2CH_CONFIG()          \
    {                                              \
        .bus_id = 0,                               \
        .num_data_lanes = 2,                       \
        .phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT, \
        .lane_bit_rate_mbps = 1000,                \
    }

/**
 * @brief MIPI-DBI panel IO configuration macro
 */
#define JD9366_PANEL_IO_DBI_CONFIG()  \
    {                                 \
        .virtual_channel = 0,         \
        .lcd_cmd_bits = 8,            \
        .lcd_param_bits = 8,          \
    }

/**
 * @brief MIPI DPI panel configuration macro for 800x1280 (~63 Hz at 80 MHz)
 *
 * Refresh rate = (dpi_clock_freq_mhz * 1e6)
 *              / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
 *              / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
 *
 * The blanking values below are the vendor's, not a tuning choice: they are what
 * docs/panel-jd9366/vendor-example-jd9366/ (the vendor's own JD9366 example)
 * ships, and what the panel's register block (C_JD9366TC_..._20250320.txt)
 * asks for. At 80 MHz they give
 *     80e6 / (876 * 1448) = 63.07 Hz,
 * which is inside every limit in docs/panel-jd9366/lcd_timing.md (PCLK
 * 63.06-81.51 MHz, HBLK <= 88, VBK <= 250).
 *
 * A 60 Hz variant (HFP 52 / VFP 192 -> 888x1500 = 60.06 Hz) was tried here and
 * reverted: the flicker that motivated it turned out to be a 5 V / USB supply
 * problem, and the vendor runs these same values at 63 Hz. Their app asks for
 * dpi_clock_freq_mhz = 76 aiming at 60 Hz, but the P4 DPI clock is only
 * PLL_F240M/n and round(240/76) = 3, so it lands on 80 MHz and 63 Hz too.
 *
 * @param[in] px_format  Pixel format of the panel (e.g., LCD_COLOR_FMT_RGB565)
 */
#define JD9366_800_1280_PANEL_60HZ_DPI_CONFIG(px_format) \
    {                                                    \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,    \
        .dpi_clock_freq_mhz = 80,                        \
        .virtual_channel = 0,                            \
        .in_color_format = px_format,                    \
        .out_color_format = px_format,                   \
        .num_fbs = 1,                                    \
        .video_timing = {                                \
            .h_size = 800,                               \
            .v_size = 1280,                              \
            .hsync_back_porch = 28,                      \
            .hsync_pulse_width = 8,                      \
            .hsync_front_porch = 40,                     \
            .vsync_back_porch = 20,                      \
            .vsync_pulse_width = 8,                      \
            .vsync_front_porch = 140,                    \
        },                                               \
    }

#ifdef __cplusplus
}
#endif

#endif // SOC_MIPI_DSI_SUPPORTED
