#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/delay.h>

#include CONFIG_LGE_BOARD_HEADER_FILE

/* ear sense driver */
static char *ear_state_string[] = {
	"0",
	"1",
	"2",
};

enum {
	EAR_STATE_EJECT = 0,
	EAR_STATE_INJECT = 1,
};

enum {
	EAR_EJECT = 0,
	EAR_INJECT = 1,
};

enum {
	NO_HEADSET = 0,
	HEADSET_WITH_OUT_MIC = 1,
	HEADSET_WITH_MIC = 2
};
static int i_headset_type;

static int u0_gpio_earsense_work_func(int *value)
{
	int state;
	int gpio_value;

	gpio_value = !gpio_get_value(GPIO_EAR_SENSE);
	printk(KERN_INFO "%s: ear sense detected : %s\n", __func__,
		gpio_value ? "injected" : "ejected");

	if (gpio_value == EAR_EJECT) {
		state = EAR_STATE_EJECT;
		i_headset_type = NO_HEADSET;
		*value = 0;
		gpio_set_value(GPIO_MIC_MODE, 0);
	}
	else {
		state = EAR_STATE_INJECT;

		gpio_set_value(GPIO_MIC_MODE, 1);
		msleep(100);
		gpio_value = !gpio_get_value(GPIO_BUTTON_DETECT);
		if (gpio_value) {
			printk(KERN_INFO "headphone was inserted!\n");
			*value = SW_HEADPHONE_INSERT;
			i_headset_type = HEADSET_WITH_OUT_MIC;
		} else {
			printk(KERN_INFO "microphone was inserted!\n");
			*value = SW_MICROPHONE_INSERT;
			i_headset_type = HEADSET_WITH_MIC;
		}
	}
	return state;
}

static char *u0_gpio_earsense_print_name(int state)
{
	if (state)
		return "Headset";
	else
		return "No Device";
}

static char *u0_gpio_earsense_print_state(int state)
{
	if (state == SW_HEADPHONE_INSERT)
		return ear_state_string[2];
	else if (state == SW_MICROPHONE_INSERT)
		return ear_state_string[1];
	else
		return ear_state_string[0];
}

static int u0_gpio_earsense_sysfs_store(const char *buf, size_t size)
{
	int state;

	if (!strncmp(buf, "eject", size - 1))
		state = EAR_STATE_EJECT;
	else if (!strncmp(buf, "inject", size - 1))
		state = EAR_STATE_INJECT;
	else
		return -EINVAL;

	return state;
}

static unsigned u0_earsense_gpios[] = {
	GPIO_EAR_SENSE,
};

/* especially to address gpio key */
static unsigned u0_hook_key_gpios[] = {
	GPIO_BUTTON_DETECT,
};

static int u0_gpio_hook_key_work_func(int *value)
{
	int gpio_value;
/* To Block Spurious Hook Key Events*/
/*Try to read gpio status 20 times with 1 msec delay.*/
	int i_val1,i_val2 = 0;
	int i_retries = 20;

	*value = KEY_MEDIA;
	gpio_value = !gpio_get_value(GPIO_BUTTON_DETECT);

/* To Block Spurious Hook Key Events*/
/*Check the gpio status after 10msec. If the value is different then fake interrupt*/
	i_val1 = gpio_value;
	do{
		i_val2 = !gpio_get_value(GPIO_BUTTON_DETECT);
		mdelay(1); /*Do we need to increase this ?? */
	}while( 0 > i_retries-- );

/* Value different means Fake interrupt. Ignore it*/
	if( i_val1 ^ i_val2)
	{ /* If the button value is not same after 10mSec Hope it's a fake interrupt*/
	  /* No need to send Hook Key event */
		printk("spurious Hook Key Event \n");
		*value = 0;
		return gpio_value;
	}

	printk(KERN_INFO "%s: hook key detected : %s\n", __func__,
		gpio_value ? "pressed" : "released");

	if (  (gpio_get_value(GPIO_EAR_SENSE))
		 ||( NO_HEADSET           == i_headset_type )
		 ||( HEADSET_WITH_OUT_MIC == i_headset_type )
		)
	{
		printk(KERN_INFO "Ignore hook key event\n");
		*value = 0;
	}

/* Headset removed but we already sent one press event.*/
/* Send release event also */
	if (   (gpio_get_value(GPIO_EAR_SENSE)) /*Indicates Headset removed*/
		&& ( (HEADSET_WITH_MIC == i_headset_type) )
		&& ( 0 == gpio_value ) /* Indicates Button Released */
	   )
	{
		/* When headset is 4 pole & it comes to this point means previously we sent a hook key press event & not released yet*/
		/* Anyway headset is removed, so what we are waiting for. Release it Now. Otherwise frameworks behaving strangely */
		printk("EarJack Removed.Sending Hook Key release \n");
		*value = KEY_MEDIA;
	}


	return gpio_value;
}

static struct lge_gpio_switch_platform_data e0_earsense_data = {
	.name = "h2w",
	.gpios = u0_earsense_gpios,
	.num_gpios = ARRAY_SIZE(u0_earsense_gpios),
	.irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.wakeup_flag = 1,
	.work_func = u0_gpio_earsense_work_func,
	.print_name = u0_gpio_earsense_print_name,
	.print_state = u0_gpio_earsense_print_state,
	.sysfs_store = u0_gpio_earsense_sysfs_store,

	/* especially to address gpio key */
	.key_gpios = u0_hook_key_gpios,
	.num_key_gpios = ARRAY_SIZE(u0_hook_key_gpios),
	.key_work_func = u0_gpio_hook_key_work_func,
};

static struct platform_device e0_earsense_device = {
	.name	= "lge-switch-gpio",
	.id		= -1,
	.dev	= {
		.platform_data = &e0_earsense_data,
	},
};

static struct platform_device *e0_earsense_devices[] __initdata = {
	&e0_earsense_device,
};

void __init add_gpio_earsense(void)
{
	int rc;

	rc = gpio_request(GPIO_MIC_MODE, "mic_en");
	if (rc) {
		pr_info("%s: %d gpio request is gailed\n", __func__, GPIO_MIC_MODE);
	} else {
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_MIC_MODE, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc)
			pr_err("%s: %d gpio tlmm config is failed\n", __func__, GPIO_MIC_MODE);
	}

	platform_add_devices(e0_earsense_devices, ARRAY_SIZE(e0_earsense_devices));
}