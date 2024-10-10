#include <linux/platform_device.h>


#if (defined(CONFIG_LEDS_MSM_PMIC_E0) || defined(CONFIG_LEDS_MSM_PMIC_E0_MODULE))
static struct platform_device gpio_device_leds = {
	.name = "pmic-leds",
	.id = -1,
};

static struct platform_device *pmic_platform_devices[] __initdata = {
    &gpio_device_leds,
};

void __init add_pmic_leds(void)
{
    platform_add_devices(pmic_platform_devices, ARRAY_SIZE(pmic_platform_devices));
}
#else
void __init add_pmic_leds(void)
{
}
#endif /* CONFIG_LEDS_MSM_PMIC_E0 */
