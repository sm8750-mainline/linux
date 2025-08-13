// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct panel_aa569_p_3_a0019_dsc {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
};

static inline
struct panel_aa569_p_3_a0019_dsc *to_panel_aa569_p_3_a0019_dsc(struct drm_panel *panel)
{
	return container_of(panel, struct panel_aa569_p_3_a0019_dsc, panel);
}

static void panel_aa569_p_3_a0019_dsc_reset(struct panel_aa569_p_3_a0019_dsc *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
    msleep(10);
    gpiod_set_value_cansleep(ctx->reset_gpio, 1);
    msleep(5);
    gpiod_set_value_cansleep(ctx->reset_gpio, 0);
    msleep(30);
	
}

static int panel_aa569_p_3_a0019_dsc_on(struct panel_aa569_p_3_a0019_dsc *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x8a, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x31);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd0, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xbe, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd2, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd3, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd4, 0x40);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf5, 0x55);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf6, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x21);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa4, 0x18);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x1f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x8f,
				     0x03, 0x08, 0x50, 0x0e, 0x01, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x02, 0x0e, 0x00, 0x00, 0x08,
				     0x02, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x9e, 0x77);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x14);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x80, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x17);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa0, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x13);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf9, 0x5c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x85, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x1c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe2,
				     0x0a, 0x0a, 0x0a, 0x1a, 0x1a, 0x1a, 0x1a,
				     0x1a, 0x0a, 0x0a, 0x0a, 0x04, 0x27, 0x12,
				     0x23, 0x09, 0x46, 0x45);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfb, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x09);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x8e,
				     0x10, 0x82, 0x00, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x88, 0xc0, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x14);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x82, 0x01);
	// Changes from downstream kernel
	//mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x07);
	//mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x91, 0x03);
	//mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x8a, 0x00);
	//mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x8b, 0xa0, 0x21);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x81,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00,
				     0x00, 0xab, 0x21, 0x00, 0x0c, 0x60, 0x05,
				     0xa0, 0x00, 0x16, 0x02, 0xd0, 0x02, 0xd0,
				     0x01, 0x55, 0x01, 0xe2, 0x00, 0x0a, 0x01,
				     0x4d, 0x00, 0x3c, 0x00, 0x0d, 0x04, 0xf4,
				     0x13, 0xa3, 0x08, 0x00, 0x0c, 0x00, 0x07,
				     0x10, 0x20, 0x00, 0x06, 0x0f, 0x0f, 0x33,
				     0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62,
				     0x69, 0x70, 0x77, 0x79, 0x7b, 0x7d, 0x7e,
				     0x00, 0x82, 0x11, 0x40, 0x19, 0xc0, 0x22,
				     0x3e, 0x32, 0x7c, 0x3a, 0xba, 0x3a, 0xf8,
				     0x3b, 0x38, 0x3b, 0x38, 0x3b, 0x76, 0x4b,
				     0x76, 0x4b, 0x74, 0x4b, 0x74, 0x5b, 0xb4,
				     0x73, 0xf4, 0x01, 0x00, 0x00, 0x00, 0x00,
				     0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5e, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x61, 0x0d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x6d, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x35, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x53, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x60, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x2d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x83, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf2, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x01, 0x80, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x84, 0x01, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x1e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x98, 0x99, 0xaa, 0x99);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x99,
				     0x88, 0x00, 0x00, 0x88, 0x00, 0x00, 0x88,
				     0x00, 0x00, 0x08, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa3, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x1e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x81, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x22);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd8,
				     0x01, 0x0c, 0x11, 0x10, 0x32, 0x19, 0x19);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdc,
				     0x84, 0x94, 0xbb, 0x00, 0x20, 0xc4, 0xa1,
				     0xcc, 0x10);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdd, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xde, 0xf1);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdf, 0x80);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe0,
				     0x02, 0x00, 0x5f, 0x21, 0x00, 0x28);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x2d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xed, 0x05, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x97, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x98, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x99, 0x0b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x9a, 0x17);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa0, 0x00, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa1, 0x02, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa4, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa5, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa6, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa7, 0x11);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xad, 0x00, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xae, 0x02, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x98, 0x66);
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x4f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x80, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x53);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x01);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x63);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf8,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf9,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xfa,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x53);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x49);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd9, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xda, 0x1e);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdb, 0xfc);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdf, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x81, 0x07, 0x1a, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x8a, 0xf6, 0x17, 0x12);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x8f, 0x12, 0x3a, 0x33);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa5, 0x09, 0x1a, 0x08);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb3, 0x30, 0x38, 0x31);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x08, 0x0d, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x03, 0x06, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xd7, 0x2f, 0x34, 0x2d);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x5b);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x81, 0x05, 0x09, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x83, 0x02, 0x0c, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x93, 0x00, 0x01, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x94, 0x05, 0x06, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa5, 0x09, 0x0c, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa7, 0xfd, 0x0f, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0x02, 0x07, 0x07);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0x05, 0x07, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc9, 0x05, 0x07, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xca, 0x00, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xcb, 0x02, 0x09, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdb, 0x00, 0x04, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xdc, 0x00, 0x06, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x5c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x81, 0x00, 0x03, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x83, 0x01, 0x08, 0x05);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x93, 0xff, 0x01, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x94, 0xff, 0x06, 0x04);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xa5, 0x01, 0x03, 0x02);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb7, 0xff, 0x01, 0x06);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb8, 0xff, 0x06, 0x03);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);

	return dsi_ctx.accum_err;
}

static int panel_aa569_p_3_a0019_dsc_off(struct panel_aa569_p_3_a0019_dsc *ctx)
{
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xff, 0x5a, 0xa5, 0x00);
	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 20);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int panel_aa569_p_3_a0019_dsc_prepare(struct drm_panel *panel)
{
	struct panel_aa569_p_3_a0019_dsc *ctx = to_panel_aa569_p_3_a0019_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	panel_aa569_p_3_a0019_dsc_reset(ctx);

	ret = panel_aa569_p_3_a0019_dsc_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);

	ret = mipi_dsi_picture_parameter_set(ctx->dsi, &pps);
	if (ret < 0) {
		dev_err(panel->dev, "failed to transmit PPS: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_compression_mode(ctx->dsi, true);
	if (ret < 0) {
		dev_err(dev, "failed to enable compression mode: %d\n", ret);
		return ret;
	}
	

	msleep(28); /* TODO: Is this panel-dependent? */

	return 0;
}

static int panel_aa569_p_3_a0019_dsc_unprepare(struct drm_panel *panel)
{
	struct panel_aa569_p_3_a0019_dsc *ctx = to_panel_aa569_p_3_a0019_dsc(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = panel_aa569_p_3_a0019_dsc_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

static const struct drm_display_mode panel_aa569_p_3_a0019_dsc_mode = {
	.clock = (1440 + 64 + 8 + 50) * (3168 + 16 + 4 + 12) * 120 / 1000,
	.hdisplay = 1440,
	.hsync_start = 1440 + 64,
	.hsync_end = 1440 + 64 + 8,
	.htotal = 1440 + 64 + 8 + 50,
	.vdisplay = 3168,
	.vsync_start = 3168 + 16,
	.vsync_end = 3168 + 16 + 4,
	.vtotal = 3168 + 16 + 4 + 12,
	.width_mm = 71,
	.height_mm = 158,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int panel_aa569_p_3_a0019_dsc_get_modes(struct drm_panel *panel,
					       struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &panel_aa569_p_3_a0019_dsc_mode);
}

static const struct drm_panel_funcs panel_aa569_p_3_a0019_dsc_panel_funcs = {
	.prepare = panel_aa569_p_3_a0019_dsc_prepare,
	.unprepare = panel_aa569_p_3_a0019_dsc_unprepare,
	.get_modes = panel_aa569_p_3_a0019_dsc_get_modes,
};

static int panel_aa569_p_3_a0019_dsc_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}


static int panel_aa569_p_3_a0019_dsc_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness_large(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops panel_aa569_p_3_a0019_dsc_bl_ops = {
	.update_status = panel_aa569_p_3_a0019_dsc_bl_update_status,
	.get_brightness = panel_aa569_p_3_a0019_dsc_bl_get_brightness,
};

static struct backlight_device *
panel_aa569_p_3_a0019_dsc_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 2047,
		.max_brightness = 4095,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &panel_aa569_p_3_a0019_dsc_bl_ops, &props);
}

static int panel_aa569_p_3_a0019_dsc_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct panel_aa569_p_3_a0019_dsc *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct panel_aa569_p_3_a0019_dsc, panel,
				   &panel_aa569_p_3_a0019_dsc_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB101010;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = panel_aa569_p_3_a0019_dsc_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");
					 

	drm_panel_add(&ctx->panel);

	/* This panel only supports DSC; unconditionally enable it */
	dsi->dsc = &ctx->dsc;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 2;

	ctx->dsc.slice_height = 22;
	ctx->dsc.slice_width = 720;
	ctx->dsi->dsc_slice_per_pkt = 2;
	ctx->dsc.slice_count = 2;
	ctx->dsc.convert_rgb = true; //works true
	ctx->dsc.bits_per_component = 10;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 fractional bits */
	ctx->dsc.block_pred_enable = true;
	

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}


static void panel_aa569_p_3_a0019_dsc_remove(struct mipi_dsi_device *dsi)
{
	struct panel_aa569_p_3_a0019_dsc *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id panel_aa569_p_3_a0019_dsc_of_match[] = {
	{ .compatible = "oneplus,dodge-panel" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, panel_aa569_p_3_a0019_dsc_of_match);

static struct mipi_dsi_driver panel_aa569_p_3_a0019_dsc_driver = {
	.probe = panel_aa569_p_3_a0019_dsc_probe,
	.remove = panel_aa569_p_3_a0019_dsc_remove,
	.driver = {
		.name = "panel-panel-aa569-p-3-a0019-dsc",
		.of_match_table = panel_aa569_p_3_a0019_dsc_of_match,
	},
};
module_mipi_dsi_driver(panel_aa569_p_3_a0019_dsc_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for AA569 P 3 A0019 dsc cmd mode panel");
MODULE_LICENSE("GPL");