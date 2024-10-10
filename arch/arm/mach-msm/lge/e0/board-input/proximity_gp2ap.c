#include <linux/platform_device.h>
#include <linux/i2c.h>

#include CONFIG_LGE_BOARD_HEADER_FILE


#if (defined(CONFIG_SENSOR_GP2AP) || defined(CONFIG_SENSOR_GP2AP_MODULE)) && defined(CONFIG_BACKLIGHT_BU61800)
extern int bu61800_ldo_enable(struct device *dev, unsigned num, unsigned enable);

static int prox_power_set(unsigned char onoff)
{
   if(onoff == 1) {
        bu61800_ldo_enable(NULL,1,1);
    } else {
        bu61800_ldo_enable(NULL,1,0);
    }

    printk("[Proximity] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");

    return 0;
}

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

static void __init e0_init_i2c_prox(int bus_num)
{
	proxi_i2c_device.id = bus_num;
	lge_init_gpio_i2c_pin(&proxi_i2c_pdata, proxi_i2c_pin[0], &prox_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &prox_i2c_bdinfo[0], 1);
	platform_device_register(&proxi_i2c_device);
}

void __init add_gp2ap_proximity(void)
{
	lge_add_gpio_i2c_device(e0_init_i2c_prox);
}
#else
static inline void __init add_gp2ap_proximity(void)
{
}
#endif // CONFIG_SENSOR_GP2AP && CONFIG_BACKLIGHT_BU61800
