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

#include <linux/platform_data/mms_ts.h>

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

static unsigned int keypad_row_gpios[] = {36, 37};
static unsigned int keypad_col_gpios[] = {33};

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))

static const unsigned short keypad_keymap_v1[] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN,
};

int v1_matrix_info_wrapper(struct gpio_event_input_devs *input_dev,
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

static int v1_gpio_matrix_power(const struct gpio_event_platform_data *pdata, bool on)
{
	return 0;
}

static struct gpio_event_matrix_info v1_keypad_matrix_info = {
	.info.func	= v1_matrix_info_wrapper,
	.keymap		= keypad_keymap_v1,
	.output_gpios	= keypad_col_gpios,
	.input_gpios	= keypad_row_gpios,
	.noutputs	= ARRAY_SIZE(keypad_col_gpios),
	.ninputs	= ARRAY_SIZE(keypad_row_gpios),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DRIVE_INACTIVE
};

static struct gpio_event_info *v1_keypad_info[] = {
	&v1_keypad_matrix_info.info
};

static struct gpio_event_platform_data v1_keypad_data = {
	.name		= "v1_keypad",
	.info		= v1_keypad_info,
	.info_count	= ARRAY_SIZE(v1_keypad_info),
	.power          = v1_gpio_matrix_power,
};

struct platform_device keypad_device_v1 = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &v1_keypad_data,
	},
};

/* input platform device */
static struct platform_device *v1_input_devices[] __initdata = {
	&hs_pdev,
	&keypad_device_v1,
};

static struct gpio_i2c_pin accel_i2c_pin[] = {
	[0] = {
		.sda_pin	= SENSOR_GPIO_I2C_SDA,
		.scl_pin	= SENSOR_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ACCEL_GPIO_INT,
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
};

static void __init v1_init_i2c_sensor(int bus_num)
{
	sensor_i2c_device.id = bus_num;
	lge_init_gpio_i2c_pin(&sensor_i2c_pdata, accel_i2c_pin[0], &sensor_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, sensor_i2c_bdinfo, ARRAY_SIZE(sensor_i2c_bdinfo));
	platform_device_register(&sensor_i2c_device);
}

static struct regulator *regulator_prox;

static int prox_power_set(struct device *dev, unsigned char onoff)
{
	int rc = -ENODEV;

	pr_info("%s: initialize regulator for prox. sensor\n", __func__);

	regulator_prox = regulator_get (dev, "prox");
	if (regulator_prox == NULL) {
		pr_err("%s: could not get regulators: prox\n", __func__);
		goto err_prox_power_set;
	} else {
		rc = regulator_set_voltage (regulator_prox, 3000000, 3000000);
		if (rc) {
			pr_err("%s: could not set prox voltages: %d\n", __func__, rc);
			goto err_prox_power_set;
		}
	}

	pr_info("%s: onoff = %d\n", __func__, onoff);

	if (regulator_prox != NULL) {
		int is_on = regulator_is_enabled(regulator_prox);

		if (onoff == is_on)
			return 0;

		if (onoff) {
			rc = regulator_enable (regulator_prox);
			if (rc < 0)
			{
				pr_err("%s: regulator_enable(regulator_prox) failed\n", __func__);
				goto err_prox_power_set;
			}
		} else {
			rc = regulator_disable (regulator_prox);
			if (rc < 0) {
				pr_err("%s: regulator_disble(regulator_prox) failed\n", __func__);
				goto err_prox_power_set;
			}
		}
	}
err_prox_power_set:
	if (regulator_prox)
		regulator_put(regulator_prox);
	regulator_prox = NULL;

	return rc;
}

static struct proximity_platform_data proxi_pdata = {
	.irq_num	= PROXI_GPIO_DOUT,
	.power		= prox_power_set,
	.methods		= 1,
	.operation_mode		= 0,
};

static struct i2c_board_info prox_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("rpr400", PROXI_I2C_ADDRESS),
		.type = "rpr400",
		.platform_data = &proxi_pdata,
	},
};

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

static void __init v1_init_i2c_prox(int bus_num)
{
	proxi_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(
		&proxi_i2c_pdata, proxi_i2c_pin[0], &prox_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &prox_i2c_bdinfo[0], 1);
	platform_device_register(&proxi_i2c_device);
}

int ts_set_power(struct device *dev, int on)
{
	static struct regulator *power = NULL;
	int rc = -ENODEV;

	if (!power)
		power = regulator_get(dev, "rfrx1");

	if (!power) {
		dev_err(dev, "regulator_get failed\n");
		goto err_ts_set_power;
	} else {
		rc = regulator_set_voltage(power, 3000000, 3000000);
		if (rc) {
			dev_err(dev, "regulator_set_voltage failed\n");
			goto err_ts_set_power;
		}
	}

	if (on) {
		rc = regulator_enable(power);
		if (rc) {
			dev_err(dev, "regulator_enable failed\n");
			goto err_ts_set_power;
		}
	} else {
		rc = regulator_disable(power);
		if (rc) {
			dev_err(dev, "regulator_enable failed\n");
			goto err_ts_set_power;
		}
	}

	return rc;

err_ts_set_power:
	if (power)
		regulator_put(power);
	power = NULL;
	return rc;
}

static struct mms_ts_platform_data ts_pdata = {
	.max_x = 240,
	.max_y = 320,
	.power = ts_set_power,
};

static struct gpio_i2c_pin ts_i2c_pin[] = {
	[0] = {
		.sda_pin	= 10,
		.scl_pin	= 9,
		.reset_pin	= 0,
		.irq_pin	= 39,
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

static struct i2c_board_info ts_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("mms_ts", 0x48),
		.type = "mms_ts",
		.platform_data = &ts_pdata,
	},
};

static int init_gpio_i2c_pin_touch(
	struct i2c_gpio_platform_data *i2c_adap_pdata,
	struct gpio_i2c_pin gpio_i2c_pin,
	struct i2c_board_info *i2c_board_info_data)
{
	i2c_adap_pdata->scl_pin = gpio_i2c_pin.scl_pin;

	if (gpio_i2c_pin.sda_pin) {
		i2c_adap_pdata->sda_pin = gpio_i2c_pin.sda_pin;
		gpio_request(gpio_i2c_pin.sda_pin, "Melfas_I2C_SDA");
		gpio_tlmm_config(
		GPIO_CFG(gpio_i2c_pin.sda_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(gpio_i2c_pin.sda_pin, 1);
		gpio_free(gpio_i2c_pin.sda_pin);
	}

	if (gpio_i2c_pin.scl_pin) {
		i2c_adap_pdata->scl_pin = gpio_i2c_pin.scl_pin;
		gpio_request(gpio_i2c_pin.scl_pin, "Melfas_I2C_SCL");
		gpio_tlmm_config(
		GPIO_CFG(gpio_i2c_pin.scl_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(gpio_i2c_pin.scl_pin, 1);
		gpio_free(gpio_i2c_pin.scl_pin);
	}

	if (gpio_i2c_pin.reset_pin) {
		gpio_request(gpio_i2c_pin.reset_pin, "Melfas_I2C_Reset");
		gpio_tlmm_config(
			GPIO_CFG(gpio_i2c_pin.reset_pin, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(gpio_i2c_pin.reset_pin, 1);
		gpio_free(gpio_i2c_pin.reset_pin);
	}

	if (gpio_i2c_pin.irq_pin) {
		i2c_board_info_data->irq = MSM_GPIO_TO_INT(gpio_i2c_pin.irq_pin);
		gpio_request(gpio_i2c_pin.irq_pin, "Melfas_I2C_INT");
		gpio_tlmm_config(
			GPIO_CFG(gpio_i2c_pin.irq_pin, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_free(gpio_i2c_pin.irq_pin);
	}

	return 0;
}

static void __init v1_init_i2c_touch(int bus_num)
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
	platform_add_devices(v1_input_devices, ARRAY_SIZE(v1_input_devices));

	lge_add_gpio_i2c_device(v1_init_i2c_touch);

	lge_add_gpio_i2c_device(v1_init_i2c_sensor);

	lge_add_gpio_i2c_device(v1_init_i2c_prox);

	msm_init_pmic_vibrator();
}

void __init qrd7627a_add_io_devices(void)
{
}
