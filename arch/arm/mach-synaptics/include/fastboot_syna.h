/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

enum fastboot_reboot_command {
	FB_REBOOT_CMD_BOOT,
	FB_REBOOT_CMD_REBOOT,
	FB_REBOOT_CMD_REBOOT_FB,
	FB_REBOOT_CMD_RECOVERY,
	FB_REBOOT_CMD_FASTBOOT,
};

/*
 * A/B Mode check
 */
#define BOOTSEL_A			0x0
#define BOOTSEL_B			0x1
#define BOOTSEL_ALL			0x2
#define BOOTSEL_DEFAULT			BOOTSEL_A
#define BOOTSEL_INVALID			0xff
#define SLOT_NAME(index)		((index) ? "b" : "a")
#define SLOT_INDEX(slot)		((*((char *)slot)) - 'a')
#define INDEX_VALID(index)		(!((unsigned int)(index) > (BOOTSEL_ALL - 1)))




int get_lock_flag(unsigned int *lock_flag, unsigned int *lock_critical_flag);
int save_lock_flag(unsigned int lock_flag, unsigned lock_critical_flag);
int fb_set_reboot_bootloader_flag(void);
int fb_set_reboot_recovery_flag(void);
int fb_set_reboot_fastboot_flag(void);
int oem_check_lock(char *partition_name);
int virtual_ab_merge_status_check(char *partition_name);
#if CONFIG_IS_ENABLED(SYNA_FASTBOOT_AB)
int get_current_slot(void);
int get_slot_count(void);
bool is_slot_successful(int slot_index);
bool is_slot_unbootable(int slot_index);
int get_slot_retry_count(int slot_index);
bool set_slot_active(int slot_index);
#endif
char *fastboot_check_partition_type(char *patition_name);
int clear_bootloader_message_to_misc(void);
void fb_mmc_flash_read_from_offset(const char *cmd, void *read_buffer, unsigned int read_bytes, unsigned int offset);
void fb_mmc_flash_read(const char *cmd, void *read_buffer, unsigned int read_bytes);


