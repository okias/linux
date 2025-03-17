// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2025 Vasiliy Doylov <nekocwd@mainlining.org>

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-async.h>
#include <media/v4l2-cci.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <linux/regmap.h>

#define BU64748_NAME "bu64748"
/* 
Actuator has 16 bit resolution? no idea, downstream driver seem to set value upto ffxx range.
Maybe it doesn't step by 1, making the resolution less. shift DAC position by 4 makes it 12 bit resolution.
I am assuming it has 12 bit resolution. No available datasheet.
*/
#define BU64748_MAX_FOCUS_POS (4096 - 1)
#define BU64748_MIN_FOCUS_POS 0
#define BU64748_DEFAULT_FOCUS_POS 2048
#define BU64748_FOCUS_STEPS 1
#define BU64748_DAC_SHIFT 4

#define BU64748_INIT_ADDR CCI_REG8(0x2)
#define BU64748_DAC_ADDR CCI_REG16(0x0)

static const char *const bu64748_supply_names[] = {
	"vcc",
	"vio"
};

struct bu64748 {
	struct regulator_bulk_data supplies[ARRAY_SIZE(bu64748_supply_names)];
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *focus;
	struct v4l2_subdev sd;
	struct regmap *regmap;
};

static inline struct bu64748 *sd_to_bu64748(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct bu64748, sd);
}

static int bu64748_set_dac(struct bu64748 *bu64748, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&bu64748->sd);
	int ret;

    ret = cci_write(bu64748->regmap, BU64748_DAC_ADDR, val << BU64748_DAC_SHIFT, NULL);
    if (ret) {
        dev_err(&client->dev, "failed to set DAC: %d\n", ret);
		return ret;
	}

	return 0;
}

static int __maybe_unused bu64748_runtime_suspend(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct bu64748 *bu64748 = sd_to_bu64748(sd);

	regulator_bulk_disable(ARRAY_SIZE(bu64748_supply_names),
			       bu64748->supplies);

	return 0;
}

static int __maybe_unused bu64748_runtime_resume(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct bu64748 *bu64748 = sd_to_bu64748(sd);
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(bu64748_supply_names),
				    bu64748->supplies);

	if (ret < 0) {
		dev_err(dev, "failed to enable regulators\n");
		return ret;
	}

	usleep_range(8000, 10000);

	return ret;
}

static int bu64748_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct bu64748 *bu64748 =
		container_of(ctrl->handler, struct bu64748, ctrls);

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE)
		return bu64748_set_dac(bu64748, ctrl->val);

	return 0;
}

static const struct v4l2_ctrl_ops bu64748_ctrl_ops = {
	.s_ctrl = bu64748_set_ctrl,
};

static int bu64748_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = pm_runtime_resume_and_get(sd->dev);
	if (ret < 0)
		return ret;

	return 0;
}

static int bu64748_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	pm_runtime_mark_last_busy(sd->dev);
	pm_runtime_put_autosuspend(sd->dev);

	return 0;
}

static const struct v4l2_subdev_internal_ops bu64748_int_ops = {
	.open = bu64748_open,
	.close = bu64748_close,
};

static const struct v4l2_subdev_core_ops bu64748_core_ops = {
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops bu64748_ops = {
	.core = &bu64748_core_ops,
};

static int bu64748_init_controls(struct bu64748 *bu64748)
{
	struct v4l2_ctrl_handler *hdl = &bu64748->ctrls;
	const struct v4l2_ctrl_ops *ops = &bu64748_ctrl_ops;

	v4l2_ctrl_handler_init(hdl, 1);

	bu64748->focus = v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE,
					      BU64748_MIN_FOCUS_POS,
					      BU64748_MAX_FOCUS_POS,
					      BU64748_FOCUS_STEPS,
						  BU64748_DEFAULT_FOCUS_POS);

	if (hdl->error)
		return hdl->error;

	bu64748->sd.ctrl_handler = hdl;

	return 0;
}

static int bu64748_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct bu64748 *bu64748;
	unsigned int i;
	// u64 id;
	int ret;

	bu64748 = devm_kzalloc(dev, sizeof(*bu64748), GFP_KERNEL);
	if (!bu64748)
		return -ENOMEM;

	bu64748->regmap = devm_cci_regmap_init_i2c(client, 8);
	if (IS_ERR(bu64748->regmap)) {
		ret = PTR_ERR(bu64748->regmap);
		dev_err(dev, "failed to initialize CCI: %d\n", ret);
		return ret;
	}

	/* Initialize subdev */
	v4l2_i2c_subdev_init(&bu64748->sd, client, &bu64748_ops);

	for (i = 0; i < ARRAY_SIZE(bu64748_supply_names); i++)
		bu64748->supplies[i].supply = bu64748_supply_names[i];

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(bu64748_supply_names),
				      bu64748->supplies);

	if (ret) {
		dev_err(dev, "failed to get regulators\n");
		return ret;
	}

	ret = regulator_bulk_enable(ARRAY_SIZE(bu64748_supply_names),
				    bu64748->supplies);
	if (ret) {
		dev_err(dev, "failed to enable regulators\n");
		return ret;
	}
	
	usleep_range(8000, 10000);

	/* Set only during initialization */
	ret = cci_write(bu64748->regmap, BU64748_INIT_ADDR, 0x0, NULL);
	if (ret) {
		dev_err(&client->dev, "failed to set INIT: %d\n", ret);
		return ret;
	}

	msleep(100);

	/* set default focus position */
	ret = bu64748_set_dac(bu64748, BU64748_DEFAULT_FOCUS_POS);
	if (ret) {
		dev_err(&client->dev, "failed to set default focus position: %d\n", ret);
		return ret;
	}

	/* Initialize controls */
	ret = bu64748_init_controls(bu64748);
	if (ret)
		goto err_free_handler;

	/* Initialize subdev */
	bu64748->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
				V4L2_SUBDEV_FL_HAS_EVENTS;
	bu64748->sd.internal_ops = &bu64748_int_ops;

	ret = media_entity_pads_init(&bu64748->sd.entity, 0, NULL);
	if (ret < 0)
		goto err_free_handler;

	bu64748->sd.entity.function = MEDIA_ENT_F_LENS;

	pm_runtime_enable(dev);
	ret = v4l2_async_register_subdev(&bu64748->sd);

	if (ret < 0) {
		dev_err(dev, "failed to register V4L2 subdev: %d", ret);
		goto err_power_off;
	}

	pm_runtime_set_autosuspend_delay(dev, 1000);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_idle(dev);

	return 0;

err_power_off:
	pm_runtime_disable(dev);
	media_entity_cleanup(&bu64748->sd.entity);
err_free_handler:
	v4l2_ctrl_handler_free(&bu64748->ctrls);

	return ret;
}

static void bu64748_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct bu64748 *bu64748 = sd_to_bu64748(sd);
	struct device *dev = &client->dev;

	v4l2_async_unregister_subdev(&bu64748->sd);
	v4l2_ctrl_handler_free(&bu64748->ctrls);
	media_entity_cleanup(&bu64748->sd.entity);
	pm_runtime_disable(dev);
}

static const struct of_device_id bu64748_of_table[] = {
	{ .compatible = "rohm,bu64748" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, bu64748_of_table);

static const struct dev_pm_ops bu64748_pm_ops = {
	SET_RUNTIME_PM_OPS(bu64748_runtime_suspend,
			   bu64748_runtime_resume, NULL)
};

static struct i2c_driver bu64748_i2c_driver = {
	.driver = {
		.name = BU64748_NAME,
		.pm = &bu64748_pm_ops,
		.of_match_table = bu64748_of_table,
	},
	.probe = bu64748_probe,
	.remove = bu64748_remove,
};
module_i2c_driver(bu64748_i2c_driver);

MODULE_AUTHOR("Joel Selvaraj <foss@joelselvaraj.com>");
MODULE_DESCRIPTION("ROHM BU64748 VCM driver");
MODULE_LICENSE("GPL");
