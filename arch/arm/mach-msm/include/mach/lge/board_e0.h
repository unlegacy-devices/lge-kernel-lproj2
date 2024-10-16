#ifndef __ASM_ARCH_MSM_BOARD_LGE_H
#define __ASM_ARCH_MSM_BOARD_LGE_H

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include "lge_proc_comm.h"
#ifdef CONFIG_LGE_BOOT_MODE
#include "lge_boot_mode.h"
#endif
#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

#define  LGE_CHG_DONE_NOTIFICATION

#ifdef CONFIG_ANDROID_RAM_CONSOLE
/* allocate 128K * 2 instead of ram_console's original size 128K
 * this is for storing kernel panic log which is used by lk loader
 * 2010-03-03, cleaneye.kim@lge.com
 */
#define MSM7X27_EBI1_CS0_BASE	PHYS_OFFSET
#define LGE_RAM_CONSOLE_SIZE    (124 * SZ_1K * 2)
#endif

#ifdef CONFIG_LGE_HANDLE_PANIC
#define LGE_CRASH_LOG_SIZE              (4 * SZ_1K)
#endif
//LGE_CHANGE_S FTM boot mode
enum lge_fboot_mode_type {
	first_boot,
	second_boot
};

#ifdef CONFIG_LGE_HW_REVISION
/* board revision information */
typedef  enum {
	EVB = 0,
	LGE_REV_A,
	LGE_REV_B,
	LGE_REV_C,
	LGE_REV_D,
	LGE_REV_E,
	LGE_REV_F,
	LGE_REV_G,
	LGE_REV_10,
	LGE_REV_11,
	LGE_REV_12,
	LGE_REV_TOT_NUM,
}lge_pcb_rev;

#define REV_EVB	0
#define REV_A	1
#define REV_B	2
#define REV_C	3
#define REV_D	4
#define REV_E	5
#define REV_F	6
#define REV_G	7
#define REV_10	8
#define REV_11	9
#define REV_12	10
#endif

/* define gpio pin number of i2c-gpio */
struct gpio_i2c_pin {
	unsigned int sda_pin;
	unsigned int scl_pin;
	unsigned int reset_pin;
	unsigned int irq_pin;
};

#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
#define GPIO_SD_DETECT_N 40
#endif

#ifdef CONFIG_MACH_MSM7X25A_E0EU
#define GPIO_SD_DETECT_N 40
/* ear sense gpio */
#define GPIO_EAR_SENSE			41
#define GPIO_BUTTON_DETECT		28
#define GPIO_MIC_MODE			127

#define SENSOR_GPIO_I2C_SCL 		49
#define SENSOR_GPIO_I2C_SDA 		48

/* accelerometer */
#define ACCEL_GPIO_INT			94
#define ACCEL_GPIO_I2C_SCL		SENSOR_GPIO_I2C_SCL
#define ACCEL_GPIO_I2C_SDA		SENSOR_GPIO_I2C_SDA
#define ACCEL_I2C_ADDRESS		0x18

#define ECOM_GPIO_I2C_SCL		SENSOR_GPIO_I2C_SCL
#define ECOM_GPIO_I2C_SDA		SENSOR_GPIO_I2C_SDA
#define ECOM_GPIO_INT			35
#define ECOM_I2C_ADDRESS		0x10

/* proximity sensor */
#define PROXI_GPIO_I2C_SCL		16
#define PROXI_GPIO_I2C_SDA		30
#define PROXI_GPIO_DOUT			17
#define PROXI_I2C_ADDRESS		0x44
#define PROXI_LDO_NO_VCC		1
#endif

/* touch screen platform data */
#ifdef CONFIG_TOUCHSCREEN_MELFAS_MMS128S
#define MELFAS_TS_NAME "melfas-ts"

#define TS_X_MIN		0
#define TS_X_MAX		240
#define TS_Y_MIN		0
#define TS_Y_MAX		320
#define TS_GPIO_I2C_SDA		10
#define TS_GPIO_I2C_SCL		9
#define TS_GPIO_IRQ		39
#define TS_I2C_SLAVE_ADDR	0x48
#define TS_TOUCH_LDO_EN		14
#define TS_TOUCH_ID		121

struct touch_platform_data {
	int ts_x_min;
	int ts_x_max;
	int ts_y_min;
	int ts_y_max;
	int (*power)(unsigned char onoff);
	int irq;
	int scl;
	int sda;
};
#endif

/* acceleration platform data */
struct acceleration_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
};

/* ecompass platform data */
struct ecom_platform_data {
	int pin_int;
	int pin_rst;
	int (*power)(unsigned char onoff);
	char accelerator_name[20];
	int fdata_sign_x;
        int fdata_sign_y;
        int fdata_sign_z;
	int fdata_order0;
	int fdata_order1;
	int fdata_order2;
	int sensitivity1g;
	s16 *h_layout;
	s16 *a_layout;
	int drdy;
};

/* proximity platform data */
struct proximity_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
	int methods;
	int operation_mode;
	int debounce;
	u8 cycle;
};

/* backlight platform data*/
struct lge_backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;	/* initial mode */
	int max_current;	/* led max current(0-7F) */
	int initialized;	/* flag which initialize on system boot */
	int version;		/* Chip version number */
};

/* android vibrator platform data */
struct android_vibrator_platform_data {
	int enable_status;
	int (*power_set)(int enable);		/* LDO Power Set Function */
	int (*pwm_set)(int enable, int gain);	/* PWM Set Function */
	int (*ic_enable_set)(int enable);	/* Motor IC Set Function */
	int (*gpio_request)(void);		/* gpio request */
	int amp_value;				/* PWM tuning value */
};

struct gpio_h2w_platform_data {
	int gpio_detect;
	int gpio_button_detect;
	int gpio_mic_mode;
};

/* gpio switch platform data */
struct lge_gpio_switch_platform_data {
	const char *name;
	unsigned *gpios;
	size_t num_gpios;
	unsigned long irqflags;
	unsigned int wakeup_flag;
	int (*work_func)(int *value);
	char *(*print_name)(int state);
	char *(*print_state)(int state);
	int (*sysfs_store)(const char *buf, size_t size);
	int (*additional_init)(void);

	/* especially to address gpio key */
	unsigned *key_gpios;
	size_t num_key_gpios;
	int (*key_work_func)(int *value);
};

/* LED flash platform data */
struct led_flash_platform_data {
	int gpio_flen;
	int gpio_en_set;
	int gpio_inh;
};

/* pp2106 qwerty platform data */
struct pp2106_platform_data {
	unsigned int reset_pin;
	unsigned int irq_pin;
	unsigned int sda_pin;
	unsigned int scl_pin;
	unsigned int keypad_row;
	unsigned int keypad_col;
	unsigned char *keycode;
	int (*power)(unsigned char onoff);
};

/* LCD panel */
enum {
	PANEL_ID_AUTO = 0,
	PANEL_ID_LGDISPLAY_1 = 1,
	PANEL_ID_TOVIS = 2,
	PANEL_ID_LGDISPLAY = 3,
	PANEL_ID_ILITEK =4,
};
/* LCD panel IL9486*/
enum {
	PANEL_ID_OLD_ILI9486 = 0,
	PANEL_ID_NEW_ILI9486 = 1,
};

struct msm_panel_ilitek_pdata {
	int gpio;
	int initialized;
	int maker_id;
	int (*lcd_power_save)(int);
};

void __init msm_msm7x2x_allocate_memory_regions(void);

#ifdef CONFIG_LGE_POWER_ON_STATUS_PATCH
void __init lge_board_pwr_on_status(void);
#endif

typedef void (gpio_i2c_init_func_t)(int bus_num);

void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func);
void __init lge_add_gpio_i2c_devices(void);
int __init lge_init_gpio_i2c_pin(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data);
int __init lge_init_gpio_i2c_pin_pullup(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data);

void __init msm_add_fb_device(void);
void __init msm_add_pmem_devices(void);

/* lge common functions to add devices */
void __init lge_add_input_devices(void);
void __init lge_add_misc_devices(void);
void __init lge_add_mmc_devices(void);
void __init lge_add_sound_devices(void);
void __init lge_add_lcd_devices(void);
void __init lge_add_camera_devices(void);
void __init lge_add_pm_devices(void);
void __init lge_add_usb_devices(void);
void __init lge_add_connectivity_devices(void);
void __init lge_add_nfc_devices(void);

void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func);

void __init lge_add_ramconsole_devices(void);
#if defined(CONFIG_ANDROID_RAM_CONSOLE) && defined(CONFIG_LGE_HANDLE_PANIC)
void __init lge_add_panic_handler_devices(void);
void lge_set_reboot_reason(unsigned reason);
#endif
int __init lge_get_uart_mode(void);
#ifdef CONFIG_LGE_HW_REVISION
lge_pcb_rev  lge_get_board_revno(void);
#endif

int get_reboot_mode(void);

enum lge_fboot_mode_type lge_get_fboot_mode(void);
unsigned lge_nv_manual_f(int val);
#ifdef CONFIG_LGE_SILENCE_RESET
unsigned lge_silence_reset_f(int val);
#endif

unsigned lge_smpl_counter_f(int val);
unsigned lge_charging_bypass_boot_f(int val);
unsigned lge_pseudo_battery_mode_f(int val);
unsigned lge_aat_partial_f(int val);
unsigned lge_aat_full_f(int val);
unsigned lge_aat_partial_or_full_f(int val);
#endif

