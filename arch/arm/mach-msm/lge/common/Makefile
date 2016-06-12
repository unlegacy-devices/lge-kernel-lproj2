EXTRA_CFLAGS += -Iarch/arm/mach-msm -Iarch/arm/tools

obj-$(CONFIG_MACH_LGE) += board-msm7627a-bt_lge.o
obj-$(CONFIG_MACH_LGE) += board-msm7627a-storage_lge.o
ifndef CONFIG_MACH_MSM7X27A_U0
obj-$(CONFIG_MACH_LGE) += board-msm7x27a-regulator_lge.o
obj-$(CONFIG_MACH_LGE) += board-msm7627a-wlan_lge.o
ifndef CONFIG_MACH_MSM7X25A_M4
obj-$(CONFIG_MACH_LGE) += board-msm7x27a_lge.o
obj-$(CONFIG_MACH_LGE) += board-msm7627a-display_lge.o
obj-$(CONFIG_MACH_LGE) += board-msm7x27a-sound_lge.o
endif
endif
