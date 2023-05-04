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

#ifndef _BACKED_BLOCK_H_
#define _BACKED_BLOCK_H_

struct backed_block_list;
struct backed_block;

enum backed_block_type {
	BACKED_BLOCK_DATA,
	BACKED_BLOCK_FILE,
	BACKED_BLOCK_FD,
	BACKED_BLOCK_FILL,
};

#define BB_DATA_NOT_MALLOC	0x0
#define BB_DATA_MALLOC		0x1

int backed_block_add_data(struct backed_block_list *bbl, void *data, int flag,
		unsigned int len, unsigned int block);

struct backed_block *backed_block_iter_new(struct backed_block_list *bbl);
struct backed_block *backed_block_iter_next(struct backed_block *bb);
unsigned int backed_block_len(struct backed_block *bb);
unsigned int backed_block_block(struct backed_block *bb);
void *backed_block_data(struct backed_block *bb);
enum backed_block_type backed_block_type(struct backed_block *bb);

struct backed_block *backed_block_iter_new(struct backed_block_list *bbl);
struct backed_block *backed_block_iter_next(struct backed_block *bb);

struct backed_block_list *backed_block_list_new(unsigned int block_size);
void backed_block_list_destroy(struct backed_block_list *bbl);

#endif
