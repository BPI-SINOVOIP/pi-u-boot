/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 *			Cheng Lu <Cheng.Lu@synaptics.com>
 */

#include <config.h>
#include <common.h>
#include <image-sparse.h>
#include <div64.h>
#include <malloc.h>
#include <memalign.h>
#include <part.h>
#include <sparse_format.h>
#include <u-boot/zlib.h>

#ifdef CONFIG_MMC_WRITE
#include <mmc.h>
#endif

#include <linux/math64.h>

#define HEADER0			'\x1f'
#define HEADER1			'\x8b'
#define	ZALLOC_ALIGNMENT	16
#define HEAD_CRC		2
#define EXTRA_FIELD		4
#define ORIG_NAME		8
#define COMMENT			0x10
#define RESERVED		0xe0
#define DEFLATED		8
#define GZIP_MAGIC_NUMBER      0x8B1F

static z_stream s;
static u32 crc = 0;
static u32 expected_crc = 0;
static u64 szexpected = 0;
static u64 totalfilled = 0;

#define MAX_WRITE_BLK (2048)

static void default_log(const char *ignored, char *response) {}

static unsigned int get_extcsd181(void)
{
#ifdef CONFIG_MMC_WRITE
	struct mmc *mmc;

	mmc = find_mmc_device(0);

	if (!mmc)
		return 0xFFFFFFFF;

	if(mmc->ext_csd)
		return mmc->ext_csd[181];
#endif

	return 0xFFFFFFFF;
}

static int sgunzip_init(struct sparse_storage *info,
				unsigned char *src, int len, char *response)
{
	int i, flags;
	u32 szuncompressed;
	int r = 0;

	totalfilled = 0;
	crc = 0;

	/* skip header */
	i = 10;
	flags = src[3];
	if (src[2] != DEFLATED || (flags & RESERVED) != 0) {
		puts("Error: Bad gzipped data\n");
		info->mssg("Error: Bad gzipped data", response);
		return -1;
	}
	if ((flags & EXTRA_FIELD) != 0)
		i = 12 + src[10] + (src[11] << 8);
	if ((flags & ORIG_NAME) != 0)
		while (src[i++] != 0)
			;
	if ((flags & COMMENT) != 0)
		while (src[i++] != 0)
			;
	if ((flags & HEAD_CRC) != 0)
		i += 2;

	if (i >= len-8) {
		puts("Error: gunzip out of data in header");
		info->mssg("Error: gunzip out of data in header", response);
		return -1;
	}

	memcpy(&expected_crc, src + len - 8, sizeof(expected_crc));
	expected_crc = le32_to_cpu(expected_crc);

	memcpy(&szuncompressed, src + len - 4, sizeof(szuncompressed));
	szexpected = le32_to_cpu(szuncompressed);

	s.zalloc = gzalloc;
	s.zfree = gzfree;

	r = inflateInit2(&s, -MAX_WBITS);
	if (r != Z_OK) {
		printf("Error: inflateInit2() returned %d\n", r);
		info->mssg("Error: inflateInit2() return error!", response);
		return -1;
	}

	s.next_in = src + i;
	s.avail_in = len - i;

	return 0;
}

static int sgunzip(struct sparse_storage *info,
				void *writebuf, int szwritebuf,
				char *response)
{
	int r = 0;
	int numfilled;

	if((writebuf == NULL) || (szwritebuf <= 0)) {
		printf("%s: wrong parameters\n", __func__);
		info->mssg("sgunzip: wrong parameters!", response);
		r = -1;
		goto end;
	}

	if (s.avail_in == 0) {
		printf("%s: weird termination\n", __func__);
		info->mssg("sgunzip: weird termination!", response);
		goto out;
	}

	s.avail_out = szwritebuf;
	s.next_out = writebuf;

	r = inflate(&s, Z_SYNC_FLUSH);
	if ((r != Z_OK) &&
	    (r != Z_STREAM_END)) {
		printf("Error: inflate() returned %d\n", r);
		info->mssg("Error: inflate() return error!", response);
		goto out;
	}
	numfilled = szwritebuf - s.avail_out;
	crc = crc32(crc, writebuf, numfilled);
	totalfilled += numfilled;

	if(r != Z_STREAM_END)
		goto end;

	if ((szexpected != totalfilled) ||
	    (crc != expected_crc))
		r = -1;
	else
		r = 0;

out:
	inflateEnd(&s);

end:
	return r;
}

static int sgunzip_write(struct sparse_storage *info,
				void * sbuff, lbaint_t blk, lbaint_t blkcnt,
				char *response)
{
	int wblktotal = 0, wblk = 0, woff = 0;

	wblktotal = blkcnt;
	woff = blk;

	do {
		wblk = (wblktotal > MAX_WRITE_BLK) ? MAX_WRITE_BLK : wblktotal;

		if(sgunzip(info, sbuff, (wblk * info->blksz), response)) {
			return -1;
		}

		if(info->write(info, woff, wblk, sbuff) != wblk)
			return 0;

		woff += wblk;
		wblktotal -= wblk;
	} while(wblktotal > 0);

	return blkcnt;
}

int write_gzipped_sparse_image(struct sparse_storage *info,
		const char *part_name, void *data, int len, char *response)
{
	lbaint_t blk;
	lbaint_t blkcnt;
	lbaint_t blks;
	uint32_t bytes_written = 0;
	unsigned int chunk;
	unsigned int offset;
	unsigned int chunk_data_sz;
	uint32_t *fill_buf = NULL;
	uint32_t fill_val;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	int fill_buf_num_blks;
	int i;
	int j;
	unsigned int erased_mem_cont = get_extcsd181();
	int ignore_fill = 0;
	sparse_header_t sheader;
	chunk_header_t cheader;
	void *sbuff = NULL;
	int szwbuff = 0;

	fill_buf_num_blks = CONFIG_IMAGE_SPARSE_FILLBUF_SIZE / info->blksz;

	if(sgunzip_init(info, data, len, response)) {
		return -1;
	}

	szwbuff = sizeof(sparse_header_t);

	if(sgunzip(info, (void *)&sheader, szwbuff, response)) {
		return -1;
	}

	if(is_sparse_image((void *)&sheader) == 0) {
		// not sparse image
		return -2;
	}

	/* Read and skip over sparse image header */
	sparse_header = &sheader;

	/* alloc 1MB buff for furture use. */
	sbuff = malloc_cache_aligned(MAX_WRITE_BLK * info->blksz);

	if (sparse_header->file_hdr_sz > sizeof(sparse_header_t)) {
		/*
		 * Skip the remaining bytes in a header that is longer than
		 * we expected.
		 */
		szwbuff = (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
		if(sgunzip(info, sbuff, szwbuff,response)) {
			free(sbuff);
			return -1;
		}
	}

	if (!info->mssg)
		info->mssg = default_log;

	debug("=== Sparse Image Header ===\n");
	debug("magic: 0x%x\n", sparse_header->magic);
	debug("major_version: 0x%x\n", sparse_header->major_version);
	debug("minor_version: 0x%x\n", sparse_header->minor_version);
	debug("file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	debug("chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	debug("blk_sz: %d\n", sparse_header->blk_sz);
	debug("total_blks: %d\n", sparse_header->total_blks);
	debug("total_chunks: %d\n", sparse_header->total_chunks);

	/*
	 * Verify that the sparse block size is a multiple of our
	 * storage backend block size
	 */
	div_u64_rem(sparse_header->blk_sz, info->blksz, &offset);
	if (offset) {
		printf("%s: Sparse image block size issue [%u]\n",
		       __func__, sparse_header->blk_sz);
		info->mssg("sparse image block size issue", response);
		return -1;
	}

	puts("Flashing Sparse Image\n");

	/* Start processing chunks */
	blk = info->start;
	for (chunk = 0; chunk < sparse_header->total_chunks; chunk++) {
		/* Read and skip over chunk header */
		if(sgunzip(info, (void *)&cheader, sizeof(chunk_header_t), response)) {
			free(sbuff);
			return -1;
		}
		chunk_header = &cheader;

		if (chunk_header->chunk_type != CHUNK_TYPE_RAW) {
			debug("=== Chunk Header ===\n");
			debug("chunk_type: 0x%x\n", chunk_header->chunk_type);
			debug("chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
			debug("total_size: 0x%x\n", chunk_header->total_sz);
		}

		if (sparse_header->chunk_hdr_sz > sizeof(chunk_header_t)) {
			/*
			 * Skip the remaining bytes in a header that is longer
			 * than we expected.
			 */
			szwbuff = (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
			if(sgunzip(info, sbuff, szwbuff, response)) {
				free(sbuff);
				return -1;
			}
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		blkcnt = chunk_data_sz / info->blksz;
		switch (chunk_header->chunk_type) {
		case CHUNK_TYPE_RAW:
			if (chunk_header->total_sz !=
			    (sparse_header->chunk_hdr_sz + chunk_data_sz)) {
				info->mssg("Bogus chunk size for chunk type Raw",
					   response);
				return -1;
			}

			if (blk + blkcnt > info->start + info->size) {
				printf(
				    "%s: Request would exceed partition size!\n",
				    __func__);
				info->mssg("Request would exceed partition size!",
					   response);
				return -1;
			}

			blks = sgunzip_write(info, sbuff, blk, blkcnt, response);
			/* blks might be > blkcnt (eg. NAND bad-blocks) */
			if (blks < blkcnt) {
				printf("%s: %s" LBAFU " [" LBAFU "]\n",
				       __func__, "Write failed, block #",
				       blk, blks);
				info->mssg("flash write failure", response);
				return -1;
			}
			blk += blks;
			bytes_written += blkcnt * info->blksz;
			total_blocks += chunk_header->chunk_sz;
			break;

		case CHUNK_TYPE_FILL:
			if (chunk_header->total_sz !=
			    (sparse_header->chunk_hdr_sz + sizeof(uint32_t))) {
				info->mssg("Bogus chunk size for chunk type FILL", response);
				free(sbuff);
				return -1;
			}

			if(sgunzip(info, sbuff, sizeof(uint32_t), response)) {
				free(sbuff);
				return -1;
			}

			ignore_fill = (*(uint32_t *)sbuff == erased_mem_cont) ? 1 : 0;

			if(ignore_fill) {
				blk += blkcnt;
				total_blocks += chunk_data_sz / sparse_header->blk_sz;
				continue;
			}

			fill_buf = (uint32_t *)
				   memalign(ARCH_DMA_MINALIGN,
					    ROUNDUP(
						info->blksz * fill_buf_num_blks,
						ARCH_DMA_MINALIGN));
			if (!fill_buf) {
				info->mssg("Malloc failed for: CHUNK_TYPE_FILL",
					   response);
				return -1;
			}

			fill_val = *(uint32_t *)sbuff;

			for (i = 0;
			     i < (info->blksz * fill_buf_num_blks /
				  sizeof(fill_val));
			     i++)
				fill_buf[i] = fill_val;

			if (blk + blkcnt > info->start + info->size) {
				printf(
				    "%s: Request would exceed partition size!\n",
				    __func__);
				info->mssg("Request would exceed partition size!",
					   response);
				return -1;
			}

			for (i = 0; i < blkcnt;) {
				j = blkcnt - i;
				if (j > fill_buf_num_blks)
					j = fill_buf_num_blks;
				blks = info->write(info, blk, j, fill_buf);
				/* blks might be > j (eg. NAND bad-blocks) */
				if (blks < j) {
					printf("%s: %s " LBAFU " [%d]\n",
					       __func__,
					       "Write failed, block #",
					       blk, j);
					info->mssg("flash write failure",
						   response);
					free(fill_buf);
					return -1;
				}
				blk += blks;
				i += j;
			}
			bytes_written += blkcnt * info->blksz;
			total_blocks += chunk_data_sz / sparse_header->blk_sz;
			free(fill_buf);
			break;

		case CHUNK_TYPE_DONT_CARE:
			blk += info->reserve(info, blk, blkcnt);
			total_blocks += chunk_header->chunk_sz;
			break;

		case CHUNK_TYPE_CRC32:
			if (chunk_header->total_sz !=
			    sparse_header->chunk_hdr_sz) {
				info->mssg("Bogus chunk size for chunk type Dont Care",
					   response);
				free(sbuff);
				return -1;
			}
			total_blocks += chunk_header->chunk_sz;
			if(sgunzip(info, sbuff, chunk_data_sz, response)) {
				free(sbuff);
				return -1;
			}
			break;

		default:
			printf("%s: Unknown chunk type: %x\n", __func__,
			       chunk_header->chunk_type);
			info->mssg("Unknown chunk type", response);
			free(sbuff);
			return -1;
		}
	}

	if(sbuff)
		free(sbuff);

	debug("Wrote %d blocks, expected to write %d blocks\n",
	      total_blocks, sparse_header->total_blks);
	printf("........ wrote %u bytes to '%s'\n", bytes_written, part_name);

	if (total_blocks != sparse_header->total_blks) {
		info->mssg("sparse image write failure", response);
		return -1;
	}

	return 0;
}
