#include <common.h>
#include "bcm.h"
#include "bcm_reg.h"

static int bcm_verify(unsigned int type, unsigned int src, unsigned int dst)
{
	unsigned status;
	unsigned int waitCount;
#ifndef CONFIG_LESSPRINT
	unsigned int i, *p32;
#endif
	volatile NOT_MAILBOX *mb = (NOT_MAILBOX *) BCM_MAILBOX;

#ifndef CONFIG_LESSPRINT
	p32 = (unsigned int *)(uintptr_t) src;
	printf("\tverify 0:0x%x, 1k:0x%x, 2k:0x%x\n", p32[1], p32[1+1024/4], p32[1+2048/4]);
#endif

	status = mb->command_fifo_status;
	// older erom will have BCM_STATUS_BOOTSTRAP_IN_PROGRESS set
	if (0 == (status & BCM_STATUS_BOOTSTRAP_IN_PROGRESS))
	{
		// wait for bcm ready
		//lgpl_printf("wait for bcm ready...\n");
		//for (waitCount=~255; waitCount; waitCount--)
		while (0 == (status & BCM_STATUS_BCM_READY))
		{
			if (status != mb->command_fifo_status)
			{
				status = mb->command_fifo_status;
				//lgpl_printf("bcm fifo staus = 0x%x\n", status);
			}
		}
	}
	mb->primitive_command_parameter0 = type;
	mb->primitive_command_parameter1 = src;
	mb->primitive_command_parameter2 = dst;
	mb->secure_processor_command = BCM_PI_IMAGE_VERIFY;

	//for (waitCount=0; waitCount<~255; waitCount++) // Wait_For_WTM_Complete( 0x10000, pCtrl );
	for (waitCount=0; ; waitCount++) // Wait_For_WTM_Complete( 0x10000, pCtrl );
	{
		//if ((mb->command_fifo_status & BCM_STATUS_BCM_CMD_FLIP) != status)
			//break;
		// wait for "command complete" or timeout
		if( mb->host_interrupt_register & 0x1 )
			break;
		mdelay(1);
	}

	/*
	  * Do a full reset to BCM HOST interrupt
	  * ----------------------------------------------------------------------------------
	  * Bit      Field Name              Access  Reset  Description
	  *                                  Mode    Value	 * 31:19    RSVD                      R/W   0h     Reserved
	  * 18       HOST_Q_FULL_RST           R/W   0h     Host Queue Full Reset
	  * 17       HOST_Q_FULL_ACC_RST       R/W   0h     Host Queue Full Access Reset
	  * 16       HOST_ADRS_PNG_EXCPTN_RST  R/W   0h     Host Address Range Exception Reset
	  * 15:1     RSVD                      R/W   0h     Reserved
	  * 0        SP_CMD_CMPLT_RST          R/W   0h     SP Command Complete Reset
	  * ----------------------------------------------------------------------------------
	  */

	mb->host_interrupt_register = 0x70001; // Clear_WTM_Interrupts( 0xffffffff, pCtrl );
	status = mb->command_return_status;
#ifndef CONFIG_LESSPRINT
	i = mb->command_status_0;
	printf("\tverify image %x, size=%d, waitcount=%d\n", status, i, waitCount);
#endif
	return status;
}


int bcm_decrypt_image(const void *src, unsigned int size, void *dst, unsigned int codetype)
{
	//In SH code base, we use 0x4 as enc_type for TZ, bootloader and kernel.
	//It should match with building TZ and kernel.
	//And for sysinit, we use 0x1.
	int ret = 0;

	//FIXME: so we'd better know the size of buffer
	flush_dcache_range((ulong)dst, (ulong)(((char *)dst) + size));

	ret = bcm_verify(codetype, (unsigned int)(uintptr_t)src, (unsigned int)(uintptr_t)dst);
	printf("VerifyImage ret=%x\n", ret);

	invalidate_dcache_range((ulong)dst, (ulong)(((char *)dst) + size));

	if (ret > 0)
		ret = 0;
	return ret;
}

