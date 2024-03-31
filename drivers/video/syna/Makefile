ifdef CONFIG_SYNA_DISPLAY_TA

B_CHIP_CFLAGS :=-DBERLIN_B_0=0x1000 \
                -DBERLIN_C_0=0x2000 \
                -DBERLIN_C_1=0x2010 \
                -DBERLIN_C_2=0x2020 \
                -DBERLIN_C_3=0x2030 \
                -DBERLIN_A_0=0x0000 \
                -DBERLIN_A_1=0x0010 \
                -DBERLIN_A_2=0x0020 \
                -DBERLIN_BG2=0x3000 \
                -DBERLIN_BG2_Z2=0x3010 \
                -DBERLIN_BG2_A0=0x3020 \
                -DBERLIN_BG2_CDP=0x3040 \
                -DBERLIN_BG3_CD=0x3040 \
                -DBERLIN_BG2_CT=0x3050 \
                -DBERLIN_BG2_Q=0x3060 \
                -DBERLIN_BG2Q_A1=0x3070 \
                -DBERLIN_BG2_DTV=0x3080 \
                -DBERLIN_BG2_DTV_A0=0x3090 \
                -DBERLIN_BG4_CD=0x30A0 \
                -DBERLIN_BG4_CT=0x30B0 \
                -DBERLIN_BG4_DTV=0x30C0 \
                -DBERLIN_BG4_CT_A0=0x30D0 \
                -DBERLIN_BG4_CDP=0x30E0 \
                -DBERLIN_BG4_CDP_A0=0x30F0 \
                -DBERLIN_BG5_CT=0x3100 \
                -DBERLIN_BG5_CT_A0=0x3110 \
                -DBERLIN_BG6_CT=0x3120 \
                -DBERLIN_VS680=0x3130 \
                -DBERLIN_VS640=0x3140



B_CHIP_CFLAGS +=-DBOARD_BG2_RDK_DMP=0 \
                -DBOARD_BG2_BUDDY_BOARD=1 \
                -DBOARD_BG2_DTV_RDK=2 \
                -DBOARD_BG2_DTV_COTEHILL=3 \
                -DBOARD_BG2_DTV_COTEHILL_A0=4 \
                -DBOARD_BG2_Z1_EP50=5 \
                -DBOARD_BG2_BOLTON=6 \
                -DBOARD_BG2_DTV_COTEHILL_A1=7 \
                -DBOARD_BG2_DTV_COTEHILL_A1_SF=8 \
                -DBOARD_BG2_BURNLEY=9 \
                -DBOARD_BG2_BEETLE=10 \
                -DBOARD_BG2_RAPTOR=11 \
                -DBOARD_BG2_SEOCHO=12 \
                -DBOARD_BG2_A0_FREUD=13 \
                -DBOARD_BG2_SDK=14 \
                -DBOARD_BG2_A0_VAPOR=15 \
                -DBOARD_BG2_RIL=16 \
                -DBOARD_BG2_HSO1000=17 \
                -DBOARD_BG2_MIDAS=18 \
                -DBOARD_BG2_IPTV=19

PLATFORM_CPPFLAGS += -DCONFIG_LESSPRINT

PLATFORM_CPPFLAGS += -DCONFIG_VPP_USE_FASTLOGO_TA

PLATFORM_CPPFLAGS += -DCONFIG_FASTLOGO
PLATFORM_CPPFLAGS += -DCONFIG_TRUSTZONE
PLATFORM_CPPFLAGS += -DCONFIG_MV_AMP_TEE_ENABLE
PLATFORM_CPPFLAGS += -DVPP_ENABLE_TZ_MALLOC_MEM_ALLOCATION

PLATFORM_CPPFLAGS += -DVPP_SUS_RES -DPE_3D_FMT_CONVERT -DPE_3D_BD
PLATFORM_CPPFLAGS += -DBERLIN -DVPP_IN_TRUST_ZONE
PLATFORM_CPPFLAGS += -DHDMI_HW_EDDC_SUPPORT

PLATFORM_CPPFLAGS += -DTZ_3_0
PLATFORM_CPPFLAGS += -DVPP_ENABLE_INTERNAL_MEM_MGR

#PLATFORM_CPPFLAGS += -DVPP_ENABLE_FLUSH_CACHE
PLATFORM_CPPFLAGS += -DCONFIG_VPP_ENABLE_PASSPAR
#PLATFORM_CPPFLAGS += -DCONFIG_VPP_ENABLE_PASSPHY
#PLATFORM_CPPFLAGS += -DCONFIG_VPP_FASTLOGO_SHOW_VBUF_INFO

#PLATFORM_CPPFLAGS += -DVPP_ENABLE_SECURE_MEM_ALLOCATION

#Already defined in tee_ca_vpp.c
#PLATFORM_CPPFLAGS += -DVPP_ENABLE_TZMEM_DYNAMIC

PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/include
PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/include
PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/thinvpp_ca
PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/thinvpp_ca/include
PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/thinvpp_ca/osal/include
PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/thinvpp_ca/common/include
PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/thinvpp_ca/ca/include

PLATFORM_CPPFLAGS += -I$(srctree)/arch/arm/mach-synaptics/driver/tz
PLATFORM_CPPFLAGS += -I$(srctree)/arch/arm/mach-synaptics/utils
PLATFORM_CPPFLAGS += -I$(srctree)/include/linux
PLATFORM_CPPFLAGS += -I$(srctree)/../../common/inc

ifdef CONFIG_TARGET_VS680
    B_CHIP_CFLAGS +=-DBERLIN_CHIP_VERSION=BERLIN_VS680
    B_CHIP_CFLAGS +=-DBERLIN_SOC_VS680
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL_LOCAL

    PLATFORM_CPPFLAGS += -I$(srctree)/../common/inc/chip/vs680/Z1
    PLATFORM_CPPFLAGS += -I$(srctree)/board/synaptics/vs680/include/
    PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/avpll/vs680/

    obj-y += vpp_ca/avpll/vs680/avpll.o
endif

ifdef CONFIG_TARGET_VS680_A0
    B_CHIP_CFLAGS +=-DBERLIN_CHIP_VERSION=BERLIN_VS680
    B_CHIP_CFLAGS +=-DBERLIN_SOC_VS680
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL_LOCAL

    PLATFORM_CPPFLAGS += -I$(srctree)/board/synaptics/vs680a0/include/
    PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/avpll/vs680/

    obj-y += vpp_ca/avpll/vs680/avpll.o
endif

ifdef CONFIG_TARGET_VS680_C05
    B_CHIP_CFLAGS +=-DBERLIN_CHIP_VERSION=BERLIN_VS680
    B_CHIP_CFLAGS +=-DBERLIN_SOC_VS680
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL_LOCAL

    PLATFORM_CPPFLAGS += -I$(srctree)/../common/inc/chip/vs680/Z1
    PLATFORM_CPPFLAGS += -I$(srctree)/board/synaptics/vs680_c05/include/
    PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/avpll/vs680/

    obj-y += vpp_ca/avpll/vs680/avpll.o
endif


ifdef CONFIG_TARGET_VS640
    B_CHIP_CFLAGS +=-DBERLIN_CHIP_VERSION=BERLIN_VS640
    B_CHIP_CFLAGS +=-DBERLIN_SOC_VS640
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL
    B_CHIP_CFLAGS +=-DCONFIG_VPP_ENABLE_AVPLL_LOCAL

    PLATFORM_CPPFLAGS += -I$(srctree)/../common/inc/chip/vs640/A0
    PLATFORM_CPPFLAGS += -I$(srctree)/board/synaptics/vs680/include/
    PLATFORM_CPPFLAGS += -I$(srctree)/drivers/video/syna/vpp_ca/avpll/vs680/

    obj-y += vpp_ca/avpll/vs680/avpll.o
endif

PLATFORM_CPPFLAGS += $(B_CHIP_CFLAGS)

#source files specific to TA
obj-y += vpp_ca/thinvpp_ca/ca/source/tee_ca_vpp.o
obj-y += vpp_ca/thinvpp_ca/ca/source/vpp_api_tz.o
obj-y += vpp_ca/thinvpp_ca/osal/source/OSAL_api.o
obj-y += vpp_ca/thinvpp_ca/vpp_ca_wrap.o
obj-y += vpp_ca/thinvpp_ca/vbuf.o
obj-y += vpp_ca/thinvpp_ca/vpp_cfg.o
obj-y += vpp_ca/mv_display.o
obj-y += vpp_ca/vpp_api_process.o
obj-y += vpp_ca/fb.o

#$(warning PLATFORM_CPPFLAGS =  $(PLATFORM_CPPFLAGS))
#$(warning srctree =  $(srctree))

endif
