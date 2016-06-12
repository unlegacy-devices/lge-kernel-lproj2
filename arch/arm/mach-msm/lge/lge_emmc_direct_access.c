/*
 * arch/arm/mach-msm/lge/lge_emmc_direct_access.c
 *
 * Copyright (C) 2010 LGE, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/div64.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <linux/crc16.h>
#include <linux/string.h>
#include CONFIG_LGE_BOARD_HEADER_FILE
#include <mach/lge/board_v1.h>
#include <mach/proc_comm.h>
#include <mach/lge/lg_diag_testmode.h>
#include <mach/lge/lg_backup_items.h>

#define TABLE_ENTRY_0             0x1BE
#define TABLE_ENTRY_1             0x1CE
#define TABLE_ENTRY_2             0x1DE
#define TABLE_ENTRY_3             0x1EE
#define TABLE_SIGNATURE           0x1FE
#define TABLE_ENTRY_SIZE          0x010

#define OFFSET_STATUS             0x00
#define OFFSET_TYPE               0x04
#define OFFSET_FIRST_SEC          0x08
#define OFFSET_SIZE               0x0C
#define COPYBUFF_SIZE             (1024 * 16)
#define BINARY_IN_TABLE_SIZE      (16 * 512)
#define MAX_FILE_ENTRIES          20

#define MMC_BOOT_TYPE 0x48
#define MMC_SYSTEM_TYPE 0x78
#define MMC_USERDATA_TYPE 0x79

#define MMC_RCA 2

#define MAX_PARTITIONS 64

#define GET_LWORD_FROM_BYTE(x)    ((unsigned)*(x) | \
        ((unsigned)*((x)+1) << 8) | \
        ((unsigned)*((x)+2) << 16) | \
        ((unsigned)*((x)+3) << 24))

#define PUT_LWORD_TO_BYTE(x, y)   do{*(x) = (y) & 0xff;     \
    *((x)+1) = ((y) >> 8) & 0xff;     \
    *((x)+2) = ((y) >> 16) & 0xff;     \
    *((x)+3) = ((y) >> 24) & 0xff; }while(0)

#define GET_PAR_NUM_FROM_POS(x) ((((x) & 0x0000FF00) >> 8) + ((x) & 0x000000FF))

#define MMC_BOOT_TYPE 0x48
#define MMC_EXT3_TYPE 0x83
#define MMC_VFAT_TYPE 0xC

#define MMC_RECOVERY_TYPE		0x60
#define MMC_MISC_TYPE 0x77
#define MMC_XCALBACKUP_TYPE 0x6E

typedef struct MmcPartition MmcPartition;

static unsigned ext3_count = 0;
static char *ext3_partitions[] = {"persist", "bsp", "blb", "tombstones", "drm", "fota", "system", "cache", "wallpaper" , "userdata", "NONE"};

static unsigned vfat_count = 0;
static char *vfat_partitions[] = {"modem", "mdm",  "NONE"};

struct MmcPartition {
    char *device_index;
    char *filesystem;
    char *name;
    unsigned dstatus;
    unsigned dtype ;
    unsigned dfirstsec;
    unsigned dsize;
};

typedef struct {
    MmcPartition *partitions;
    int partitions_allocd;
    int partition_count;
} MmcState;

static MmcState g_mmc_state = {
    NULL,   // partitions
    0,      // partitions_allocd
    -1      // partition_count
};

typedef struct {
	char ret[32];
} testmode_rsp_from_diag_type;

#define FACTORY_RESET_STR_SIZE 11
#define FACTORY_RESET_STR "FACT_RESET_"

#define MMC_DEVICENAME "/dev/block/mmcblk0"

#ifdef CONFIG_LGE_SILENCE_RESET
#define SILENT_RESET_SIZE 2
static unsigned char silent_reset_buf[SILENT_RESET_SIZE]={0,};
#endif /* CONFIG_LGE_SILENCE_RESET */

extern char* remote_get_sw_version(void);
void swv_get_func(struct work_struct *work);
static struct workqueue_struct *swv_dload_wq;
struct work_struct swv_work;

int lge_erase_block(int secnum, size_t size);
int lge_write_block(unsigned int secnum, unsigned char *buf, size_t size);
int lge_read_block(unsigned int secnum, unsigned char *buf, size_t size);

static int dummy_arg;

int boot_info = 0;

static int boot_info_write(const char *val, struct kernel_param *kp)
{
	unsigned long flag = 0;

	if (val == NULL) {
		printk(KERN_ERR "%s, NULL buf\n", __func__);
		return -1;
	}

	flag = simple_strtoul(val,NULL,10);
	boot_info = (int)flag;
	printk(KERN_INFO "%s, flag : %d\n", __func__, boot_info);

	return 0;
}
module_param_call(boot_info, boot_info_write, param_get_int, &boot_info, S_IWUSR | S_IRUGO);

#ifdef CONFIG_LGE_NFC
int boot_info_nfc = 0;
module_param(boot_info_nfc, int, S_IWUSR | S_IRUGO);
#endif //CONFIG_LGE_NFC

int is_factory=0;

static int get_is_factory(char *buffer, struct kernel_param *kp)
{
	return sprintf(buffer, "%s", is_factory ? "yes" : "no");
}
module_param_call(is_factory, NULL, get_is_factory, &is_factory, 0444);

int db_integrity_ready = 0;
module_param(db_integrity_ready, int, S_IWUSR | S_IRUGO);

int fpri_crc_ready = 0;
module_param(fpri_crc_ready, int, S_IWUSR | S_IRUGO);

int file_crc_ready = 0;
module_param(file_crc_ready, int, S_IWUSR | S_IRUGO);

int db_dump_ready = 0;
module_param(db_dump_ready, int, S_IWUSR | S_IRUGO);

int db_copy_ready = 0;
module_param(db_copy_ready, int, S_IWUSR | S_IRUGO);

int external_memory_test_diag = 0;
module_param(external_memory_test_diag, int, S_IWUSR | S_IRUGO);

int fota_id_check = 0;
module_param(fota_id_check, int, S_IWUSR | S_IRUGO);

static char *fota_id_read = "fail";
module_param(fota_id_read, charp, S_IWUSR | S_IRUGO);

testmode_rsp_from_diag_type integrity_ret;
static int integrity_ret_write(const char *val, struct kernel_param *kp)
{
	memcpy(integrity_ret.ret, val, 32);
	return 0;
}
static int integrity_ret_read(char *buf, struct kernel_param *kp)
{
	memcpy(buf, integrity_ret.ret, 32);
	return 0;
}

module_param_call(integrity_ret, integrity_ret_write, integrity_ret_read, &dummy_arg, S_IWUSR | S_IRUGO);

static char *lge_strdup(const char *str)
{
	size_t len;
	char *copy;
	
	len = strlen(str) + 1;
	copy = kmalloc(len, GFP_KERNEL);
	if (copy == NULL)
		return NULL;
	memcpy(copy, str, len);
	return copy;
}

int lge_erase_block(int bytes_pos, size_t erase_size)
{
	unsigned char *erasebuf;
	unsigned written = 0;
	erasebuf = kmalloc(erase_size, GFP_KERNEL);
	if (!erasebuf) {
		printk("%s, allocation failed, expected size : %d\n", __func__, erase_size);
		return 0;
	}
	memset(erasebuf, 0xff, erase_size);
	written += lge_write_block(bytes_pos, erasebuf, erase_size);

	kfree(erasebuf);

	return written;
}
EXPORT_SYMBOL(lge_erase_block);

int lge_write_block(unsigned int bytes_pos, unsigned char *buf, size_t size)
{
	struct file *phMscd_Filp = NULL;
	mm_segment_t old_fs;
	unsigned int write_bytes = 0;

	if ((buf == NULL) || size <= 0) {
		printk(KERN_ERR "%s, NULL buffer or NULL size : %d\n", __func__, size);
		return 0;
	}
		
	old_fs=get_fs();
	set_fs(get_ds());

	phMscd_Filp = filp_open(MMC_DEVICENAME, O_RDWR | O_SYNC, 0);
	if( !phMscd_Filp)
	{
		printk(KERN_ERR "%s, Can not access 0x%x bytes postition\n", __func__, bytes_pos );
		goto write_fail;
	}

	phMscd_Filp->f_pos = (loff_t)bytes_pos;
	write_bytes = phMscd_Filp->f_op->write(phMscd_Filp, buf, size, &phMscd_Filp->f_pos);

	if(write_bytes <= 0)
	{
		printk(KERN_ERR "%s, Can not write 0x%x bytes postition %d size \n", __func__, bytes_pos, size);
		goto write_fail;
	}

write_fail:
	if(phMscd_Filp != NULL)
		filp_close(phMscd_Filp,NULL);
	set_fs(old_fs); 
	return write_bytes;
	
}
EXPORT_SYMBOL(lge_write_block);

int lge_read_block(unsigned int bytes_pos, unsigned char *buf, size_t size)
{
	struct file *phMscd_Filp = NULL;
	mm_segment_t old_fs;
	unsigned int read_bytes = 0;

	if ((buf == NULL) || size <= 0) {
		printk(KERN_ERR "%s, NULL buffer or NULL size : %d\n", __func__, size);
		return 0;
	}
		
	old_fs=get_fs();
	set_fs(get_ds());

	phMscd_Filp = filp_open(MMC_DEVICENAME, O_RDONLY, 0);
	if (!phMscd_Filp) {
		printk(KERN_ERR "%s, Can not access 0x%x bytes postition\n", __func__, bytes_pos );
		goto read_fail;
	}

	phMscd_Filp->f_pos = (loff_t)bytes_pos;
	read_bytes = phMscd_Filp->f_op->read(phMscd_Filp, buf, size, &phMscd_Filp->f_pos);

	if (read_bytes <= 0) {
		printk(KERN_ERR "%s, Can not read 0x%x bytes postition %d size \n", __func__, bytes_pos, size);
		goto read_fail;
	}

read_fail:
	if(phMscd_Filp != NULL)
		filp_close(phMscd_Filp,NULL);
	set_fs(old_fs); 
	return read_bytes;
}
EXPORT_SYMBOL(lge_read_block);

const MmcPartition *lge_mmc_find_partition_by_name(const char *name)
{
    if (g_mmc_state.partitions != NULL) {
        int i;
        for (i = 0; i < g_mmc_state.partitions_allocd; i++) {
            MmcPartition *p = &g_mmc_state.partitions[i];
            if (p->device_index !=NULL && p->name != NULL) {
                if (strcmp(p->name, name) == 0) {
                    return p;
                }
            }
        }
    }
    return NULL;
}
EXPORT_SYMBOL(lge_mmc_find_partition_by_name);

void lge_mmc_print_partition_status(void)
{
    if (g_mmc_state.partitions != NULL) 
    {
        int i;
        for (i = 0; i < g_mmc_state.partitions_allocd; i++) 
        {
            MmcPartition *p = &g_mmc_state.partitions[i];
            if (p->device_index !=NULL && p->name != NULL) {
                printk(KERN_INFO"Partition Name: %s\n",p->name);
                printk(KERN_INFO"Partition Name: %s\n",p->device_index);
            }
        }
    }
    return;
}
EXPORT_SYMBOL(lge_mmc_print_partition_status);

static void lge_mmc_partition_name (MmcPartition *mbr, unsigned int type) {
	char *name;
	name = kmalloc(64, GFP_KERNEL);
    switch(type)
    {
		case MMC_MISC_TYPE:
            sprintf(name,"misc");
            mbr->name = lge_strdup(name);
			break;
		case MMC_RECOVERY_TYPE:
            sprintf(name,"recovery");
            mbr->name = lge_strdup(name);
			break;
		case MMC_XCALBACKUP_TYPE:
            sprintf(name,"xcalbackup");
            mbr->name = lge_strdup(name);
			break;
        case MMC_BOOT_TYPE:
            sprintf(name,"boot");
            mbr->name = lge_strdup(name);
            break;
        case MMC_EXT3_TYPE:
            if (strcmp("NONE", ext3_partitions[ext3_count])) {
                strcpy((char *)name,(const char *)ext3_partitions[ext3_count]);
                mbr->name = lge_strdup(name);
                ext3_count++;
            }
            mbr->filesystem = lge_strdup("ext3");
            break;
        case MMC_VFAT_TYPE:
            if (strcmp("NONE", vfat_partitions[vfat_count])) {
                strcpy((char *)name,(const char *)vfat_partitions[vfat_count]);
                mbr->name = lge_strdup(name);
                vfat_count++;
            }
            mbr->filesystem = lge_strdup("vfat");
            break;
    };
	kfree(name);
}

int lge_mmc_read_mbr (MmcPartition *mbr) {
	unsigned char *buffer = NULL;
	char *device_index = NULL;
	int idx, i;
	unsigned mmc_partition_count = 0;
	unsigned int dtype;
	unsigned int dfirstsec;
	unsigned int EBR_first_sec;
	unsigned int EBR_current_sec;
	int ret = -1;

	struct file *phMscd_Filp = NULL;
	mm_segment_t old_fs;

	old_fs=get_fs();
	set_fs(get_ds());

	buffer = kmalloc(512, GFP_KERNEL);
	device_index = kmalloc(128, GFP_KERNEL);
	if ((buffer == NULL) || (device_index == NULL)) {
		printk("%s, allocation failed\n", __func__);
		goto ERROR2;
	}

	phMscd_Filp = filp_open(MMC_DEVICENAME, O_RDONLY, 0);
	if (!phMscd_Filp) {
		printk(KERN_ERR "%s, Can't open device\n", __func__ );
		goto ERROR2;
	}

	phMscd_Filp->f_pos = (loff_t)0;
	if (phMscd_Filp->f_op->read(phMscd_Filp, buffer, 512, &phMscd_Filp->f_pos) != 512)
	{
		printk(KERN_ERR "%s, Can't read device: \"%s\"\n", __func__, MMC_DEVICENAME);
		goto ERROR1;
	}

	/* Check to see if signature exists */
	if ((buffer[TABLE_SIGNATURE] != 0x55) || \
			(buffer[TABLE_SIGNATURE + 1] != 0xAA)) {
		printk(KERN_ERR "Incorrect mbr signatures!\n");
		goto ERROR1;
	}
	idx = TABLE_ENTRY_0;
	for (i = 0; i < 4; i++) {
		mbr[mmc_partition_count].dstatus = \
		            buffer[idx + i * TABLE_ENTRY_SIZE + OFFSET_STATUS];
		mbr[mmc_partition_count].dtype   = \
		            buffer[idx + i * TABLE_ENTRY_SIZE + OFFSET_TYPE];
		mbr[mmc_partition_count].dfirstsec = \
		            GET_LWORD_FROM_BYTE(&buffer[idx + \
		                                i * TABLE_ENTRY_SIZE + \
		                                OFFSET_FIRST_SEC]);
		mbr[mmc_partition_count].dsize  = \
		            GET_LWORD_FROM_BYTE(&buffer[idx + \
		                                i * TABLE_ENTRY_SIZE + \
		                                OFFSET_SIZE]);
		dtype  = mbr[mmc_partition_count].dtype;
		dfirstsec = mbr[mmc_partition_count].dfirstsec;
		lge_mmc_partition_name(&mbr[mmc_partition_count], \
		                mbr[mmc_partition_count].dtype);

		sprintf(device_index, "%sp%d", MMC_DEVICENAME, (mmc_partition_count+1));
		mbr[mmc_partition_count].device_index = lge_strdup(device_index);

		mmc_partition_count++;
		if (mmc_partition_count == MAX_PARTITIONS)
			goto SUCCESS;
	}

	/* See if the last partition is EBR, if not, parsing is done */
	if (dtype != 0x05)
	{
		goto SUCCESS;
	}

	EBR_first_sec = dfirstsec;
	EBR_current_sec = dfirstsec;

	phMscd_Filp->f_pos = (loff_t)(EBR_first_sec * 512);
	if (phMscd_Filp->f_op->read(phMscd_Filp, buffer, 512, &phMscd_Filp->f_pos) != 512)
	{
		printk(KERN_ERR "%s, Can't read device: \"%s\"\n", __func__, MMC_DEVICENAME);
		goto ERROR1;
	}

	/* Loop to parse the EBR */
	for (i = 0;; i++)
	{

		if ((buffer[TABLE_SIGNATURE] != 0x55) || (buffer[TABLE_SIGNATURE + 1] != 0xAA))
		{
		break;
		}
		mbr[mmc_partition_count].dstatus = \
                    buffer[TABLE_ENTRY_0 + OFFSET_STATUS];
		mbr[mmc_partition_count].dtype   = \
                    buffer[TABLE_ENTRY_0 + OFFSET_TYPE];
		mbr[mmc_partition_count].dfirstsec = \
                    GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_0 + \
                                        OFFSET_FIRST_SEC])    + \
                                        EBR_current_sec;
		mbr[mmc_partition_count].dsize = \
                    GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_0 + \
                                        OFFSET_SIZE]);
		lge_mmc_partition_name(&mbr[mmc_partition_count], \
                        mbr[mmc_partition_count].dtype);

		sprintf(device_index, "%sp%d", MMC_DEVICENAME, (mmc_partition_count+1));
		mbr[mmc_partition_count].device_index = lge_strdup(device_index);

		mmc_partition_count++;
		if (mmc_partition_count == MAX_PARTITIONS)
		goto SUCCESS;

		dfirstsec = GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_1 + OFFSET_FIRST_SEC]);
		if(dfirstsec == 0)
		{
			/* Getting to the end of the EBR tables */
			break;
		}
		
		 /* More EBR to follow - read in the next EBR sector */
		 phMscd_Filp->f_pos = (loff_t)((EBR_first_sec + dfirstsec) * 512);
		 if (phMscd_Filp->f_op->read(phMscd_Filp, buffer, 512, &phMscd_Filp->f_pos) != 512)
		 {
			 printk(KERN_ERR "%s, Can't read device: \"%s\"\n", __func__, MMC_DEVICENAME);
			 goto ERROR1;
		 }

		EBR_current_sec = EBR_first_sec + dfirstsec;
	}

SUCCESS:
    ret = mmc_partition_count;
ERROR1:
    if(phMscd_Filp != NULL)
		filp_close(phMscd_Filp,NULL);
ERROR2:
	set_fs(old_fs);
	if(buffer != NULL)
		kfree(buffer);
	if(device_index != NULL)
		kfree(device_index);
    return ret;
}

static int lge_mmc_partition_initialied = 0;
int lge_mmc_scan_partitions(void) {
    int i;

	if (lge_mmc_partition_initialied)
		return g_mmc_state.partition_count;
	
    if (g_mmc_state.partitions == NULL) {
        const int nump = MAX_PARTITIONS;
        MmcPartition *partitions = kmalloc(nump * sizeof(*partitions), GFP_KERNEL);
        if (partitions == NULL) {
            return -1;
        }
        g_mmc_state.partitions = partitions;
        g_mmc_state.partitions_allocd = nump;
        memset(partitions, 0, nump * sizeof(*partitions));
    }
    g_mmc_state.partition_count = 0;
    ext3_count = 0;
    vfat_count = 0;

    /* Initialize all of the entries to make things easier later.
     * (Lets us handle sparsely-numbered partitions, which
     * may not even be possible.)
     */
    for (i = 0; i < g_mmc_state.partitions_allocd; i++) {
        MmcPartition *p = &g_mmc_state.partitions[i];
        if (p->device_index != NULL) {
            kfree(p->device_index);
            p->device_index = NULL;
        }
        if (p->name != NULL) {
            kfree(p->name);
            p->name = NULL;
        }
        if (p->filesystem != NULL) {
            kfree(p->filesystem);
            p->filesystem = NULL;
        }
    }

    g_mmc_state.partition_count = lge_mmc_read_mbr(g_mmc_state.partitions);
    if(g_mmc_state.partition_count == -1)
    {
        printk(KERN_ERR"Error in reading mbr!\n");
        g_mmc_state.partition_count = -1;
    }
	if ( g_mmc_state.partition_count != -1 )
		lge_mmc_partition_initialied = 1;
    return g_mmc_state.partition_count;
}
EXPORT_SYMBOL(lge_mmc_scan_partitions);

int lge_emmc_misc_write(unsigned int blockNo, const char* buffer, int size);
int lge_emmc_misc_write_pos(unsigned int blockNo, const char* buffer, int size, int pos);
int lge_emmc_misc_read(unsigned int blockNo, char* buffer, int size);

#ifdef CONFIG_LGE_SILENCE_RESET
int lge_emmc_misc_write(unsigned int blockNo, const char* buffer, int size);
int lge_emmc_misc_read(unsigned int blockNo, char* buffer, int size);

static int write_SilentReset_flag(const char *val, struct kernel_param *kp)
{
	int size;

	memcpy(silent_reset_buf, val, SILENT_RESET_SIZE);
	silent_reset_buf[SILENT_RESET_SIZE-1] = '\0';
	printk("%s val[%s] -> silent_reset_buf[%s] \n", __func__, val, silent_reset_buf);
	size = lge_emmc_misc_write(36, silent_reset_buf, SILENT_RESET_SIZE);
	if (size == SILENT_RESET_SIZE)
		printk("<6>" "write %d block\n", size);
	else
		printk("<6>" "write fail (%d)\n", size);

	if(silent_reset_buf[0] == '1'){
		set_kernel_silencemode(0);
		lge_silence_reset_f(0);
	}
	else{
		set_kernel_silencemode(1);
		lge_silence_reset_f(1);
	}

	printk("%s,lge_silent_reset_flag=%d \n", __func__, get_kernel_silencemode());
	return 0;
}

static int read_SilentReset_flag(char *buf, struct kernel_param *kp)
{
	printk("%s,lge_silent_reset_flag=%d \n", __func__, get_kernel_silencemode());

	if(get_kernel_silencemode() == 0)
		sprintf(buf, "1"); // panic display mode
	else	
		sprintf(buf, "0"); // silence reset mode		
		
	return strlen(buf)+1;
}
module_param_call(silent_reset, write_SilentReset_flag, read_SilentReset_flag, NULL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);  
#endif 

static int write_SMPL_flag(const char *val, struct kernel_param *kp)
{
	lge_smpl_counter_f(0);
	return 0;
}

static int read_SMPL_flag(char *buf, struct kernel_param *kp)
{
	sprintf(buf, "%d", lge_smpl_counter_f(-1));
	return strlen(buf)+1;
}
module_param_call(smpl_counter, write_SMPL_flag, read_SMPL_flag, NULL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);  

static int write_ChargingBypassBoot_flag(const char *val, struct kernel_param *kp)
{
	if (*val == '1')
		lge_charging_bypass_boot_f(1);
	else
		lge_charging_bypass_boot_f(0);
	return 0;
}

static int read_ChargingBypassBoot_flag(char *buf, struct kernel_param *kp)
{
	sprintf(buf, "%d", lge_charging_bypass_boot_f(-1));
	return strlen(buf)+1;
}
module_param_call(charging_bypass_boot, write_ChargingBypassBoot_flag, read_ChargingBypassBoot_flag, NULL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);  

static int write_PseudoBatteryMode_flag(const char *val, struct kernel_param *kp)
{
	if (*val == '1')
		lge_pseudo_battery_mode_f(1);
	else
		lge_pseudo_battery_mode_f(0);
	return 0;
}

static int read_PseudoBatteryMode_flag(char *buf, struct kernel_param *kp)
{
	sprintf(buf, "%d", lge_pseudo_battery_mode_f(-1));
	return strlen(buf)+1;
}
module_param_call(pseudo_battery_mode, write_PseudoBatteryMode_flag, read_PseudoBatteryMode_flag, NULL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);  

int lge_get_frst_flag(void);
int lge_set_frst_flag(int flag);
int lge_get_esd_flag(void);
int lge_set_esd_flag(int flag);

int frst_write_block(const char *buf, struct kernel_param *kp)
{
	int expected = -1;
	int changeto = -1;

	// skip null
	if (buf == NULL)
		return -1;

	// skip invalid format
	if (buf[0] < '0' || buf[0] > '7')
		return -1;

	changeto = buf[0]-'0';

	// check expected value format
	if (buf[1] == '-') {
		if (buf[2] < '0' || buf[2] > '7')
			return -1;

		expected = changeto;
		changeto = buf[2]-'0';

		// compare expected flag and current flag
		if (lge_get_frst_flag() != expected)
			return -1;
	}

	// set frst flag
	lge_set_frst_flag(changeto);
	return 1;
}

int frst_read_block(char *buf, struct kernel_param *kp)
{
	int flag;

	if (buf == NULL)
		return -1;

	flag = lge_get_frst_flag();
	if (flag == -1)
		return -1;

	buf[0] = '0' + flag;
	buf[1] = '\0';
	return 2;
}
module_param_call(frst_flag, frst_write_block, frst_read_block, NULL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

static int fota_write_block(const char *val, struct kernel_param *kp)
{

	const MmcPartition *pMisc_part; 
	unsigned long bootmsg_bytes_pos = 0;
	unsigned long fotamsg_bytes_pos = 0;

	
	unsigned int bootmsg_size, fotamsg_size;
	unsigned int fota_reboot_result;

	lge_mmc_scan_partitions();
	pMisc_part = lge_mmc_find_partition_by_name("misc");
	if (pMisc_part == NULL) {
		printk(KERN_INFO"NO MISC\n");
		return -1;
	}
	
	bootmsg_bytes_pos = (pMisc_part->dfirstsec*512);

	printk(KERN_INFO"writing block\n");

	bootmsg_size= sizeof("boot-recovery");

	fota_reboot_result = lge_write_block(bootmsg_bytes_pos, "boot-recovery", bootmsg_size);

	if ( fota_reboot_result != bootmsg_size ) 
	{
		printk(KERN_INFO"%s: write block fail\n", __func__);
		return -1;
	}

	fotamsg_bytes_pos = ((pMisc_part->dfirstsec+3)*512);

	printk(KERN_INFO"%lx writing block\n", fotamsg_bytes_pos);

	fotamsg_size= sizeof("fota-recovery");

	fota_reboot_result = lge_write_block(fotamsg_bytes_pos, "fota-recovery", fotamsg_size);

	if ( fota_reboot_result != fotamsg_size ) 
	{
		printk(KERN_INFO"%s: write block fail\n", __func__);
		return -1;
	}

	printk(KERN_INFO"fota write block\n");
	return 0;
}
module_param_call(fota_recovery_reboot, fota_write_block, param_get_bool, &dummy_arg, S_IWUSR | S_IRUGO);

int lge_emmc_misc_write_pos(unsigned int blockNo, const char* buffer, int size, int pos)
{
	struct file *fp_misc = NULL;
	mm_segment_t old_fs;
	int write_bytes = -1;

	// exception handling
	if ((buffer == NULL) || size <= 0) {
		printk(KERN_ERR "%s, NULL buffer or NULL size : %d\n", __func__, size);
		return -1;
	}

	old_fs=get_fs();
	set_fs(get_ds());

	// try to open
	fp_misc = filp_open("/dev/block/platform/msm_sdcc.3/by-num/p8", O_WRONLY | O_SYNC, 0);
	if (IS_ERR(fp_misc)) {
		printk(KERN_ERR "%s, Can not access MISC\n", __func__);
		goto misc_write_fail;
	}

	fp_misc->f_pos = (loff_t) (512 * blockNo + pos);
	write_bytes = fp_misc->f_op->write(fp_misc, buffer, size, &fp_misc->f_pos);

	if (write_bytes <= 0) {
		printk(KERN_ERR "%s, Can not write (MISC) \n", __func__);
		goto misc_write_fail;
	}

misc_write_fail:
	if (!IS_ERR(fp_misc))
		filp_close(fp_misc, NULL);

	set_fs(old_fs);	
	return write_bytes;
}
EXPORT_SYMBOL(lge_emmc_misc_write_pos);

int lge_emmc_misc_write(unsigned int blockNo, const char* buffer, int size)
{
	return lge_emmc_misc_write_pos(blockNo, buffer, size, 0);
}
EXPORT_SYMBOL(lge_emmc_misc_write);

int lge_emmc_misc_read(unsigned int blockNo, char* buffer, int size)
{
	struct file *fp_misc = NULL;
	mm_segment_t old_fs;
	int read_bytes = -1;

	// exception handling
	if ((buffer == NULL) || size <= 0) {
		printk(KERN_ERR "%s, NULL buffer or NULL size : %d\n", __func__, size);
		return 0;
	}

	old_fs=get_fs();
	set_fs(get_ds());

	// try to open
	fp_misc = filp_open("/dev/block/platform/msm_sdcc.3/by-num/p8", O_RDONLY | O_SYNC, 0);
	if(IS_ERR(fp_misc)) {
		printk(KERN_ERR "%s, Can not access MISC\n", __func__);
		goto misc_read_fail;
	}

	fp_misc->f_pos = (loff_t) (512 * blockNo);
	read_bytes = fp_misc->f_op->read(fp_misc, buffer, size, &fp_misc->f_pos);

	if (read_bytes <= 0) {
		printk(KERN_ERR "%s, Can not read (MISC) \n", __func__);
		goto misc_read_fail;
	}

misc_read_fail:
	if (!IS_ERR(fp_misc))
		filp_close(fp_misc, NULL);

	set_fs(old_fs);	
	return read_bytes;
}
EXPORT_SYMBOL(lge_emmc_misc_read);

int lge_get_frst_flag(void)
{
	unsigned char buffer[16]={0,};
	if (lge_emmc_misc_read(8, buffer, sizeof(buffer)) <= 0) {
		pr_info(" *** (MISC) FRST Flag read fail\n");
		return -1;
	}
	if (buffer[0] == 0xff || buffer[0] == 0)
		buffer[0] = '0';

	pr_info(" *** (MISC) FRST Flag read: %d\n", buffer[0]-'0');

	if (buffer[0] < '0' || buffer[0] > '6')
		return -1;

	return buffer[0] - '0';
}
EXPORT_SYMBOL(lge_get_frst_flag);

int lge_set_frst_flag(int flag)
{
	char buffer[4]={0,};
	if (flag < 0 || flag > 6)
		return -1;

	buffer[0] = '0' + flag;
	if (lge_emmc_misc_write(8, buffer, sizeof(buffer)) <= 0)
		return -1;
	pr_info(" *** (MISC) FRST Flag write: %c\n", buffer[0]);
	return 0;
}
EXPORT_SYMBOL(lge_set_frst_flag);

int lge_get_esd_flag(void)
{
	unsigned char buffer[8]={0,};

	if (lge_emmc_misc_read(36, buffer, sizeof(buffer)) <= 0) {
		pr_info(" *** (MISC) lge_get_esd_flag read fail\n");
		return -1;
	}

	pr_info(" *** (MISC) lge_get_esd_flag: %d\n", buffer[0]-'0');
	return buffer[5] - '0';
}
EXPORT_SYMBOL(lge_get_esd_flag);

int lge_set_esd_flag(int flag)
{
	char buffer[8]={0,};	

	buffer[0] = '0' + flag;
	if (lge_emmc_misc_write_pos(36, buffer, sizeof(buffer), 5) <= 0)
		return -1;

	pr_info(" *** (MISC) lge_set_esd_flag write: %c\n", buffer[0]);
	return 0;
}
EXPORT_SYMBOL(lge_set_esd_flag);

int lge_emmc_wallpaper_write_pos(unsigned int blockNo, const char* buffer, int size, int pos)
{
	struct file *fp_wallpaper = NULL;
	mm_segment_t old_fs;
	int write_bytes = -1;

	// exception handling
	if ((buffer == NULL) || size <= 0) {
		printk(KERN_ERR "%s, NULL buffer or NULL size : %d\n", __func__, size);
		return -1;
	}

	old_fs=get_fs();
	set_fs(get_ds());

	// try to open
	fp_wallpaper = filp_open("/dev/block/mmcblk0p6", O_RDWR | O_SYNC, 0);
	if (IS_ERR(fp_wallpaper)) {
		printk(KERN_ERR "%s, Can not access WALLPAPER\n", __func__);
		goto wallpaper_write_fail;
	}

	fp_wallpaper->f_pos = (loff_t) (512 * blockNo + pos);
	write_bytes = fp_wallpaper->f_op->write(fp_wallpaper, buffer, size, &fp_wallpaper->f_pos);

	if (write_bytes <= 0) {
		printk(KERN_ERR "[sunny], write_bytes = [%d]\n", write_bytes );
		printk(KERN_ERR "%s, Can not write (WALLPAPER) \n", __func__);
		goto wallpaper_write_fail;
	}

wallpaper_write_fail:
	if (!IS_ERR(fp_wallpaper))
		filp_close(fp_wallpaper, NULL);

	set_fs(old_fs);	
	return write_bytes;
}
EXPORT_SYMBOL(lge_emmc_wallpaper_write_pos);


int lge_emmc_wallpaper_write(unsigned int blockNo, const char* buffer, int size)
{
	return lge_emmc_wallpaper_write_pos(blockNo, buffer, size, 0);
}
EXPORT_SYMBOL(lge_emmc_wallpaper_write);

extern unsigned char swv_buff[100];
void swv_get_func(struct work_struct *work)
{
	char *envp[] = {
		"HOME=/",
		"TERM=linux",
		NULL,
	};

	unsigned* swv_buffer;
	char fac_version[50];
	char sw_version[50];
	unsigned ret;
	swv_buffer = kzalloc(100, GFP_KERNEL);

	remote_get_sw_version();

	memcpy(&sw_version[0],swv_buff,50);
	//printk(KERN_INFO "sw_version VALUE = %s\n",sw_version);
	memcpy(&fac_version[0],(&swv_buff[0])+50,50);
	//printk(KERN_INFO "fac_version VALUE = %s\n",fac_version);
	sw_version[49] = '\0';
	fac_version[49] = '\0';
	{
		char *argv[] = {
		"setprop",
		"lge.version.factorysw",
		fac_version,
		NULL,
		};
		if ((ret = call_usermodehelper("/system/bin/setprop", argv, envp, UMH_WAIT_PROC)) != 0) {
			printk(KERN_ERR "%s lge.version.factorysw set failed to run \": %i\n",__func__, ret);
		}
		else{
			printk(KERN_INFO "%s lge.version.factorysw set execute ok\n", __func__);
		}
	}
	{
		char *argv[] = {
		"setprop",
		"ro.lge.factoryversion",
		fac_version,
		NULL,
		};
		if ((ret = call_usermodehelper("/system/bin/setprop", argv, envp, UMH_WAIT_PROC)) != 0) {
			printk(KERN_ERR "%s ro.lge.factoryversion set failed to run \": %i\n",__func__, ret);
		}
		else{
			printk(KERN_INFO "%s ro.lge.factoryversion set execute ok\n", __func__);
		}
	}	{
		char *argv[] = {
		"setprop",
		"lge.version.sw",
		sw_version,
		NULL,
		};
		if ((ret = call_usermodehelper("/system/bin/setprop", argv, envp, UMH_WAIT_PROC)) != 0) {
			printk(KERN_ERR "%s lge.version.sw set failed to run \": %i\n",__func__, ret);
		}
		else{
			printk(KERN_INFO "%s  lge.version.sw set execute ok\n", __func__);
		}
	}
	kfree(swv_buffer);
}

int lge_sw_version_read(const char *val, struct kernel_param *kp)
{
	printk(KERN_INFO"%s: started\n", __func__);
	
	queue_work(swv_dload_wq, &swv_work);

	return 0;
}
module_param_call(lge_swv,lge_sw_version_read, NULL, NULL, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

static int __init lge_emmc_direct_access_init(void)
{
	printk(KERN_INFO"%s: started\n", __func__);
	swv_dload_wq = create_singlethread_workqueue("swv_dload_wq");
	if (swv_dload_wq == NULL)
		printk(KERN_INFO"%s: swv wq error\n", __func__);

	INIT_WORK(&swv_work,swv_get_func);
	printk(KERN_INFO"%s: finished\n", __func__);
	return 0;
}

static void __exit lge_emmc_direct_access_exit(void)
{
	return;
}

module_init(lge_emmc_direct_access_init);
module_exit(lge_emmc_direct_access_exit);

MODULE_DESCRIPTION("LGE emmc direct access apis");
MODULE_AUTHOR("SeHyun Kim <sehyuny.kim@lge.com>");
MODULE_LICENSE("GPL");
