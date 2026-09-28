// SPDX-License-Identifier: GPL-2.0-only
/*
 * Motorola Moto G4 Plus (athene) 5.46" 1080x1920 DSI command mode panel
 *
 * Copyright (c) 2026, David Heidelberg <david@ixit.cz>
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

static const struct regulator_bulk_data lgp546_supplies[] = {
	{ .supply = "vddio" },
	{ .supply = "vsp" },
	{ .supply = "vsn" },
};

struct lgp546_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset_gpio;
};

static inline struct lgp546_panel *to_lgp546_panel(struct drm_panel *panel)
{
	return container_of(panel, struct lgp546_panel, panel);
}

static void lgp546_reset(struct lgp546_panel *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(11000, 12000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(21);
}

static int lgp546_prepare(struct drm_panel *panel)
{
	struct lgp546_panel *ctx = to_lgp546_panel(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(lgp546_supplies), ctx->supplies);
	if (ret)
		return ret;

	lgp546_reset(ctx);

	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	/* Unlock the vendor command set */
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb0, 0x00);

	/* CABC: full PWM on RGB */
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xfa, 0x20);
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xce, 0x5d, 0x40, 0x43, 0x49,
					 0x55, 0x62, 0x71, 0x82, 0x94, 0xa8,
					 0xb9, 0xcb, 0xdb, 0xe9, 0xfe, 0xfe,
					 0xff, 0x03, 0x15, 0x04, 0x04, 0x00,
					 0x00);
	/* CABC: UI mode */
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xb8, 0x07, 0x8b, 0x19, 0x08,
					 0x0c, 0x21);
	/* CABC: moving image mode */
	mipi_dsi_generic_write_seq_multi(&dsi_ctx, 0xba, 0x07, 0x7a, 0x64, 0x18,
					 0x30, 0x8d);

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_SET_DISPLAY_BRIGHTNESS,
				     0xab);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5e, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY,
				     0x2c);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x01);

	if (dsi_ctx.accum_err) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(lgp546_supplies),
				       ctx->supplies);
	}

	return dsi_ctx.accum_err;
}

static int lgp546_unprepare(struct drm_panel *panel)
{
	struct lgp546_panel *ctx = to_lgp546_panel(panel);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(lgp546_supplies), ctx->supplies);

	return 0;
}

static int lgp546_enable(struct drm_panel *panel)
{
	struct lgp546_panel *ctx = to_lgp546_panel(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);

	return dsi_ctx.accum_err;
}

static int lgp546_disable(struct drm_panel *panel)
{
	struct lgp546_panel *ctx = to_lgp546_panel(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 10);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 85);

	return dsi_ctx.accum_err;
}

static const struct drm_display_mode lgp546_mode = {
	.clock = (1080 + 24 + 4 + 40) * (1920 + 16 + 2 + 16) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 24,
	.hsync_end = 1080 + 24 + 4,
	.htotal = 1080 + 24 + 4 + 40,
	.vdisplay = 1920,
	.vsync_start = 1920 + 16,
	.vsync_end = 1920 + 16 + 2,
	.vtotal = 1920 + 16 + 2 + 16,
	.width_mm = 68,
	.height_mm = 121,
};

static int lgp546_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &lgp546_mode);
}

static const struct drm_panel_funcs lgp546_panel_funcs = {
	.prepare = lgp546_prepare,
	.unprepare = lgp546_unprepare,
	.enable = lgp546_enable,
	.disable = lgp546_disable,
	.get_modes = lgp546_get_modes,
};

static int lgp546_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct lgp546_panel *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct lgp546_panel, panel,
				   &lgp546_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev, ARRAY_SIZE(lgp546_supplies),
					    lgp546_supplies, &ctx->supplies);
	if (ret < 0)
		return ret;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "failed to get reset GPIO\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	/* The panel expects the data lanes to be in LP11 before reset */
	ctx->panel.prepare_prev_first = true;

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "failed to get backlight\n");

	drm_panel_add(&ctx->panel);

	ret = devm_mipi_dsi_attach(dev, dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "failed to attach to DSI host\n");
	}

	return 0;
}

static void lgp546_remove(struct mipi_dsi_device *dsi)
{
	struct lgp546_panel *ctx = mipi_dsi_get_drvdata(dsi);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id lgp546_of_match[] = {
	{ .compatible = "lgphilips,546-1080p" },
	{ }
};
MODULE_DEVICE_TABLE(of, lgp546_of_match);

static struct mipi_dsi_driver lgp546_driver = {
	.probe = lgp546_probe,
	.remove = lgp546_remove,
	.driver = {
		.name = "panel-lgphilips-546-1080p",
		.of_match_table = lgp546_of_match,
	},
};
module_mipi_dsi_driver(lgp546_driver);

MODULE_AUTHOR("David Heidelberg <david@ixit.cz>");
MODULE_DESCRIPTION("DRM driver for the Motorola Moto G4 panel");
MODULE_LICENSE("GPL");
