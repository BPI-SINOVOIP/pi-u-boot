#ifndef __FLASH_TS_H
#define __FLASH_TS_H

#define DRV_NAME                "fts"
static inline int is_power_of_2(unsigned int n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

#define likely(x)               __builtin_expect(!!(x), 1)
#define unlikely(x)             __builtin_expect(!!(x), 0)

/* Keep in sync with 'struct flash_ts' */
#define FLASH_TS_HDR_SIZE       (4 * sizeof(uint32_t))
#define FLASH_TS_MAX_SIZE       (16 * 1024)
#define FLASH_TS_MAX_DATA_SIZE  (FLASH_TS_MAX_SIZE - FLASH_TS_HDR_SIZE)

#define FLASH_TS_MAGIC          0x53542a46

struct flash_ts_key {
	char *name;
	char *value;
};

/* Physical flash layout */
struct flash_ts {
	u32 magic;		/* "F*TS" */
	u32 crc;		/* doesn't include magic and crc fields */
	u32 len;		/* real size of data */
	u32 version;		/* generation counter, must be positive */

	/* data format is very similar to Unix environment:
	 *   key1=value1\0key2=value2\0\0
	 */
	char data[FLASH_TS_MAX_DATA_SIZE];
};

struct flash_ts_priv {
	struct blk_desc *blk;

	int init;

	loff_t base;            /* NAND partition base offset */
	size_t size;            /* NAND partition size */
	size_t erasesize;       /* block size */
	size_t writesize;       /* page size */

	/* chunk size, >= sizeof(struct flash_ts) */
	size_t chunk;

	/* current record offset within MTD device */
	loff_t offset;

	/* in-memory copy of flash content */
	struct flash_ts cache;

	/* temporary buffers
	 *  - one backup for failure rollback
	 *  - another for read-after-write verification
	 */
	struct flash_ts cache_tmp_backup;
	struct flash_ts cache_tmp_verify;
};

#endif