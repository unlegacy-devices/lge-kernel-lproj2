/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "msm_sensor.h"
#include "msm.h"
#include "msm_ispif.h"
#include "t4k28_reg.h"

#define SENSOR_NAME "t4k28"
#define PLATFORM_DRIVER_NAME "msm_camera_t4k28"
#define t4k28_obj t4k28_##obj

#define SENSOR_REG_PAGE_ADDR 0x03
#define SENSOR_REG_PAGE_0 0x00

DEFINE_MUTEX(t4k28_mut);
static struct msm_sensor_ctrl_t t4k28_s_ctrl;

static int PREV_EFFECT = -1;
static int PREV_ISO = -1;
static int PREV_WB = -1;
static int PREV_FPS = -1;
static int PREV_BESTSHOT = -1;
static int DELAY_START = 0;

static struct v4l2_subdev_info t4k28_subdev_info[] = {
	{
		.code = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt = 1,
		.order = 0,
	},
};

static struct msm_camera_i2c_conf_array t4k28_init_conf[] = {
	{&t4k28_recommend_settings[0],
	ARRAY_SIZE(t4k28_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array t4k28_confs[] = {
	{&t4k28_snap_settings[0],
	ARRAY_SIZE(t4k28_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&t4k28_prev_settings[0],
	ARRAY_SIZE(t4k28_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t t4k28_dimensions[] = {
	{
		.x_output = 0x640,
		.y_output = 0x4B0,
		.line_length_pclk = 0x800,
		.frame_length_lines = 0x600,
		.vt_pixel_clk = 45600000,
		.op_pixel_clk = 45600000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x320,
		.y_output = 0x258,
		.line_length_pclk = 0x400,
		.frame_length_lines = 0x300,
		.vt_pixel_clk = 45600000,
		.op_pixel_clk = 45600000,
		.binning_factor = 1,
	},
};

static struct msm_camera_i2c_conf_array t4k28_exposure_confs[][1] = {
	{{t4k28_exposure[0], ARRAY_SIZE(t4k28_exposure[0]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[1], ARRAY_SIZE(t4k28_exposure[1]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[2], ARRAY_SIZE(t4k28_exposure[2]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[3], ARRAY_SIZE(t4k28_exposure[3]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[4], ARRAY_SIZE(t4k28_exposure[4]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[5], ARRAY_SIZE(t4k28_exposure[5]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[6], ARRAY_SIZE(t4k28_exposure[6]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[7], ARRAY_SIZE(t4k28_exposure[7]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[8], ARRAY_SIZE(t4k28_exposure[8]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[9], ARRAY_SIZE(t4k28_exposure[9]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[10], ARRAY_SIZE(t4k28_exposure[10]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[11], ARRAY_SIZE(t4k28_exposure[11]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[12], ARRAY_SIZE(t4k28_exposure[12]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_exposure[13], ARRAY_SIZE(t4k28_exposure[13]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int t4k28_exposure_enum_map[] = {
	MSM_V4L2_EXPOSURE_N6,
	MSM_V4L2_EXPOSURE_N5,
	MSM_V4L2_EXPOSURE_N4,
	MSM_V4L2_EXPOSURE_N3,
	MSM_V4L2_EXPOSURE_N2,
	MSM_V4L2_EXPOSURE_N1,
	MSM_V4L2_EXPOSURE_D,
	MSM_V4L2_EXPOSURE_P1,
	MSM_V4L2_EXPOSURE_P2,
	MSM_V4L2_EXPOSURE_P3,
	MSM_V4L2_EXPOSURE_P4,
	MSM_V4L2_EXPOSURE_P5,
	MSM_V4L2_EXPOSURE_P6,
};

static struct msm_camera_i2c_enum_conf_array t4k28_exposure_enum_confs = {
	.conf = &t4k28_exposure_confs[0][0],
	.conf_enum = t4k28_exposure_enum_map,
	.num_enum = ARRAY_SIZE(t4k28_exposure_enum_map),
	.num_index = ARRAY_SIZE(t4k28_exposure_confs),
	.num_conf = ARRAY_SIZE(t4k28_exposure_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_conf_array t4k28_special_effect_confs[][1] = {
	{{t4k28_special_effect[0], ARRAY_SIZE(t4k28_special_effect[0]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_special_effect[1], ARRAY_SIZE(t4k28_special_effect[1]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_special_effect[2], ARRAY_SIZE(t4k28_special_effect[2]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_special_effect[3], ARRAY_SIZE(t4k28_special_effect[3]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_special_effect[4], ARRAY_SIZE(t4k28_special_effect[4]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int t4k28_special_effect_enum_map[] = {
	MSM_V4L2_EFFECT_OFF,
	MSM_V4L2_EFFECT_MONO,
	MSM_V4L2_EFFECT_NEGATIVE,
	MSM_V4L2_EFFECT_SEPIA,
	MSM_V4L2_EFFECT_MAX,
};

static struct msm_camera_i2c_enum_conf_array
		 t4k28_special_effect_enum_confs = {
	.conf = &t4k28_special_effect_confs[0][0],
	.conf_enum = t4k28_special_effect_enum_map,
	.num_enum = ARRAY_SIZE(t4k28_special_effect_enum_map),
	.num_index = ARRAY_SIZE(t4k28_special_effect_confs),
	.num_conf = ARRAY_SIZE(t4k28_special_effect_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_conf_array t4k28_wb_oem_confs[][1] = {
	{{t4k28_wb_oem[0], ARRAY_SIZE(t4k28_wb_oem[0]), 0,	//Auto
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_wb_oem[1], ARRAY_SIZE(t4k28_wb_oem[1]), 0,	//Incandescent
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_wb_oem[2], ARRAY_SIZE(t4k28_wb_oem[2]), 0,	//fluorescent
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_wb_oem[3], ARRAY_SIZE(t4k28_wb_oem[3]), 0,	//daylight
		MSM_CAMERA_I2C_BYTE_DATA},},
	{{t4k28_wb_oem[4], ARRAY_SIZE(t4k28_wb_oem[4]), 0,	//cloudy
		MSM_CAMERA_I2C_BYTE_DATA},},
};

static int t4k28_wb_oem_enum_map[] = {
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
};

static struct msm_camera_i2c_enum_conf_array t4k28_wb_oem_enum_confs = {
	.conf = &t4k28_wb_oem_confs[0][0],
	.conf_enum = t4k28_wb_oem_enum_map,
	.num_enum = ARRAY_SIZE(t4k28_wb_oem_enum_map),
	.num_index = ARRAY_SIZE(t4k28_wb_oem_confs),
	.num_conf = ARRAY_SIZE(t4k28_wb_oem_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_conf_array t4k28_iso_confs[][1] = {
	{{t4k28_iso[0], ARRAY_SIZE(t4k28_iso[0]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},		//AUTO
	{{t4k28_iso[1], ARRAY_SIZE(t4k28_iso[1]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},		//DEBLUR
	{{t4k28_iso[2], ARRAY_SIZE(t4k28_iso[2]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},		//100
	{{t4k28_iso[3], ARRAY_SIZE(t4k28_iso[3]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},		//200
	{{t4k28_iso[4], ARRAY_SIZE(t4k28_iso[4]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},		//400
	{{t4k28_iso[5], ARRAY_SIZE(t4k28_iso[5]), 0,
		MSM_CAMERA_I2C_BYTE_DATA},},		//800
};

static int t4k28_iso_enum_map[] = {
	MSM_V4L2_ISO_AUTO ,
	MSM_V4L2_ISO_DEBLUR,
	MSM_V4L2_ISO_100,
	MSM_V4L2_ISO_200,
	MSM_V4L2_ISO_400,
	MSM_V4L2_ISO_800,
};

static struct msm_camera_i2c_enum_conf_array t4k28_iso_enum_confs = {
	.conf = &t4k28_iso_confs[0][0],
	.conf_enum = t4k28_iso_enum_map,
	.num_enum = ARRAY_SIZE(t4k28_iso_enum_map),
	.num_index = ARRAY_SIZE(t4k28_iso_confs),
	.num_conf = ARRAY_SIZE(t4k28_iso_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_conf_array t4k28_fps_range_confs[][3] = {
	{
		{&t4k28_attached_fps_settings0[0],
				ARRAY_SIZE(t4k28_attached_fps_settings0), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		{&t4k28_auto_fps_settings0[0],
				ARRAY_SIZE(t4k28_auto_fps_settings0), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		{&t4k28_fixed_fps_settings0[0],
				ARRAY_SIZE(t4k28_fixed_fps_settings0), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
};

static int t4k28_fps_range_enum_map[] = {
	MSM_V4L2_FPS_15_15,
	MSM_V4L2_FPS_7P5_30,
	MSM_V4L2_FPS_30_30,
};

static struct msm_camera_i2c_enum_conf_array t4k28_fps_range_enum_confs = {
	.conf = &t4k28_fps_range_confs[0][0],
	.conf_enum = t4k28_fps_range_enum_map,
	.num_enum = ARRAY_SIZE(t4k28_fps_range_enum_map),
	.num_index = ARRAY_SIZE(t4k28_fps_range_confs),
	.num_conf = ARRAY_SIZE(t4k28_fps_range_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_conf_array t4k28_bestshot_mode_confs[][1] = {
	{
		//SCENE OFF
		{&t4k28_scene_normal_settings[0],
				ARRAY_SIZE(t4k28_scene_normal_settings), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE AUTO
		{&t4k28_scene_normal_settings[0],
			ARRAY_SIZE(t4k28_scene_normal_settings), 10,
			MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE LANDSCAPE
		{&t4k28_scene_landscape_settings[0],
				ARRAY_SIZE(t4k28_scene_landscape_settings), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE SNOW //NOT USE
		{&t4k28_no_active_settings[0],
				ARRAY_SIZE(t4k28_no_active_settings), 0,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE BEACH //NOT USE
		{&t4k28_no_active_settings[0],
				ARRAY_SIZE(t4k28_no_active_settings), 0,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE SUNSET
		{&t4k28_scene_sunset_settings[0],
				ARRAY_SIZE(t4k28_scene_sunset_settings), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE NIGHT
		{&t4k28_scene_night_settings[0],
				ARRAY_SIZE(t4k28_scene_night_settings), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE PORTRAIT
		{&t4k28_scene_portrait_settings[0],
				ARRAY_SIZE(t4k28_scene_portrait_settings), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE BACKLIGHT //NOT USE
		{&t4k28_no_active_settings[0],
				ARRAY_SIZE(t4k28_no_active_settings), 0,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
	{
		//SCENE SPORTS
		{&t4k28_scene_sport_settings[0],
				ARRAY_SIZE(t4k28_scene_sport_settings), 10,
				MSM_CAMERA_I2C_BYTE_DATA},
	},
};

static int t4k28_bestshot_mode_enum_map[] = {
	MSM_V4L2_BESTSHOT_OFF,
	MSM_V4L2_BESTSHOT_AUTO,
	MSM_V4L2_BESTSHOT_LANDSCAPE,
	MSM_V4L2_BESTSHOT_SNOW,			//NOT USE
	MSM_V4L2_BESTSHOT_BEACH,		//NOT USE
	MSM_V4L2_BESTSHOT_SUNSET,
	MSM_V4L2_BESTSHOT_NIGHT,
	MSM_V4L2_BESTSHOT_PORTRAIT,
	MSM_V4L2_BESTSHOT_BACKLIGHT,		//NOT USE
	MSM_V4L2_BESTSHOT_SPORTS,
};

static struct msm_camera_i2c_enum_conf_array t4k28_bestshot_mode_enum_confs = {
	.conf = &t4k28_bestshot_mode_confs[0][0],
	.conf_enum = t4k28_bestshot_mode_enum_map,
	.num_enum = ARRAY_SIZE(t4k28_bestshot_mode_enum_map),
	.num_index = ARRAY_SIZE(t4k28_bestshot_mode_confs),
	.num_conf = ARRAY_SIZE(t4k28_bestshot_mode_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

int t4k28_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	int retry = 0;
	switch (ctrl_info->ctrl_id) {
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		if (PREV_WB == value || PREV_WB == -1) {
			PREV_WB = value;
			pr_err("%s SKIP due to duplicate enum num: %d,"
					" value = %d\n", __func__,
					ctrl_info->ctrl_id, value);
			return rc;
		} else
			PREV_WB = value;
		break;
	case V4L2_CID_SPECIAL_EFFECT:
		if (PREV_EFFECT == value || PREV_EFFECT == -1) {
			PREV_EFFECT = value;
			pr_err("%s SKIP due to duplicate enum num: %d,"
					" value = %d\n", __func__,
					ctrl_info->ctrl_id, value);
			return rc;
		} else
			PREV_EFFECT = value;
		break;
	case MSM_V4L2_PID_ISO:
		if (PREV_ISO == value || PREV_ISO == -1) {
			PREV_ISO = value;
			pr_err("%s SKIP due to duplicate enum num: %d,"
					" value = %d\n", __func__,
					ctrl_info->ctrl_id, value);
			return rc;
		} else
			PREV_ISO = value;
		break;
	case V4L2_CID_FPS_RANGE:
		if (PREV_FPS == value || PREV_FPS == -1) {
			PREV_FPS = value;
			pr_err("%s SKIP due to duplicate enum num: %d,"
					"value = %d\n", __func__,
					ctrl_info->ctrl_id, value);
			return rc;
		} else
			PREV_FPS = value;
		break;
	case V4L2_CID_BESTSHOT_MODE:
		if (PREV_BESTSHOT == value || PREV_BESTSHOT == -1) {
			PREV_BESTSHOT = value;
			pr_err("%s SKIP due to duplicate enum num: %d,"
					" value = %d\n", __func__,
					ctrl_info->ctrl_id, value);
			return rc;
		} else
			PREV_BESTSHOT = value;
		break;
	default:
		break;
	}

	for (retry = 0; retry < 3; ++retry) {
		rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);

		if (rc < 0)
			pr_err("%s:write failed for enum num: %d, value = %d,"
					" retry = %d\n", __func__,
					ctrl_info->ctrl_id, value, retry);
		else
			break;
	}
	pr_err("%s: write done for enum num: %d, value = %d\n", __func__,
			ctrl_info->ctrl_id, value);
	return rc;
}

struct msm_sensor_v4l2_ctrl_info_t t4k28_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_EXPOSURE,
		.min = MSM_V4L2_EXPOSURE_N6,
		.max = MSM_V4L2_EXPOSURE_P6,
		.step = 1,
		.enum_cfg_settings = &t4k28_exposure_enum_confs,
		.s_v4l2_ctrl = t4k28_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SPECIAL_EFFECT,
		.min = MSM_V4L2_EFFECT_OFF,
		.max = MSM_V4L2_EFFECT_NEGATIVE,
		.step = 1,
		.enum_cfg_settings = &t4k28_special_effect_enum_confs,
		.s_v4l2_ctrl = t4k28_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.min = MSM_V4L2_WB_AUTO,
		.max = MSM_V4L2_WB_CLOUDY_DAYLIGHT,
		.step = 1,
		.enum_cfg_settings = &t4k28_wb_oem_enum_confs,
		.s_v4l2_ctrl = t4k28_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = MSM_V4L2_PID_ISO,
		.min = MSM_V4L2_ISO_AUTO,
		.max = MSM_V4L2_ISO_800,
		.step = 1,
		.enum_cfg_settings = &t4k28_iso_enum_confs,
		.s_v4l2_ctrl = t4k28_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_FPS_RANGE,
		.min = MSM_V4L2_FPS_15_15,
		.max = MSM_V4L2_FPS_30_30,
		.step = 1,
		.enum_cfg_settings = &t4k28_fps_range_enum_confs,
		.s_v4l2_ctrl = t4k28_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_BESTSHOT_MODE,
		.min = MSM_V4L2_BESTSHOT_OFF,
		.max = MSM_V4L2_BESTSHOT_SPORTS,
		.step = 1,
		.enum_cfg_settings = &t4k28_bestshot_mode_enum_confs,
		.s_v4l2_ctrl = t4k28_msm_sensor_s_ctrl_by_enum,
	},
};

static struct msm_camera_csi_params t4k28_csic_params = {
	.data_format	= CSI_8BIT,
	.lane_cnt	= 1,
	.lane_assign	= 0xe4,
	.dpcm_scheme	= 0,
	.settle_cnt	= 0x19,
};

static struct msm_camera_csi_params *t4k28_csic_params_array[] = {
	&t4k28_csic_params,
	&t4k28_csic_params,
};

// not used
static struct msm_sensor_output_reg_addr_t t4k28_reg_addr = {
	.x_output = 0x26,
	.y_output = 0x28,
	.line_length_pclk = 0x26,
	.frame_length_lines = 0x28,
};

static struct msm_sensor_id_info_t t4k28_id_info = {
	.sensor_id_reg_addr = 0x3000,
	.sensor_id = 0x0840,
};

static const struct i2c_device_id t4k28_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&t4k28_s_ctrl},
	{ }
};

static struct i2c_driver t4k28_i2c_driver = {
	.id_table = t4k28_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client t4k28_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int rc = 0;
	pr_err("[CAMERA] t4k28\n");

	rc = i2c_add_driver(&t4k28_i2c_driver);

	return rc;
}

static struct v4l2_subdev_core_ops t4k28_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

int8_t t4k28_get_snapshot_data(struct msm_sensor_ctrl_t *s_ctrl,
		struct snapshot_soc_data_cfg *snapshot_data)
{
	int rc = 0;
	unsigned short analogGain1 = 0;
	unsigned short analogGain2 = 0;
	unsigned short analogGain = 0;
	int exposureTime = 0;
	unsigned int isoSpeed = 0;
	unsigned short Exposure1 = 0;
	unsigned short Exposure2 = 0;
	uint32_t ExposureTotal = 0;

	//ISO Speed
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3561,
			&analogGain1, MSM_CAMERA_I2C_BYTE_DATA);
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3562,
			&analogGain2, MSM_CAMERA_I2C_BYTE_DATA);

	if (rc < 0) {
		pr_err("%s: error to get analog & digital gain \n", __func__);
		return rc;
	}
	//Exposure Time
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x355f, &Exposure1,
			MSM_CAMERA_I2C_BYTE_DATA);
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3560, &Exposure2,
			MSM_CAMERA_I2C_BYTE_DATA);

	if (rc < 0) {
		pr_err("%s: error to get exposure time \n", __func__);
		return rc;
	}

	//ISO speed
	analogGain = ((analogGain1<<8)|analogGain2);
	isoSpeed = ((analogGain / 32) * 100);

	//Exposure Time
	ExposureTotal = ((Exposure1<<8)|Exposure2);
	if (ExposureTotal <= 0)
		exposureTime = 600000;
	else
		exposureTime = ExposureTotal;

	snapshot_data->iso_speed = isoSpeed;
	snapshot_data->exposure_time = exposureTime;

	return 0;
}

static int32_t t4k28_i2c_write_b_sensor(struct msm_camera_i2c_client *client,
		u8 baddr, u8 bdata)
{
	int32_t rc = -EIO;
	u8 buf[2];
	memset(buf, 0, sizeof(buf));

	if (DELAY_START == 1) {
		if (baddr == 0xFE)
			msleep(bdata);
		DELAY_START = 0;
		return 0;
	} else {
		if (baddr == 0x03 && bdata == 0xFE) {
			DELAY_START = 1;
			return 0;
		} else {
			buf[0] = baddr;
			buf[1] = bdata;

			rc = msm_camera_i2c_txdata(client, buf, 2);
			if (rc < 0)
				pr_err("i2c_write_w failed, addr = 0x%x,"
						" val = 0x%x!\n", baddr, bdata);
		}
	}
	return rc;
}

static int32_t t4k28_i2c_write_b_table(struct msm_camera_i2c_client *client,
		struct msm_camera_i2c_reg_conf *reg_conf_tbl, uint16_t size)
{
	int i;
	int32_t rc = 0;
	for (i = 0; i < size; i++) {
		rc = t4k28_i2c_write_b_sensor(client, reg_conf_tbl->reg_addr,
				reg_conf_tbl->reg_data);
		if (rc < 0) {
			pr_err("t4k28_i2c_write_b_table fail\n");
			break;
		}
		reg_conf_tbl++;
	}
	pr_info("%s: %d	Exit \n",__func__, __LINE__);
	return rc;
}

int32_t t4k28_sensor_write_conf_array(struct msm_camera_i2c_client *client,
			struct msm_camera_i2c_conf_array *array, uint16_t index)
{
	int32_t rc = 0;
	int32_t retry = 0;

	for (retry = 0; retry < 3; ++retry) {
		rc = t4k28_i2c_write_b_table(client,
		(struct msm_camera_i2c_reg_conf *) array[index].conf,
				array[index].size);

		if (rc < 0)
			pr_err("%s:Sensor Mode Setting Fail,"
					" try again retry = %d\n", __func__,
					retry);
		else
			break;
	}

	if (array[index].delay > 20)
		msleep(array[index].delay);
	else
		usleep_range(array[index].delay*1000,
					(array[index].delay+1)*1000);
	return rc;
}

int32_t t4k28_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;

	if (update_type == MSM_SENSOR_REG_INIT) {
		pr_err("Register INIT with Flicker 60Hz Mode\n");
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		PREV_EFFECT = -1;
		PREV_ISO = -1;
		PREV_WB = -1;
		PREV_FPS = -1;
		PREV_BESTSHOT = -1;
		DELAY_START = 0;
		msm_sensor_write_init_settings(s_ctrl);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		pr_err("PERIODIC : %d\n", res);
		if (!csi_config) {
			s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
			msleep(20);
			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			pr_err("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			pr_err("CSI config is done\n");
			mb();
			msleep(50);
			csi_config = 1;
			s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		}
		msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE,
			&s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);
		if (res ==0)
			msleep(120);
	}
	return rc;
}

int t4k28_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	pr_info("%s: E %d\n", __func__, __LINE__);
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		goto request_gpio_failed;
	}

	// 1. RESET LOW
	if (data->sensor_reset_enable) {
		rc = gpio_request(data->sensor_reset, "t4k28_reset");
		if (rc < 0)
			pr_err("%s: gpio_request:CAM_RESET %d failed\n",
					__func__, data->sensor_reset);
		rc = gpio_direction_output(data->sensor_reset, 0);
		if (rc < 0)
			pr_err("%s: gpio:CAM_RESET %d direction can't be set\n",
					 __func__, data->sensor_reset);
	}

	// 2. PWDN LOW
	rc = gpio_request(data->sensor_pwd, "t4k28_pwdn");
	if (rc < 0)
		pr_err("%s: gpio_request:CAM_PWDN %d failed\n", __func__,
				data->sensor_pwd);
	rc = gpio_direction_output(data->sensor_pwd, 0);
	if (rc < 0)
		pr_err("%s: gpio:CAM_PWDN %d direction can't be set\n",
				__func__, data->sensor_pwd);
	msleep(1);

	// 3. CAM PWR ON
	rc = msm_camera_config_gpio_table(data, 1);
	if (rc < 0) {
		pr_err("%s: config gpio failed\n", __func__);
		goto config_gpio_failed;
	}
	msleep(1);

	// 4. MCLK Enable
	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;
	pr_err("MCLK set\n");

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}
	msleep(1);

	// 6. RESET HIGH
	rc = gpio_direction_output(data->sensor_reset, 1);
	if (rc < 0)
		pr_err("%s: gpio:CAM_RESET %d direction can't be set\n",
				__func__, data->sensor_reset);
	msleep(1);

	pr_info("%s: X %d\n", __func__, __LINE__);

	return rc;
enable_clk_failed:
	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	data->sensor_platform_info->ext_power_ctrl(0);
	return rc;
}

int t4k28_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	int32_t rc = 0;
	pr_info("%s: E %d\n", __func__, __LINE__);

	// 1. RESET LOW
	msleep(1);
	if (data->sensor_reset_enable) {
		rc = gpio_direction_output(data->sensor_reset, 0);
		if (rc < 0)
			pr_err("%s: gpio:CAM_RESET %d direction can't be set\n",
					__func__, data->sensor_reset);
		gpio_free(data->sensor_reset);
	}
	msleep(1);

	// 2. MCLK disable
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	msleep(1);

	gpio_free(data->sensor_pwd);
	msleep(1);

	// 4. CAM PWR OFF
	msm_camera_config_gpio_table(data, 0);
	msm_camera_request_gpio_table(data, 0);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);

	kfree(s_ctrl->reg_ptr);
	pr_info("%s: X %d\n", __func__, __LINE__);
	return 0;
}

int32_t t4k28_camera_i2c_write_tbl(struct msm_camera_i2c_client *client,
 struct msm_camera_i2c_reg_conf *reg_conf_tbl, uint16_t size,
 enum msm_camera_i2c_data_type data_type)
{
	int i;
	int32_t rc = -EFAULT;
	for (i = 0; i < size; i++) {
		enum msm_camera_i2c_data_type dt;
		if (reg_conf_tbl->cmd_type == MSM_CAMERA_I2C_CMD_POLL) {
			rc = msm_camera_i2c_poll(client, reg_conf_tbl->reg_addr,
				reg_conf_tbl->reg_data, reg_conf_tbl->dt);
		} else {
			if (reg_conf_tbl->dt == 0)
				dt = data_type;
			else
				dt = reg_conf_tbl->dt;

			switch (dt) {
			case MSM_CAMERA_I2C_BYTE_DATA:
			case MSM_CAMERA_I2C_WORD_DATA:
				rc = msm_camera_i2c_write(
					client,
					reg_conf_tbl->reg_addr,
					reg_conf_tbl->reg_data, dt);
				break;
			default:
				pr_err("%s: Unsupport data type: %d\n",
					__func__, dt);
				break;
			}
		}
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}
	return rc;
}

void t4k28_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	t4k28_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->start_stream_conf,
		s_ctrl->msm_sensor_reg->start_stream_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
}

void t4k28_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	t4k28_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->stop_stream_conf,
		s_ctrl->msm_sensor_reg->stop_stream_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
}

int32_t t4k28_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint16_t chipid = 0;

	rc = msm_camera_i2c_write(
			s_ctrl->sensor_i2c_client,
			SENSOR_REG_PAGE_ADDR,
			SENSOR_REG_PAGE_0,
			MSM_CAMERA_I2C_BYTE_DATA);

	rc = msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_id_info->sensor_id_reg_addr, &chipid,
			MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0) {
		pr_err("%s: %s: read id failed\n", __func__,
			s_ctrl->sensordata->sensor_name);
		return rc;
	}

	if (chipid != s_ctrl->sensor_id_info->sensor_id) {
		pr_err("t4k28_match chip id doesnot match\n");
		return -ENODEV;
	}
	return rc;
}

static struct v4l2_subdev_video_ops t4k28_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops t4k28_subdev_ops = {
	.core = &t4k28_subdev_core_ops,
	.video  = &t4k28_subdev_video_ops,
};

static struct msm_sensor_fn_t t4k28_func_tbl = {
	.sensor_start_stream = t4k28_sensor_start_stream,
	.sensor_stop_stream = t4k28_sensor_stop_stream,
	.sensor_csi_setting = t4k28_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = t4k28_sensor_power_up,
	.sensor_power_down = t4k28_sensor_power_down,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_match_id = t4k28_match_id,
	.sensor_get_soc_snapshotdata = t4k28_get_snapshot_data,
};

static struct msm_sensor_reg_t t4k28_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = t4k28_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(t4k28_start_settings),
	.stop_stream_conf = t4k28_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(t4k28_stop_settings),
	.init_settings = &t4k28_init_conf[0],
	.init_size = ARRAY_SIZE(t4k28_init_conf),
	.mode_settings = &t4k28_confs[0],
	.output_settings = &t4k28_dimensions[0],
	.num_conf = ARRAY_SIZE(t4k28_confs),
};

static struct msm_sensor_ctrl_t t4k28_s_ctrl = {
	.msm_sensor_reg = &t4k28_regs,
	.msm_sensor_v4l2_ctrl_info = t4k28_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(t4k28_v4l2_ctrl_info),
	.sensor_i2c_client = &t4k28_sensor_i2c_client,
	.sensor_i2c_addr = 0x78,
	.sensor_output_reg_addr = &t4k28_reg_addr,
	.sensor_id_info = &t4k28_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &t4k28_csic_params_array[0],
	.msm_sensor_mutex = &t4k28_mut,
	.sensor_i2c_driver = &t4k28_i2c_driver,
	.sensor_v4l2_subdev_info = t4k28_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(t4k28_subdev_info),
	.sensor_v4l2_subdev_ops = &t4k28_subdev_ops,
	.func_tbl = &t4k28_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Toshiba 2M YUV sensor driver");
MODULE_LICENSE("GPL v2");

