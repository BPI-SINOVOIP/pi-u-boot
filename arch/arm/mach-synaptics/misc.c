/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Shaojun Feng <Shaojun.feng@synaptics.com>
 */
#include <common.h>

unsigned int get_max_malloc_size(void)
{
	unsigned int max_size = CONFIG_SYS_MALLOC_LEN;
	// depends on the heap size defined in config
	if(max_size > (1 << 30)) // > 1GB
		return (max_size - (100 << 20));
	if(max_size > (400 << 20)) // > 400MB
		return (max_size - (40 << 20));

	//don't hope to come here
	return (max_size - (10 << 20));
}
