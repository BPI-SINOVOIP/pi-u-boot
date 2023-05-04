/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Based on the Linux Flash-based transactional key-value Driver
 * Copyright (C) 2010 Google, Inc.
 *
 * Copyright (C) 2016 Marvell Technology Group Ltd.
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaoming Lu <xmlu@marvell.com>
 *
 * Copyright (C) 2019 Synaptics Incorporated
 * Author: Cheng Lu <Cheng.Lu@synaptics.com>
 *
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <flash_ts.h>
#include <sdhci.h>
#include <misc.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

extern unsigned long str2ul(const char *cp,char **endp,unsigned int base);

static int is_blank(const void *buf, size_t size)
{
	size_t i;
	const unsigned int *data = (const unsigned int *)buf;
	size = size / sizeof(*data);
	{
		if (data[0] != 0x00000000 && data[0] != 0xffffffff)
			return 0;

		for (i = 0; i < size; i++)
			if (data[i] != data[0])
				return 0;
		return 1;
	}
}

static int flash_block_isbad(struct flash_ts_priv *ts, loff_t off)
{
	return 0;
}

static int flash_ts_erase(struct flash_ts_priv *ts, loff_t off)
{
	int blk_start = (ts->base + off) / ts->writesize;
	int blk_cnt = ts->erasesize / ts->writesize;
	int res = blk_derase(ts->blk, blk_start, blk_cnt);
	if (res != blk_cnt) {
		printf("%s: blk_derase failed, errono %d\n", __func__, res);
		return -EIO;
	}
	return 0;
}

static int flash_ts_write(struct flash_ts_priv *ts, loff_t off,
		       const void *buf, size_t size)
{
	int blk_start = (ts->base + off) / ts->writesize;
	int blk_cnt = size / ts->writesize;
	int res = blk_dwrite(ts->blk, blk_start, blk_cnt, buf);
	if (res != blk_cnt) {
		printf("%s: blk_dwrite failed, errono %d\n", __func__, res);
		return -EIO;
	}
	return 0;
}

static int flash_ts_read(struct flash_ts_priv *ts, loff_t off, void *buf, size_t size)
{
	int blk_start = (ts->base + off) / ts->writesize;
	int blk_cnt = size / ts->writesize;
	int res = blk_dread(ts->blk, blk_start, blk_cnt, buf);
	if (res != blk_cnt) {
		printf("%s: blk_dread failed, errono %d\n", __func__, res);
		return -EIO;
	}
	return 0;
}

static char *flash_ts_find(struct flash_ts_priv *ts, const char *key,
			   size_t key_len)
{
	char *s = ts->cache.data;
	while (*s) {
		if (!strncmp(s, key, key_len)) {
			if (s[key_len] == '=')
				return s;
		}

		s += strlen(s) + 1;
	}
	return NULL;
}


static inline uint32_t flash_ts_crc(const struct flash_ts *cache)
{
	const unsigned char *p;
	uint32_t crc = 0;
	size_t len;

	/* skip magic and crc fields */
	len = cache->len + 2 * sizeof(uint32_t);
	p = (const unsigned char*)&cache->len;

	while (len--) {
		int i;

		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
	}
	return crc ^ ~0;
}

/* Verifies cache consistency and locks it */
static int __flash_ts_verify(struct flash_ts_priv *ts)
{
	if (likely(ts)) {
		if (unlikely(ts->cache.crc != flash_ts_crc(&ts->cache))) {
			printf(DRV_NAME
			       ": memory corruption detected\n");
			return -EFAULT;
		}
	} else {
		printf(DRV_NAME ": not initialized yet\n");
		return -ENOSPC;
	}

	return 0;
}

static int flash_ts_commit(struct flash_ts_priv *ts)
{
	loff_t off = ts->offset + ts->chunk;
	/* we try to make two passes to handle non-erased blocks
	 * this should only matter for the inital pass over the whole device.
	 */
	int max_iterations = (ts->size / ts->erasesize) * 2;
	size_t size = (FLASH_TS_HDR_SIZE + ts->cache.len + ts->chunk - 1)
		       & ~(ts->chunk - 1);

	/* fill unused part of data */
	memset(ts->cache.data + ts->cache.len, 0xff,
	       sizeof(ts->cache.data) - ts->cache.len);

	while (max_iterations--) {
		/* wrap around */
		if (off >= ts->size)
			off = 0;

		/* new block? */
		if (!(off & (ts->erasesize - 1))) {
			if (flash_block_isbad(ts, off)) {
				/* skip this block */
				off += ts->erasesize;
				continue;
			}

			if (unlikely(flash_ts_erase(ts, off))) {
				/* skip this block */
				off += ts->erasesize;
				continue;
			}
		}

		/* write and read back to veryfy */
		if (flash_ts_write(ts, off, &ts->cache, size) ||
		    flash_ts_read(ts, off, &ts->cache_tmp_verify, size)) {
			/* hmm, probably unclean block, skip it for now */
			off = (off + ts->erasesize) & ~(ts->erasesize - 1);
			continue;
		}

		/* compare */
		if (memcmp(&ts->cache, &ts->cache_tmp_verify, size)) {
			printf(DRV_NAME
			       ": record v%u read mismatch @ 0x%08x\n",
				(unsigned)ts->cache.version, (unsigned)off);
			/* skip this block for now */
			off = (off + ts->erasesize) & ~(ts->erasesize - 1);
			continue;
		}

		/* for new block, erase the previous block after write done,
		 * it's to speed up flash_ts_scan
		 */
		if (!(off & (ts->erasesize - 1))) {
			loff_t pre_block_base = ts->offset & ~(ts->erasesize - 1);
			loff_t cur_block_base = off & ~(ts->erasesize - 1);
			if (cur_block_base != pre_block_base)
				flash_ts_erase(ts, pre_block_base);
		}
		ts->offset = off;
		printf(DRV_NAME ": record v%u commited @ 0x%08x\n",
		       (unsigned)ts->cache.version, (unsigned)off);
		return 0;
	}

	printf(DRV_NAME ": commit failure\n");
	return -EIO;
}

static int flash_ts_set(struct flash_ts_priv *ts, const char *key, const char *value)
{
	size_t klen = strlen(key);
	size_t vlen = strlen(value);
	char *p;

	int res = __flash_ts_verify(ts);
	if (res)
		return res;

	/* save current cache contents so we can restore it on failure */
	memcpy(&ts->cache_tmp_backup, &ts->cache, sizeof(ts->cache_tmp_backup));

	p = flash_ts_find(ts, key, klen);
	if (p) {
		/* we are replacing existing entry,
		 * empty value (vlen == 0) removes entry completely.
		 */
		size_t cur_len = strlen(p) + 1;
		size_t new_len = vlen ? klen + 1 + vlen + 1 : 0;

		if (cur_len != new_len) {
			/* we need to move stuff around */

			if ((ts->cache.len - cur_len) + new_len >
			     sizeof(ts->cache.data))
				goto no_space;

			memmove(p + new_len, p + cur_len,
				ts->cache.len - (p - ts->cache.data + cur_len));

			ts->cache.len = (ts->cache.len - cur_len) + new_len;
		} else if (!strcmp(p + klen + 1, value)) {
			/* skip update if new value is the same as the old one */
			res = 0;
			goto out;
		}

		if (vlen) {
			p += klen + 1;
			memcpy(p, value, vlen);
			p[vlen] = '\0';
		}
	} else {
		size_t len = klen + 1 + vlen + 1;

		/* don't do anything if value is empty */
		if (!vlen) {
			res = 0;
			goto out;
		}

		if (ts->cache.len + len > sizeof(ts->cache.data))
			goto no_space;

		/* add new entry at the end */
		p = ts->cache.data + ts->cache.len - 1;
		memcpy(p, key, klen);
		p += klen;
		*p++ = '=';
		memcpy(p, value, vlen);
		p += vlen;
		*p++ = '\0';
		*p = '\0';
		ts->cache.len += len;
	}

	++ts->cache.version;
	ts->cache.crc = flash_ts_crc(&ts->cache);
	res = flash_ts_commit(ts);
	if (unlikely(res))
		memcpy(&ts->cache, &ts->cache_tmp_backup, sizeof(ts->cache));
	goto out;

    no_space:
	printf(DRV_NAME ": no space left for '%s=%s'\n", key, value);
	res = -ENOSPC;
    out:
	return res;
}

static void flash_ts_get(struct flash_ts_priv *ts, const char *key, char *value, unsigned int size)
{
	size_t klen = strlen(key);
	const char *p;

	*value = '\0';

	int res = __flash_ts_verify(ts);
	if (res)
		return;

	p = flash_ts_find(ts, key, klen);
	if (p)
		strncpy(value, p + klen + 1, size);
}

static inline uint32_t flash_ts_check_header(const struct flash_ts *cache)
{
	if (cache->magic == FLASH_TS_MAGIC &&
	    cache->version &&
	    cache->len && cache->len <= sizeof(cache->data) &&
	    cache->crc == flash_ts_crc(cache) &&
	    /* check correct null-termination */
	    !cache->data[cache->len - 1] &&
	    (cache->len == 1 || !cache->data[cache->len - 2])) {
		/* all is good */
		return cache->version;
	}

	return 0;
}

static int flash_ts_scan(struct flash_ts_priv *ts)
{
	int res, good_blocks = 0;
	loff_t off = 0;

	do {
		/* new block ? */
		if (!(off & (ts->erasesize - 1))) {
			if (flash_block_isbad(ts, off)) {
				printf(DRV_NAME
				       ": skipping bad block @ 0x%08x\n",
				       (unsigned)off);
				off += ts->erasesize;
				continue;
			} else
				++good_blocks;
		}

		res = flash_ts_read(ts, off, &ts->cache_tmp_verify,
				 sizeof(ts->cache_tmp_verify));
		if (!res) {
			uint32_t version =
			    flash_ts_check_header(&ts->cache_tmp_verify);
			if (version > ts->cache.version) {
				memcpy(&ts->cache, &ts->cache_tmp_verify,
				       sizeof(ts->cache));
				ts->offset = off;
			}
			if (0 == version &&
				is_blank(&ts->cache_tmp_verify,
					sizeof(ts->cache_tmp_verify))) {
				/* skip the whole block if chunk is blank */
				off = (off + ts->erasesize) & ~(ts->erasesize - 1);
			} else
				off += ts->chunk;
		} else {
			off += ts->chunk;
		}
	} while (off < ts->size);

	if (unlikely(!good_blocks)) {
		printf(DRV_NAME ": no good blocks\n");
		return -ENODEV;
	}

	if (unlikely(good_blocks < 2))
		printf(DRV_NAME ": less than 2 good blocks,"
				" reliability is not guaranteed\n");
	return 0;
}

/* Round-up to the next power-of-2,
 * from "Hacker's Delight" by Henry S. Warren.
 */
static inline uint32_t clp2(uint32_t x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

static int _flash_ts_init(struct flash_ts_priv *ts)
{
	/* make sure both page and block sizes are power-of-2
	 * (this will make chunk size determination simpler).
	 */
	if (unlikely(!is_power_of_2(ts->writesize) ||
		     !is_power_of_2(ts->erasesize))) {
		printf(DRV_NAME ": unsupported flash geometry\n");
		return -ENODEV;
	}

	/* determine chunk size so it doesn't cross block boundary,
	 * is multiple of page size and there is no wasted space in a block.
	 * We assume page and block sizes are power-of-2.
	 */
	ts->chunk = clp2((sizeof(struct flash_ts) + ts->writesize - 1) &
			 ~(ts->writesize - 1));
	if (unlikely(ts->chunk > ts->erasesize)) {
		printf(DRV_NAME ": NAND block size is too small\n");
		return -ENODEV;
	}

	/* default empty state */
	ts->offset = ts->size - ts->chunk;
	ts->cache.magic = FLASH_TS_MAGIC;
	ts->cache.version = 0;
	ts->cache.len = 1;
	ts->cache.data[0] = '\0';
	ts->cache.crc = flash_ts_crc(&ts->cache);

	/* scan flash partition for the most recent record */
	int res = flash_ts_scan(ts);
	if (unlikely(res))
		return res;

	if (ts->cache.version)
		printf(DRV_NAME ": v%u loaded from 0x%08x\n",
		       (unsigned)ts->cache.version, (unsigned)ts->offset);

	return res;
}

int flash_ts_init(struct udevice *dev)
{
	struct flash_ts_priv *ts = dev_get_priv(dev);
	int res = 0;

	struct mmc *mmc = find_mmc_device(CONFIG_FLASH_MMC_DEV);

	if (mmc == NULL) {
		pr_err("invalid mmc device\n");
		return -ENODEV;
	}

	ts->blk = blk_get_dev("mmc", CONFIG_FLASH_MMC_DEV);
	if (!ts->blk || ts->blk->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid mmc device\n");
		return -ENODEV;
	}

	disk_partition_t info;
	int ret = part_get_info_by_name(ts->blk, "fts", &info);
	if (ret < 0) {
		pr_err("cannot find partition: '%s'\n", "fts");
		return ret;
	}

	ts->size = info.size * info.blksz;
	ts->base = info.start * info.blksz;
	ts->erasesize = 512 * 1024;
	ts->writesize = 512;

	ts->size = info.size * info.blksz;

	res = _flash_ts_init(ts);
	if (res)
		return res;
	ts->init = 1;

	return 0;
}

static int flash_ts_emmc_write(struct udevice *dev, int offset, const void *buf, int size)
{
	struct flash_ts_priv *ts = dev_get_priv(dev);
	int res;
	if (!ts->init) {
		res = flash_ts_init(dev);
		if (res)
			return res;
	} else {
		res = blk_dselect_hwpart(ts->blk, 0);
		if (res) {
			printf("%s: blk_dselect_hwpart failed\n", __func__);
			return res;
		}
	}
	const struct flash_ts_key *key = (struct flash_ts_key *)buf;
	return flash_ts_set(ts, key->name, key->value);
}

static int flash_ts_emmc_read(struct udevice *dev, int offset, void *buf, int size)
{
	struct flash_ts_priv *ts = dev_get_priv(dev);
	int res = 0;
	if (!ts->init) {
		res = flash_ts_init(dev);
		if (res)
			return -ENODEV;
	}
	struct flash_ts_key *key = (struct flash_ts_key *)buf;
	flash_ts_get(ts, key->name, key->value, size);
	return 0;
}

int flash_ts_probe(struct udevice *dev)
{
	struct flash_ts_priv *ts = dev_get_priv(dev);
	ts->init = 0;
	return 0;
}

const struct misc_ops flash_ts_ops = {
	.read = flash_ts_emmc_read,
	.write = flash_ts_emmc_write,
};

const struct udevice_id flash_ts_ids[] = {
	{ .compatible = "google,flash-ts-mmc" },
	{}
};

U_BOOT_DRIVER(flash_ts) = {
	.name	= "flash_ts",
	.id	= UCLASS_MISC,
	.of_match = flash_ts_ids,
	.probe	= flash_ts_probe,
	.priv_auto_alloc_size = sizeof(struct flash_ts_priv),
	.ops	= &flash_ts_ops,
};
