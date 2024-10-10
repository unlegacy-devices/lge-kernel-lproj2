#include "./board-input/board-input.h"

void __init msm7627a_add_io_devices(void)
{
        add_bosch_sensors();
        add_gp2ap_proximity();
        add_melfes_touchscreen();
        add_gpio_keypad();
        add_gpio_earsense();
        add_pmic_leds();

        msm_init_pmic_vibrator();
}
