/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE 1

#include <common.h>
#include <blk.h>
#include <malloc.h>

#include <linux/stat.h>
#include <linux/types.h>

#include "output_file.h"
#include "sparse_format.h"

struct output_file_ops {
	int (*open)(struct output_file *, struct blk_desc *, lbaint_t);
	int (*skip)(struct output_file *, int64_t);
	int (*pad)(struct output_file *, int64_t);
	int (*write)(struct output_file *, void *, int);
	void (*close)(struct output_file *);
};

struct sparse_file_ops {
	int (*write_data_chunk)(struct output_file *out, unsigned int len,
			void *data);
	int (*write_skip_chunk)(struct output_file *out, int64_t len);
	int (*write_end_chunk)(struct output_file *out);
};

struct output_file {
	int64_t cur_out_ptr;
	unsigned int chunk_cnt;
	struct output_file_ops *ops;
	struct sparse_file_ops *sparse_ops;
	unsigned int block_size;
	int64_t len;
	char *zero_buf;
	uint32_t *fill_buf;
	char *buf;
};

struct output_file_blk {
	struct output_file out;
	struct blk_desc *dev;
	lbaint_t offset;
};

#define to_output_file_blk(_o) \
	container_of((_o), struct output_file_blk, out)

static int blk_open(struct output_file *out, struct blk_desc *dev, lbaint_t offset)
{
	struct output_file_blk *outb = to_output_file_blk(out);

	debug("%s: dev=%p, offset=0x"LBAF"\n", __func__, dev, offset);
	outb->dev = dev;
	outb->offset = offset;
	return 0;
}

static int blk_skip(struct output_file *out, int64_t cnt)
{
	struct output_file_blk *outb = to_output_file_blk(out);

	assert(cnt % 512 == 0);
	outb->out.cur_out_ptr += cnt;
	return 0;
}

static int blk_pad(struct output_file *out, int64_t len)
{
	struct output_file_blk *outb = to_output_file_blk(out);

	assert(len % 512 == 0);
	if (outb->out.cur_out_ptr < len)
		outb->out.cur_out_ptr = len;
	return 0;
}

static int blk_write(struct output_file *out, void *data, int len)
{
	int ret;
	struct output_file_blk *outb = to_output_file_blk(out);

	assert(len % 512 == 0);
	assert(outb->out.cur_out_ptr % 512 == 0);

	debug("%s: dev=%p, cur_out_ptr=0x%llx, len=%x\n", __func__, outb->dev,
		outb->out.cur_out_ptr, len);
	ret = blk_dwrite(outb->dev, outb->offset + outb->out.cur_out_ptr / 512, len / 512, data);
	if (ret < 0) {
		printf("Error: write");
		return ret;
	} else if (ret != len / 512) {
		printf("Error: incomplete write");
		return -EIO;
	}
	outb->out.cur_out_ptr += len;

	return 0;
}

static void blk_close(struct output_file *out)
{
	struct output_file_blk *outb = to_output_file_blk(out);
	free(outb);
}

static struct output_file_ops blk_ops = {
	.open = blk_open,
	.skip = blk_skip,
	.pad = blk_pad,
	.write = blk_write,
	.close = blk_close,
};

static int write_normal_data_chunk(struct output_file *out, unsigned int len,
		void *data)
{
	int ret;
	unsigned int rnd_up_len = ALIGN(len, out->block_size);

	ret = out->ops->write(out, data, len);
	if (ret < 0) {
		return ret;
	}

	if (rnd_up_len > len) {
		ret = out->ops->skip(out, rnd_up_len - len);
	}

	return ret;
}

static int write_normal_skip_chunk(struct output_file *out, int64_t len)
{
	return out->ops->skip(out, len);
}

int write_normal_end_chunk(struct output_file *out)
{
	return out->ops->pad(out, out->len);
}

static struct sparse_file_ops normal_file_ops = {
		.write_data_chunk = write_normal_data_chunk,
		.write_skip_chunk = write_normal_skip_chunk,
		.write_end_chunk = write_normal_end_chunk,
};

void output_file_close(struct output_file *out)
{
	out->sparse_ops->write_end_chunk(out);
	if (out->zero_buf)
		free(out->zero_buf);
	out->ops->close(out);
}

static int output_file_init(struct output_file *out, int block_size,
		int64_t len, int chunks)
{
	out->len = len;
	out->block_size = block_size;
	out->cur_out_ptr = 0ll;
	out->chunk_cnt = 0;

	out->zero_buf = calloc(block_size, 1);
	if (!out->zero_buf) {
		printf("Error: malloc zero_buf");
		return -ENOMEM;
	}

	out->sparse_ops = &normal_file_ops;

	return 0;
}

static struct output_file *output_file_new_blk(void)
{
	struct output_file_blk *outb = calloc(1, sizeof(struct output_file_blk));
	if (!outb) {
		printf("Error: malloc struct outb");
		return NULL;
	}

	outb->out.ops = &blk_ops;

	return &outb->out;
}

struct output_file *output_file_open_blk(struct blk_desc *dev, lbaint_t start,
		unsigned int block_size, int64_t len, int chunks)
{
	int ret;
	struct output_file *out;

	out = output_file_new_blk();
	if (!out) {
		return NULL;
	}

	out->ops->open(out, dev, start);

	ret = output_file_init(out, block_size, len, chunks);
	if (ret < 0) {
		free(out);
		return NULL;
	}

	return out;
}

/* Write a contiguous region of data blocks from a memory buffer */
int write_data_chunk(struct output_file *out, unsigned int len, void *data)
{
	return out->sparse_ops->write_data_chunk(out, len, data);
}

int write_skip_chunk(struct output_file *out, int64_t len)
{
	return out->sparse_ops->write_skip_chunk(out, len);
}
