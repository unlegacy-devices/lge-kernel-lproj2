#include <linux/platform_device.h>
#include <linux/gpio_event.h>
#include <mach/rpc_server_handset.h>
#include <asm/gpio.h>

#include CONFIG_LGE_BOARD_HEADER_FILE

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
		for(i = 0; i < ARRAY_SIZE(keypad_row_gpios); i++)  {
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

void __init add_gpio_keypad(void)
{
  hs_platform_data.ignore_end_key = true;
  platform_add_devices(v3_input_devices, ARRAY_SIZE(v3_input_devices));
}
