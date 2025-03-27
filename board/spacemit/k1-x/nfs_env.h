#ifndef __NFS_ENV_H__
#define __NFS_ENV_H__

#ifdef CONFIG_ENV_IS_IN_NFS

#define NFS_LOAD_ADDR  CONFIG_FASTBOOT_BUF_ADDR

struct mac_mapping {
    char mac_addr[18];
    char bootfs_path[256];
    char rootfs_path[256];
};

int eth_parse_enetaddr(const char *addr, uint8_t *enetaddr);
int check_bootfs_exists(void);
int load_env_from_nfs(void);

char* parse_mtdparts_and_find_bootfs(void);
enum board_boot_mode get_boot_mode(void);
void read_from_eeprom(struct tlvinfo_tlv **tlv_data, u8 tcode);
int run_net_flash_command(void);

#endif // CONFIG_ENV_IS_IN_NFS

#endif // __NFS_ENV_H__