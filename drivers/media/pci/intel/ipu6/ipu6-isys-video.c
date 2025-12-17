// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013--2024 Intel Corporation
 */

#include <linux/align.h>
#include <linux/bits.h>
#include <linux/bug.h>
#include <linux/completion.h>
#include <linux/container_of.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/math64.h>
#include <linux/minmax.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-v4l2.h>

#include "ipu6.h"
#include "ipu6-bus.h"
#include "ipu6-cpd.h"
#include "ipu6-fw-isys.h"
#include "ipu6-isys.h"
#include "ipu6-isys-csi2.h"
#include "ipu6-isys-queue.h"
#include "ipu6-isys-video.h"
#include "ipu6-platform-regs.h"

const struct ipu6_isys_pixelformat ipu6_isys_pfmts[] = {
	{ V4L2_PIX_FMT_SBGGR12, 16, 12, MEDIA_BUS_FMT_SBGGR12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SGBRG12, 16, 12, MEDIA_BUS_FMT_SGBRG12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SGRBG12, 16, 12, MEDIA_BUS_FMT_SGRBG12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SRGGB12, 16, 12, MEDIA_BUS_FMT_SRGGB12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SBGGR10, 16, 10, MEDIA_BUS_FMT_SBGGR10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SGBRG10, 16, 10, MEDIA_BUS_FMT_SGBRG10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SGRBG10, 16, 10, MEDIA_BUS_FMT_SGRBG10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SRGGB10, 16, 10, MEDIA_BUS_FMT_SRGGB10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_SBGGR8, 8, 8, MEDIA_BUS_FMT_SBGGR8_1X8,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW8 },
	{ V4L2_PIX_FMT_SGBRG8, 8, 8, MEDIA_BUS_FMT_SGBRG8_1X8,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW8 },
	{ V4L2_PIX_FMT_SGRBG8, 8, 8, MEDIA_BUS_FMT_SGRBG8_1X8,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW8 },
	{ V4L2_PIX_FMT_SRGGB8, 8, 8, MEDIA_BUS_FMT_SRGGB8_1X8,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW8 },
	{ V4L2_PIX_FMT_SBGGR12P, 12, 12, MEDIA_BUS_FMT_SBGGR12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW12 },
	{ V4L2_PIX_FMT_SGBRG12P, 12, 12, MEDIA_BUS_FMT_SGBRG12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW12 },
	{ V4L2_PIX_FMT_SGRBG12P, 12, 12, MEDIA_BUS_FMT_SGRBG12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW12 },
	{ V4L2_PIX_FMT_SRGGB12P, 12, 12, MEDIA_BUS_FMT_SRGGB12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW12 },
	{ V4L2_PIX_FMT_SBGGR10P, 10, 10, MEDIA_BUS_FMT_SBGGR10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW10 },
	{ V4L2_PIX_FMT_SGBRG10P, 10, 10, MEDIA_BUS_FMT_SGBRG10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW10 },
	{ V4L2_PIX_FMT_SGRBG10P, 10, 10, MEDIA_BUS_FMT_SGRBG10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW10 },
	{ V4L2_PIX_FMT_SRGGB10P, 10, 10, MEDIA_BUS_FMT_SRGGB10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW10 },

	{ V4L2_PIX_FMT_GREY, 8, 8, MEDIA_BUS_FMT_Y8_1X8,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW8 },
	{ V4L2_PIX_FMT_Y10, 16, 10, MEDIA_BUS_FMT_Y10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_Y12, 16, 12, MEDIA_BUS_FMT_Y12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_Y16, 16, 16, MEDIA_BUS_FMT_Y16_1X16,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16 },
	{ V4L2_PIX_FMT_Y10P, 10, 10, MEDIA_BUS_FMT_Y10_1X10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW10 },
	{ V4L2_PIX_FMT_Y12P, 12, 12, MEDIA_BUS_FMT_Y12_1X12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW12 },

	{ V4L2_PIX_FMT_UYVY, 16, 16, MEDIA_BUS_FMT_UYVY8_1X16,
	  IPU6_FW_ISYS_FRAME_FORMAT_UYVY},
	{ V4L2_PIX_FMT_YUYV, 16, 16, MEDIA_BUS_FMT_YUYV8_1X16,
	  IPU6_FW_ISYS_FRAME_FORMAT_YUYV},
	{ V4L2_PIX_FMT_RGB565, 16, 16, MEDIA_BUS_FMT_RGB565_1X16,
	  IPU6_FW_ISYS_FRAME_FORMAT_RGB565 },
	{ V4L2_PIX_FMT_BGR24, 24, 24, MEDIA_BUS_FMT_RGB888_1X24,
	  IPU6_FW_ISYS_FRAME_FORMAT_RGBA888 },

	{ V4L2_PIX_FMT_RAW_8, 8, 8, MEDIA_BUS_FMT_RAW_8,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW8 },
	{ V4L2_PIX_FMT_RAW_CSI2_10, 10, 10, MEDIA_BUS_FMT_RAW_10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW10 },
	{ V4L2_PIX_FMT_RAW_CSI2_12, 12, 12, MEDIA_BUS_FMT_RAW_12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW12 },

	{ V4L2_META_FMT_GENERIC_8, 8, 8, MEDIA_BUS_FMT_META_8,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW8, true },
	{ V4L2_META_FMT_GENERIC_CSI2_10, 10, 10, MEDIA_BUS_FMT_META_10,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW10, true },
	{ V4L2_META_FMT_GENERIC_CSI2_12, 12, 12, MEDIA_BUS_FMT_META_12,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW12, true },
	{ V4L2_META_FMT_GENERIC_16, 16, 16, MEDIA_BUS_FMT_META_16,
	  IPU6_FW_ISYS_FRAME_FORMAT_RAW16, true },
};

static int video_open(struct file *file)
{
	struct ipu6_isys_video *av = video_drvdata(file);
	struct ipu6_isys *isys = av->isys;
	struct ipu6_bus_device *adev = isys->adev;

	mutex_lock(&isys->mutex);
	if (isys->need_reset) {
		mutex_unlock(&isys->mutex);
		dev_warn(&adev->auxdev.dev, "isys power cycle required\n");
		return -EIO;
	}
	mutex_unlock(&isys->mutex);

	return v4l2_fh_open(file);
}

const struct ipu6_isys_pixelformat *
ipu6_isys_get_isys_format(u32 pixelformat, u32 type)
{
	const struct ipu6_isys_pixelformat *default_pfmt = NULL;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ipu6_isys_pfmts); i++) {
		const struct ipu6_isys_pixelformat *pfmt = &ipu6_isys_pfmts[i];

		if (type && ((!pfmt->is_meta &&
			      type != V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
			     (pfmt->is_meta &&
			      type != V4L2_BUF_TYPE_META_CAPTURE)))
			continue;

		if (!default_pfmt)
			default_pfmt = pfmt;

		if (pfmt->pixelformat != pixelformat)
			continue;

		return pfmt;
	}

	return default_pfmt;
}

static int ipu6_isys_vidioc_querycap(struct file *file, void *fh,
				     struct v4l2_capability *cap)
{
	struct ipu6_isys_video *av = video_drvdata(file);

	strscpy(cap->driver, IPU6_ISYS_NAME, sizeof(cap->driver));
	strscpy(cap->card, av->isys->media_dev.model, sizeof(cap->card));

	return 0;
}

static int ipu6_isys_vidioc_enum_fmt(struct file *file, void *fh,
				     struct v4l2_fmtdesc *f)
{
	unsigned int i, num_found;

	for (i = 0, num_found = 0; i < ARRAY_SIZE(ipu6_isys_pfmts); i++) {
		if ((ipu6_isys_pfmts[i].is_meta &&
		     f->type != V4L2_BUF_TYPE_META_CAPTURE) ||
		    (!ipu6_isys_pfmts[i].is_meta &&
		     f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE))
			continue;

		if (f->mbus_code && f->mbus_code != ipu6_isys_pfmts[i].code)
			continue;

		if (num_found < f->index) {
			num_found++;
			continue;
		}

		f->flags = 0;
		f->pixelformat = ipu6_isys_pfmts[i].pixelformat;

		return 0;
	}

	return -EINVAL;
}

static int ipu6_isys_vidioc_enum_framesizes(struct file *file, void *fh,
					    struct v4l2_frmsizeenum *fsize)
{
	unsigned int i;

	if (fsize->index > 0)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(ipu6_isys_pfmts); i++) {
		if (fsize->pixel_format != ipu6_isys_pfmts[i].pixelformat)
			continue;

		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->stepwise.min_width = IPU6_ISYS_MIN_WIDTH;
		fsize->stepwise.max_width = IPU6_ISYS_MAX_WIDTH;
		fsize->stepwise.min_height = IPU6_ISYS_MIN_HEIGHT;
		fsize->stepwise.max_height = IPU6_ISYS_MAX_HEIGHT;
		fsize->stepwise.step_width = 2;
		fsize->stepwise.step_height = 2;

		return 0;
	}

	return -EINVAL;
}

static int ipu6_isys_vidioc_g_fmt_vid_cap(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	struct ipu6_isys_video *av = video_drvdata(file);

	f->fmt.pix = av->pix_fmt;

	return 0;
}

static int ipu6_isys_vidioc_g_fmt_meta_cap(struct file *file, void *fh,
					   struct v4l2_format *f)
{
	struct ipu6_isys_video *av = video_drvdata(file);

	f->fmt.meta = av->meta_fmt;

	return 0;
}

static void ipu6_isys_try_fmt_cap(struct ipu6_isys_video *av, u32 type,
				  u32 *format, u32 *width, u32 *height,
				  u32 *bytesperline, u32 *sizeimage)
{
	const struct ipu6_isys_pixelformat *pfmt =
		ipu6_isys_get_isys_format(*format, type);

	*format = pfmt->pixelformat;
	*width = clamp(*width, IPU6_ISYS_MIN_WIDTH, IPU6_ISYS_MAX_WIDTH);
	*height = clamp(*height, IPU6_ISYS_MIN_HEIGHT, IPU6_ISYS_MAX_HEIGHT);

	if (pfmt->bpp != pfmt->bpp_packed)
		*bytesperline = *width * DIV_ROUND_UP(pfmt->bpp, BITS_PER_BYTE);
	else
		*bytesperline = DIV_ROUND_UP(*width * pfmt->bpp, BITS_PER_BYTE);

	*bytesperline = ALIGN(*bytesperline, 64);

	/*
	 * (height + 1) * bytesperline due to a hardware issue: the DMA unit
	 * is a power of two, and a line should be transferred as few units
	 * as possible. The result is that up to line length more data than
	 * the image size may be transferred to memory after the image.
	 * Another limitation is the GDA allocation unit size. For low
	 * resolution it gives a bigger number. Use larger one to avoid
	 * memory corruption.
	 */
	*sizeimage = *bytesperline * *height +
		max(*bytesperline,
		    av->isys->pdata->ipdata->isys_dma_overshoot);
}

static void __ipu6_isys_vidioc_try_fmt_vid_cap(struct ipu6_isys_video *av,
					       struct v4l2_format *f)
{
	ipu6_isys_try_fmt_cap(av, f->type, &f->fmt.pix.pixelformat,
			      &f->fmt.pix.width, &f->fmt.pix.height,
			      &f->fmt.pix.bytesperline, &f->fmt.pix.sizeimage);

	f->fmt.pix.field = V4L2_FIELD_NONE;
	f->fmt.pix.colorspace = V4L2_COLORSPACE_RAW;
	f->fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	f->fmt.pix.quantization = V4L2_QUANTIZATION_DEFAULT;
	f->fmt.pix.xfer_func = V4L2_XFER_FUNC_DEFAULT;
}

static int ipu6_isys_vidioc_try_fmt_vid_cap(struct file *file, void *fh,
					    struct v4l2_format *f)
{
	struct ipu6_isys_video *av = video_drvdata(file);

	if (vb2_is_busy(&av->aq.vbq))
		return -EBUSY;

	__ipu6_isys_vidioc_try_fmt_vid_cap(av, f);

	return 0;
}

static int __ipu6_isys_vidioc_try_fmt_meta_cap(struct ipu6_isys_video *av,
					       struct v4l2_format *f)
{
	ipu6_isys_try_fmt_cap(av, f->type, &f->fmt.meta.dataformat,
			      &f->fmt.meta.width, &f->fmt.meta.height,
			      &f->fmt.meta.bytesperline,
			      &f->fmt.meta.buffersize);

	return 0;
}

static int ipu6_isys_vidioc_try_fmt_meta_cap(struct file *file, void *fh,
					     struct v4l2_format *f)
{
	struct ipu6_isys_video *av = video_drvdata(file);

	__ipu6_isys_vidioc_try_fmt_meta_cap(av, f);

	return 0;
}

static int ipu6_isys_vidioc_s_fmt_vid_cap(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	struct ipu6_isys_video *av = video_drvdata(file);

	ipu6_isys_vidioc_try_fmt_vid_cap(file, fh, f);
	av->pix_fmt = f->fmt.pix;

	return 0;
}

static int ipu6_isys_vidioc_s_fmt_meta_cap(struct file *file, void *fh,
					   struct v4l2_format *f)
{
	struct ipu6_isys_video *av = video_drvdata(file);

	if (vb2_is_busy(&av->aq.vbq))
		return -EBUSY;

	ipu6_isys_vidioc_try_fmt_meta_cap(file, fh, f);
	av->meta_fmt = f->fmt.meta;

	return 0;
}

static int ipu6_isys_vidioc_reqbufs(struct file *file, void *priv,
				    struct v4l2_requestbuffers *p)
{
	struct ipu6_isys_video *av = video_drvdata(file);
	int ret;

	av->aq.vbq.is_multiplanar = V4L2_TYPE_IS_MULTIPLANAR(p->type);
	av->aq.vbq.is_output = V4L2_TYPE_IS_OUTPUT(p->type);

	ret = vb2_queue_change_type(&av->aq.vbq, p->type);
	if (ret)
		return ret;

	return vb2_ioctl_reqbufs(file, priv, p);
}

static int ipu6_isys_vidioc_create_bufs(struct file *file, void *priv,
					struct v4l2_create_buffers *p)
{
	struct ipu6_isys_video *av = video_drvdata(file);
	int ret;

	av->aq.vbq.is_multiplanar = V4L2_TYPE_IS_MULTIPLANAR(p->format.type);
	av->aq.vbq.is_output = V4L2_TYPE_IS_OUTPUT(p->format.type);

	ret = vb2_queue_change_type(&av->aq.vbq, p->format.type);
	if (ret)
		return ret;

	return vb2_ioctl_create_bufs(file, priv, p);
}

static int link_validate(struct media_link *link)
{
	struct ipu6_isys_video *av =
		container_of(link->sink, struct ipu6_isys_video, pad);
	struct device *dev = &av->isys->adev->auxdev.dev;
	struct v4l2_subdev_state *s_state;
	struct v4l2_subdev *s_sd;
	struct v4l2_mbus_framefmt *s_fmt;
	struct media_pad *s_pad;
	u32 s_stream, code;
	int ret = -EPIPE;

	if (!link->source->entity)
		return ret;

	s_sd = media_entity_to_v4l2_subdev(link->source->entity);
	s_state = v4l2_subdev_get_unlocked_active_state(s_sd);
	if (!s_state)
		return ret;

	dev_dbg(dev, "validating link \"%s\":%u -> \"%s\"\n",
		link->source->entity->name, link->source->index,
		link->sink->entity->name);

	s_pad = media_pad_remote_pad_first(&av->pad);
	s_stream = ipu6_isys_get_src_stream_by_src_pad(s_sd, s_pad->index);

	v4l2_subdev_lock_state(s_state);

	s_fmt = v4l2_subdev_state_get_format(s_state, s_pad->index, s_stream);
	if (!s_fmt) {
		dev_err(dev, "failed to get source pad format\n");
		goto unlock;
	}

	code = ipu6_isys_get_isys_format(ipu6_isys_get_format(av), 0)->code;

	if (s_fmt->width != ipu6_isys_get_frame_width(av) ||
	    s_fmt->height != ipu6_isys_get_frame_height(av) ||
	    s_fmt->code != code) {
		dev_dbg(dev, "format mismatch %dx%d,%x != %dx%d,%x\n",
			s_fmt->width, s_fmt->height, s_fmt->code,
			ipu6_isys_get_frame_width(av),
			ipu6_isys_get_frame_height(av), code);
		goto unlock;
	}

	v4l2_subdev_unlock_state(s_state);

	return 0;
unlock:
	v4l2_subdev_unlock_state(s_state);

	return ret;
}

static int ipu6_isys_fw_pin_cfg(struct ipu6_isys_video *av,
				struct ipu6_fw_isys_stream_cfg_data_abi *cfg,
				struct ipu6_isys_stream *stream,
				struct media_pad *src_pad,
				struct v4l2_mbus_frame_desc_entry *entry)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(src_pad->entity);
	struct v4l2_subdev_state *state = v4l2_subdev_get_locked_active_state(sd);
	struct ipu6_fw_isys_input_pin_info_abi *input_pin;
	struct ipu6_fw_isys_output_pin_info_abi *output_pin;
	struct ipu6_isys_queue *aq = &av->aq;
	const struct ipu6_isys_pixelformat *pfmt =
		ipu6_isys_get_isys_format(ipu6_isys_get_format(av), 0);
	struct v4l2_mbus_framefmt *fmt;
	struct v4l2_rect v4l2_crop;
	struct ipu6_isys *isys = av->isys;
	int input_pins = cfg->nof_input_pins++;
	int output_pins;
	u32 src_stream;

	src_stream = __ipu6_isys_get_src_stream_by_src_pad(state, src_pad->index);
	fmt = v4l2_subdev_state_get_format(state, src_pad->index, src_stream);
	v4l2_crop = *v4l2_subdev_state_get_crop(state, src_pad->index, src_stream);

	input_pin = &cfg->input_pins[input_pins];
	input_pin->input_res.width = fmt->width;
	input_pin->input_res.height = fmt->height;
	input_pin->dt = entry->bus.csi2.dt;
	input_pin->bits_per_pix = pfmt->bpp_packed;
	input_pin->mapped_dt = 0x40; /* invalid mipi data type */
	input_pin->mipi_decompression = 0;
	input_pin->capture_mode = IPU6_FW_ISYS_CAPTURE_MODE_REGULAR;
	input_pin->mipi_store_mode = pfmt->bpp == pfmt->bpp_packed ?
		IPU6_FW_ISYS_MIPI_STORE_MODE_DISCARD_LONG_HEADER :
		IPU6_FW_ISYS_MIPI_STORE_MODE_NORMAL;
	input_pin->crop_first_and_last_lines = v4l2_crop.top & 1;

	output_pins = cfg->nof_output_pins++;
	aq->fw_output = output_pins;
	stream->output_pins_queue[output_pins] = aq;

	output_pin = &cfg->output_pins[output_pins];
	output_pin->input_pin_id = input_pins;
	output_pin->output_res.width = ipu6_isys_get_frame_width(av);
	output_pin->output_res.height = ipu6_isys_get_frame_height(av);

	output_pin->stride = ipu6_isys_get_bytes_per_line(av);
	if (pfmt->bpp != pfmt->bpp_packed)
		output_pin->pt = IPU6_FW_ISYS_PIN_TYPE_RAW_SOC;
	else
		output_pin->pt = IPU6_FW_ISYS_PIN_TYPE_MIPI;
	output_pin->ft = pfmt->css_pixelformat;
	output_pin->send_irq = 1;
	memset(output_pin->ts_offsets, 0, sizeof(output_pin->ts_offsets));
	output_pin->s2m_pixel_soc_pixel_remapping =
		S2M_PIXEL_SOC_PIXEL_REMAPPING_FLAG_NO_REMAPPING;
	output_pin->csi_be_soc_pixel_remapping =
		CSI_BE_SOC_PIXEL_REMAPPING_FLAG_NO_REMAPPING;

	output_pin->snoopable = true;
	output_pin->error_handling_enable = false;
	output_pin->sensor_type = isys->sensor_type++;
	if (isys->sensor_type > isys->pdata->ipdata->sensor_type_end)
		isys->sensor_type = isys->pdata->ipdata->sensor_type_start;

	return 0;
}

static int ipu6_isys_start_stream_firmware(struct ipu6_isys_stream *stream,
					   struct v4l2_mbus_frame_desc *desc)
{
	struct ipu6_fw_isys_stream_cfg_data_abi *stream_cfg;
	struct ipu6_fw_isys_frame_buff_set_abi *buf = NULL;
	struct ipu6_isys_buffer_list bl;
	struct device *dev = &stream->asd->isys->adev->auxdev.dev;
	struct v4l2_subdev_state *csi2_state =
		v4l2_subdev_get_locked_active_state(&stream->asd->sd);
	struct isys_fw_msgs *msg = NULL;
	struct ipu6_isys_queue *aq;
	int ret, retout, tout;
	u16 send_type;

	ret = ipu6_isys_buffer_list_get(stream, &bl);
	if (ret < 0) {
		dev_warn(dev, "no buffer available, DRIVER BUG?\n");
		return ret;
	}

	msg = ipu6_get_fw_msg_buf(stream);
	if (!msg)
		return -ENOMEM;

	stream_cfg = &msg->fw_msg.stream;
	stream_cfg->src = stream->asd->source;
	stream_cfg->vc = stream->vc;
	stream_cfg->isl_use = 0;
	stream_cfg->sensor_type = IPU6_FW_ISYS_SENSOR_MODE_NORMAL;

	list_for_each_entry(aq, &stream->queues, node) {
		struct ipu6_isys_video *__av = ipu6_isys_queue_to_video(aq);
		struct media_pad *remote_pad =
			media_pad_remote_pad_first(&__av->pad);
		u64 source_streams = 1;
		unsigned int sink_stream =
			__ffs(v4l2_subdev_state_xlate_streams(csi2_state,
							      remote_pad->index,
							      CSI2_PAD_SINK,
							      &source_streams));
		struct v4l2_mbus_frame_desc_entry *entry;
		unsigned int i;

		for (i = 0; i < V4L2_FRAME_DESC_ENTRY_MAX; i++) {
			entry = &desc->entry[i];

			if (entry->stream == sink_stream)
				break;
		}

		if (i == V4L2_FRAME_DESC_ENTRY_MAX) {
			dev_err(dev, "cannot find frame desc entry for sink stream %u\n",
				sink_stream);
			return -EINVAL;
		}

		ret = ipu6_isys_fw_pin_cfg(__av, stream_cfg, stream, remote_pad,
					   entry);
		if (ret < 0) {
			ipu6_put_fw_msg_buf(stream->isys,
					    (uintptr_t)stream_cfg);
			return ret;
		}
	}

	ipu6_fw_isys_dump_stream_cfg(dev, stream_cfg);

	stream->nr_output_pins = stream_cfg->nof_output_pins;

	reinit_completion(&stream->stream_open_completion);

	ret = ipu6_fw_isys_complex_cmd(stream->isys, stream->stream_handle,
				       stream_cfg, msg->dma_addr,
				       sizeof(*stream_cfg),
				       IPU6_FW_ISYS_SEND_TYPE_STREAM_OPEN);
	if (ret < 0) {
		dev_err(dev, "can't open stream (%d)\n", ret);
		ipu6_put_fw_msg_buf(stream->isys, (uintptr_t)stream_cfg);
		return ret;
	}

	tout = wait_for_completion_timeout(&stream->stream_open_completion,
					   IPU6_FW_CALL_TIMEOUT_JIFFIES);

	ipu6_put_fw_msg_buf(stream->isys, (uintptr_t)stream_cfg);

	if (!tout) {
		dev_err(dev, "stream open time out\n");
		return -ETIMEDOUT;
	}
	if (stream->error) {
		dev_err(dev, "stream open error: %d\n", stream->error);
		return -EIO;
	}
	dev_dbg(dev, "start stream: open complete\n");

	msg = ipu6_get_fw_msg_buf(stream);
	if (!msg)
		return -ENOMEM;
	buf = &msg->fw_msg.frame;
	ipu6_isys_buf_to_fw_frame_buf(buf, stream, &bl);
	ipu6_isys_buffer_list_queue(&bl, IPU6_ISYS_BUFFER_LIST_FL_ACTIVE, 0);

	reinit_completion(&stream->stream_start_completion);

	send_type = IPU6_FW_ISYS_SEND_TYPE_STREAM_START_AND_CAPTURE;
	ipu6_fw_isys_dump_frame_buff_set(dev, buf, stream_cfg->nof_output_pins);
	ret = ipu6_fw_isys_complex_cmd(stream->isys, stream->stream_handle, buf,
				       msg->dma_addr, sizeof(*buf), send_type);
	if (ret < 0) {
		dev_err(dev, "can't start streaming (%d)\n", ret);
		goto out_stream_close;
	}

	tout = wait_for_completion_timeout(&stream->stream_start_completion,
					   IPU6_FW_CALL_TIMEOUT_JIFFIES);
	if (!tout) {
		dev_err(dev, "stream start time out\n");
		ret = -ETIMEDOUT;
		goto out_stream_close;
	}
	if (stream->error) {
		dev_err(dev, "stream start error: %d\n", stream->error);
		ret = -EIO;
		goto out_stream_close;
	}
	dev_dbg(dev, "start stream: complete\n");

	return 0;

out_stream_close:
	reinit_completion(&stream->stream_close_completion);

	retout = ipu6_fw_isys_simple_cmd(stream->isys,
					 stream->stream_handle,
					 IPU6_FW_ISYS_SEND_TYPE_STREAM_CLOSE);
	if (retout < 0) {
		dev_dbg(dev, "can't close stream (%d)\n", retout);
		return retout;
	}

	tout = wait_for_completion_timeout(&stream->stream_close_completion,
					   IPU6_FW_CALL_TIMEOUT_JIFFIES);
	if (!tout)
		dev_err(dev, "stream close time out\n");
	else if (stream->error)
		dev_err(dev, "stream close error: %d\n", stream->error);
	else
		dev_dbg(dev, "stream close complete\n");

	return ret;
}

static int ipu6_isys_stop_streaming_firmware(struct ipu6_isys_stream *stream,
					     struct v4l2_mbus_frame_desc *desc)
{
	struct device *dev = &stream->isys->adev->auxdev.dev;
	int ret, tout;

	reinit_completion(&stream->stream_stop_completion);

	ret = ipu6_fw_isys_simple_cmd(stream->isys, stream->stream_handle,
				      IPU6_FW_ISYS_SEND_TYPE_STREAM_FLUSH);

	if (ret < 0) {
		dev_err(dev, "can't stop stream (%d)\n", ret);
		return 0;
	}

	tout = wait_for_completion_timeout(&stream->stream_stop_completion,
					   IPU6_FW_CALL_TIMEOUT_JIFFIES);
	if (!tout)
		dev_warn(dev, "stream stop time out\n");
	else if (stream->error)
		dev_warn(dev, "stream stop error: %d\n", stream->error);
	else
		dev_dbg(dev, "stop stream: complete\n");

	return 0;
}

static int ipu6_isys_close_streaming_firmware(struct ipu6_isys_stream *stream,
					      struct v4l2_mbus_frame_desc *desc)
{
	struct device *dev = &stream->isys->adev->auxdev.dev;
	struct ipu6_isys_csi2 *csi2 = ipu6_isys_subdev_to_csi2(stream->asd);
	int ret, tout;

	reinit_completion(&stream->stream_close_completion);

	ret = ipu6_fw_isys_simple_cmd(stream->isys, stream->stream_handle,
				      IPU6_FW_ISYS_SEND_TYPE_STREAM_CLOSE);
	if (ret < 0) {
		dev_err(dev, "can't close stream (%d)\n", ret);
		return 0;
	}

	tout = wait_for_completion_timeout(&stream->stream_close_completion,
					   IPU6_FW_CALL_TIMEOUT_JIFFIES);
	if (!tout)
		dev_warn(dev, "stream close time out\n");
	else if (stream->error)
		dev_warn(dev, "stream close error: %d\n", stream->error);
	else
		dev_dbg(dev, "close stream: complete\n");

	scoped_guard(spinlock_irqsave, &stream->isys->power_lock) {
		stream->isys->streams_by_handle[stream->stream_handle] = NULL;
		csi2->streams_by_vc[stream->vc] = NULL;
	}

	return 0;
}

static int call_on_streams(struct ipu6_isys_csi2 *csi2,
			   int (*start)(struct ipu6_isys_stream *stream,
					struct v4l2_mbus_frame_desc *desc),
			   int (*stop)(struct ipu6_isys_stream *stream,
				       struct v4l2_mbus_frame_desc *desc),
			   struct ipu6_isys_stream *end,
			   struct v4l2_mbus_frame_desc *desc)
{
	struct ipu6_isys_stream *stream;

	list_for_each_entry(stream, &csi2->streams, csi2_entry) {
		int ret;

		if (stream == end)
			break;

		ret = start(stream, desc);
		if (ret && stop) {
			call_on_streams(csi2, stop, NULL, stream, desc);

			return ret;
		}
	}

	return 0;
}

void ipu6_isys_free_streams_firmware(struct ipu6_isys_csi2 *csi2)
{
	struct ipu6_isys_stream *stream, *safe;

	list_for_each_entry_safe(stream, safe, &csi2->streams, csi2_entry) {
		list_del(&stream->csi2_entry);
		ida_free(&csi2->isys->streams, stream->stream_handle);
		kfree(stream);
	}
}

int ipu6_isys_alloc_start_streams_firmware(struct ipu6_isys_csi2 *csi2,
					   struct v4l2_subdev_state *csi2_state,
					   struct v4l2_mbus_frame_desc *desc)
{
	struct device *dev = &csi2->isys->adev->auxdev.dev;
	struct v4l2_subdev_route *route;
	int ret;

	for_each_active_route(&csi2_state->routing, route) {
		struct media_pad *vdev_pad =
			media_pad_remote_pad_first(&csi2->asd.pad[route->source_pad]);
		struct v4l2_mbus_frame_desc_entry *entry = NULL;
		struct ipu6_isys_stream *stream;

		for (unsigned int i = 0; i < desc->num_entries; i++) {
			if (desc->entry[i].stream != route->sink_stream)
				continue;

			entry = &desc->entry[i];
			break;
		}

		if (!entry) {
			dev_dbg(dev, "cannot find stream %u in frame desc\n",
				route->sink_stream);
			ret = -EINVAL;
			goto err_free_streams;
		}

		list_for_each_entry(stream, &csi2->streams, csi2_entry)
			if (stream->vc == entry->bus.csi2.vc)
				break;

		if (list_entry_is_head(stream, &csi2->streams, csi2_entry)) {
			unsigned long flags;

			stream = kzalloc(sizeof(*stream), GFP_KERNEL);
			if (!stream) {
				ret = -ENOMEM;
				goto err_free_streams;
			}

			ret = ida_alloc_max(&csi2->isys->streams,
					    IPU6_ISYS_MAX_STREAMS - 1,
					    GFP_KERNEL);
			if (ret < 0) {
				kfree(stream);
				goto err_free_streams;
			}

			stream->stream_handle = ret;
			mutex_init(&stream->mutex);
			init_completion(&stream->stream_open_completion);
			init_completion(&stream->stream_close_completion);
			init_completion(&stream->stream_start_completion);
			init_completion(&stream->stream_stop_completion);
			INIT_LIST_HEAD(&stream->queues);
			stream->isys = csi2->asd.isys;
			stream->asd = &csi2->asd;
			stream->vc = entry->bus.csi2.vc;

			scoped_guard(spinlock_irqsave, &stream->isys->power_lock) {
				stream->isys->streams_by_handle[stream->stream_handle] =
					stream;
				csi2->streams_by_vc[stream->vc] = stream;
			}

			list_add(&stream->csi2_entry, &csi2->streams);
		}

		struct ipu6_isys_video *av =
			container_of_const(vdev_pad, struct ipu6_isys_video,
					   pad);
		struct ipu6_isys_queue *aq = &av->aq;

		list_add(&aq->node, &stream->queues);

		stream->nr_output_pins++;
		av->stream = stream;
	}

	ret = call_on_streams(csi2, ipu6_isys_start_stream_firmware,
			      ipu6_isys_stop_streaming_firmware, NULL, desc);

	if (!ret)
		return 0;

err_free_streams:
	ipu6_isys_free_streams_firmware(csi2);
	ipu6_isys_free_streams_firmware(csi2);

	return ret;
}

void ipu6_isys_stop_streams_firmware(struct ipu6_isys_csi2 *csi2)
{
	call_on_streams(csi2, ipu6_isys_stop_streaming_firmware, NULL, NULL,
			NULL);
}

void ipu6_isys_close_streams_firmware(struct ipu6_isys_csi2 *csi2)
{
	call_on_streams(csi2, ipu6_isys_close_streaming_firmware, NULL, NULL,
			NULL);
}

int ipu6_isys_video_set_streaming(struct ipu6_isys_video *av, int state)
{
	struct device *dev = &av->isys->adev->auxdev.dev;
	struct v4l2_subdev *sd;
	struct media_pad *r_pad;
	int ret = 0;

	sd = &av->csi2->asd.sd;
	r_pad = media_pad_remote_pad_first(&av->pad);

	if (!state) {
		/* stop sub-device which connects with video */
		dev_dbg(dev, "stream off %s pad:%d\n", sd->name, r_pad->index);
		ret = v4l2_subdev_disable_streams(sd, r_pad->index, 1);
		if (ret) {
			dev_err(dev, "stream off %s failed with %d\n", sd->name,
				ret);
			return ret;
		}
	} else {
		/* start sub-device which connects with video */
		dev_dbg(dev, "stream on %s pad %d\n", sd->name, r_pad->index);
		ret = v4l2_subdev_enable_streams(sd, r_pad->index, 1);
		if (ret)
			dev_err(dev, "stream on %s failed with %d\n", sd->name,
				ret);
	}

	av->streaming = state;

	return ret;
}

static const struct v4l2_ioctl_ops ipu6_v4l2_ioctl_ops = {
	.vidioc_querycap = ipu6_isys_vidioc_querycap,
	.vidioc_enum_fmt_vid_cap = ipu6_isys_vidioc_enum_fmt,
	.vidioc_enum_fmt_meta_cap = ipu6_isys_vidioc_enum_fmt,
	.vidioc_enum_framesizes = ipu6_isys_vidioc_enum_framesizes,
	.vidioc_g_fmt_vid_cap = ipu6_isys_vidioc_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap = ipu6_isys_vidioc_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap = ipu6_isys_vidioc_try_fmt_vid_cap,
	.vidioc_g_fmt_meta_cap = ipu6_isys_vidioc_g_fmt_meta_cap,
	.vidioc_s_fmt_meta_cap = ipu6_isys_vidioc_s_fmt_meta_cap,
	.vidioc_try_fmt_meta_cap = ipu6_isys_vidioc_try_fmt_meta_cap,
	.vidioc_reqbufs = ipu6_isys_vidioc_reqbufs,
	.vidioc_create_bufs = ipu6_isys_vidioc_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
};

static const struct media_entity_operations entity_ops = {
	.link_validate = link_validate,
};

static const struct v4l2_file_operations isys_fops = {
	.owner = THIS_MODULE,
	.poll = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = vb2_fop_mmap,
	.open = video_open,
	.release = vb2_fop_release,
};

int ipu6_isys_fw_open(struct ipu6_isys *isys)
{
	struct ipu6_bus_device *adev = isys->adev;
	const struct ipu6_isys_internal_pdata *ipdata = isys->pdata->ipdata;
	int ret;

	ret = pm_runtime_resume_and_get(&adev->auxdev.dev);
	if (ret < 0)
		return ret;

	mutex_lock(&isys->mutex);

	if (isys->ref_count++)
		goto unlock;

	ipu6_configure_spc(adev->isp, &ipdata->hw_variant,
			   IPU6_CPD_PKG_DIR_ISYS_SERVER_IDX, isys->pdata->base,
			   adev->pkg_dir, adev->pkg_dir_dma_addr);

	/*
	 * Buffers could have been left to wrong queue at last closure.
	 * Move them now back to empty buffer queue.
	 */
	ipu6_cleanup_fw_msg_bufs(isys);

	if (isys->fwcom) {
		/*
		 * Something went wrong in previous shutdown. As we are now
		 * restarting isys we can safely delete old context.
		 */
		dev_warn(&adev->auxdev.dev, "clearing old context\n");
		ipu6_fw_isys_cleanup(isys);
	}

	ret = ipu6_fw_isys_init(isys, ipdata->num_parallel_streams);
	if (ret < 0)
		goto out;

unlock:
	mutex_unlock(&isys->mutex);

	return 0;

out:
	isys->ref_count--;
	mutex_unlock(&isys->mutex);
	pm_runtime_put(&adev->auxdev.dev);

	return ret;
}

void ipu6_isys_fw_close(struct ipu6_isys *isys)
{
	mutex_lock(&isys->mutex);

	isys->ref_count--;
	if (!isys->ref_count) {
		ipu6_fw_isys_close(isys);
		if (isys->fwcom) {
			isys->need_reset = true;
			dev_warn(&isys->adev->auxdev.dev,
				 "failed to close fw isys\n");
		}
	}

	mutex_unlock(&isys->mutex);

	if (isys->need_reset)
		pm_runtime_put_sync(&isys->adev->auxdev.dev);
	else
		pm_runtime_put(&isys->adev->auxdev.dev);
}

/*
 * Do everything that's needed to initialise things related to video
 * buffer queue, video node, and the related media entity. The caller
 * is expected to assign isys field and set the name of the video
 * device.
 */
int ipu6_isys_video_init(struct ipu6_isys_video *av)
{
	struct v4l2_format format = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.fmt.pix = {
			.width = 1920,
			.height = 1080,
		},
	};
	struct v4l2_format format_meta = {
		.type = V4L2_BUF_TYPE_META_CAPTURE,
		.fmt.meta = {
			.width = 1920,
			.height = 4,
		},
	};
	int ret;

	mutex_init(&av->mutex);
	av->vdev.device_caps = V4L2_CAP_STREAMING | V4L2_CAP_IO_MC |
			       V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_META_CAPTURE;
	av->vdev.vfl_dir = VFL_DIR_RX;

	ret = ipu6_isys_queue_init(&av->aq);
	if (ret)
		goto out_mutex_destroy;

	av->pad.flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	ret = media_entity_pads_init(&av->vdev.entity, 1, &av->pad);
	if (ret)
		goto out_vb2_queue_release;

	av->vdev.entity.ops = &entity_ops;
	av->vdev.release = video_device_release_empty;
	av->vdev.fops = &isys_fops;
	av->vdev.v4l2_dev = &av->isys->v4l2_dev;
	av->vdev.dev_parent = &av->isys->adev->isp->pdev->dev;
	if (!av->vdev.ioctl_ops)
		av->vdev.ioctl_ops = &ipu6_v4l2_ioctl_ops;
	av->vdev.queue = &av->aq.vbq;
	av->vdev.lock = &av->mutex;

	__ipu6_isys_vidioc_try_fmt_vid_cap(av, &format);
	av->pix_fmt = format.fmt.pix;
	__ipu6_isys_vidioc_try_fmt_meta_cap(av, &format_meta);
	av->meta_fmt = format_meta.fmt.meta;

	video_set_drvdata(&av->vdev, av);

	ret = video_register_device(&av->vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto out_media_entity_cleanup;

	return ret;

out_media_entity_cleanup:
	vb2_video_unregister_device(&av->vdev);
	media_entity_cleanup(&av->vdev.entity);

out_vb2_queue_release:
	vb2_queue_release(&av->aq.vbq);

out_mutex_destroy:
	mutex_destroy(&av->mutex);

	return ret;
}

void ipu6_isys_video_cleanup(struct ipu6_isys_video *av)
{
	vb2_video_unregister_device(&av->vdev);
	media_entity_cleanup(&av->vdev.entity);
	mutex_destroy(&av->mutex);
}

u32 ipu6_isys_get_format(struct ipu6_isys_video *av)
{
	if (av->aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return av->pix_fmt.pixelformat;

	if (av->aq.vbq.type == V4L2_BUF_TYPE_META_CAPTURE)
		return av->meta_fmt.dataformat;

	return 0;
}

u32 ipu6_isys_get_data_size(struct ipu6_isys_video *av)
{
	if (av->aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return av->pix_fmt.sizeimage;

	if (av->aq.vbq.type == V4L2_BUF_TYPE_META_CAPTURE)
		return av->meta_fmt.buffersize;

	return 0;
}

u32 ipu6_isys_get_bytes_per_line(struct ipu6_isys_video *av)
{
	if (av->aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return av->pix_fmt.bytesperline;

	if (av->aq.vbq.type == V4L2_BUF_TYPE_META_CAPTURE)
		return av->meta_fmt.bytesperline;

	return 0;
}

u32 ipu6_isys_get_frame_width(struct ipu6_isys_video *av)
{
	if (av->aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return av->pix_fmt.width;

	if (av->aq.vbq.type == V4L2_BUF_TYPE_META_CAPTURE)
		return av->meta_fmt.width;

	return 0;
}

u32 ipu6_isys_get_frame_height(struct ipu6_isys_video *av)
{
	if (av->aq.vbq.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return av->pix_fmt.height;

	if (av->aq.vbq.type == V4L2_BUF_TYPE_META_CAPTURE)
		return av->meta_fmt.height;

	return 0;
}
