/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright © 2013-2020 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
#ifndef _GENIMG_H
#define _GENIMG_H

#define	IMAGE_INFO_VERSION_1_0_0_0 0x01000000
#define	IMG_INFO_MAGIC 0xE11D
struct img_info {
	unsigned int	version;
	unsigned short	magic;
	unsigned char		reserved1[2];
	unsigned int	image_size;
	unsigned int	image_offset;
	unsigned char		reserved2[48];
};

#define PREPEND_IMAGE_INFO_SIZE sizeof(struct img_info)

#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
	((unsigned int)(char)(ch0) | ((unsigned int)(char)(ch1) << 8) | \
	((unsigned int)(char)(ch2) << 16) | ((unsigned int)(char)(ch3) << 24))

#define IMAGE_CHUNK_ID_SM_FW		MAKE_FOURCC('S', 'M', 'F', 'W')
#define IMAGE_CHUNK_ID_FASTLOGO		MAKE_FOURCC('L', 'O', 'G', 'O')
#define IMAGE_CHUNK_ID_KEYMASTER	MAKE_FOURCC('C', 'Y', 'P', 'T')
#define IMAGE_CHUNK_ID_LINUX_DTB	MAKE_FOURCC('L', 'D', 'T', 'B')
#define IMAGE_CHUNK_ID_DHUB			MAKE_FOURCC('D', 'H', 'U', 'B')
#define IMAGE_HEADER_MAGIC			MAKE_FOURCC('I', 'M', '*', 'H')

#define MAX_IMAGE_CHUNK_ENTRY 128

struct img_header {
	unsigned int header_magic_num;	/* 'IM*H' */
	unsigned int header_size;
	unsigned int header_version;
	unsigned int header_reserved;
	char image_name[32];
	unsigned int image_version;
	unsigned int reserved[2];
	unsigned int chunk_num;
	struct chunk_param{
		unsigned int id;
		unsigned int offset;	 /* start from image header, 16 bytes aligned */
		unsigned int size;
		unsigned int attr0;		/* reserved */
		/* data can be in (dest_start, dest_start + dest_size)
		 * if dest_size == 0, then chunk data must always place at
		 * dest_start.
		 */
		unsigned long long dest_start;       /* destination address if it gets */
		//unsigned int reserved_dest_start;
		unsigned int dest_size;        /* destination address region */
		unsigned int attr1;
	} chunk[0];
};

extern int find_chunk(unsigned int id, struct img_header *buff);

#endif //_GENIMG_H
