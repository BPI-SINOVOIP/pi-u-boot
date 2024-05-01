// SPDX-License-Identifier: GPL-2.0
/*
 * spacemit emac driver
 *
 * Copyright (C) 2023 Spacemit
 *
 */

#ifndef _SPACEMIT_K1X_EMAC_H_
#define _SPACEMIT_K1X_EMAC_H_

/* DMA register set */
#define DMA_CONFIGURATION                   0x0000
#define DMA_CONTROL                         0x0004
#define DMA_STATUS_IRQ                      0x0008
#define DMA_INTERRUPT_ENABLE                0x000C

#define DMA_TRANSMIT_AUTO_POLL_COUNTER      0x0010
#define DMA_TRANSMIT_POLL_DEMAND            0x0014
#define DMA_RECEIVE_POLL_DEMAND             0x0018

#define DMA_TRANSMIT_BASE_ADDRESS           0x001C
#define DMA_RECEIVE_BASE_ADDRESS            0x0020
#define DMA_MISSED_FRAME_COUNTER            0x0024
#define DMA_STOP_FLUSH_COUNTER              0x0028

#define DMA_CURRENT_TRANSMIT_DESCRIPTOR_POINTER     0x0030
#define DMA_CURRENT_TRANSMIT_BUFFER_POINTER         0x0034
#define DMA_CURRENT_RECEIVE_DESCRIPTOR_POINTER      0x0038
#define DMA_CURRENT_RECEIVE_BUFFER_POINTER          0x003C

/* MAC Register set */
#define MAC_GLOBAL_CONTROL                  0x0100
#define MAC_TRANSMIT_CONTROL                0x0104
#define MAC_RECEIVE_CONTROL                 0x0108
#define MAC_MAXIMUM_FRAME_SIZE              0x010C
#define MAC_TRANSMIT_JABBER_SIZE            0x0110
#define MAC_RECEIVE_JABBER_SIZE             0x0114
#define MAC_ADDRESS_CONTROL                 0x0118
#define MAC_ADDRESS1_HIGH                   0x0120
#define MAC_ADDRESS1_MED                    0x0124
#define MAC_ADDRESS1_LOW                    0x0128
#define MAC_ADDRESS2_HIGH                   0x012C
#define MAC_ADDRESS2_MED                    0x0130
#define MAC_ADDRESS2_LOW                    0x0134
#define MAC_ADDRESS3_HIGH                   0x0138
#define MAC_ADDRESS3_MED                    0x013C
#define MAC_ADDRESS3_LOW                    0x0140
#define MAC_ADDRESS4_HIGH                   0x0144
#define MAC_ADDRESS4_MED                    0x0148
#define MAC_ADDRESS4_LOW                    0x014C
#define MAC_MULTICAST_HASH_TABLE1           0x0150
#define MAC_MULTICAST_HASH_TABLE2           0x0154
#define MAC_MULTICAST_HASH_TABLE3           0x0158
#define MAC_MULTICAST_HASH_TABLE4           0x015C
#define MAC_FC_CONTROL                      0x0160
#define MAC_FC_PAUSE_FRAME_GENERATE         0x0164
#define MAC_FC_SOURCE_ADDRESS_HIGH          0x0168
#define MAC_FC_SOURCE_ADDRESS_MED           0x016C
#define MAC_FC_SOURCE_ADDRESS_LOW           0x0170
#define MAC_FC_DESTINATION_ADDRESS_HIGH     0x0174
#define MAC_FC_DESTINATION_ADDRESS_MED      0x0178
#define MAC_FC_DESTINATION_ADDRESS_LOW      0x017C
#define MAC_FC_PAUSE_TIME_VALUE             0x0180
#define MAC_MDIO_CONTROL                    0x01A0
#define MAC_MDIO_DATA                       0x01A4
#define MAC_RX_STATCTR_CONTROL              0x01A8
#define MAC_RX_STATCTR_DATA_HIGH            0x01AC
#define MAC_RX_STATCTR_DATA_LOW             0x01B0
#define MAC_TX_STATCTR_CONTROL              0x01B4
#define MAC_TX_STATCTR_DATA_HIGH            0x01B8
#define MAC_TX_STATCTR_DATA_LOW             0x01BC
#define MAC_TRANSMIT_FIFO_ALMOST_FULL       0x01C0
#define MAC_TRANSMIT_PACKET_START_THRESHOLD 0x01C4
#define MAC_RECEIVE_PACKET_START_THRESHOLD  0x01C8
#define MAC_STATUS_IRQ                      0x01E0
#define MAC_INTERRUPT_ENABLE                0x01E4

/* DMA_CONFIGURATION (0x0000) register bit info
 * 0-DMA controller in normal operation mode,
 * 1-DMA controller reset to default state,
 * clearing all internal state information
 */
#define MREGBIT_SOFTWARE_RESET              BIT(0)
#define MREGBIT_BURST_1WORD                 BIT(1)
#define MREGBIT_BURST_2WORD                 BIT(2)
#define MREGBIT_BURST_4WORD                 BIT(3)
#define MREGBIT_BURST_8WORD                 BIT(4)
#define MREGBIT_BURST_16WORD                BIT(5)
#define MREGBIT_BURST_32WORD                BIT(6)
#define MREGBIT_BURST_64WORD                BIT(7)
#define MREGBIT_BURST_LENGTH                GENMASK(7, 1)
#define MREGBIT_DESCRIPTOR_SKIP_LENGTH      GENMASK(12, 8)
/* For Receive and Transmit DMA operate in Big-Endian mode for Descriptors. */
#define MREGBIT_DESCRIPTOR_BYTE_ORDERING    BIT(13)
#define MREGBIT_BIG_LITLE_ENDIAN            BIT(14)
#define MREGBIT_TX_RX_ARBITRATION           BIT(15)
#define MREGBIT_WAIT_FOR_DONE               BIT(16)
#define MREGBIT_STRICT_BURST                BIT(17)
#define MREGBIT_DMA_64BIT_MODE              BIT(18)

/* DMA_CONTROL (0x0004) register bit info */
#define MREGBIT_START_STOP_TRANSMIT_DMA     BIT(0)
#define MREGBIT_START_STOP_RECEIVE_DMA      BIT(1)

/* DMA_STATUS_IRQ (0x0008) register bit info */
#define MREGBIT_TRANSMIT_TRANSFER_DONE_IRQ          BIT(0)
#define MREGBIT_TRANSMIT_DES_UNAVAILABLE_IRQ        BIT(1)
#define MREGBIT_TRANSMIT_DMA_STOPPED_IRQ            BIT(2)
#define MREGBIT_RECEIVE_TRANSFER_DONE_IRQ           BIT(4)
#define MREGBIT_RECEIVE_DES_UNAVAILABLE_IRQ         BIT(5)
#define MREGBIT_RECEIVE_DMA_STOPPED_IRQ             BIT(6)
#define MREGBIT_RECEIVE_MISSED_FRAME_IRQ            BIT(7)
#define MREGBIT_MAC_IRQ                             BIT(8)
#define MREGBIT_TRANSMIT_DMA_STATE                  GENMASK(18, 16)
#define MREGBIT_RECEIVE_DMA_STATE                   GENMASK(23, 20)

/* DMA_INTERRUPT_ENABLE ( 0x000C) register bit info */
#define MREGBIT_TRANSMIT_TRANSFER_DONE_INTR_ENABLE      BIT(0)
#define MREGBIT_TRANSMIT_DES_UNAVAILABLE_INTR_ENABLE    BIT(1)
#define MREGBIT_TRANSMIT_DMA_STOPPED_INTR_ENABLE        BIT(2)
#define MREGBIT_RECEIVE_TRANSFER_DONE_INTR_ENABLE       BIT(4)
#define MREGBIT_RECEIVE_DES_UNAVAILABLE_INTR_ENABLE     BIT(5)
#define MREGBIT_RECEIVE_DMA_STOPPED_INTR_ENABLE         BIT(6)
#define MREGBIT_RECEIVE_MISSED_FRAME_INTR_ENABLE        BIT(7)
#define MREGBIT_MAC_INTR_ENABLE                         BIT(8)

/* MAC_GLOBAL_CONTROL (0x0100) register bit info */
#define MREGBIT_SPEED                       GENMASK(1, 0)
#define MREGBIT_SPEED_10M                   0x0
#define MREGBIT_SPEED_100M                  BIT(0)
#define MREGBIT_SPEED_1000M                 BIT(1)
#define MREGBIT_FULL_DUPLEX_MODE            BIT(2)
#define MREGBIT_RESET_RX_STAT_COUNTERS      BIT(3)
#define MREGBIT_RESET_TX_STAT_COUNTERS      BIT(4)

/* MAC_TRANSMIT_CONTROL (0x0104) register bit info */
#define MREGBIT_TRANSMIT_ENABLE             BIT(0)
#define MREGBIT_INVERT_FCS                  BIT(1)
#define MREGBIT_DISABLE_FCS_INSERT          BIT(2)
#define MREGBIT_TRANSMIT_AUTO_RETRY         BIT(3)
#define MREGBIT_IFG_LEN                     GENMASK(6, 4)
#define MREGBIT_PREAMBLE_LENGTH             GENMASK(9, 7)

/* MAC_RECEIVE_CONTROL (0x0108) register bit info */
#define MREGBIT_RECEIVE_ENABLE              BIT(0)
#define MREGBIT_DISABLE_FCS_CHECK           BIT(1)
#define MREGBIT_STRIP_FCS                   BIT(2)
#define MREGBIT_STORE_FORWARD               BIT(3)
#define MREGBIT_STATUS_FIRST                BIT(4)
#define MREGBIT_PASS_BAD_FRAMES             BIT(5)
#define MREGBIT_ACOOUNT_VLAN                BIT(6)

/* MAC_MAXIMUM_FRAME_SIZE (0x010C) register bit info */
#define MREGBIT_MAX_FRAME_SIZE              GENMASK(13, 0)

/* MAC_TRANSMIT_JABBER_SIZE (0x0110) register bit info */
#define MREGBIT_TRANSMIT_JABBER_SIZE        GENMASK(15, 0)

/* MAC_RECEIVE_JABBER_SIZE (0x0114) register bit info */
#define MREGBIT_RECEIVE_JABBER_SIZE         GENMASK(15, 0)

/* MAC_ADDRESS_CONTROL     (0x0118) register bit info */
#define MREGBIT_MAC_ADDRESS1_ENABLE             BIT(0)
#define MREGBIT_MAC_ADDRESS2_ENABLE             BIT(1)
#define MREGBIT_MAC_ADDRESS3_ENABLE             BIT(2)
#define MREGBIT_MAC_ADDRESS4_ENABLE             BIT(3)
#define MREGBIT_INVERSE_MAC_ADDRESS1_ENABLE     BIT(4)
#define MREGBIT_INVERSE_MAC_ADDRESS2_ENABLE     BIT(5)
#define MREGBIT_INVERSE_MAC_ADDRESS3_ENABLE     BIT(6)
#define MREGBIT_INVERSE_MAC_ADDRESS4_ENABLE     BIT(7)
#define MREGBIT_PROMISCUOUS_MODE                BIT(8)

/* MAC_ADDRESSx_HIGH (0x0120) register bit info */
#define MREGBIT_MAC_ADDRESS1_01_BYTE            GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS1_02_BYTE            GENMASK(15, 8)
/* MAC_ADDRESSx_MED (0x0124) register bit info */
#define MREGBIT_MAC_ADDRESS1_03_BYTE            GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS1_04_BYTE            GENMASK(15, 8)
/* MAC_ADDRESSx_LOW (0x0128) register bit info */
#define MREGBIT_MAC_ADDRESS1_05_BYTE            GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS1_06_BYTE            GENMASK(15, 8)

/* MAC_FC_CONTROL (0x0160) register bit info */
#define MREGBIT_FC_DECODE_ENABLE                BIT(0)
#define MREGBIT_FC_GENERATION_ENABLE            BIT(1)
#define MREGBIT_AUTO_FC_GENERATION_ENABLE       BIT(2)
#define MREGBIT_MULTICAST_MODE                  BIT(3)
#define MREGBIT_BLOCK_PAUSE_FRAMES              BIT(4)

/* MAC_FC_PAUSE_FRAME_GENERATE (0x0164) register bit info */
#define MREGBIT_GENERATE_PAUSE_FRAME            BIT(0)

/* MAC_FC_SRC/DST_ADDRESS_HIGH (0x0168) register bit info */
#define MREGBIT_MAC_ADDRESS_01_BYTE             GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS_02_BYTE             GENMASK(15, 8)
/* MAC_FC_SRC/DST_ADDRESS_MED (0x016C) register bit info */
#define MREGBIT_MAC_ADDRESS_03_BYTE             GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS_04_BYTE             GENMASK(15, 8)
/* MAC_FC_SRC/DSTD_ADDRESS_LOW (0x0170) register bit info */
#define MREGBIT_MAC_ADDRESS_05_BYTE             GENMASK(7, 0)
#define MREGBIT_MAC_ADDRESS_06_BYTE             GENMASK(15, 8)

/* MAC_FC_PAUSE_TIME_VALUE (0x0180) register bit info */
#define MREGBIT_MAC_FC_PAUSE_TIME               GENMASK(15, 0)

/* MAC_MDIO_CONTROL (0x01A0) register bit info */
#define MREGBIT_PHY_ADDRESS                     GENMASK(4, 0)
#define MREGBIT_REGISTER_ADDRESS                GENMASK(9, 5)
#define MREGBIT_MDIO_READ_WRITE                 BIT(10)
#define MREGBIT_START_MDIO_TRANS                BIT(15)

/* MAC_MDIO_DATA (0x01A4) register bit info */
#define MREGBIT_MDIO_DATA                       GENMASK(15, 0)

/* MAC_RX_STATCTR_CONTROL (0x01A8) register bit info */
#define MREGBIT_RX_COUNTER_NUMBER               GENMASK(4, 0)
#define MREGBIT_START_RX_COUNTER_READ           BIT(15)

/* MAC_RX_STATCTR_DATA_HIGH (0x01AC) register bit info */
#define MREGBIT_RX_STATCTR_DATA_HIGH            GENMASK(15, 0)
/* MAC_RX_STATCTR_DATA_LOW (0x01B0) register bit info */
#define MREGBIT_RX_STATCTR_DATA_LOW             GENMASK(15, 0)

/* MAC_TX_STATCTR_CONTROL (0x01B4) register bit info */
#define MREGBIT_TX_COUNTER_NUMBER               GENMASK(4, 0)
#define MREGBIT_START_TX_COUNTER_READ           BIT(15)

/* MAC_TX_STATCTR_DATA_HIGH (0x01B8) register bit info */
#define MREGBIT_TX_STATCTR_DATA_HIGH            GENMASK(15, 0)
/* MAC_TX_STATCTR_DATA_LOW (0x01BC) register bit info */
#define MREGBIT_TX_STATCTR_DATA_LOW             GENMASK(15, 0)

/* MAC_TRANSMIT_FIFO_ALMOST_FULL (0x01C0) register bit info */
#define MREGBIT_TX_FIFO_AF                      GENMASK(13, 0)

/* MAC_TRANSMIT_PACKET_START_THRESHOLD (0x01C4) register bit info */
#define MREGBIT_TX_PACKET_START_THRESHOLD       GENMASK(13, 0)

/* MAC_RECEIVE_PACKET_START_THRESHOLD (0x01C8) register bit info */
#define MREGBIT_RX_PACKET_START_THRESHOLD       GENMASK(13, 0)

/* MAC_STATUS_IRQ  (0x01E0) register bit info */
#define MREGBIT_MAC_UNDERRUN_IRQ                BIT(0)
#define MREGBIT_MAC_JABBER_IRQ                  BIT(1)

/* MAC_INTERRUPT_ENABLE (0x01E4) register bit info */
#define MREGBIT_MAC_UNDERRUN_INTERRUPT_ENABLE   BIT(0)
#define MREGBIT_JABBER_INTERRUPT_ENABLE         BIT(1)

/* Receive Descriptors */
/* MAC_RECEIVE_DESCRIPTOR0 () register bit info */
#define MREGBIT_FRAME_LENGTH                    GENMASK(13, 0)
#define MREGBIT_APPLICATION_STATUS              GENMASK(28, 14)
#define MREGBIT_LAST_DESCRIPTOR                 BIT(29)
#define MREGBIT_FIRST_DESCRIPTOR                BIT(30)
#define MREGBIT_OWN_BIT                         BIT(31)

/* MAC_RECEIVE_DESCRIPTOR1 () register bit info */
#define MREGBIT_BUFFER1_SIZE                    GENMASK(11, 0)
#define MREGBIT_BUFFER2_SIZE                    GENMASK(23, 12)
#define MREGBIT_SECOND_ADDRESS_CHAINED          BIT(25)
#define MREGBIT_END_OF_RING                     BIT(26)

/* MAC_RECEIVE_DESCRIPTOR2 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1                 GENMASK(31, 0)

/* MAC_RECEIVE_DESCRIPTOR3 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1                 GENMASK(31, 0)

/* Transmit Descriptors */
/* TD_TRANSMIT_DESCRIPTOR0 () register bit info */
#define MREGBIT_TX_PACKET_STATUS                GENMASK(29, 0)
#define MREGBIT_OWN_BIT                         BIT(31)

/* TD_TRANSMIT_DESCRIPTOR1 () register bit info */
#define MREGBIT_BUFFER1_SIZE                    GENMASK(11, 0)
#define MREGBIT_BUFFER2_SIZE                    GENMASK(23, 12)
#define MREGBIT_FORCE_EOP_ERROR                 BIT(24)
#define MREGBIT_SECOND_ADDRESS_CHAINED          BIT(25)
#define MREGBIT_END_OF_RING                     BIT(26)
#define MREGBIT_DISABLE_PADDING                 BIT(27)
#define MREGBIT_ADD_CRC_DISABLE                 BIT(28)
#define MREGBIT_FIRST_SEGMENT                   BIT(29)
#define MREGBIT_LAST_SEGMENT                    BIT(30)
#define MREGBIT_INTERRUPT_ON_COMPLETION         BIT(31)

/* TD_TRANSMIT_DESCRIPTOR2 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1                 GENMASK(31, 0)

/* TD_TRANSMIT_DESCRIPTOR3 () register bit info */
#define MREGBIT_BUFFER_ADDRESS1                 GENMASK(31, 0)

#define EMAC_RX_BUFFER_1024                 1024
#define EMAC_RX_BUFFER_2048                 2048
#define EMAC_RX_BUFFER_4096                 4096

#define MAX_DATA_PWR_TX_DES                 11
#define MAX_DATA_LEN_TX_DES                 2048 //2048=1<<11

#define MAX_TX_STATS_NUM                    12
#define MAX_RX_STATS_NUM                    25

/* The sizes (in bytes) of a ethernet packet */
#define ETHERNET_HEADER_SIZE                14
#define MAXIMUM_ETHERNET_FRAME_SIZE         1518  //With FCS
#define MINIMUM_ETHERNET_FRAME_SIZE         64  //With FCS
#define ETHERNET_FCS_SIZE                   4
#define MAXIMUM_ETHERNET_PACKET_SIZE \
        (MAXIMUM_ETHERNET_FRAME_SIZE - ETHERNET_FCS_SIZE)

#define MINIMUM_ETHERNET_PACKET_SIZE \
        (MINIMUM_ETHERNET_FRAME_SIZE - ETHERNET_FCS_SIZE)

#define CRC_LENGTH                  ETHERNET_FCS_SIZE
#define MAX_JUMBO_FRAME_SIZE        0x3F00

#define TX_STORE_FORWARD_MODE       0x5EE

/* only works for sizes that are powers of 2 */
#define EMAC_ROUNDUP(i, size) ((i) = (((i) + (size) - 1) & ~((size) - 1)))

/* number of descriptors are required for len */
#define EMAC_TXD_COUNT(S, X) (((S) >> (X)) + 1)

/* calculate the number of descriptors unused */
#define EMAC_DESC_UNUSED(R) \
    ((((R)->nxt_clean > (R)->nxt_use) ? 0 : (R)->total_cnt) + \
    (R)->nxt_clean - (R)->nxt_use - 1)


#endif //_SPACEMIT_K1X_EMAC_H_
