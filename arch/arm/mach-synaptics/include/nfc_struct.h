/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Synaptics Incorporated
 *
 */

#ifndef _NFC_STRUCT_
#define _NFC_STRUCT_

//tbl 2.1, 2.10, 2.27
typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                                        :24;
        unsigned int                                trd_num                 :3;
        unsigned int                                                        :3;
        unsigned int                                ct                      :2;
    } dma;
    union
    {
        struct
        {
            unsigned int                            cmd_type                :16;
            unsigned int                            vol_id                  :4;
            unsigned int                            interrupt               :1;
            unsigned int                            dma_sel                 :1;
            unsigned int                                                    :2;
            unsigned int                            trd_num                 :3;
            unsigned int                                                    :3;
            unsigned int                            ct                      :2;
        } read, program;
        struct
        {
            unsigned int                            cmd_type                :16;
            unsigned int                            vol_id                  :4;
            unsigned int                            interrupt               :1;
            unsigned int                                                    :3;
            unsigned int                            trd_num                 :3;
            unsigned int                                                    :3;
            unsigned int                            ct                      :2;
        } copyback, erase,
          reset, set_features;
        struct
        {
            unsigned int                            cmd_type                :16;
            unsigned int                                                    :4;
            unsigned int                            interrupt               :1;
            unsigned int                                                    :3;
            unsigned int                            trd_num                 :3;
            unsigned int                                                    :3;
            unsigned int                            ct                      :2;
        } thread_reset;
    } pio;
    struct
    {
        unsigned int                                                        :20;
        unsigned int                                interrupt               :1;
        unsigned int                                                        :3;
        unsigned int                                trd_num                 :3;
        unsigned int                                                        :3;
        unsigned int                                ct                      :2;
    } gen;
} nfc_cmd_0_reg_t;

// tbl 2.11, 2.14, 2.17, 2.20, 2.22, 2.24
typedef union
{
    unsigned int u[1];
    union
    {
        struct
        {
            unsigned int                            row_addr                :24;
            unsigned int                            bank                    :3;
            unsigned int                                                    :5;
        } read, program,
          copyback, erase, reset;
        struct
        {
            unsigned int                            feature_addr            :8;
            unsigned int                                                    :16;
            unsigned int                            bank                    :3;
            unsigned int                                                    :5;
        } set_features;
    } pio;
} nfc_cmd_1_reg_t;

//tbl 2.2, 2.12, 2.15, 2.18, 2.25, 2.28
typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                desc_addr_low           :32;
    } dma;
    union
    {
        struct
        {
            unsigned int                            mem_addr_ptr_l          :32;
        } read, program;
        struct
        {
            unsigned int                            row_addr                :24;
            unsigned int                            bank                    :3;
            unsigned int                                                    :5;
        } copyback;
        struct
        {
            unsigned int                            feature_val             :32;
        } set_features;
    } pio;
    struct
    {
        unsigned int                                cmd_val_l               :32;
    } gen;
} nfc_cmd_2_reg_t;

//tbl 2.29
typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                cmd_val_h               :32;
    } gen;
} nfc_cmd_3_reg_t;

//tbl 2.30
typedef union
{
    unsigned long long ul;
    unsigned int u[2];
    struct
    {
        unsigned int                                inst_type               :6;
        unsigned int                                twb_active              :1;
        unsigned int                                jedec_supp              :1;
        unsigned int                                                        :3;
        union
        {
            struct
            {
                unsigned int                                                :5;
                unsigned int                        cmd                     :8;
                unsigned long long                                          :40;
            } cmd_seq;
            struct
            {
                unsigned int                        nbytes                  :3;
                unsigned int                                                :2;
                unsigned int                        addr0                   :8;
                unsigned int                        addr1                   :8;
                unsigned int                        addr2                   :8;
                unsigned int                        addr3                   :8;
                unsigned int                        addr4                   :8;
                unsigned int                        addr5                   :8;
            } addr_seq;
            struct
            {
                unsigned int                        direction               :1;
                unsigned int                        ecc_en                  :1;
                unsigned int                        scrambler_en            :1;
                unsigned int                        erased_page_det_en      :1;
                unsigned int                                                :1;
                unsigned int                        sector_size             :16;
                unsigned int                        sector_cnt              :8;
                unsigned int                        last_sector_size        :16;
                unsigned int                        corr_cap                :3;
                unsigned int                                                :5;
            } data_seq;
            struct
            {
                unsigned int                        f2_en                   :1;
                unsigned long long                                          :52;
            }   read_status;
            struct
            {
                unsigned long long                                          :53;
            } reset, synchronous_reset;
            struct
            {
                unsigned int                                                :5;
                unsigned int                        addr0                   :8;
                unsigned long long                                          :40;
            } volume_select, odt_configure,
              set_features, get_features,
              read_id, read_parameter_page,
              zq_calibration_short,
              zq_calibration_long;
            struct
            {
                unsigned int                                                :5;
                unsigned int                        addr0                   :8;
                unsigned int                        addr1                   :8;
                unsigned int                                                :32;
            } change_write_column,
              lun_get_features,
              lun_set_features;
            struct
            {
                unsigned int                        nbytes                  :3;
                unsigned int                                                :2;
                unsigned int                        addr0                   :8;
                unsigned int                        addr1                   :8;
                unsigned int                        addr2                   :8;
                unsigned int                                                :24;
            } erase, read_status_enhanced,
              multi_plane_block_erase,
              lun_reset;
            struct
            {
                unsigned int                        nbytes                  :3;
                unsigned int                                                :2;
                unsigned int                        addr0                   :8;
                unsigned int                        addr1                   :8;
                unsigned int                        addr2                   :8;
                unsigned int                        addr3                   :8;
                unsigned int                        addr4                   :8;
                unsigned int                                                :8;
            } read, write, read_cache_random,
              copyback_read, copyback_program,
              change_read_column,
              change_read_column_enhanced,
              change_read_colunm_jedec,
              multi_plane_read,
              small_data_move;
            struct
            {
                unsigned int                        nbytes                  :3;
                unsigned int                                                :2;
                unsigned int                        addr0                   :8;
                unsigned int                        addr1                   :8;
                unsigned int                        addr2                   :8;
                unsigned int                        addr3                   :8;
                unsigned int                        addr4                   :8;
                unsigned int                        addr5                   :8;
            } multi_plane_block_erase_onfi_jedec;
        };
    };
} nfc_cmd_layout_t;

//tbl 2.5
typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                row_addr                :24;
        unsigned int                                bank                    :3;
        unsigned int                                                        :5;
    };
} nfc_flash_ptr_field_t;

//tbl 2.6
typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                mem_ptr_cont            :1;
        unsigned int                                flash_ptr_cont          :1;
        unsigned int                                                        :2;
        unsigned int                                target_vol              :4;
        unsigned int                                interrupt               :1;
        unsigned int                                cont                    :1;
        unsigned int                                dma_sel                 :1;
        unsigned int                                                        :21;
    };
} nfc_cmd_flags_field_t;    //only low 16bits used

//tbl 2.7, 2.26, 2.32
typedef union
{
    unsigned int u[1];
    union
    {
        struct
        {
            unsigned int                            desc_err                :1;
            unsigned int                            ecc_err                 :1;
            unsigned int                            max_err                 :8;
            unsigned int                                                    :1;
            unsigned int                            erased_page             :1;
            unsigned int                            dev_err                 :1;
            unsigned int                                                    :1;
            unsigned int                            fail                    :1;
            unsigned int                            complete                :1;
            unsigned int                            bus_err                 :1;
            unsigned int                                                    :7;
            unsigned int                            err_idx                 :8;
        } dma;
        struct
        {
            unsigned int                            cmd_err                 :1;
            unsigned int                            ecc_err                 :1;
            unsigned int                            max_err                 :8;
            unsigned int                                                    :1;
            unsigned int                            erased_page             :1;
            unsigned int                            dev_err                 :1;
            unsigned int                                                    :1;
            unsigned int                            fail                    :1;
            unsigned int                            complete                :1;
            unsigned int                            bus_err                 :1;
            unsigned int                                                    :7;
            unsigned int                            err_idx                 :8;
        } pio;
        struct
        {
            unsigned int                            cmd_err                 :1;
            unsigned int                            ecc_err                 :1;
            unsigned int                            max_err                 :8;
            unsigned int                                                    :1;
            unsigned int                            erased_page             :1;
            unsigned int                                                    :2;
            unsigned int                            fail                    :1;
            unsigned int                            complete                :1;
            unsigned int                            bus_err                 :1;
            unsigned int                                                    :15;
        } gen;
    };
} nfc_status_field_t;

//tbl 2.8
typedef union
{
    unsigned int u[1];
    struct
    {
        unsigned int                                increment               :1;
        unsigned int                                greater                 :1;
        unsigned int                                valid                   :1;
        unsigned int                                                        :13;
        unsigned int                                end_val                 :8;
        unsigned int                                start_val               :8;
    };
} nfc_sync_arg_field_t;

//tbl 2.3
typedef union
{
    unsigned int u[14];
    struct
    {
        unsigned int                                next_ptr;
        unsigned int                                                        :32;
        union
        {
            nfc_flash_ptr_field_t                   flash_ptr;
            unsigned int                            copyback_src_addr;
        };
        unsigned int                                                        :32;
        unsigned int                                cmd_type; //only low 16bits used
        nfc_cmd_flags_field_t                       cmd_flags;
        union
        {
            unsigned int                            mem_ptr;
            unsigned int                            copyback_dst_addr;
        };
        unsigned int                                                        :32;
        nfc_status_field_t                          status;
        unsigned int                                                        :32;
        unsigned int                                sync_flag_ptr;
        unsigned int                                                        :32;
        nfc_sync_arg_field_t                        sync_args;
        unsigned int                                                        :32;
    };
} nfc_cmd_desc_t;


#endif
