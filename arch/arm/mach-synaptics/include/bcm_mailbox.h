/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */
#ifndef INCLUDE_BCM_MAILBOX_H
#define INCLUDE_BCM_MAILBOX_H

typedef struct _BCM_MAILBOX_COMMAND
{
   unsigned int arg0;
   unsigned int arg1;
   unsigned int arg2;
   unsigned int arg3;
   unsigned int arg4;
   unsigned int arg5;
   unsigned int arg6;
   unsigned int arg7;
   unsigned int arg8;
   unsigned int arg9;
   unsigned int arg10;
   unsigned int arg11;
   unsigned int arg12;
   unsigned int arg13;
   unsigned int arg14;
   unsigned int arg15;
   unsigned int command;
} BCM_MAILBOX_COMMAND;

typedef struct _BCM_MAILBOX_STATUS
{
   unsigned int return_status;
   unsigned int arg0;
   unsigned int arg1;
   unsigned int arg2;
   unsigned int arg3;
   unsigned int arg4;
   unsigned int arg5;
   unsigned int arg6;
   unsigned int arg7;
   unsigned int arg8;
   unsigned int arg9;
   unsigned int arg10;
   unsigned int arg11;
   unsigned int arg12;
   unsigned int arg13;
   unsigned int arg14;
   unsigned int arg15;
} BCM_MAILBOX_STATUS;

// the command mailbox structure, which appears in the Client's memory space
typedef struct _BCM_MAILBOX
{
   unsigned int command_parameter0;                  // 0x0 Write Only
   unsigned int command_parameter1;                  // 0x4 Write Only
   unsigned int command_parameter2;                  // 0x8 Write Only
   unsigned int command_parameter3;                  // 0xc Write Only
   unsigned int command_parameter4;                  // 0x10 Write Only
   unsigned int command_parameter5;                  // 0x14 Write Only
   unsigned int command_parameter6;                  // 0x18 Write Only
   unsigned int command_parameter7;                  // 0x1c Write Only
   unsigned int command_parameter8;                  // 0x20 Write Only
   unsigned int command_parameter9;                  // 0x24 Write Only
   unsigned int command_parameter10;                 // 0x28 Write Only
   unsigned int command_parameter11;                 // 0x2c Write Only
   unsigned int command_parameter12;                 // 0x30 Write Only
   unsigned int command_parameter13;                 // 0x34 Write Only
   unsigned int command_parameter14;                 // 0x38 Write Only: linked-list dma host-read pointer
   unsigned int command_parameter15;                 // 0x3c Write Only: linked-list dma host-write pointer
   unsigned int secure_processor_command;            // 0x40 Write Only
   unsigned int reserved_0x44[15];
   unsigned int command_return_status;               // 0x80 Read Only
   unsigned int command_status0;                     // 0x84 Read Only
   unsigned int command_status1;                     // 0x88 Read Only
   unsigned int command_status2;                     // 0x8c Read Only
   unsigned int command_status3;                     // 0x90 Read Only
   unsigned int command_status4;                     // 0x94 Read Only
   unsigned int command_status5;                     // 0x98 Read Only
   unsigned int command_status6;                     // 0x9c Read Only
   unsigned int command_status7;                     // 0xa0 Read Only
   unsigned int command_status8;                     // 0xa4 Read Only
   unsigned int command_status9;                     // 0xa8 Read Only
   unsigned int command_status10;                    // 0xac Read Only
   unsigned int command_status11;                    // 0xb0 Read Only
   unsigned int command_status12;                    // 0xb4 Read Only
   unsigned int command_status13;                    // 0xb8 Read Only
   unsigned int command_status14;                    // 0xbc Read Only
   unsigned int command_status15;                    // 0xc0 Read Only
   unsigned int command_fifo_status;                 // 0xc4 Read Only
   unsigned int host_interrupt_register;             // 0xc8 Write-to-Clear
   unsigned int host_interrupt_mask;                 // 0xcc Write-to-Clear
   unsigned int host_exception_address;              // 0xd0 Write-to-Clear
   unsigned int sp_trust_register;                   // 0xd4 Read Only
   unsigned int wtm_identification;                  // 0xd8 Read Only
   unsigned int wtm_revision;                        // 0xdc Read Only
} _BCM_MAILBOX;

// the command mailbox registers which appears in the Client's memory space
#ifndef MEMMAP_BCM_REG_BASE
#define MEMMAP_BCM_REG_BASE                 0xF7930000
#endif

#define BCM_MAILBOX_BASE                    (MEMMAP_BCM_REG_BASE)
#define BCM_PRIMITIVE_COMMAND_PARAMETER0    (BCM_MAILBOX_BASE+0x00)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER1    (BCM_MAILBOX_BASE+0x04)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER2    (BCM_MAILBOX_BASE+0x08)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER3    (BCM_MAILBOX_BASE+0x0c)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER4    (BCM_MAILBOX_BASE+0x10)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER5    (BCM_MAILBOX_BASE+0x14)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER6    (BCM_MAILBOX_BASE+0x18)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER7    (BCM_MAILBOX_BASE+0x1c)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER8    (BCM_MAILBOX_BASE+0x20)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER9    (BCM_MAILBOX_BASE+0x24)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER10   (BCM_MAILBOX_BASE+0x28)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER11   (BCM_MAILBOX_BASE+0x2c)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER12   (BCM_MAILBOX_BASE+0x30)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER13   (BCM_MAILBOX_BASE+0x34)  //Write Only
#define BCM_PRIMITIVE_COMMAND_PARAMETER14   (BCM_MAILBOX_BASE+0x38)  //Write Only: linked-list dma host-read pointer
#define BCM_PRIMITIVE_COMMAND_PARAMETER15   (BCM_MAILBOX_BASE+0x3c)  //Write Only: linked-list dma host-write pointer
#define BCM_SECURE_PROCESSOR_COMMAND        (BCM_MAILBOX_BASE+0x40)  //Write Only

#define BCM_COMMAND_RETURN_STATUS           (BCM_MAILBOX_BASE+0x80)  //Read Only
#define BCM_COMMAND_STATUS_0                (BCM_MAILBOX_BASE+0x84)  //Read Only
#define BCM_COMMAND_STATUS_1                (BCM_MAILBOX_BASE+0x88)  //Read Only
#define BCM_COMMAND_STATUS_2                (BCM_MAILBOX_BASE+0x8c)  //Read Only
#define BCM_COMMAND_STATUS_3                (BCM_MAILBOX_BASE+0x90)  //Read Only
#define BCM_COMMAND_STATUS_4                (BCM_MAILBOX_BASE+0x94)  //Read Only
#define BCM_COMMAND_STATUS_5                (BCM_MAILBOX_BASE+0x98)  //Read Only
#define BCM_COMMAND_STATUS_6                (BCM_MAILBOX_BASE+0x9c)  //Read Only
#define BCM_COMMAND_STATUS_7                (BCM_MAILBOX_BASE+0xa0)  //Read Only
#define BCM_COMMAND_STATUS_8                (BCM_MAILBOX_BASE+0xa4)  //Read Only
#define BCM_COMMAND_STATUS_9                (BCM_MAILBOX_BASE+0xa8)  //Read Only
#define BCM_COMMAND_STATUS_10               (BCM_MAILBOX_BASE+0xac)  //Read Only
#define BCM_COMMAND_STATUS_11               (BCM_MAILBOX_BASE+0xb0)  //Read Only
#define BCM_COMMAND_STATUS_12               (BCM_MAILBOX_BASE+0xb4)  //Read Only
#define BCM_COMMAND_STATUS_13               (BCM_MAILBOX_BASE+0xb8)  //Read Only
#define BCM_COMMAND_STATUS_14               (BCM_MAILBOX_BASE+0xbc)  //Read Only
#define BCM_COMMAND_STATUS_15               (BCM_MAILBOX_BASE+0xc0)  //Read Only
#define BCM_COMMAND_FIFO_STATUS             (BCM_MAILBOX_BASE+0xc4)  //Read Only

#define BCM_HOST_INTERRUPT_REGISTER         (BCM_MAILBOX_BASE+0xC8)  //Write to clear
#define BCM_HOST_INTERRUPT_MASK             (BCM_MAILBOX_BASE+0xcc)  //Write-to-Clear
#define BCM_HOST_EXCEPTION_ADDRESS          (BCM_MAILBOX_BASE+0xd0)  //Write-to-Clear
#define BCM_SP_TRUST_REGISTER               (BCM_MAILBOX_BASE+0xd4)  //Read Only
#define BCM_WTM_IDENTIFICATION              (BCM_MAILBOX_BASE+0xd8)  //Read Only
#define BCM_WTM_REVISION                    (BCM_MAILBOX_BASE+0xdc)  //Read Only

static inline void bcm_memclr(void *p, int numInt)
{
        volatile unsigned int *pStart = (unsigned int *)p;
        volatile unsigned int *pEnd = pStart + numInt;
        while (pStart < pEnd)
                *pStart++ = 0;
}

static inline void bcm_clear_command(BCM_MAILBOX_COMMAND * pCmd)
{
        bcm_memclr(pCmd, sizeof(BCM_MAILBOX_COMMAND)/sizeof(int));
}

static  inline void bcm_clear_status(BCM_MAILBOX_STATUS * pStatus)
{
        bcm_memclr(pStatus, sizeof(BCM_MAILBOX_STATUS)/sizeof(int));
}

void bcm_clear_interrupts(unsigned int mask);
void execute_bcm_command(BCM_MAILBOX_COMMAND *pCmd, BCM_MAILBOX_STATUS *pStatus);
int bcm_wait_for_boot(void);
int bcm_usb_console_func(const unsigned int primitive_id);
int bcm_usb_boot_func(BCM_MAILBOX_COMMAND* cmd);
int bcm_generate_random_number(unsigned char *buf, const unsigned int size);
#endif
