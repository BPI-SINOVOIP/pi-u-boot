#define SOC_SECM_BASE			0xf7fd0000
#define SM_SECM_DTCM_SIZE		(4 * 1024)
#define SM_BOOTMODE_ADDR		(SOC_SECM_BASE + SM_SECM_DTCM_SIZE - 64 - 96)

#define SOC_DTCM_BASE			0xf7fa0000
#define SM_DTCM_SIZE			0x00004000
#define SM_MSG_EXTRA_BUF_ADDR		(SOC_DTCM_BASE + SM_DTCM_SIZE - 512)
#define SM_MSG_EXTRA_BUF_SIZE		494
#define SM_MSG_EXTRA_BUF_ADDR_OFF	512

enum fastboot_command {
	CMD_NULL = 0,
	CMD_BOOT,
	CMD_CONTINUE,
	CMD_REBOOT,
	CMD_REBOOT_FB,
	CMD_RECOVERY,
	CMD_END
};

struct fastboot_cmd {
	enum fastboot_command command;
	unsigned int param[3];
};

struct bootflag {
	unsigned int magic;
	unsigned int flag;
	struct fastboot_cmd cmd;
	unsigned long mm_addr;
	unsigned int mf_addr;
	unsigned int mf_size;
	char serial_no[64];
	char product[32];
};

#define BOOTFLAG_MAGIC			(0x5AA5BEAF)
#define BOOTFLAG_RECOVERY 		(0xDEADDEAD)
#define BOOTFLAG_FASTBOOT		(0xDEAD5A5A)
