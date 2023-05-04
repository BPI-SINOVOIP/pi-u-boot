/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef _NFC_REG_
#define _NFC_REG_

#define RA__CMD_REG0                                        0x0000
#define BA__CMD_REG0__cmd0                                  0x0000
#define MSK__CMD_REG0__cmd0                                 0xFFFFFFFF
#define LSb__CMD_REG0__cmd0                                 0

#define RA__CMD_REG1                                        0x0004
#define BA__CMD_REG1__cmd1                                  0x0004
#define MSK__CMD_REG1__cmd1                                 0xFFFFFFFF
#define LSb__CMD_REG1__cmd1                                 0

#define RA__CMD_REG2                                        0x0008
#define BA__CMD_REG2__cmd2                                  0x0008
#define MSK__CMD_REG2__cmd2                                 0xFFFFFFFF
#define LSb__CMD_REG2__cmd2                                 0

#define RA__CMD_REG3                                        0x000c
#define BA__CMD_REG3__cmd3                                  0x000c
#define MSK__CMD_REG3__cmd3                                 0xFFFFFFFF
#define LSb__CMD_REG3__cmd3                                 0

#define RA__CMD_STATUS_PTR                                  0x0010
#define BA__CMD_STATUS_PTR__thrd_status_sel                 0x0010
#define MSK__CMD_STATUS_PTR__thrd_status_sel                0x7
#define LSb__CMD_STATUS_PTR__thrd_status_sel                0

#define RA__CMD_STATUS                                      0x0014
#define BA__CMD_STATUS__cmd_status                          0x0014
#define MSK__CMD_STATUS__cmd_status                         0xFFFFFFFF
#define LSb__CMD_STATUS__cmd_status                         0

#define RA__INTR_STATUS                                     0x0110
#define BA__INTR_STATUS__sdma_err                           0x0110
#define MSK__INTR_STATUS__sdma_err                          0x1
#define LSb__INTR_STATUS__sdma_err                          22
#define BA__INTR_STATUS__sdma_trigg                         0x0110
#define MSK__INTR_STATUS__sdma_trigg                        0x1
#define LSb__INTR_STATUS__sdma_trigg                        21
#define BA__INTR_STATUS__cmd_ignored                        0x0110
#define MSK__INTR_STATUS__cmd_ignored                       0x1
#define LSb__INTR_STATUS__cmd_ignored                       20
#define BA__INTR_STATUS__ddma_terr                          0x0110
#define MSK__INTR_STATUS__ddma_terr                         0x1
#define LSb__INTR_STATUS__ddma_terr                         18
#define BA__INTR_STATUS__cdma_terr                          0x0110
#define MSK__INTR_STATUS__cdma_terr                         0x1
#define LSb__INTR_STATUS__cdma_terr                         17
#define BA__INTR_STATUS__cdma_idle                          0x0110
#define MSK__INTR_STATUS__cdma_idle                         0x1
#define LSb__INTR_STATUS__cdma_idle                         16

#define RA__INTR_ENABLE                                     0x0114
#define BA__INTR_ENABLE__intr_en                            0x0114
#define MSK__INTR_ENABLE__intr_en                           0x1
#define LSb__INTR_ENABLE__intr_en                           31
#define BA__INTR_ENABLE__sdma_err_en                        0x0114
#define MSK__INTR_ENABLE__sdma_err_en                       0x1
#define LSb__INTR_ENABLE__sdma_err_en                       22
#define BA__INTR_ENABLE__sdma_trigg_en                      0x0114
#define MSK__INTR_ENABLE__sdma_trigg_en                     0x1
#define LSb__INTR_ENABLE__sdma_trigg_en                     21
#define BA__INTR_ENABLE__cmd_ignored_en                     0x0114
#define MSK__INTR_ENABLE__cmd_ignored_en                    0x1
#define LSb__INTR_ENABLE__cmd_ignored_en                    20
#define BA__INTR_ENABLE__ddma_terr_en                       0x0114
#define MSK__INTR_ENABLE__ddma_terr_en                      0x1
#define LSb__INTR_ENABLE__ddma_terr_en                      18
#define BA__INTR_ENABLE__cdma_terr_en                       0x0114
#define MSK__INTR_ENABLE__cdma_terr_en                      0x1
#define LSb__INTR_ENABLE__cdma_terr_en                      17
#define BA__INTR_ENABLE__cdma_idle_en                       0x0114
#define MSK__INTR_ENABLE__cdma_idle_en                      0x1
#define LSb__INTR_ENABLE__cdma_idle_en                      16

#define RA__CTRL_STATUS                                     0x0118
#define BA__CTRL_STATUS__sdma_paused                        0x0118
#define MSK__CTRL_STATUS__sdma_paused                       0x1
#define LSb__CTRL_STATUS__sdma_paused                       16
#define BA__CTRL_STATUS__init_fail                          0x0118
#define MSK__CTRL_STATUS__init_fail                         0x1
#define LSb__CTRL_STATUS__init_fail                         10
#define BA__CTRL_STATUS__init_comp                          0x0118
#define MSK__CTRL_STATUS__init_comp                         0x1
#define LSb__CTRL_STATUS__init_comp                         9
#define BA__CTRL_STATUS__ctrl_busy                          0x0118
#define MSK__CTRL_STATUS__ctrl_busy                         0x1
#define LSb__CTRL_STATUS__ctrl_busy                         8
#define BA__CTRL_STATUS__mc_busy                            0x0118
#define MSK__CTRL_STATUS__mc_busy                           0x1
#define LSb__CTRL_STATUS__mc_busy                           3
#define BA__CTRL_STATUS__cmd_eng_busy                       0x0118
#define MSK__CTRL_STATUS__cmd_eng_busy                      0x1
#define LSb__CTRL_STATUS__cmd_eng_busy                      2
#define BA__CTRL_STATUS__mdma_busy                          0x0118
#define MSK__CTRL_STATUS__mdma_busy                         0x1
#define LSb__CTRL_STATUS__mdma_busy                         1
#define BA__CTRL_STATUS__sdma_busy                          0x0118
#define MSK__CTRL_STATUS__sdma_busy                         0x1
#define LSb__CTRL_STATUS__sdma_busy                         0

#define RA__TRD_STATUS                                      0x0120
#define BA__TRD_STATUS__trd_busy                            0x0120
#define MSK__TRD_STATUS__trd_busy                           0xFF
#define LSb__TRD_STATUS__trd_busy                           0

#define RA__TRD_ERROR_INTR_STATUS                           0x0128
#define BA__TRD_ERROR_INTR_STATUS__trd7_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd7_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd7_error_stat         7
#define BA__TRD_ERROR_INTR_STATUS__trd6_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd6_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd6_error_stat         6
#define BA__TRD_ERROR_INTR_STATUS__trd5_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd5_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd5_error_stat         5
#define BA__TRD_ERROR_INTR_STATUS__trd4_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd4_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd4_error_stat         4
#define BA__TRD_ERROR_INTR_STATUS__trd3_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd3_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd3_error_stat         3
#define BA__TRD_ERROR_INTR_STATUS__trd2_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd2_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd2_error_stat         2
#define BA__TRD_ERROR_INTR_STATUS__trd1_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd1_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd1_error_stat         1
#define BA__TRD_ERROR_INTR_STATUS__trd0_error_stat          0x0128
#define MSK__TRD_ERROR_INTR_STATUS__trd0_error_stat         0x1
#define LSb__TRD_ERROR_INTR_STATUS__trd0_error_stat         0

#define RA__TRD_ERROR_INTR_EN                               0x0130
#define BA__TRD_ERROR_INTR_EN__trd_error_intr_en            0x0130
#define MSK__TRD_ERROR_INTR_EN__trd_error_intr_en           0xFF
#define LSb__TRD_ERROR_INTR_EN__trd_error_intr_en           0

#define RA__TRD_COMP_INTR_STATUS                            0x0138
#define BA__TRD_COMP_INTR_STATUS__trd7_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd7_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd7_comp                7
#define BA__TRD_COMP_INTR_STATUS__trd6_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd6_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd6_comp                6
#define BA__TRD_COMP_INTR_STATUS__trd5_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd5_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd5_comp                5
#define BA__TRD_COMP_INTR_STATUS__trd4_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd4_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd4_comp                4
#define BA__TRD_COMP_INTR_STATUS__trd3_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd3_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd3_comp                3
#define BA__TRD_COMP_INTR_STATUS__trd2_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd2_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd2_comp                2
#define BA__TRD_COMP_INTR_STATUS__trd1_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd1_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd1_comp                1
#define BA__TRD_COMP_INTR_STATUS__trd0_comp                 0x0138
#define MSK__TRD_COMP_INTR_STATUS__trd0_comp                0x1
#define LSb__TRD_COMP_INTR_STATUS__trd0_comp                0

#define RA__DMA_TARGET_ERROR_L                              0x0140
#define BA__DMA_TARGET_ERROR_L__target_err_l                0x0140
#define MSK__DMA_TARGET_ERROR_L__target_err_l               0xFFFFFFFF
#define LSb__DMA_TARGET_ERROR_L__target_err_l               0

#define RA__TRD_TIMEOUT_INTR_STATUS                         0x014c
#define BA__TRD_TIMEOUT_INTR_STATUS__trd7_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd7_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd7_timeout_stat     7
#define BA__TRD_TIMEOUT_INTR_STATUS__trd6_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd6_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd6_timeout_stat     6
#define BA__TRD_TIMEOUT_INTR_STATUS__trd5_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd5_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd5_timeout_stat     5
#define BA__TRD_TIMEOUT_INTR_STATUS__trd4_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd4_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd4_timeout_stat     4
#define BA__TRD_TIMEOUT_INTR_STATUS__trd3_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd3_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd3_timeout_stat     3
#define BA__TRD_TIMEOUT_INTR_STATUS__trd2_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd2_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd2_timeout_stat     2
#define BA__TRD_TIMEOUT_INTR_STATUS__trd1_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd1_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd1_timeout_stat     1
#define BA__TRD_TIMEOUT_INTR_STATUS__trd0_timeout_stat      0x014c
#define MSK__TRD_TIMEOUT_INTR_STATUS__trd0_timeout_stat     0x1
#define LSb__TRD_TIMEOUT_INTR_STATUS__trd0_timeout_stat     0

#define RA__TRD_TIMEOUT_INTR_EN                             0x0154
#define BA__TRD_TIMEOUT_INTR_EN__trd_timeout_intr_en        0x0154
#define MSK__TRD_TIMEOUT_INTR_EN__trd_timeout_intr_en       0xFF
#define LSb__TRD_TIMEOUT_INTR_EN__trd_timeout_intr_en       0

#define RA__TRANSFER_CFG_0                                  0x0400
#define BA__TRANSFER_CFG_0__offset                          0x0400
#define MSK__TRANSFER_CFG_0__offset                         0xFFFF
#define LSb__TRANSFER_CFG_0__offset                         16
#define BA__TRANSFER_CFG_0__sector_cnt                      0x0400
#define MSK__TRANSFER_CFG_0__sector_cnt                     0xFF
#define LSb__TRANSFER_CFG_0__sector_cnt                     0

#define RA__TRANSFER_CFG_1                                  0x0404
#define BA__TRANSFER_CFG_1__last_sector_size                0x0404
#define MSK__TRANSFER_CFG_1__last_sector_size               0xFFFF
#define LSb__TRANSFER_CFG_1__last_sector_size               16
#define BA__TRANSFER_CFG_1__sector_size                     0x0404
#define MSK__TRANSFER_CFG_1__sector_size                    0xFFFF
#define LSb__TRANSFER_CFG_1__sector_size                    0

#define RA__LONG_POLLING                                    0x0408
#define BA__LONG_POLLING__long_polling                      0x0408
#define MSK__LONG_POLLING__long_polling                     0xFFFF
#define LSb__LONG_POLLING__long_polling                     0

#define RA__SHORT_POLLING                                   0x040c
#define BA__SHORT_POLLING__short_polling                    0x040c
#define MSK__SHORT_POLLING__short_polling                   0xFFFF
#define LSb__SHORT_POLLING__short_polling                   0

#define RA__RDST_CTRL_0                                     0x0410
#define BA__RDST_CTRL_0__ready_mask                         0x0410
#define MSK__RDST_CTRL_0__ready_mask                        0xFF
#define LSb__RDST_CTRL_0__ready_mask                        24
#define BA__RDST_CTRL_0__ready_value                        0x0410
#define MSK__RDST_CTRL_0__ready_value                       0xFF
#define LSb__RDST_CTRL_0__ready_value                       16
#define BA__RDST_CTRL_0__rb_enable                          0x0410
#define MSK__RDST_CTRL_0__rb_enable                         0x1
#define LSb__RDST_CTRL_0__rb_enable                         0

#define RA__RDST_CTRL_1                                     0x0414
#define BA__RDST_CTRL_1__error_mask                         0x0414
#define MSK__RDST_CTRL_1__error_mask                        0xFF
#define LSb__RDST_CTRL_1__error_mask                        24
#define BA__RDST_CTRL_1__error_value                        0x0414
#define MSK__RDST_CTRL_1__error_value                       0xFF
#define LSb__RDST_CTRL_1__error_value                       16

#define RA__LUN_STATUS_CMD                                  0x0418
#define BA__LUN_STATUS_CMD__lun_stat_sel                    0x0418
#define MSK__LUN_STATUS_CMD__lun_stat_sel                   0x1
#define LSb__LUN_STATUS_CMD__lun_stat_sel                   0

#define RA__LUN_INTERLEAVED_CMD                             0x041c
#define BA__LUN_INTERLEAVED_CMD__program_after_read         0x041c
#define MSK__LUN_INTERLEAVED_CMD__program_after_read        0x1
#define LSb__LUN_INTERLEAVED_CMD__program_after_read        6
#define BA__LUN_INTERLEAVED_CMD__lun_col_cmd                0x041c
#define MSK__LUN_INTERLEAVED_CMD__lun_col_cmd               0x3
#define LSb__LUN_INTERLEAVED_CMD__lun_col_cmd               0

#define RA__LUN_ADDR_OFFSET                                 0x0420
#define BA__LUN_ADDR_OFFSET__lun_addr_offset                0x0420
#define MSK__LUN_ADDR_OFFSET__lun_addr_offset               0x1F
#define LSb__LUN_ADDR_OFFSET__lun_addr_offset               0

#define RA__NF_DEV_LAYOUT                                   0x0424
#define BA__NF_DEV_LAYOUT__blk_addr_idx                     0x0424
#define MSK__NF_DEV_LAYOUT__blk_addr_idx                    0x1F
#define LSb__NF_DEV_LAYOUT__blk_addr_idx                    27
#define BA__NF_DEV_LAYOUT__LN                               0x0424
#define MSK__NF_DEV_LAYOUT__LN                              0xF
#define LSb__NF_DEV_LAYOUT__LN                              20
#define BA__NF_DEV_LAYOUT__lun_en                           0x0424
#define MSK__NF_DEV_LAYOUT__lun_en                          0x1
#define LSb__NF_DEV_LAYOUT__lun_en                          16
#define BA__NF_DEV_LAYOUT__PPB                              0x0424
#define MSK__NF_DEV_LAYOUT__PPB                             0xFFFF
#define LSb__NF_DEV_LAYOUT__PPB                             0

#define RA__ECC_CONFIG_0                                    0x0428
#define BA__ECC_CONFIG_0__corr_str                          0x0428
#define MSK__ECC_CONFIG_0__corr_str                         0x7
#define LSb__ECC_CONFIG_0__corr_str                         8
#define BA__ECC_CONFIG_0__scrambler_en                      0x0428
#define MSK__ECC_CONFIG_0__scrambler_en                     0x1
#define LSb__ECC_CONFIG_0__scrambler_en                     4
#define BA__ECC_CONFIG_0__erase_det_en                      0x0428
#define MSK__ECC_CONFIG_0__erase_det_en                     0x1
#define LSb__ECC_CONFIG_0__erase_det_en                     1
#define BA__ECC_CONFIG_0__ecc_enable                        0x0428
#define MSK__ECC_CONFIG_0__ecc_enable                       0x1
#define LSb__ECC_CONFIG_0__ecc_enable                       0

#define RA__ECC_CONFIG_1                                    0x042c
#define BA__ECC_CONFIG_1__erase_det_lvl                     0x042c
#define MSK__ECC_CONFIG_1__erase_det_lvl                    0xFF
#define LSb__ECC_CONFIG_1__erase_det_lvl                    0

#define RA__DEVICE_CTRL                                     0x0430
#define BA__DEVICE_CTRL__four_addr_seq_en                   0x0430
#define MSK__DEVICE_CTRL__four_addr_seq_en                  0x1
#define LSb__DEVICE_CTRL__four_addr_seq_en                  7
#define BA__DEVICE_CTRL__chrc_wdth                          0x0430
#define MSK__DEVICE_CTRL__chrc_wdth                         0x1
#define LSb__DEVICE_CTRL__chrc_wdth                         5
#define BA__DEVICE_CTRL__time_out_en                        0x0430
#define MSK__DEVICE_CTRL__time_out_en                       0x1
#define LSb__DEVICE_CTRL__time_out_en                       4
#define BA__DEVICE_CTRL__cont_on_err                        0x0430
#define MSK__DEVICE_CTRL__cont_on_err                       0x1
#define LSb__DEVICE_CTRL__cont_on_err                       3
#define BA__DEVICE_CTRL__ce_pin_reduct                      0x0430
#define MSK__DEVICE_CTRL__ce_pin_reduct                     0x1
#define LSb__DEVICE_CTRL__ce_pin_reduct                     1
#define BA__DEVICE_CTRL__ce_hold                            0x0430
#define MSK__DEVICE_CTRL__ce_hold                           0x1
#define LSb__DEVICE_CTRL__ce_hold                           0

#define RA__MULTIPLANE_CONFIG                               0x0434
#define BA__MULTIPLANE_CONFIG__pl_status_en                 0x0434
#define MSK__MULTIPLANE_CONFIG__pl_status_en                0x1
#define LSb__MULTIPLANE_CONFIG__pl_status_en                26
#define BA__MULTIPLANE_CONFIG__last_wr_cmd                  0x0434
#define MSK__MULTIPLANE_CONFIG__last_wr_cmd                 0x1
#define LSb__MULTIPLANE_CONFIG__last_wr_cmd                 25
#define BA__MULTIPLANE_CONFIG__mpl_erase_seq                0x0434
#define MSK__MULTIPLANE_CONFIG__mpl_erase_seq               0x1
#define LSb__MULTIPLANE_CONFIG__mpl_erase_seq               24
#define BA__MULTIPLANE_CONFIG__mpl_rd_seq                   0x0434
#define MSK__MULTIPLANE_CONFIG__mpl_rd_seq                  0x7
#define LSb__MULTIPLANE_CONFIG__mpl_rd_seq                  21
#define BA__MULTIPLANE_CONFIG__mpl_prg_seq                  0x0434
#define MSK__MULTIPLANE_CONFIG__mpl_prg_seq                 0x3
#define LSb__MULTIPLANE_CONFIG__mpl_prg_seq                 16
#define BA__MULTIPLANE_CONFIG__mpl_pl_num                   0x0434
#define MSK__MULTIPLANE_CONFIG__mpl_pl_num                  0x3
#define LSb__MULTIPLANE_CONFIG__mpl_pl_num                  8
#define BA__MULTIPLANE_CONFIG__mpl_cpbk_rd_seq              0x0434
#define MSK__MULTIPLANE_CONFIG__mpl_cpbk_rd_seq             0x1
#define LSb__MULTIPLANE_CONFIG__mpl_cpbk_rd_seq             4
#define BA__MULTIPLANE_CONFIG__mpl_wr_en                    0x0434
#define MSK__MULTIPLANE_CONFIG__mpl_wr_en                   0x1
#define LSb__MULTIPLANE_CONFIG__mpl_wr_en                   1
#define BA__MULTIPLANE_CONFIG__mpl_rd_en                    0x0434
#define MSK__MULTIPLANE_CONFIG__mpl_rd_en                   0x1
#define LSb__MULTIPLANE_CONFIG__mpl_rd_en                   0

#define RA__CACHE_CONFIG                                    0x0438
#define BA__CACHE_CONFIG__cache_wr_en                       0x0438
#define MSK__CACHE_CONFIG__cache_wr_en                      0x1
#define LSb__CACHE_CONFIG__cache_wr_en                      1
#define BA__CACHE_CONFIG__cache_rd_en                       0x0438
#define MSK__CACHE_CONFIG__cache_rd_en                      0x1
#define LSb__CACHE_CONFIG__cache_rd_en                      0

#define RA__DMA_SETTINGS                                    0x043c
#define BA__DMA_SETTINGS__sdma_err_rsp                      0x043c
#define MSK__DMA_SETTINGS__sdma_err_rsp                     0x1
#define LSb__DMA_SETTINGS__sdma_err_rsp                     17
#define BA__DMA_SETTINGS__OTE                               0x043c
#define MSK__DMA_SETTINGS__OTE                              0x1
#define LSb__DMA_SETTINGS__OTE                              16
#define BA__DMA_SETTINGS__burst_sel                         0x043c
#define MSK__DMA_SETTINGS__burst_sel                        0xFF
#define LSb__DMA_SETTINGS__burst_sel                        0

#define RA__SDMA_SIZE                                       0x0440
#define BA__SDMA_SIZE__sdma_size                            0x0440
#define MSK__SDMA_SIZE__sdma_size                           0xFFFFFFFF
#define LSb__SDMA_SIZE__sdma_size                           0

#define RA__SDMA_TRD_NUM                                    0x0444
#define BA__SDMA_TRD_NUM__sdma_trd                          0x0444
#define MSK__SDMA_TRD_NUM__sdma_trd                         0x7
#define LSb__SDMA_TRD_NUM__sdma_trd                         0

#define RA__TIME_OUT                                        0x0448
#define BA__TIME_OUT__time_out_val                          0x0448
#define MSK__TIME_OUT__time_out_val                         0xFFFFFFFF
#define LSb__TIME_OUT__time_out_val                         0

#define RA__SDMA_ADDR0                                      0x044c
#define BA__SDMA_ADDR0__sdma_addr_l                         0x044c
#define MSK__SDMA_ADDR0__sdma_addr_l                        0xFFFFFFFF
#define LSb__SDMA_ADDR0__sdma_addr_l                        0

#define RA__CTRL_VERSION                                    0x0800
#define BA__CTRL_VERSION__ctrl_fix                          0x0800
#define MSK__CTRL_VERSION__ctrl_fix                         0xFF
#define LSb__CTRL_VERSION__ctrl_fix                         8
#define BA__CTRL_VERSION__ctrl_rev                          0x0800
#define MSK__CTRL_VERSION__ctrl_rev                         0xFF
#define LSb__CTRL_VERSION__ctrl_rev                         0

#define RA__CTRL_FEATURES_REG                               0x0804
#define BA__CTRL_FEATURES_REG__nf_16b_supp                  0x0804
#define MSK__CTRL_FEATURES_REG__nf_16b_supp                 0x1
#define LSb__CTRL_FEATURES_REG__nf_16b_supp                 29
#define BA__CTRL_FEATURES_REG__nvddr2_3                     0x0804
#define MSK__CTRL_FEATURES_REG__nvddr2_3                    0x1
#define LSb__CTRL_FEATURES_REG__nvddr2_3                    28
#define BA__CTRL_FEATURES_REG__nvddr                        0x0804
#define MSK__CTRL_FEATURES_REG__nvddr                       0x1
#define LSb__CTRL_FEATURES_REG__nvddr                       27
#define BA__CTRL_FEATURES_REG__async_supp                   0x0804
#define MSK__CTRL_FEATURES_REG__async_supp                  0x1
#define LSb__CTRL_FEATURES_REG__async_supp                  26
#define BA__CTRL_FEATURES_REG__n_banks                      0x0804
#define MSK__CTRL_FEATURES_REG__n_banks                     0x3
#define LSb__CTRL_FEATURES_REG__n_banks                     24
#define BA__CTRL_FEATURES_REG__sfr_intf                     0x0804
#define MSK__CTRL_FEATURES_REG__sfr_intf                    0x3
#define LSb__CTRL_FEATURES_REG__sfr_intf                    22
#define BA__CTRL_FEATURES_REG__dma_data_width               0x0804
#define MSK__CTRL_FEATURES_REG__dma_data_width              0x1
#define LSb__CTRL_FEATURES_REG__dma_data_width              21
#define BA__CTRL_FEATURES_REG__dma_addr_width               0x0804
#define MSK__CTRL_FEATURES_REG__dma_addr_width              0x1
#define LSb__CTRL_FEATURES_REG__dma_addr_width              20
#define BA__CTRL_FEATURES_REG__dma_intf                     0x0804
#define MSK__CTRL_FEATURES_REG__dma_intf                    0x3
#define LSb__CTRL_FEATURES_REG__dma_intf                    18
#define BA__CTRL_FEATURES_REG__ecc_available                0x0804
#define MSK__CTRL_FEATURES_REG__ecc_available               0x1
#define LSb__CTRL_FEATURES_REG__ecc_available               17
#define BA__CTRL_FEATURES_REG__boot_available               0x0804
#define MSK__CTRL_FEATURES_REG__boot_available              0x1
#define LSb__CTRL_FEATURES_REG__boot_available              16
#define BA__CTRL_FEATURES_REG__pre_fetch_available          0x0804
#define MSK__CTRL_FEATURES_REG__pre_fetch_available         0x1
#define LSb__CTRL_FEATURES_REG__pre_fetch_available         15
#define BA__CTRL_FEATURES_REG__di_par_available             0x0804
#define MSK__CTRL_FEATURES_REG__di_par_available            0x1
#define LSb__CTRL_FEATURES_REG__di_par_available            14
#define BA__CTRL_FEATURES_REG__ext_cmd_cnt                  0x0804
#define MSK__CTRL_FEATURES_REG__ext_cmd_cnt                 0x1
#define LSb__CTRL_FEATURES_REG__ext_cmd_cnt                 13
#define BA__CTRL_FEATURES_REG__rmp_available                0x0804
#define MSK__CTRL_FEATURES_REG__rmp_available               0x1
#define LSb__CTRL_FEATURES_REG__rmp_available               12
#define BA__CTRL_FEATURES_REG__ext_status                   0x0804
#define MSK__CTRL_FEATURES_REG__ext_status                  0x1
#define LSb__CTRL_FEATURES_REG__ext_status                  11
#define BA__CTRL_FEATURES_REG__control_data                 0x0804
#define MSK__CTRL_FEATURES_REG__control_data                0x1
#define LSb__CTRL_FEATURES_REG__control_data                10
#define BA__CTRL_FEATURES_REG__write_protect                0x0804
#define MSK__CTRL_FEATURES_REG__write_protect               0x1
#define LSb__CTRL_FEATURES_REG__write_protect               9
#define BA__CTRL_FEATURES_REG__di_crc_available             0x0804
#define MSK__CTRL_FEATURES_REG__di_crc_available            0x1
#define LSb__CTRL_FEATURES_REG__di_crc_available            8
#define BA__CTRL_FEATURES_REG__n_threads                    0x0804
#define MSK__CTRL_FEATURES_REG__n_threads                   0xF
#define LSb__CTRL_FEATURES_REG__n_threads                   0

#define RA__MANUFACTURER_ID                                 0x0808
#define BA__MANUFACTURER_ID__dId                            0x0808
#define MSK__MANUFACTURER_ID__dId                           0xFF
#define LSb__MANUFACTURER_ID__dId                           16
#define BA__MANUFACTURER_ID__mId                            0x0808
#define MSK__MANUFACTURER_ID__mId                           0xFF
#define LSb__MANUFACTURER_ID__mId                           0

#define RA__NF_DEVICE_AREAS                                 0x080c
#define BA__NF_DEVICE_AREAS__spare_area_size                0x080c
#define MSK__NF_DEVICE_AREAS__spare_area_size               0xFFFF
#define LSb__NF_DEVICE_AREAS__spare_area_size               16
#define BA__NF_DEVICE_AREAS__main_area_size                 0x080c
#define MSK__NF_DEVICE_AREAS__main_area_size                0xFFFF
#define LSb__NF_DEVICE_AREAS__main_area_size                0

#define RA__DEVICE_PARAMS_0                                 0x0810
#define BA__DEVICE_PARAMS_0__device_type                    0x0810
#define MSK__DEVICE_PARAMS_0__device_type                   0x3
#define LSb__DEVICE_PARAMS_0__device_type                   30
#define BA__DEVICE_PARAMS_0__bits_per_cell                  0x0810
#define MSK__DEVICE_PARAMS_0__bits_per_cell                 0xFF
#define LSb__DEVICE_PARAMS_0__bits_per_cell                 16
#define BA__DEVICE_PARAMS_0__plane_addr_bits                0x0810
#define MSK__DEVICE_PARAMS_0__plane_addr_bits               0xFF
#define LSb__DEVICE_PARAMS_0__plane_addr_bits               8
#define BA__DEVICE_PARAMS_0__no_of_luns                     0x0810
#define MSK__DEVICE_PARAMS_0__no_of_luns                    0xFF
#define LSb__DEVICE_PARAMS_0__no_of_luns                    0

#define RA__DEVICE_PARAMS_1                                 0x0814
#define BA__DEVICE_PARAMS_1__ReadId_4                       0x0814
#define MSK__DEVICE_PARAMS_1__ReadId_4                      0xFF
#define LSb__DEVICE_PARAMS_1__ReadId_4                      8
#define BA__DEVICE_PARAMS_1__ReadId_3                       0x0814
#define MSK__DEVICE_PARAMS_1__ReadId_3                      0xFF
#define LSb__DEVICE_PARAMS_1__ReadId_3                      0

#define RA__DEVICE_FEATURES                                 0x0818
#define BA__DEVICE_FEATURES__optional_commands              0x0818
#define MSK__DEVICE_FEATURES__optional_commands             0xFFFF
#define LSb__DEVICE_FEATURES__optional_commands             16
#define BA__DEVICE_FEATURES__device_features                0x0818
#define MSK__DEVICE_FEATURES__device_features               0xFFFF
#define LSb__DEVICE_FEATURES__device_features               0

#define RA__DEVICE_BLOCKS_PER_LUN                           0x081c
#define BA__DEVICE_BLOCKS_PER_LUN__no_of_blocks             0x081c
#define MSK__DEVICE_BLOCKS_PER_LUN__no_of_blocks            0xFFFFFFFF
#define LSb__DEVICE_BLOCKS_PER_LUN__no_of_blocks            0

#define RA__DEVICE_REVISION                                 0x0820
#define BA__DEVICE_REVISION__revisions                      0x0820
#define MSK__DEVICE_REVISION__revisions                     0xFFFF
#define LSb__DEVICE_REVISION__revisions                     0

#define RA__ONFI_TIMING_MODES_0                             0x0824
#define BA__ONFI_TIMING_MODES_0__nv_ddr_modes               0x0824
#define MSK__ONFI_TIMING_MODES_0__nv_ddr_modes              0xFF
#define LSb__ONFI_TIMING_MODES_0__nv_ddr_modes              16
#define BA__ONFI_TIMING_MODES_0__sdr_modes                  0x0824
#define MSK__ONFI_TIMING_MODES_0__sdr_modes                 0xFFFF
#define LSb__ONFI_TIMING_MODES_0__sdr_modes                 0

#define RA__ONFI_TIMING_MODES_1                             0x0828
#define BA__ONFI_TIMING_MODES_1__nv_ddr3_modes              0x0828
#define MSK__ONFI_TIMING_MODES_1__nv_ddr3_modes             0xFFFF
#define LSb__ONFI_TIMING_MODES_1__nv_ddr3_modes             16
#define BA__ONFI_TIMING_MODES_1__nv_ddr2_modes              0x0828
#define MSK__ONFI_TIMING_MODES_1__nv_ddr2_modes             0xFFFF
#define LSb__ONFI_TIMING_MODES_1__nv_ddr2_modes             0

#define RA__ONFI_ITERLV_OP_ATTR                             0x082c
#define BA__ONFI_ITERLV_OP_ATTR__iterlv_op                  0x082c
#define MSK__ONFI_ITERLV_OP_ATTR__iterlv_op                 0xFF
#define LSb__ONFI_ITERLV_OP_ATTR__iterlv_op                 0

#define RA__ONFI_SYNC_OPT_0                                 0x0830
#define BA__ONFI_SYNC_OPT_0__onfi_tccs_min                  0x0830
#define MSK__ONFI_SYNC_OPT_0__onfi_tccs_min                 0xFFFF
#define LSb__ONFI_SYNC_OPT_0__onfi_tccs_min                 16
#define BA__ONFI_SYNC_OPT_0__nvddr_supp_ft                  0x0830
#define MSK__ONFI_SYNC_OPT_0__nvddr_supp_ft                 0xFF
#define LSb__ONFI_SYNC_OPT_0__nvddr_supp_ft                 0

#define RA__ONFI_SYNC_OPT_1                                 0x0834
#define BA__ONFI_SYNC_OPT_1__warmup_cycles                  0x0834
#define MSK__ONFI_SYNC_OPT_1__warmup_cycles                 0xFF
#define LSb__ONFI_SYNC_OPT_1__warmup_cycles                 24
#define BA__ONFI_SYNC_OPT_1__nvddr2_3_features              0x0834
#define MSK__ONFI_SYNC_OPT_1__nvddr2_3_features             0xFF
#define LSb__ONFI_SYNC_OPT_1__nvddr2_3_features             16
#define BA__ONFI_SYNC_OPT_1__adv_cmd_supp                   0x0834
#define MSK__ONFI_SYNC_OPT_1__adv_cmd_supp                  0xFF
#define LSb__ONFI_SYNC_OPT_1__adv_cmd_supp                  0

#define RA__BCH_CFG_0                                       0x0838
#define BA__BCH_CFG_0__bch_corr_3                           0x0838
#define MSK__BCH_CFG_0__bch_corr_3                          0xFF
#define LSb__BCH_CFG_0__bch_corr_3                          24
#define BA__BCH_CFG_0__bch_corr_2                           0x0838
#define MSK__BCH_CFG_0__bch_corr_2                          0xFF
#define LSb__BCH_CFG_0__bch_corr_2                          16
#define BA__BCH_CFG_0__bch_corr_1                           0x0838
#define MSK__BCH_CFG_0__bch_corr_1                          0xFF
#define LSb__BCH_CFG_0__bch_corr_1                          8
#define BA__BCH_CFG_0__bch_corr_0                           0x0838
#define MSK__BCH_CFG_0__bch_corr_0                          0xFF
#define LSb__BCH_CFG_0__bch_corr_0                          0

#define RA__BCH_CFG_1                                       0x083c
#define BA__BCH_CFG_1__bch_corr_7                           0x083c
#define MSK__BCH_CFG_1__bch_corr_7                          0xFF
#define LSb__BCH_CFG_1__bch_corr_7                          24
#define BA__BCH_CFG_1__bch_corr_6                           0x083c
#define MSK__BCH_CFG_1__bch_corr_6                          0xFF
#define LSb__BCH_CFG_1__bch_corr_6                          16
#define BA__BCH_CFG_1__bch_corr_5                           0x083c
#define MSK__BCH_CFG_1__bch_corr_5                          0xFF
#define LSb__BCH_CFG_1__bch_corr_5                          8
#define BA__BCH_CFG_1__bch_corr_4                           0x083c
#define MSK__BCH_CFG_1__bch_corr_4                          0xFF
#define LSb__BCH_CFG_1__bch_corr_4                          0

#define RA__BCH_CFG_2                                       0x0840
#define BA__BCH_CFG_2__bch_sect_1                           0x0840
#define MSK__BCH_CFG_2__bch_sect_1                          0xFFFF
#define LSb__BCH_CFG_2__bch_sect_1                          16
#define BA__BCH_CFG_2__bch_sect_0                           0x0840
#define MSK__BCH_CFG_2__bch_sect_0                          0xFFFF
#define LSb__BCH_CFG_2__bch_sect_0                          0

#define RA__BCH_CFG_3                                       0x0844
#define BA__BCH_CFG_3__bch_syndrome_factor                  0x0844
#define MSK__BCH_CFG_3__bch_syndrome_factor                 0xFF
#define LSb__BCH_CFG_3__bch_syndrome_factor                 24
#define BA__BCH_CFG_3__bch_metadata_size                    0x0844
#define MSK__BCH_CFG_3__bch_metadata_size                   0xFF
#define LSb__BCH_CFG_3__bch_metadata_size                   16
#define BA__BCH_CFG_3__bch_chien_factor                     0x0844
#define MSK__BCH_CFG_3__bch_chien_factor                    0xFF
#define LSb__BCH_CFG_3__bch_chien_factor                    8
#define BA__BCH_CFG_3__bch_brlk_factor                      0x0844
#define MSK__BCH_CFG_3__bch_brlk_factor                     0xFF
#define LSb__BCH_CFG_3__bch_brlk_factor                     0

#define RA__WP_SETTINGS                                     0x1000
#define BA__WP_SETTINGS__WP                                 0x1000
#define MSK__WP_SETTINGS__WP                                0x1
#define LSb__WP_SETTINGS__WP                                0

#define RA__RBN_SETTINGS                                    0x1004
#define BA__RBN_SETTINGS__Rbn                               0x1004
#define MSK__RBN_SETTINGS__Rbn                              0x1
#define LSb__RBN_SETTINGS__Rbn                              0

#define RA__SKIP_BYTES_CONF                                 0x100c
#define BA__SKIP_BYTES_CONF__marker                         0x100c
#define MSK__SKIP_BYTES_CONF__marker                        0xFFFF
#define LSb__SKIP_BYTES_CONF__marker                        16
#define BA__SKIP_BYTES_CONF__skip_bytes                     0x100c
#define MSK__SKIP_BYTES_CONF__skip_bytes                    0xFF
#define LSb__SKIP_BYTES_CONF__skip_bytes                    0

#define RA__SKIP_BYTES_OFFSET                               0x1010
#define BA__SKIP_BYTES_OFFSET__skip_bytes_offset            0x1010
#define MSK__SKIP_BYTES_OFFSET__skip_bytes_offset           0xFFFFFF
#define LSb__SKIP_BYTES_OFFSET__skip_bytes_offset           0

#define RA__ASYNC_TOGGLE_TIMINGS                            0x101c
#define BA__ASYNC_TOGGLE_TIMINGS__tRH                       0x101c
#define MSK__ASYNC_TOGGLE_TIMINGS__tRH                      0x1F
#define LSb__ASYNC_TOGGLE_TIMINGS__tRH                      24
#define BA__ASYNC_TOGGLE_TIMINGS__tRP                       0x101c
#define MSK__ASYNC_TOGGLE_TIMINGS__tRP                      0x1F
#define LSb__ASYNC_TOGGLE_TIMINGS__tRP                      16
#define BA__ASYNC_TOGGLE_TIMINGS__tWH                       0x101c
#define MSK__ASYNC_TOGGLE_TIMINGS__tWH                      0x1F
#define LSb__ASYNC_TOGGLE_TIMINGS__tWH                      8
#define BA__ASYNC_TOGGLE_TIMINGS__tWP                       0x101c
#define MSK__ASYNC_TOGGLE_TIMINGS__tWP                      0x1F
#define LSb__ASYNC_TOGGLE_TIMINGS__tWP                      0

#define RA__TIMINGS0                                        0x1024
#define BA__TIMINGS0__tADL                                  0x1024
#define MSK__TIMINGS0__tADL                                 0xFF
#define LSb__TIMINGS0__tADL                                 24
#define BA__TIMINGS0__tCCS                                  0x1024
#define MSK__TIMINGS0__tCCS                                 0xFF
#define LSb__TIMINGS0__tCCS                                 16
#define BA__TIMINGS0__tWHR                                  0x1024
#define MSK__TIMINGS0__tWHR                                 0xFF
#define LSb__TIMINGS0__tWHR                                 8
#define BA__TIMINGS0__tRHW                                  0x1024
#define MSK__TIMINGS0__tRHW                                 0xFF
#define LSb__TIMINGS0__tRHW                                 0

#define RA__TIMINGS1                                        0x1028
#define BA__TIMINGS1__tRHZ                                  0x1028
#define MSK__TIMINGS1__tRHZ                                 0xFF
#define LSb__TIMINGS1__tRHZ                                 24
#define BA__TIMINGS1__tWB                                   0x1028
#define MSK__TIMINGS1__tWB                                  0xFF
#define LSb__TIMINGS1__tWB                                  16
#define BA__TIMINGS1__tCWAW                                 0x1028
#define MSK__TIMINGS1__tCWAW                                0xFF
#define LSb__TIMINGS1__tCWAW                                8
#define BA__TIMINGS1__tVDLY                                 0x1028
#define MSK__TIMINGS1__tVDLY                                0xFF
#define LSb__TIMINGS1__tVDLY                                0

#define RA__TIMINGS2                                        0x102c
#define BA__TIMINGS2__tFEAT                                 0x102c
#define MSK__TIMINGS2__tFEAT                                0x3FF
#define LSb__TIMINGS2__tFEAT                                16
#define BA__TIMINGS2__CS_hold_time                          0x102c
#define MSK__TIMINGS2__CS_hold_time                         0x3F
#define LSb__TIMINGS2__CS_hold_time                         8
#define BA__TIMINGS2__CS_setup_time                         0x102c
#define MSK__TIMINGS2__CS_setup_time                        0x3F
#define LSb__TIMINGS2__CS_setup_time                        0

#define RA__PHY_CTRL_REG                                    0x2080
#define BA__PHY_CTRL_REG__phony_dqs_timing                  0x2080
#define MSK__PHY_CTRL_REG__phony_dqs_timing                 0x1F
#define LSb__PHY_CTRL_REG__phony_dqs_timing                 4

/************************************************************/

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int cmd0                                   :32;
    };
} nfc_reg_cmd_reg0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int cmd1                                   :32;
    };
} nfc_reg_cmd_reg1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int cmd2                                   :32;
    };
} nfc_reg_cmd_reg2_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int cmd3                                   :32;
    };
} nfc_reg_cmd_reg3_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int thrd_status_sel                        :3;
        unsigned int                                        :29;
    };
} nfc_reg_cmd_status_ptr_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int cmd_status                             :32;
    };
} nfc_reg_cmd_status_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                        :16;
        unsigned int cdma_idle                              :1;
        unsigned int cdma_terr                              :1;
        unsigned int ddma_terr                              :1;
        unsigned int                                        :1;
        unsigned int cmd_ignored                            :1;
        unsigned int sdma_trigg                             :1;
        unsigned int sdma_err                               :1;
        unsigned int                                        :9;
    };
} nfc_reg_intr_status_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                        :16;
        unsigned int cdma_idle_en                           :1;
        unsigned int cdma_terr_en                           :1;
        unsigned int ddma_terr_en                           :1;
        unsigned int                                        :1;
        unsigned int cmd_ignored_en                         :1;
        unsigned int sdma_trigg_en                          :1;
        unsigned int sdma_err_en                            :1;
        unsigned int                                        :8;
        unsigned int intr_en                                :1;
    };
} nfc_reg_intr_enable_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int sdma_busy                              :1;
        unsigned int mdma_busy                              :1;
        unsigned int cmd_eng_busy                           :1;
        unsigned int mc_busy                                :1;
        unsigned int                                        :4;
        unsigned int ctrl_busy                              :1;
        unsigned int init_comp                              :1;
        unsigned int init_fail                              :1;
        unsigned int                                        :5;
        unsigned int sdma_paused                            :1;
        unsigned int                                        :15;
    };
} nfc_reg_ctrl_status_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int trd_busy                               :8;
        unsigned int                                        :24;
    };
} nfc_reg_trd_status_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int trd0_error_stat                        :1;
        unsigned int trd1_error_stat                        :1;
        unsigned int trd2_error_stat                        :1;
        unsigned int trd3_error_stat                        :1;
        unsigned int trd4_error_stat                        :1;
        unsigned int trd5_error_stat                        :1;
        unsigned int trd6_error_stat                        :1;
        unsigned int trd7_error_stat                        :1;
        unsigned int                                        :24;
    };
} nfc_reg_trd_error_intr_status_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int trd_error_intr_en                      :8;
        unsigned int                                        :24;
    };
} nfc_reg_trd_error_intr_en_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int trd0_comp                              :1;
        unsigned int trd1_comp                              :1;
        unsigned int trd2_comp                              :1;
        unsigned int trd3_comp                              :1;
        unsigned int trd4_comp                              :1;
        unsigned int trd5_comp                              :1;
        unsigned int trd6_comp                              :1;
        unsigned int trd7_comp                              :1;
        unsigned int                                        :24;
    };
} nfc_reg_trd_comp_intr_status_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int target_err_l                           :32;
    };
} nfc_reg_dma_target_error_l_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int trd0_timeout_stat                      :1;
        unsigned int trd1_timeout_stat                      :1;
        unsigned int trd2_timeout_stat                      :1;
        unsigned int trd3_timeout_stat                      :1;
        unsigned int trd4_timeout_stat                      :1;
        unsigned int trd5_timeout_stat                      :1;
        unsigned int trd6_timeout_stat                      :1;
        unsigned int trd7_timeout_stat                      :1;
        unsigned int                                        :24;
    };
} nfc_reg_trd_timeout_intr_status_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int trd_timeout_intr_en                    :8;
        unsigned int                                        :24;
    };
} nfc_reg_trd_timeout_intr_en_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int sector_cnt                             :8;
        unsigned int                                        :8;
        unsigned int offset                                 :16;
    };
} nfc_reg_transfer_cfg_0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int sector_size                            :16;
        unsigned int last_sector_size                       :16;
    };
} nfc_reg_transfer_cfg_1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int long_polling                           :16;
        unsigned int                                        :16;
    };
} nfc_reg_long_polling_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int short_polling                          :16;
        unsigned int                                        :16;
    };
} nfc_reg_short_polling_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int rb_enable                              :1;
        unsigned int                                        :15;
        unsigned int ready_value                            :8;
        unsigned int ready_mask                             :8;
    };
} nfc_reg_rdst_ctrl_0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                        :16;
        unsigned int error_value                            :8;
        unsigned int error_mask                             :8;
    };
} nfc_reg_rdst_ctrl_1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int lun_stat_sel                           :1;
        unsigned int                                        :31;
    };
} nfc_reg_lun_status_cmd_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int lun_col_cmd                            :2;
        unsigned int                                        :4;
        unsigned int program_after_read                     :1;
        unsigned int                                        :25;
    };
} nfc_reg_lun_interleaved_cmd_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int lun_addr_offset                        :5;
        unsigned int                                        :27;
    };
} nfc_reg_lun_addr_offset_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int PPB                                    :16;
        unsigned int lun_en                                 :1;
        unsigned int                                        :3;
        unsigned int LN                                     :4;
        unsigned int                                        :3;
        unsigned int blk_addr_idx                           :5;
    };
} nfc_reg_nf_dev_layout_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int ecc_enable                             :1;
        unsigned int erase_det_en                           :1;
        unsigned int                                        :2;
        unsigned int scrambler_en                           :1;
        unsigned int                                        :3;
        unsigned int corr_str                               :3;
        unsigned int                                        :21;
    };
} nfc_reg_ecc_config_0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int erase_det_lvl                          :8;
        unsigned int                                        :24;
    };
} nfc_reg_ecc_config_1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int ce_hold                                :1;
        unsigned int ce_pin_reduct                          :1;
        unsigned int                                        :1;
        unsigned int cont_on_err                            :1;
        unsigned int time_out_en                            :1;
        unsigned int chrc_wdth                              :1;
        unsigned int                                        :1;
        unsigned int four_addr_seq_en                       :1;
        unsigned int                                        :24;
    };
} nfc_reg_device_ctrl_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int mpl_rd_en                              :1;
        unsigned int mpl_wr_en                              :1;
        unsigned int                                        :2;
        unsigned int mpl_cpbk_rd_seq                        :1;
        unsigned int                                        :3;
        unsigned int mpl_pl_num                             :2;
        unsigned int                                        :6;
        unsigned int mpl_prg_seq                            :2;
        unsigned int                                        :3;
        unsigned int mpl_rd_seq                             :3;
        unsigned int mpl_erase_seq                          :1;
        unsigned int last_wr_cmd                            :1;
        unsigned int pl_status_en                           :1;
        unsigned int                                        :5;
    };
} nfc_reg_multiplane_config_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int cache_rd_en                            :1;
        unsigned int cache_wr_en                            :1;
        unsigned int                                        :30;
    };
} nfc_reg_cache_config_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int burst_sel                              :8;
        unsigned int                                        :8;
        unsigned int OTE                                    :1;
        unsigned int sdma_err_rsp                           :1;
        unsigned int                                        :14;
    };
} nfc_reg_dma_settings_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int sdma_size                              :32;
    };
} nfc_reg_sdma_size_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int sdma_trd                               :3;
        unsigned int                                        :29;
    };
} nfc_reg_sdma_trd_num_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int time_out_val                           :32;
    };
} nfc_reg_time_out_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int sdma_addr_l                            :32;
    };
} nfc_reg_sdma_addr0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int ctrl_rev                               :8;
        unsigned int ctrl_fix                               :8;
        unsigned int                                        :16;
    };
} nfc_reg_ctrl_version_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int n_threads                              :4;
        unsigned int                                        :4;
        unsigned int di_crc_available                       :1;
        unsigned int write_protect                          :1;
        unsigned int control_data                           :1;
        unsigned int ext_status                             :1;
        unsigned int rmp_available                          :1;
        unsigned int ext_cmd_cnt                            :1;
        unsigned int di_par_available                       :1;
        unsigned int pre_fetch_available                    :1;
        unsigned int boot_available                         :1;
        unsigned int ecc_available                          :1;
        unsigned int dma_intf                               :2;
        unsigned int dma_addr_width                         :1;
        unsigned int dma_data_width                         :1;
        unsigned int sfr_intf                               :2;
        unsigned int n_banks                                :2;
        unsigned int async_supp                             :1;
        unsigned int nvddr                                  :1;
        unsigned int nvddr2_3                               :1;
        unsigned int nf_16b_supp                            :1;
        unsigned int                                        :2;
    };
} nfc_reg_ctrl_features_reg_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int mId                                    :8;
        unsigned int                                        :8;
        unsigned int dId                                    :8;
        unsigned int                                        :8;
    };
} nfc_reg_manufacturer_id_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int main_area_size                         :16;
        unsigned int spare_area_size                        :16;
    };
} nfc_reg_nf_device_areas_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int no_of_luns                             :8;
        unsigned int plane_addr_bits                        :8;
        unsigned int bits_per_cell                          :8;
        unsigned int                                        :6;
        unsigned int device_type                            :2;
    };
} nfc_reg_device_params_0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int ReadId_3                               :8;
        unsigned int ReadId_4                               :8;
        unsigned int                                        :16;
    };
} nfc_reg_device_params_1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int device_features                        :16;
        unsigned int optional_commands                      :16;
    };
} nfc_reg_device_features_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int no_of_blocks                           :32;
    };
} nfc_reg_device_blocks_per_lun_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int revisions                              :16;
        unsigned int                                        :16;
    };
} nfc_reg_device_revision_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int sdr_modes                              :16;
        unsigned int nv_ddr_modes                           :8;
        unsigned int                                        :8;
    };
} nfc_reg_onfi_timing_modes_0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int nv_ddr2_modes                          :16;
        unsigned int nv_ddr3_modes                          :16;
    };
} nfc_reg_onfi_timing_modes_1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int iterlv_op                              :8;
        unsigned int                                        :24;
    };
} nfc_reg_onfi_iterlv_op_attr_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int nvddr_supp_ft                          :8;
        unsigned int                                        :8;
        unsigned int onfi_tccs_min                          :16;
    };
} nfc_reg_onfi_sync_opt_0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int adv_cmd_supp                           :8;
        unsigned int                                        :8;
        unsigned int nvddr2_3_features                      :8;
        unsigned int warmup_cycles                          :8;
    };
} nfc_reg_onfi_sync_opt_1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int bch_corr_0                             :8;
        unsigned int bch_corr_1                             :8;
        unsigned int bch_corr_2                             :8;
        unsigned int bch_corr_3                             :8;
    };
} nfc_reg_bch_cfg_0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int bch_corr_4                             :8;
        unsigned int bch_corr_5                             :8;
        unsigned int bch_corr_6                             :8;
        unsigned int bch_corr_7                             :8;
    };
} nfc_reg_bch_cfg_1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int bch_sect_0                             :16;
        unsigned int bch_sect_1                             :16;
    };
} nfc_reg_bch_cfg_2_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int bch_brlk_factor                        :8;
        unsigned int bch_chien_factor                       :8;
        unsigned int bch_metadata_size                      :8;
        unsigned int bch_syndrome_factor                    :8;
    };
} nfc_reg_bch_cfg_3_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int WP                                     :1;
        unsigned int                                        :31;
    };
} nfc_reg_wp_settings_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int Rbn                                    :1;
        unsigned int                                        :31;
    };
} nfc_reg_rbn_settings_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int skip_bytes                             :8;
        unsigned int                                        :8;
        unsigned int marker                                 :16;
    };
} nfc_reg_skip_bytes_conf_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int skip_bytes_offset                      :24;
        unsigned int                                        :8;
    };
} nfc_reg_skip_bytes_offset_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int tWP                                    :5;
        unsigned int                                        :3;
        unsigned int tWH                                    :5;
        unsigned int                                        :3;
        unsigned int tRP                                    :5;
        unsigned int                                        :3;
        unsigned int tRH                                    :5;
        unsigned int                                        :3;
    };
} nfc_reg_async_toggle_timings_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int tRHW                                   :8;
        unsigned int tWHR                                   :8;
        unsigned int tCCS                                   :8;
        unsigned int tADL                                   :8;
    };
} nfc_reg_timings0_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int tVDLY                                  :8;
        unsigned int tCWAW                                  :8;
        unsigned int tWB                                    :8;
        unsigned int tRHZ                                   :8;
    };
} nfc_reg_timings1_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int CS_setup_time                          :6;
        unsigned int                                        :2;
        unsigned int CS_hold_time                           :6;
        unsigned int                                        :2;
        unsigned int tFEAT                                  :10;
        unsigned int                                        :6;
    };
} nfc_reg_timings2_t;

typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                        :4;
        unsigned int phony_dqs_timing                       :5;
        unsigned int                                        :23;
    };
} nfc_reg_phy_ctrl_reg_t;

#endif
