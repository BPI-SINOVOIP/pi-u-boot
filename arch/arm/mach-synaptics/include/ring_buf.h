/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef _RING_BUF_H_
#define _RING_BUF_H_

typedef void *rb_handle;

rb_handle ring_buffer_attach(uint32_t base_address, uint32_t size);

int32_t ring_buffer_push(rb_handle handle, uint8_t *buffer, uint32_t size);
int32_t ring_buffer_pop(rb_handle handle, uint8_t *buffer, uint32_t max_size);

int32_t ring_buffer_not_empty(rb_handle handle);

#endif /* _RING_BUF_H_ */
