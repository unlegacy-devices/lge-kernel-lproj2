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
 *
 */

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio_event.h>
#include <linux/leds.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/i2c.h>
#include <linux/input/rmi_platformdata.h>
#include <linux/input/rmi_i2c.h>
#include <linux/delay.h>
#include <linux/atmel_maxtouch.h>
#include <linux/input/ft5x06_ts.h>
#include <linux/leds-msm-tricolor.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <mach/rpc_server_handset.h>
#include <mach/pmic.h>
#include <linux/ktime.h>

#ifdef CONFIG_LEDS_LP5521
#include <linux/leds-lp5521.h>
#endif

#include "../../devices.h"
#include "../../board-msm7627a.h"
#include "../../devices-msm7x2xa.h"
#include CONFIG_LGE_BOARD_HEADER_FILE

#define ATMEL_TS_I2C_NAME "maXTouch"
#define ATMEL_X_OFFSET 13
#define ATMEL_Y_OFFSET 0

/* handset device */
static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500,
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static unsigned int keypad_row_gpios[] = {36, 37, 38};
static unsigned int keypad_col_gpios[] = {33, 32};

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))

static const unsigned short keypad_keymap_v3[] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEDOWN,
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(1, 2)] = KEY_HOMEPAGE,
};

int v3_matrix_info_wrapper(struct gpio_event_input_devs *input_dev,
			   struct gpio_event_info *info, void **data, int func)
{
	int ret;
	int i;
	if (func == GPIO_EVENT_FUNC_RESUME) {
		for (i = 0; i < ARRAY_SIZE(keypad_row_gpios); i++)  {
			gpio_tlmm_config(GPIO_CFG(keypad_row_gpios[i], 0, GPIO_CFG_INPUT,
				 GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		}
	}

	ret = gpio_event_matrix_func(input_dev, info, data, func);
	return ret ;
}

static int v3_gpio_matrix_power(const struct gpio_event_platform_data *pdata, bool on)
{
	return 0;
}

static struct gpio_event_matrix_info v3_keypad_matrix_info = {
	.info.func	= v3_matrix_info_wrapper,
	.keymap		= keypad_keymap_v3,
	.output_gpios	= keypad_col_gpios,
	.input_gpios	= keypad_row_gpios,
	.noutputs	= ARRAY_SIZE(keypad_col_gpios),
	.ninputs	= ARRAY_SIZE(keypad_row_gpios),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DRIVE_INACTIVE
};

static struct gpio_event_info *v3_keypad_info[] = {
	&v3_keypad_matrix_info.info
};

static struct gpio_event_platform_data v3_keypad_data = {
	.name		= "vee3_keypad",
	.info		= v3_keypad_info,
	.info_count	= ARRAY_SIZE(v3_keypad_info),
	.power          = v3_gpio_matrix_power,
};

struct platform_device keypad_device_v3 = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &v3_keypad_data,
	},
};

/* input platform device */
static struct platform_device *v3_input_devices[] __initdata = {
	&hs_pdev,
	&keypad_device_v3,
};

#if defined(CONFIG_SENSORS_BMM050) || defined(CONFIG_SENSORS_BMA250)
static struct gpio_i2c_pin accel_i2c_pin[] = {
	[0] = {
		.sda_pin	= SENSOR_GPIO_I2C_SDA,
		.scl_pin	= SENSOR_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ACCEL_GPIO_INT,
	},
};

static struct gpio_i2c_pin ecom_i2c_pin[] = {
	[0] = {
		.sda_pin	= SENSOR_GPIO_I2C_SDA,
		.scl_pin	= SENSOR_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ECOM_GPIO_INT,
	},
};

static struct i2c_gpio_platform_data sensor_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device sensor_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &sensor_i2c_pdata,
};

static struct i2c_board_info sensor_i2c_bdinfo[] = {
	[0] = {
#ifdef CONFIG_SENSORS_BMA2X2
		I2C_BOARD_INFO("bma2x2", ACCEL_I2C_ADDRESS),
		.type = "bma2x2",
#else
		I2C_BOARD_INFO("bma250", ACCEL_I2C_ADDRESS),
		.type = "bma250",
#endif
	},
	[1] = {
		I2C_BOARD_INFO("bmm050", ECOM_I2C_ADDRESS),
		.type = "bmm050",
	},
};

static void __init v3_init_i2c_sensor(int bus_num)
{
	sensor_i2c_device.id = bus_num;
	lge_init_gpio_i2c_pin(&sensor_i2c_pdata, accel_i2c_pin[0], &sensor_i2c_bdinfo[0]);
	lge_init_gpio_i2c_pin(&sensor_i2c_pdata, ecom_i2c_pin[0], &sensor_i2c_bdinfo[1]);
	i2c_register_board_info(bus_num, sensor_i2c_bdinfo, ARRAY_SIZE(sensor_i2c_bdinfo));
	platform_device_register(&sensor_i2c_device);
}
#endif

#ifdef CONFIG_SENSOR_APDS9190
extern int rt9396_ldo_enable(struct device *dev, unsigned num, unsigned enable);
#endif

/* proximity */
static int prox_power_set(unsigned char onoff)
{
#ifdef CONFIG_SENSOR_APDS9190
	if (onoff == 1)
		rt9396_ldo_enable(NULL, 1, 1);
	else
		rt9396_ldo_enable(NULL, 1, 0);
	pr_debug("%s: Proximity probe enter when power on\n", __func__);
#endif
	return 0;
}

#ifdef CONFIG_SENSOR_APDS9190
static struct proximity_platform_data proxi_pdata = {
	.irq_num	= PROXI_GPIO_DOUT,
	.power		= prox_power_set,
	.methods		= 1,
	.operation_mode		= 0,
};

static struct i2c_board_info prox_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("proximity_apds9190", PROXI_I2C_ADDRESS),
		.type = "proximity_apds9190",
		.platform_data = &proxi_pdata,
	},
};
#else
static struct proximity_platform_data proxi_pdata = {
	.irq_num = PROXI_GPIO_DOUT,
	.power = prox_power_set,
	.methods = 0,
	.operation_mode = 0,
	.debounce = 0,
	.cycle = 2,
};

static struct i2c_board_info prox_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("proximity_gp2ap", PROXI_I2C_ADDRESS),
		.type = "proximity_gp2ap",
		.platform_data = &proxi_pdata,
	},
};
#endif

static struct gpio_i2c_pin proxi_i2c_pin[] = {
	[0] = {
		.sda_pin	= PROXI_GPIO_I2C_SDA,
		.scl_pin	= PROXI_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= PROXI_GPIO_DOUT,
	},
};

static struct i2c_gpio_platform_data proxi_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device proxi_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &proxi_i2c_pdata,
};

static void __init v3_init_i2c_prox(int bus_num)
{
	proxi_i2c_device.id = bus_num;
	lge_init_gpio_i2c_pin(&proxi_i2c_pdata, proxi_i2c_pin[0], &prox_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &prox_i2c_bdinfo[0], 1);
	platform_device_register(&proxi_i2c_device);
}

#ifdef CONFIG_LEDS_LP5521
static struct lp5521_led_config lp5521_led_config[] = {
	{
		.name = "R",
		.chan_nr	= 0,
		.led_current	= 110,
		.max_current	= 255,
	},
	{
		.name = "G",
		.chan_nr	= 1,
		.led_current	= 110,
		.max_current	= 255,
	},
	{
		.name = "B",
		.chan_nr	= 2,
		.led_current	= 110,
		.max_current	= 255,
	},
};

static u8 mode1_red[] = {0xE0, 0x0C, 0x40, 0x00, 0x0C, 0x2F, 0x06, 0x28, 0x05, 0x2D, 0x06, 0x2A, 0x06, 0x25, 0x03, 0xDC, 0x02, 0xFA, 0x48, 0x00, 0x03, 0x54, 0x44, 0x01, 0x23, 0x06, 0x31, 0x84, 0x06, 0xA8, 0x0C, 0xAF};
static u8 mode1_green[] = {0xE0, 0x80, 0x40, 0x00, 0x52, 0x00, 0x0B, 0x15, 0x05, 0x2D, 0x03, 0x48, 0x03, 0x4B, 0x09, 0x1B, 0x02, 0x63, 0x19, 0x89, 0x03, 0xCA, 0x04, 0xC1, 0x05, 0xB2, 0x06, 0xA6, 0x12, 0x8D, 0x52, 0x00};
static u8 mode1_blue[] = {0xE0, 0x80, 0x40, 0x00, 0x12, 0xFE, 0x40, 0xC0, 0x0A, 0x18, 0x06, 0xA6, 0x06, 0xAA, 0x03, 0xCF, 0x04, 0xB6, 0x52, 0x00};

static u8 mode2_red[] = {0x40, 0xff, 0x4d, 0x00, 0x0a, 0xff, 0x0a, 0xfe, 0xc0, 0x00};
static u8 mode2_green[] = {0x40, 0xff, 0x4d, 0x00, 0x0a, 0xff, 0x0a, 0xfe, 0xc0, 0x00};
static u8 mode2_blue[] = {0x40, 0xff, 0x4d, 0x00, 0x0a, 0xff, 0x0a, 0xfe, 0xc0, 0x00};

static u8 mode3_red[] = {0x40, 0x1a, 0x42, 0x18, 0x12, 0x65, 0x12, 0x65, 0x66, 0x00, 0x12, 0xe5, 0x12, 0xe5, 0x42, 0x98};

static u8 mode4_green[] = {0x40, 0xff};

static u8 mode5_red[] = {0x40, 0x19, 0x27, 0x19, 0xe0, 0x04, 0x0c, 0x65, 0xe0, 0x04, 0x0c, 0x65, 0xe0, 0x04, 0x0c, 0xe5, 0xe0, 0x04, 0x0c, 0xe5, 0xe0, 0x04, 0x29, 0x98, 0xe0, 0x04, 0x5a, 0x00};
static u8 mode5_green[] = {0x40, 0x0c, 0x43, 0x0b, 0xe0, 0x80, 0x19, 0x30, 0xe0, 0x80, 0x19, 0x30, 0xe0, 0x80, 0x19, 0xb0, 0xe0, 0x80, 0x19, 0xb0, 0xe0, 0x80, 0x43, 0x8b, 0xe0, 0x80, 0x5a, 0x00};

static u8 mode6_red[] = {0xE0, 0x0C, 0x40, 0x00, 0x0C, 0x2F, 0x06, 0x28, 0x05, 0x2D, 0x06, 0x2A, 0x06, 0x25, 0x03, 0xDC, 0x02, 0xFA, 0x48, 0x00, 0x03, 0x54, 0x44, 0x01, 0x23, 0x06, 0x31, 0x84, 0x06, 0xA8, 0x0C, 0xAF};
static u8 mode6_green[] = {0xE0, 0x80, 0x40, 0x00, 0x52, 0x00, 0x0B, 0x15, 0x05, 0x2D, 0x03, 0x48, 0x03, 0x4B, 0x09, 0x1B, 0x02, 0x63, 0x19, 0x89, 0x03, 0xCA, 0x04, 0xC1, 0x05, 0xB2, 0x06, 0xA6, 0x12, 0x8D, 0x52, 0x00};
static u8 mode6_blue[] = {0xE0, 0x80, 0x40, 0x00, 0x12, 0xFE, 0x40, 0xC0, 0x0A, 0x18, 0x06, 0xA6, 0x06, 0xAA, 0x03, 0xCF, 0x04, 0xB6, 0x52, 0x00,};

static u8 mode7_red[] = {};
static u8 mode7_green[] = {0x40, 0x00, 0x10, 0xfe, 0x40, 0xff, 0x02, 0xd4, 0x02, 0xd4, 0x02, 0xd4, 0x48, 0x00, 0x40, 0xff, 0x02, 0xd4, 0x02, 0xd4, 0x02, 0xd4, 0x25, 0xfe};
static u8 mode7_blue[] = {};

static u8 mode8_red[] = {0x40, 0x00, 0x10, 0xFE, 0x40, 0xE6, 0xE2, 0x00, 0x03, 0xF2, 0xE2, 0x00, 0x03, 0xF2, 0xE2, 0x00, 0x48, 0x00, 0x40, 0xE6, 0xE2, 0x00, 0x03, 0xF2, 0xE2, 0x00, 0x03, 0xF2, 0xE2, 0x00, 0x25, 0xFE,};
static u8 mode8_green[] = {0x40, 0x00, 0x10, 0xFE, 0x40, 0x66, 0x4F, 0x00, 0x0B, 0xA8, 0xE0, 0x80, 0x0B, 0xA8, 0xE0, 0x80, 0x40, 0x66, 0x4F, 0x00, 0x09, 0xB2, 0xE0, 0x80, 0x09, 0xB2, 0xE0, 0x80, 0x1A, 0xFE,};
static u8 mode8_blue[] = {0x40, 0x00, 0x10, 0xFE, 0x40, 0x73, 0x4F, 0x00, 0x08, 0xBC, 0xE0, 0x80, 0x0F, 0x9E, 0xE0, 0x80, 0x40, 0x73, 0x4F, 0x00, 0x05, 0xD5, 0xE0, 0x80, 0x10, 0x9C, 0xE0, 0x80, 0x1A, 0xFE,};

static struct lp5521_led_pattern board_led_patterns[] = {
	{
		.r = mode1_red,
		.g = mode1_green,
		.b = mode1_blue,
		.size_r = ARRAY_SIZE(mode1_red),
		.size_g = ARRAY_SIZE(mode1_green),
		.size_b = ARRAY_SIZE(mode1_blue),
	},
	{
		.r = mode2_red,
		.g = mode2_green,
		.b = mode2_blue,
		.size_r = ARRAY_SIZE(mode2_red),
		.size_g = ARRAY_SIZE(mode2_green),
		.size_b = ARRAY_SIZE(mode2_blue),
		},
	{
		.r = mode3_red,
		.size_r = ARRAY_SIZE(mode3_red),
	},
	{
		.g = mode4_green,
		.size_g = ARRAY_SIZE(mode4_green),
	},
	{
		.r = mode5_red,
		.g = mode5_green,
		.size_r = ARRAY_SIZE(mode5_red),
		.size_g = ARRAY_SIZE(mode5_green),
	},
	{
		.r = mode6_red,
		.g = mode6_green,
		.b = mode6_blue,
		.size_r = ARRAY_SIZE(mode6_red),
		.size_g = ARRAY_SIZE(mode6_green),
		.size_b = ARRAY_SIZE(mode6_blue),
	},
	{
		.r = mode7_red,
		.g = mode7_green,
		.b = mode7_blue,
		.size_r = ARRAY_SIZE(mode7_red),
		.size_g = ARRAY_SIZE(mode7_green),
		.size_b = ARRAY_SIZE(mode7_blue),
	},
	{
		.r = mode8_red,
		.g = mode8_green,
		.b = mode8_blue,
		.size_r = ARRAY_SIZE(mode8_red),
		.size_g = ARRAY_SIZE(mode8_green),
		.size_b = ARRAY_SIZE(mode8_blue),
	},
};

#define LP5521_ENABLE PM8921_GPIO_PM_TO_SYS(21)

static struct gpio_i2c_pin rgb_i2c_pin[] = {
	[0] = {
		.sda_pin	= RGB_GPIO_I2C_SDA,
		.scl_pin	= RGB_GPIO_I2C_SCL,
		.reset_pin	= 0,
	},
};

static int lp5521_setup(void)
{
	int rc = 0;

	pr_debug("lp5521_enable\n");
	rc = gpio_request(RGB_GPIO_RGB_EN, "lp5521_led");

	if (rc) {
		pr_debug("lp5521_request failed\n");
		return rc;
	}

	rc = gpio_tlmm_config(GPIO_CFG(RGB_GPIO_RGB_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	if (rc) {
		pr_debug("lp5521_config failed\n");
		return rc;
	}
	return rc;
}

static void lp5521_enable(bool state)
{
	if (state) {
		gpio_set_value(RGB_GPIO_RGB_EN, 1);
		pr_debug("lp5521_enable_set\n");
	} else {
		gpio_set_value(RGB_GPIO_RGB_EN, 0);
		pr_debug("lp5521_disable_set\n");
	}

	return;
}

#define LP5521_CONFIGS	(LP5521_PWM_HF | LP5521_PWRSAVE_EN | LP5521_CP_MODE_AUTO | LP5521_CLK_SRC_EXT)

static struct lp5521_platform_data lp5521_pdata = {
	.led_config = lp5521_led_config,
	.num_channels = ARRAY_SIZE(lp5521_led_config),
	.clock_mode = LP5521_CLOCK_EXT,
	.update_config = LP5521_CONFIGS,
	.patterns = board_led_patterns,
	.num_patterns = ARRAY_SIZE(board_led_patterns),
	.setup_resources = lp5521_setup,
	.enable = lp5521_enable
};

static struct i2c_board_info lp5521_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("lp5521", 0x32),
		.platform_data = &lp5521_pdata,
	},
};
static struct i2c_gpio_platform_data rgb_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 1,
};

static struct platform_device rgb_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &rgb_i2c_pdata,
};

static void __init lp5521_init_i2c_rgb(int bus_num)
{
	int rc = 0;

	rgb_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin_pullup(&rgb_i2c_pdata, rgb_i2c_pin[0], &lp5521_board_info[0]);

	i2c_register_board_info(bus_num, lp5521_board_info, ARRAY_SIZE(lp5521_board_info));

	platform_device_register(&rgb_i2c_device);

	rc = gpio_tlmm_config(GPIO_CFG(RGB_GPIO_RGB_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	if (rc)
		pr_err("%s: Error requesting GPIO gpio_tlmm_config, ret %d\n", __func__, rc);
	else
		pr_err("%s: success gpio_tlmm_config, ret %d\n", __func__, rc);
}
#endif /*LP5521*/

static struct gpio_i2c_pin ts_i2c_pin[] = {
	[0] = {
		.sda_pin	= TS_GPIO_I2C_SDA,
		.scl_pin	= TS_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= TS_GPIO_IRQ,
	},
};

static struct i2c_gpio_platform_data ts_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 1,
};

static struct platform_device ts_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &ts_i2c_pdata,
};

static struct regulator *regulator_ts;
static char is_touch_Initialized;
int ts_set_vreg(unsigned char onoff)
{
	int rc;

	if (1) {
		regulator_ts = regulator_get(NULL, "rfrx1");
		if (regulator_ts == NULL)
			pr_err("%s: regulator_get(regulator_ts) failed\n", __func__);

		rc = regulator_set_voltage(regulator_ts, 3000000, 3000000);
		if (rc < 0)
			pr_err("%s: regulator_set_voltage(regulator_ts) failed\n", __func__);

		is_touch_Initialized = 1;
	}

	if (onoff) {
		rc = regulator_enable(regulator_ts);
		if (rc < 0)
			pr_err("%s: regulator_enable(regulator_ts) failed\n", __func__);

	} else {
		rc = regulator_disable(regulator_ts);
		if (rc < 0)
			pr_err("%s: regulator_disble(regulator_ts) failed\n", __func__);
	}

	return rc;
}

static struct touch_platform_data ts_pdata = {
	.ts_x_min = TS_X_MIN,
	.ts_x_max = TS_X_MAX,
	.ts_y_min = TS_Y_MIN,
	.ts_y_max = TS_Y_MAX,
	.power    = ts_set_vreg,
	.irq      = TS_GPIO_IRQ,
	.scl      = TS_GPIO_I2C_SCL,
	.sda      = TS_GPIO_I2C_SDA,
};

static struct i2c_board_info ts_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("touch_mcs8000", TS_I2C_SLAVE_ADDR),
		.type = "touch_mcs8000",
		.platform_data = &ts_pdata,
	},
};

/* this routine should be checked for nessarry */
static int init_gpio_i2c_pin_touch(
	struct i2c_gpio_platform_data *i2c_adap_pdata,
	struct gpio_i2c_pin gpio_i2c_pin,
	struct i2c_board_info *i2c_board_info_data)
{
	i2c_adap_pdata->sda_pin = gpio_i2c_pin.sda_pin;
	i2c_adap_pdata->scl_pin = gpio_i2c_pin.scl_pin;

	gpio_request(TS_GPIO_I2C_SDA, "Melfas_I2C_SDA");
	gpio_request(TS_GPIO_I2C_SCL, "Melfas_I2C_SCL");
	gpio_request(TS_GPIO_IRQ, "Melfas_I2C_INT");

	gpio_tlmm_config(
		GPIO_CFG(gpio_i2c_pin.sda_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(
		GPIO_CFG(gpio_i2c_pin.scl_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(gpio_i2c_pin.sda_pin, 1);
	gpio_set_value(gpio_i2c_pin.scl_pin, 1);

	if (gpio_i2c_pin.reset_pin) {
		gpio_tlmm_config(
			GPIO_CFG(gpio_i2c_pin.reset_pin, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(gpio_i2c_pin.reset_pin, 1);
	}

	if (gpio_i2c_pin.irq_pin) {
		gpio_tlmm_config(
			GPIO_CFG(gpio_i2c_pin.irq_pin, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		i2c_board_info_data->irq =
			MSM_GPIO_TO_INT(gpio_i2c_pin.irq_pin);
	}

	gpio_free(TS_GPIO_I2C_SDA);
	gpio_free(TS_GPIO_I2C_SCL);
	gpio_free(TS_GPIO_IRQ);

	return 0;
}

static void __init v3_init_i2c_touch(int bus_num)
{
	ts_i2c_device.id = bus_num;

	init_gpio_i2c_pin_touch(
		&ts_i2c_pdata, ts_i2c_pin[0], &ts_i2c_bdinfo[0]);
	i2c_register_board_info(
		bus_num, &ts_i2c_bdinfo[0], 1);
	platform_device_register(&ts_i2c_device);
}

void __init msm7627a_add_io_devices(void)
{
	hs_platform_data.ignore_end_key = true;
	platform_add_devices(v3_input_devices, ARRAY_SIZE(v3_input_devices));

	lge_add_gpio_i2c_device(v3_init_i2c_touch);

#if defined(CONFIG_SENSORS_BMM050) || defined(CONFIG_SENSORS_BMA250)
	lge_add_gpio_i2c_device(v3_init_i2c_sensor);
#endif
	lge_add_gpio_i2c_device(v3_init_i2c_prox);

#ifdef CONFIG_LEDS_LP5521
	lge_add_gpio_i2c_device(lp5521_init_i2c_rgb);
#endif
	msm_init_pmic_vibrator();
}

void __init qrd7627a_add_io_devices(void)
{
}
