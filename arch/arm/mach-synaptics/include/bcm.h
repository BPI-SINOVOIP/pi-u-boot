#define CODETYPE_SYS_INIT	1
#define CODETYPE_MINILOADER	1
#define CODETYPE_TZK		4
#define CODETYPE_BOOTLOADER	5
#define CODETYPE_PT		5
#define CODETYPE_BLI		5
#define CODETYPE_KERNEL		5
#define CODETYPE_KEYSTORE	5

#define BCM_MAILBOX				0xF7930000	// MEMMAP_BCM_REG_BASE
#define BCM_PI_IMAGE_VERIFY			0x004E
#define BCM_STATUS_BCM_FAULT			(1<<10)
#define BCM_STATUS_BOOTSTRAP_IN_PROGRESS	(1<<9)
#define BCM_STATUS_BCM_READY			(1<<8)
#define BCM_STATUS_BCM_CMD_FLIP			(1<<7)

extern int bcm_decrypt_image(const void *src, unsigned int size, void *dst, unsigned int codetype);