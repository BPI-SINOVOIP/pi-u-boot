/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <common.h>
#include <malloc.h>

#include "include/sparse/sparse.h"

#include "sparse_file.h"

#include "output_file.h"
#include "backed_block.h"
#include "sparse_format.h"

struct sparse_file *sparse_file_new(unsigned int block_size, int64_t len)
{
	struct sparse_file *s = calloc(sizeof(struct sparse_file), 1);
	if (!s) {
		return NULL;
	}

	s->backed_block_list = backed_block_list_new(block_size);
	if (!s->backed_block_list) {
		free(s);
		return NULL;
	}

	s->block_size = block_size;
	s->len = len;

	return s;
}

void sparse_file_destroy(struct sparse_file *s)
{
	backed_block_list_destroy(s->backed_block_list);
	free(s);
}

int sparse_file_add_data(struct sparse_file *s,
		void *data, int flag, unsigned int len, unsigned int block)
{
	return backed_block_add_data(s->backed_block_list, data, flag, len, block);
}

unsigned int sparse_count_chunks(struct sparse_file *s)
{
	struct backed_block *bb;
	unsigned int last_block = 0;
	unsigned int chunks = 0;

	for (bb = backed_block_iter_new(s->backed_block_list); bb;
			bb = backed_block_iter_next(bb)) {
		if (backed_block_block(bb) > last_block) {
			/* If there is a gap between chunks, add a skip chunk */
			chunks++;
		}
		chunks++;
		last_block = backed_block_block(bb) +
				DIV_ROUND_UP(backed_block_len(bb), s->block_size);
	}
	if (last_block < DIV_ROUND_UP(s->len, s->block_size)) {
		chunks++;
	}

	return chunks;
}

static int sparse_file_write_block(struct output_file *out,
		struct backed_block *bb)
{
	int ret = -EINVAL;

	switch (backed_block_type(bb)) {
	case BACKED_BLOCK_DATA:
		ret = write_data_chunk(out, backed_block_len(bb), backed_block_data(bb));
		break;
	case BACKED_BLOCK_FILE:
		printf("unsupport BACKED_BLOCK_FILE\n");
		break;
	case BACKED_BLOCK_FD:
		printf("unsupport BACKED_BLOCK_FD\n");
		break;
	case BACKED_BLOCK_FILL:
		printf("unsupport BACKED_BLOCK_FILL\n");
		break;
	}

	return ret;
}

static int write_all_blocks(struct sparse_file *s, struct output_file *out)
{
	struct backed_block *bb;
	unsigned int last_block = 0;
	int64_t pad;
	int ret = 0;

	for (bb = backed_block_iter_new(s->backed_block_list); bb;
			bb = backed_block_iter_next(bb)) {
		if (backed_block_block(bb) > last_block) {
			unsigned int blocks = backed_block_block(bb) - last_block;
			write_skip_chunk(out, (int64_t)blocks * s->block_size);
		}
		ret = sparse_file_write_block(out, bb);
		if (ret)
			return ret;
		last_block = backed_block_block(bb) +
				DIV_ROUND_UP(backed_block_len(bb), s->block_size);
	}

	pad = s->len - (int64_t)last_block * s->block_size;
	assert(pad >= 0);
	if (pad > 0) {
		write_skip_chunk(out, pad);
	}

	return 0;
}

int sparse_file_write(struct sparse_file *s,
		struct blk_desc *dev,
		lbaint_t start)
{
	int ret;
	int chunks;
	struct output_file *out;

	chunks = sparse_count_chunks(s);
	out = output_file_open_blk(dev, start, s->block_size, s->len, chunks);

	if (!out)
		return -ENOMEM;
	ret = write_all_blocks(s, out);

	output_file_close(out);

	return ret;
}
