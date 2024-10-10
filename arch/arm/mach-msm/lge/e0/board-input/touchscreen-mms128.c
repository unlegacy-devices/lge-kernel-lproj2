#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <asm/gpio.h>
#include <linux/regulator/consumer.h>

#include CONFIG_LGE_BOARD_HEADER_FILE

#if defined(CONFIG_TOUCHSCREEN_MELFAS_MMS128S)
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
static char is_touch_Initialized = 0;
int ts_set_vreg(unsigned char onoff)
{
	int rc;
	
#if defined(CONFIG_MACH_MSM7X25A_E0EU)
	if (1) {
		regulator_ts = regulator_get(NULL, "rfrx1");
		if (regulator_ts == NULL)
			pr_err("%s: regulator_get(regulator_ts) failed\n",__func__);
			
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

#endif

	return rc;
}

static struct touch_platform_data ts_pdata = {
	.ts_x_min = TS_X_MIN,
	.ts_x_max = TS_X_MAX,
	.ts_y_min = TS_Y_MIN,
	.ts_y_max = TS_Y_MAX,
	.power 	  = ts_set_vreg,
	.irq 	  = TS_GPIO_IRQ,
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

static void __init v3eu_init_i2c_touch(int bus_num)
{
	ts_i2c_device.id = bus_num;

	init_gpio_i2c_pin_touch(
		&ts_i2c_pdata, ts_i2c_pin[0], &ts_i2c_bdinfo[0]);
	i2c_register_board_info(
		bus_num, &ts_i2c_bdinfo[0], 1);
	platform_device_register(&ts_i2c_device);
}

void __init add_melfes_touchscreen(void)
{
	lge_add_gpio_i2c_device(v3eu_init_i2c_touch);
}

#else
static inline void __init add_melfes_touchscreen(void)
{
}
#endif // CONFIG_TOUCHSCREEN_MELFAS_MMS128S
