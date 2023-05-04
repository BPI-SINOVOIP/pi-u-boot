/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */
#include "bcm_mailbox.h"
#include "bcm_primitive.h"
#include "bcm_status.h"

#include <common.h>

#if 0
#define bcm_display printf
#else
#define bcm_display(...)
#endif

#define devmem_phy_to_virt(x) (x)

static void bcm_send_command(const BCM_MAILBOX_COMMAND *pCmd)
{
        volatile _BCM_MAILBOX * biu = (_BCM_MAILBOX *)devmem_phy_to_virt(BCM_MAILBOX_BASE);

		// we have to flush dcache before send command to bcm
		flush_dcache_all();

        biu->command_parameter0  = pCmd->arg0;
        biu->command_parameter1  = pCmd->arg1;
        biu->command_parameter2  = pCmd->arg2;
        biu->command_parameter3  = pCmd->arg3;
        biu->command_parameter4  = pCmd->arg4;
        biu->command_parameter5  = pCmd->arg5;
        biu->command_parameter6  = pCmd->arg6;
        biu->command_parameter7  = pCmd->arg7;
        biu->command_parameter8  = pCmd->arg8;
        biu->command_parameter9  = pCmd->arg9;
        biu->command_parameter10 = pCmd->arg10;
        biu->command_parameter11 = pCmd->arg11;
        biu->command_parameter12 = pCmd->arg12;
        biu->command_parameter13 = pCmd->arg13;
        biu->command_parameter14 = pCmd->arg14;
        biu->command_parameter15 = pCmd->arg15;
        biu->secure_processor_command = pCmd->command;
        bcm_display("Sending BCM command, command=0x%x\n", pCmd->command);
}


static int bcm_wait_for_complete(int wait_count)
{
        volatile _BCM_MAILBOX * biu = (_BCM_MAILBOX *)devmem_phy_to_virt(BCM_MAILBOX_BASE);
        volatile unsigned int intr_stat;

        while(1) {
                intr_stat = biu->host_interrupt_register; // 0xc8 Write-to-Clear
                if(intr_stat & 1)
                        break;
		udelay(1000);
        }

        return 0;
}

static void bcm_get_status(BCM_MAILBOX_STATUS *pStatus)
{
        volatile _BCM_MAILBOX * biu = (_BCM_MAILBOX *)devmem_phy_to_virt(BCM_MAILBOX_BASE);

        pStatus->return_status = biu->command_return_status;
        pStatus->arg0  = biu->command_status0;
        pStatus->arg1  = biu->command_status1;
        pStatus->arg2  = biu->command_status2;
        pStatus->arg3  = biu->command_status3;
        pStatus->arg4  = biu->command_status4;
        pStatus->arg5  = biu->command_status5;
        pStatus->arg6  = biu->command_status6;
        pStatus->arg7  = biu->command_status7;
        pStatus->arg8  = biu->command_status8;
        pStatus->arg9  = biu->command_status9;
        pStatus->arg10 = biu->command_status10;
        pStatus->arg11 = biu->command_status11;
        pStatus->arg12 = biu->command_status12;
        pStatus->arg13 = biu->command_status13;
        pStatus->arg14 = biu->command_status14;
        pStatus->arg15 = biu->command_status15;

        if(pStatus->return_status!=0)
                bcm_display("!!!!!!BCM Return ERROR, return_status=0x%x!!!!!\n", pStatus->return_status);
}

void bcm_clear_interrupts(unsigned int mask)
{
        volatile _BCM_MAILBOX * biu = (_BCM_MAILBOX *)devmem_phy_to_virt(BCM_MAILBOX_BASE);
#if 0
        biu->host_interrupt_register = mask; // 0xc8 Write-to-Clear
#else
        biu->host_interrupt_register = 0x70001;
#endif
}

void execute_bcm_command(BCM_MAILBOX_COMMAND *pCmd, BCM_MAILBOX_STATUS *pStatus)
{
        bcm_send_command(pCmd);
        bcm_wait_for_complete(0);
        bcm_get_status(pStatus);
        bcm_clear_interrupts(~0);
}

int bcm_wait_for_boot(void)
{
        volatile _BCM_MAILBOX * biu = (_BCM_MAILBOX *)devmem_phy_to_virt(BCM_MAILBOX_BASE);
        unsigned int intr_stat;
        unsigned int fifo_stat;

        while(1) {
                intr_stat = biu->host_interrupt_register; // 0xc8 Write-to-Clear
                fifo_stat = biu->command_fifo_status; // 0xc4 Read Only

                if( (intr_stat & 0x00010001) != 0) {
                        return -1;
                }

                if (fifo_stat & 0x100) {
                        bcm_display("\n=========================================");
                        bcm_display("BCM booted successfully. status=%0x", fifo_stat);
                        bcm_display("=========================================\n");
                        break;
                }
        }

        return 0;
}

int bcm_usb_console_func(const unsigned int primitive_id)
{
#ifndef USB_BOOT
	BCM_MAILBOX_COMMAND cmd;
	BCM_MAILBOX_STATUS status;

	bcm_wait_for_boot();
	bcm_clear_command(&cmd);
	bcm_clear_status(&status);
	cmd.command = primitive_id;
	execute_bcm_command(&cmd, &status);
	if(status.return_status != STATUS_SUCCESS)
		return -1;
#endif

	return 0;
}
//clear fields not used in cmd before call.
int bcm_usb_boot_func(BCM_MAILBOX_COMMAND* cmd)
{
	BCM_MAILBOX_STATUS status;

	bcm_clear_status(&status);
	execute_bcm_command(cmd, &status);
	if(status.return_status != STATUS_SUCCESS)
		return -1;

	return 0;
}
