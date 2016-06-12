/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <asm/mach-types.h>
#include <mach/msm_iomap.h>
#include <mach/board.h>
#include <mach/irqs-7xxx.h>
#include "devices-msm7x2xa.h"
#include "board-msm7627a.h"

#include <mach/vreg.h>
#include CONFIG_LGE_BOARD_HEADER_FILE

static struct camera_vreg_t msm_cam_vreg[] = {
};

static uint32_t t4k28_cam_off_gpio_table[] = {
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), // mclk
	GPIO_CFG(42, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t t4k28_cam_on_gpio_table[] = {
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), // mclk
	GPIO_CFG(42, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static struct gpio t4k28_cam_req_gpio[] = {
};

static struct msm_gpio_set_tbl t4k28_cam_gpio_set_tbl[] = {
};

static struct msm_camera_gpio_conf gpio_conf_t4k28 = {
	.camera_off_table = t4k28_cam_off_gpio_table,
	.camera_off_table_size = ARRAY_SIZE(t4k28_cam_off_gpio_table),
	.camera_on_table = t4k28_cam_on_gpio_table,
	.camera_on_table_size = ARRAY_SIZE(t4k28_cam_on_gpio_table),
	.cam_gpio_req_tbl = t4k28_cam_req_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(t4k28_cam_req_gpio),
	.cam_gpio_set_tbl = t4k28_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(t4k28_cam_gpio_set_tbl),
	.gpio_no_mux = 1,
};

static struct regulator *regulator_cam_iovdd;
static struct regulator *regulator_cam_avdd;
static struct regulator *regulator_cam_dvdd;

static int msm_camera_vreg_config(int vreg_en)
{
	int rc = 0;
	int is_on;

	pr_info("%s: vreg_en = %d\n", __func__, vreg_en);

	if (vreg_en) {
		/* CAM_AVDD */
		if (regulator_cam_avdd != NULL) {
			is_on = regulator_is_enabled(regulator_cam_avdd);
			if (vreg_en != is_on) {
				rc |= regulator_enable(regulator_cam_avdd);
				if (rc)
					pr_err("%s: could not %sable cam_avdd regulators: %d\n", __func__, vreg_en ? "en" : "dis", rc);
			}
		}

		/* CAM_IOVDD */
		if (regulator_cam_iovdd != NULL) {
			is_on = regulator_is_enabled(regulator_cam_iovdd);
			if (vreg_en != is_on) {
				rc |= regulator_enable(regulator_cam_iovdd);
				if (rc)
					pr_err("%s: could not %sable cam_iovdd regulators: %d\n", __func__, vreg_en ? "en" : "dis", rc);
			}
		}

		/* CAM_DVDD */
		if (regulator_cam_avdd != NULL) {
			is_on = regulator_is_enabled(regulator_cam_dvdd);
			if (vreg_en != is_on) {
				rc |= regulator_enable(regulator_cam_dvdd);
				if(rc)
					pr_err("%s: could not %sable cam_dvdd regulators: %d\n", __func__, vreg_en ? "en" : "dis", rc);
			}
		}
	} else {
		/* CAM_DVDD */
		if (regulator_cam_dvdd != NULL) {
			is_on = regulator_is_enabled(regulator_cam_dvdd);
			if (vreg_en != is_on) {
				rc |= regulator_disable(regulator_cam_dvdd);
				if (rc)
					pr_err("%s: could not %sable cam_dvdd regulators: %d\n", __func__, vreg_en ? "en" : "dis", rc);
			}
		}

		/* CAM_IOVDD */
		if (regulator_cam_iovdd != NULL) {
			is_on = regulator_is_enabled(regulator_cam_iovdd);
			if (vreg_en != is_on) {
				rc |= regulator_disable(regulator_cam_iovdd);
				if (rc)
					pr_err("%s: could not %sable cam_iovdd regulators: %d\n", __func__, vreg_en ? "en" : "dis", rc);
			}
		}

		/* CAM_AVDD */
		if (regulator_cam_avdd != NULL) {
			is_on = regulator_is_enabled(regulator_cam_avdd);
			if (vreg_en != is_on) {
				rc |= regulator_disable(regulator_cam_avdd);
				if (rc)
					pr_err("%s: could not %sable cam_avdd regulators: %d\n", __func__, vreg_en ? "en" : "dis", rc);
			}
		}
	}

	return rc;
}

static int32_t msm_camera_7x27a_ext_power_ctrl(int enable)
{
	int rc = 0;
	if (enable)
		rc = msm_camera_vreg_config(1);
	else
		rc = msm_camera_vreg_config(0);
	return rc;
}

struct msm_camera_device_platform_data msm_camera_device_data_csi1[] = {
	{
		.csid_core = 1,
		.ioclk = {
			.vfe_clk_rate = 192000000,
		},
	},
	{
		.csid_core = 1,
		.ioclk = {
			.vfe_clk_rate = 266667000,
		},
	},
};

struct msm_camera_device_platform_data msm_camera_device_data_csi0[] = {
	{
		.csid_core = 0,
		.ioclk = {
			.vfe_clk_rate = 192000000,
		},
	},
	{
		.csid_core = 0,
		.ioclk = {
			.vfe_clk_rate = 266667000,
		},
	},
};

static struct msm_camera_sensor_flash_data flash_t4k28 = {
	.flash_type             = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_t4k28 = {
	.mount_angle	= 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_t4k28,
};

static struct msm_camera_sensor_info msm_camera_sensor_t4k28_data = {
	.sensor_name    = "t4k28",
	.sensor_reset_enable = 1,
	.sensor_reset           = 34, 		// GPIO_34
	.sensor_pwd 			= 42, 		// GPIO_42
	.pdata                  = &msm_camera_device_data_csi1[0],
	.flash_data             = &flash_t4k28,
	.sensor_platform_info   = &sensor_board_info_t4k28,
	.csi_if                 = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static void __init msm7x27a_init_cam(void)
{
	struct msm_camera_sensor_info *s_info;

	if (!(machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
				|| machine_is_msm7627a_qrd1()
				|| machine_is_msm8625_ffa())) {
		sensor_board_info_t4k28.cam_vreg = NULL;
		sensor_board_info_t4k28.num_vreg = 0;
		s_info = &msm_camera_sensor_t4k28_data;
		s_info->sensor_platform_info->ext_power_ctrl =
			msm_camera_7x27a_ext_power_ctrl;
	}
	platform_device_register(&msm_camera_server);
	platform_device_register(&msm7x27a_device_csic0);
	platform_device_register(&msm7x27a_device_csic1);
	platform_device_register(&msm7x27a_device_clkctl);
	platform_device_register(&msm7x27a_device_vfe);
}

static struct i2c_board_info i2c_camera_devices[] = {
	{
		I2C_BOARD_INFO("t4k28", 0x78),
		.platform_data = &msm_camera_sensor_t4k28_data,
	},
};

void __init msm7627a_camera_init(void)
{
	int rc;

	pr_debug("msm7627a_camera_init Entered\n");

	/* CAM_IOVDD */
	regulator_cam_iovdd = regulator_get(NULL, "cam_iovdd");
	if (regulator_cam_iovdd == NULL)
		pr_err("%s: could not get regulators: cam_iovdd\n", __func__);
	else {
		rc = regulator_set_voltage(regulator_cam_iovdd, 1800000, 1800000);
		if (rc)
			pr_err("%s: could not set cam_iovdd voltages: %d\n", __func__, rc);
	}

	/* CAM_AVDD */
	regulator_cam_avdd = regulator_get(NULL, "cam_avdd");
	if (regulator_cam_avdd == NULL)
		pr_err("%s: could not get regulators: cam_avdd\n", __func__);
	else {
		rc = regulator_set_voltage(regulator_cam_avdd, 2800000, 2800000);
		if (rc)
			pr_err("%s: could not set cam_avdd voltages: %d\n", __func__, rc);
	}

	/* CAM_DVDD */
	regulator_cam_dvdd = regulator_get(NULL, "cam_dvdd");
	if (regulator_cam_dvdd == NULL)
		pr_err("%s: could not get regulators: cam_dvdd\n", __func__);
	else {
		rc = regulator_set_voltage(regulator_cam_dvdd, 1800000, 1800000);
		if (rc)
			pr_err("%s: could not set cam_dvdd voltages: %d\n", __func__, rc);
	}

	msm7x27a_init_cam();
	pr_debug("i2c_register_board_info\n");
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID, i2c_camera_devices,
			ARRAY_SIZE(i2c_camera_devices));
}
