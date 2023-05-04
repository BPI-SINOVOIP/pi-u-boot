//  **********************************************************************
//  *                               NOTICE                               *
//  *                                                                    *
//  *       COPYRIGHT MARVELL INTERNATIONAL LTD. AND ITS AFFILIATES      *
//  *                        ALL RIGHTS RESERVED                         *
//  *                                                                    *
//  * The source code for this computer program is  CONFIDENTIAL  and a  *
//  * TRADE SECRET of MARVELL  INTERNATIONAL  LTD. AND  ITS  AFFILIATES  *
//  * ("MARVELL"). The receipt or possession of  this  program does not  *
//  * convey any rights to  reproduce or  disclose  its contents, or to  *
//  * manufacture,  use, or  sell  anything  that it  may  describe, in  *
//  * whole or in part, without the specific written consent of MARVELL. *
//  * Any  reproduction  or  distribution  of this  program without the  *
//  * express written consent of MARVELL is a violation of the copyright *
//  * laws and may subject you to criminal prosecution.                  *
//  *                                                                    *
//  **********************************************************************

#ifndef BCMREG_H
#define BCMREG_H

typedef unsigned int BCM_U32;
typedef unsigned char BCM_U8;

// the command mailbox, which appears in the Client's memory space
typedef struct _NOT_MAILBOX
{
    BCM_U32 primitive_command_parameter0;                  // 0x0 Write Only
    BCM_U32 primitive_command_parameter1;                  // 0x4 Write Only
    BCM_U32 primitive_command_parameter2;                  // 0x8 Write Only
    BCM_U32 primitive_command_parameter3;                  // 0xc Write Only
    BCM_U32 primitive_command_parameter4;                  // 0x10 Write Only
    BCM_U32 primitive_command_parameter5;                  // 0x14 Write Only
    BCM_U32 primitive_command_parameter6;                  // 0x18 Write Only
    BCM_U32 primitive_command_parameter7;                  // 0x1c Write Only
    BCM_U32 primitive_command_parameter8;                  // 0x20 Write Only
    BCM_U32 primitive_command_parameter9;                  // 0x24 Write Only
    BCM_U32 primitive_command_paramete10;                  // 0x28 Write Only
    BCM_U32 primitive_command_parameter11;                 // 0x2c Write Only
    BCM_U32 primitive_command_parameter12;                 // 0x30 Write Only
    BCM_U32 primitive_command_parameter13;                 // 0x34 Write Only
    BCM_U32 primitive_command_parameter14;                 // 0x38 Write Only: linked-list dma host-read pointer
    BCM_U32 primitive_command_parameter15;                 // 0x3c Write Only: linked-list dma host-write pointer
    BCM_U32 secure_processor_command;                      // 0x40 Write Only
    BCM_U8  reserved_0x44[60];
    BCM_U32 command_return_status;                         // 0x80 Read Only
    BCM_U32 command_status_0;                              // 0x84 Read Only
    BCM_U32 command_status_1;                              // 0x88 Read Only
    BCM_U32 command_status_2;                              // 0x8c Read Only
    BCM_U32 command_status_3;                              // 0x90 Read Only
    BCM_U32 command_status_4;                              // 0x94 Read Only
    BCM_U32 command_status_5;                              // 0x98 Read Only
    BCM_U32 command_status_6;                              // 0x9c Read Only
    BCM_U32 command_status_7;                              // 0xa0 Read Only
    BCM_U32 command_status_8;                              // 0xa4 Read Only
    BCM_U32 command_status_9;                              // 0xa8 Read Only
    BCM_U32 command_status_10;                             // 0xac Read Only
    BCM_U32 command_status_11;                             // 0xb0 Read Only
    BCM_U32 command_status_12;                             // 0xb4 Read Only
    BCM_U32 command_status_13;                             // 0xb8 Read Only
    BCM_U32 command_status_14;                             // 0xbc Read Only
    BCM_U32 command_status_15;                             // 0xc0 Read Only
    BCM_U32 command_fifo_status;                           // 0xc4 Read Only
    BCM_U32 host_interrupt_register;                       // 0xc8 Write-to-Clear
    BCM_U32 host_interrupt_mask;                           // 0xcc Write-to-Clear
    BCM_U32 host_exception_address;                        // 0xd0 Write-to-Clear
    BCM_U32 sp_trust_register;                             // 0xd4 Read Only
    BCM_U32 wtm_identification;                            // 0xd8 Read Only
    BCM_U32 wtm_revision;                                  // 0xdc Read Only
} NOT_MAILBOX;


typedef struct
{
    BCM_U32 command_parameter_0;                           // 0x0
    BCM_U32 command_parameter_1;                           // 0x4
    BCM_U32 command_parameter_2;                           // 0x8
    BCM_U32 command_parameter_3;                           // 0xc
    BCM_U32 command_parameter_4;                           // 0x10
    BCM_U32 command_parameter_5;                           // 0x14
    BCM_U32 command_parameter_6;                           // 0x18
    BCM_U32 command_parameter_7;                           // 0x1c
    BCM_U32 command_parameter_8;                           // 0x20
    BCM_U32 command_parameter_9;                           // 0x24
    BCM_U32 command_parameter_10;                          // 0x28
    BCM_U32 command_parameter_11;                          // 0x2c
    BCM_U32 command_parameter_12;                          // 0x30
    BCM_U32 command_parameter_13;                          // 0x34
    BCM_U32 command_parameter_14;                          // 0x38
    BCM_U32 command_parameter_15;                          // 0x3c
    BCM_U32 secure_processor_command;                      // 0x40
    BCM_U8  reserved_0x44[60];
    BCM_U32 command_return_status;                         // 0x80
    BCM_U32 command_status_0;                              // 0x84
    BCM_U32 command_status_1;                              // 0x88
    BCM_U32 command_status_2;                              // 0x8c
    BCM_U32 command_status_3;                              // 0x90
    BCM_U32 command_status_4;                              // 0x94
    BCM_U32 command_status_5;                              // 0x98
    BCM_U32 command_status_6;                              // 0x9c
    BCM_U32 command_status_7;                              // 0xa0
    BCM_U32 command_status_8;                              // 0xa4
    BCM_U32 command_status_9;                              // 0xa8
    BCM_U32 command_status_10;                             // 0xac
    BCM_U32 command_status_11;                             // 0xb0
    BCM_U32 command_status_12;                             // 0xb4
    BCM_U32 command_status_13;                             // 0xb8
    BCM_U32 command_status_14;                             // 0xbc
    BCM_U32 command_status_15;                             // 0xc0
    BCM_U32 command_fifo_status;                           // 0xc4
    BCM_U32 host_interrupt_reset;                          // 0xc8
    BCM_U32 host_interrupt_mask;                           // 0xcc
    BCM_U32 host_exception_address;                        // 0xd0
    BCM_U32 sp_trust_register;                             // 0xd4
    BCM_U32 wtm_identification;                            // 0xd8
    BCM_U32 wtm_revision;                                  // 0xdc
    BCM_U32 context_status;                                // 0xe0
    BCM_U8  reserved_0xe4[300];
    BCM_U32 sp_interrupt_set;                              // 0x210
    BCM_U32 sp_sram_control;                               // 0x214
    BCM_U32 sp_interrupt_reset;                            // 0x218
    BCM_U32 sp_interrupt_mask;                             // 0x21c
    BCM_U32 sp_control;                                    // 0x220
    BCM_U32 sp_exception_address;                          // 0x224
    BCM_U32 sp_exception_data;                             // 0x228
    BCM_U32 sp_exception_parity;                           // 0x22c
    BCM_U8  reserved_0x230[4];
    BCM_U32 host_interrupt_set;                            // 0x234
    BCM_U8  reserved_0x238[16];
    BCM_U32 trust_level;                                   // 0x248
    BCM_U8  reserved_0x24c[436];
} BCM_BIU; // Space: 1024 bytes

#endif
