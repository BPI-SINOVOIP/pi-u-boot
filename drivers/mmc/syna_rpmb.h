#ifndef SYNA_RPMB_H
#define SYNA_RPMB_H

#include <config.h>
#include <common.h>

#define RPMB_KEY_SIZE	32
#define RPMB_DATA_SIZE	256
#define RPMB_FILE_PATH_LEN	63
#define MAC_LEN			32
#define NONCE_LEN		16

typedef struct {
	uint8_t  stuff[196];
	uint8_t  keyMAC[MAC_LEN];
	uint8_t  data[RPMB_DATA_SIZE];
	uint8_t  nonce[NONCE_LEN];
	uint32_t writeCounter;
	uint16_t addr;
	uint16_t blockCount;
	uint16_t result;
	uint16_t reqResp;
} rpmbFrame;

#define RPMB_FAST_CALL_SUB_ID	0x4

enum {
	RPMB_CONFIG_CMD,
	RPMB_ENABLE_CMD,
	RPMB_FRAME_PACK_CMD,
	RPMB_FRAME_UNPACK_CMD,
};

enum {
	RPMB_CAPACITY_ERROR = 0xF0,
	RPMB_CONFIG_ERROR = 0xF1,
	RPMB_ENABLE_ERROR = 0xF2,
	RPMB_KEY_PROGRAM_ERROR = 0xF3,
	RPMB_READ_COUNTER_ERROR = 0xF4,
	RPMB_KEY_NOT_PROGRAM_ERROR = 0xF5,
	RRPMB_FRAME_PACK_ERROR = 0xF6,
	RRPMB_FRAME_UNPACK_ERROR = 0xF7,
};

struct rpmb_fastcall_param {
	int rpmb_sub_cmd;               //in
	int rpmb_size;                  //in
	int rpmb_authkey_flag;          //out
	uint8_t rpmb_authkey[32];       //out
	rpmbFrame frame;                // in/out
};

struct rpmb_fastcall_generic_param {
	/*
	 * the first is always the sub command id
	 */
	unsigned int sub_cmd_id;
	unsigned int param_len;
	struct rpmb_fastcall_param rpmb_param;
};

#endif /* SYNA_RPMB_H */
