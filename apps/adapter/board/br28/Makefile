
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件
#
# 注意： Linux 下编译方式：
#     1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
#     2. 下载后，解压到 /opt/jieli 目录下，保证
#       /opt/jieli/common/bin/clang 存在（注意目录层次）
#     3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#       可以通过 ulimit -n 8096 来设置一个较大的值
#

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := llvm-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := C:/JL/pi32/pi32v2-lib/r3-large
SYS_INC_DIR := C:/JL/pi32/pi32v2-include
EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
FIXBAT          := ..\..\..\..\tools\utils\fixbat.exe # 用于处理 utf8->gbk 编码问题
POST_SCRIPT     := ..\..\..\..\cpu\br28\tools\download.bat
RUN_POST_SCRIPT := $(POST_SCRIPT)
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3-large
SYS_INC_DIR := $(TOOL_DIR)/../include
EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
FIXBAT          := touch # Linux下不需要处理 bat 编码问题
POST_SCRIPT     := ../../../../cpu/br28/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := ../../../../cpu/br28/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs
# 工程路径前缀
ROOT_PREFIX := ../../../..

# 编译参数设置
CFLAGS := \
	-target pi32v2 \
	-mcpu=r3 \
	-integrated-as \
	-flto \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-Wno-shift-negative-value \
	-Wundef \
	-Wframe-larger-than=256 \
	-Wincompatible-pointer-types \
	-Wreturn-type \
	-Wimplicit-function-declaration \
	-mllvm -pi32v2-large-program=true \
	-fms-extensions \
	-fdiscrete-bitfield-abi \
	-w \


# C++额外的编译参数
CXXFLAGS :=


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_CPU_BR28 \
	-DCONFIG_USB_SUPPORT_MRX_TX \
	-DCONFIG_PRINT_IN_MASK \
	-DUSB_EP_PROTECT=0 \
	-DCONFIG_NEW_BREDR_ENABLE \
	-DCONFIG_NEW_MODEM_ENABLE \
	-DCONFIG_NEW_TWS_FORWARD_ENABLE \
	-DCONFIG_UCOS_ENABLE \
	-DCONFIG_EQ_SUPPORT_ASYNC \
	-DEQ_CORE_V1 \
	-DCONFIG_AAC_CODEC_FFT_USE_MUTEX \
	-DCONFIG_DNS_ENABLE \
	-DCONFIG_DMS_MALLOC \
	-DCONFIG_MMU_ENABLE \
	-DCONFIG_SBC_CODEC_HW \
	-DCONFIG_MSBC_CODEC_HW \
	-DCONFIG_AEC_M=256 \
	-DCONFIG_SUPPORT_WIFI_DETECT \
	-DCONFIG_AUDIO_ONCHIP \
	-DCONFIG_MEDIA_DEVELOP_ENABLE \
	-DCONFIG_SOUND_PLATFORM_ENABLE=1 \
	-DCONFIG_SUPPORT_EX_TWS_ADJUST \
	-D__GCC_PI32V2__ \
	-DCONFIG_EFFECT_CORE_V2_ENABLE \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_TOUCH_ENABLE_CONFIG=0 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DCONFIG_EVENT_KEY_MAP_ENABLE=0 \
	-DTIMER_POOL_NUM_CONFIG=15 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=0 \
	-DVFS_ENABLE=1 \
	-DUSE_SDFILE_NEW=1 \
	-DSDFILE_STORAGE=1 \
	-DVFS_FILE_POOL_NUM_CONFIG=1 \
	-DFS_VERSION=0x020001 \
	-DFATFS_VERSION=0x020101 \
	-DSDFILE_VERSION=0x020000 \
	-DVM_MAX_PAGE_ALIGN_SIZE_CONFIG=8*1024 \
	-DVM_MAX_SECTOR_ALIGN_SIZE_CONFIG=16*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-DCONFIG_TWS_ENABLE \
	-DCONFIG_ADAPTER_ENABLE \
	-DCONFIG_NEW_CFG_TOOL_ENABLE \
	-DCONFIG_LITE_AEC_ENABLE=0 \
	-DAUDIO_REC_POOL_NUM=1 \
	-DAUDIO_DEC_POOL_NUM=3 \
	-DCONFIG_LMP_CONN_SUSPEND_ENABLE \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_LINK_DISTURB_SCAN_ENABLE=0 \
	-DCONFIG_UPDATA_ENABLE \
	-DCONFIG_OTA_UPDATA_ENABLE \
	-DCONFIG_ITEM_FORMAT_VM \
	-DSUPPORT_BLUETOOTH_PROFILE_RELEASE \
	-DCONFIG_SOUNDBOX \
	-DAPP_DONGLE=0 \
	-DAPP_DONGLE_1T2=1 \
	-DAPP_WIRELESS_MIC_1T1=2 \
	-DAPP_WIRELESS_MIC_2T1=3 \
	-DAPP_WIRELESS_MIC_1TN=4 \
	-DAPP_WIRELESS_DUPLEX=5 \
	-DAPP_MAIN=3 \
	-DCONFIG_2T1_ROLE_SEL=1 \


DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
INCLUDES := \
	-I../../../../include_lib \
	-I../../../../include_lib/driver \
	-I../../../../include_lib/driver/device \
	-I../../../../include_lib/driver/cpu/br28 \
	-I../../../../include_lib/system \
	-I../../../../include_lib/system/generic \
	-I../../../../include_lib/system/device \
	-I../../../../include_lib/system/fs \
	-I../../../../include_lib/system/ui \
	-I../../../../include_lib/btctrler \
	-I../../../../include_lib/btctrler/port/br28 \
	-I../../../../include_lib/update \
	-I../../../../include_lib/agreement \
	-I../../../../include_lib/btstack/third_party/common \
	-I../../../../include_lib/btstack/third_party/rcsp \
	-I../../../../include_lib/media/media_develop \
	-I../../../../include_lib/media/media_develop/media \
	-I../../../../include_lib/media/media_develop/media/cpu/br28 \
	-I../../../../include_lib/media/media_develop/media/cpu/br28/asm \
	-I../../../../cpu/br28 \
	-I../../../../cpu/br28/audio_way \
	-I../../../../cpu/br28/ui_driver/LED_HBS117 \
	-I../../../../apps/adapter/include \
	-I../../../../apps/adapter/include/common \
	-I../../../../apps/adapter/include/common/bt_common \
	-I../../../../apps/adapter/include/user_api \
	-I../../../../apps/common \
	-I../../../../apps/common/device \
	-I../../../../apps/common/audio \
	-I../../../../apps/common/audio/stream \
	-I../../../../apps/common/power_manage \
	-I../../../../apps/common/dev_manager \
	-I../../../../apps/common/file_operate \
	-I../../../../apps/common/iap \
	-I../../../../apps/common/include \
	-I../../../../apps/common/test \
	-I../../../../apps/common/config/include \
	-I../../../../apps/adapter/board/br28 \
	-I../../../../include_lib/btstack \
	-I../../../../apps/common/third_party_profile/common \
	-I../../../../apps/common/third_party_profile/jieli \
	-I../../../../apps/common/third_party_profile/jieli/trans_data_demo \
	-I../../../../apps/common/third_party_profile/jieli/online_db \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/adv_rcsp_protocol \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/adv_app_setting \
	-I../../../../apps/common/recorder \
	-I../../../../include_lib/media \
	-I../../../../apps/adapter/audio \
	-I../../../../apps/adapter/audio/decoder \
	-I../../../../apps/adapter/audio/encoder \
	-I../../../../apps/adapter/audio/wireless \
	-I../../../../apps/adapter/idev \
	-I../../../../apps/adapter/idev/idev_usb \
	-I../../../../apps/adapter/idev/idev_bt \
	-I../../../../apps/adapter/idev/idev_bt/include \
	-I../../../../apps/adapter/idev/idev_mic \
	-I../../../../apps/adapter/odev \
	-I../../../../apps/adapter/odev/odev_bt \
	-I../../../../apps/adapter/odev/odev_bt/odev_edr \
	-I../../../../apps/adapter/odev/odev_bt/odev_ble \
	-I../../../../apps/adapter/odev/odev_bt/include \
	-I../../../../apps/adapter/odev/odev_dac \
	-I../../../../apps/adapter/process \
	-I../../../../apps/common/device/usb \
	-I../../../../apps/common/device/usb/device \
	-I../../../../apps/common/device/usb/host \
	-I$(SYS_INC_DIR) \


# 需要编译的 .c 文件
c_SRC_FILES := \
	../../../../apps/adapter/adapter_charge.c \
	../../../../apps/adapter/adapter_uart_demo.c \
	../../../../apps/adapter/app_key.c \
	../../../../apps/adapter/app_main.c \
	../../../../apps/adapter/app_main/wireless_mic_2t1/2t1_rx.c \
	../../../../apps/adapter/app_main/wireless_mic_2t1/2t1_rx_product_test.c \
	../../../../apps/adapter/audio/adapter_adc.c \
	../../../../apps/adapter/audio/adapter_audio_miceq.c \
	../../../../apps/adapter/audio/adapter_audio_musiceq.c \
	../../../../apps/adapter/audio/adapter_audio_stream.c \
	../../../../apps/adapter/audio/adapter_media.c \
	../../../../apps/adapter/audio/adapter_recorder.c \
	../../../../apps/adapter/audio/adapter_stream_demo.c \
	../../../../apps/adapter/audio/decoder/adapter_dec_dual_jla.c \
	../../../../apps/adapter/audio/decoder/adapter_dec_esco.c \
	../../../../apps/adapter/audio/decoder/adapter_dec_jla.c \
	../../../../apps/adapter/audio/decoder/adapter_dec_mic.c \
	../../../../apps/adapter/audio/decoder/adapter_dec_tone.c \
	../../../../apps/adapter/audio/decoder/adapter_dec_uac_spk.c \
	../../../../apps/adapter/audio/decoder/adapter_decoder.c \
	../../../../apps/adapter/audio/encoder/adapter_enc_esco.c \
	../../../../apps/adapter/audio/encoder/adapter_enc_sbc.c \
	../../../../apps/adapter/audio/encoder/adapter_enc_uac_mic.c \
	../../../../apps/adapter/audio/encoder/adapter_encoder.c \
	../../../../apps/adapter/audio/wireless/adapter_wireless_dec.c \
	../../../../apps/adapter/audio/wireless/adapter_wireless_enc.c \
	../../../../apps/adapter/bluetooth/ble/wl_mic_1t1_rx.c \
	../../../../apps/adapter/bluetooth/ble/wl_mic_1t1_tx.c \
	../../../../apps/adapter/bluetooth/ble/wl_mic_1tN_rx.c \
	../../../../apps/adapter/bluetooth/ble/wl_mic_1tN_tx.c \
	../../../../apps/adapter/board/br28/board_wireless_mic_2t1_rx/board_wireless_mic_2t1_rx.c \
	../../../../apps/adapter/board/br28/board_wireless_mic_2t1_rx/key_table/adkey_table.c \
	../../../../apps/adapter/board/br28/board_wireless_mic_2t1_rx/key_table/iokey_table.c \
	../../../../apps/adapter/board/br28/board_wireless_mic_2t1_rx/key_table/irkey_table.c \
	../../../../apps/adapter/board/br28/board_wireless_mic_2t1_rx/key_table/rdec_key_table.c \
	../../../../apps/adapter/board/br28/board_wireless_mic_2t1_rx/key_table/touch_key_table.c \
	../../../../apps/adapter/board/br28/undef_func.c \
	../../../../apps/adapter/common/bt_common/bt_edr_fun.c \
	../../../../apps/adapter/common/bt_common/bt_hci_event.c \
	../../../../apps/adapter/common/bt_common/bt_product_test.c \
	../../../../apps/adapter/common/bt_common/bt_status_event.c \
	../../../../apps/adapter/common/bt_common/dongle_1t2.c \
	../../../../apps/adapter/common/dev_status.c \
	../../../../apps/adapter/common/init.c \
	../../../../apps/adapter/common/task_table.c \
	../../../../apps/adapter/common/tone_table.c \
	../../../../apps/adapter/common/user_cfg.c \
	../../../../apps/adapter/duplex_mode_switch.c \
	../../../../apps/adapter/idev/adapter_idev.c \
	../../../../apps/adapter/idev/idev_bt/adapter_idev_bt.c \
	../../../../apps/adapter/idev/idev_bt/idev_ble/adapter_idev_ble.c \
	../../../../apps/adapter/idev/idev_bt/idev_ble/idev_ble_wireless.c \
	../../../../apps/adapter/idev/idev_mic/adapter_idev_mic.c \
	../../../../apps/adapter/idev/idev_usb/adapter_idev_usb.c \
	../../../../apps/adapter/idev/idev_usb/adapter_usb_hid.c \
	../../../../apps/adapter/log_config/app_config.c \
	../../../../apps/adapter/log_config/lib_btctrler_config.c \
	../../../../apps/adapter/log_config/lib_btstack_config.c \
	../../../../apps/adapter/log_config/lib_driver_config.c \
	../../../../apps/adapter/log_config/lib_media_config.c \
	../../../../apps/adapter/log_config/lib_system_config.c \
	../../../../apps/adapter/log_config/lib_update_config.c \
	../../../../apps/adapter/odev/adapter_odev.c \
	../../../../apps/adapter/odev/odev_bt/adapter_odev_bt.c \
	../../../../apps/adapter/odev/odev_bt/odev_ble/adapter_odev_ble.c \
	../../../../apps/adapter/odev/odev_bt/odev_ble/odev_ble_dongle.c \
	../../../../apps/adapter/odev/odev_bt/odev_ble/odev_ble_wireless.c \
	../../../../apps/adapter/odev/odev_bt/odev_edr/adapter_odev_edr.c \
	../../../../apps/adapter/odev/odev_dac/adapter_odev_dac.c \
	../../../../apps/adapter/odev/odev_usb/adapter_odev_usb.c \
	../../../../apps/adapter/power_manage/app_charge.c \
	../../../../apps/adapter/power_manage/app_chargestore.c \
	../../../../apps/adapter/power_manage/app_power_manage.c \
	../../../../apps/adapter/process/adapter_process.c \
	../../../../apps/adapter/rf_test.c \
	../../../../apps/adapter/ui/led/io_led.c \
	../../../../apps/adapter/ui_manage.c \
	../../../../apps/adapter/version.c \
	../../../../apps/common/audio/audio_digital_vol.c \
	../../../../apps/common/audio/audio_utils.c \
	../../../../apps/common/audio/sine_make.c \
	../../../../apps/common/audio/stream/stream_HighSampleRateMicSystem.c \
	../../../../apps/common/audio/stream/stream_entry.c \
	../../../../apps/common/audio/stream/stream_eq_dccs.c \
	../../../../apps/common/audio/stream/stream_llns.c \
	../../../../apps/common/audio/stream/stream_src.c \
	../../../../apps/common/audio/stream/stream_sync.c \
	../../../../apps/common/audio/uartPcmSender.c \
	../../../../apps/common/bt_common/bt_test_api.c \
	../../../../apps/common/config/app_config.c \
	../../../../apps/common/config/app_sound_box_tool.c \
	../../../../apps/common/config/bt_profile_config.c \
	../../../../apps/common/config/cfg_tool_share_osc_functions/cfg_tool_share_osc.c \
	../../../../apps/common/config/cfg_tool_share_osc_functions/cfg_tool_share_osc_single_io_send.c \
	../../../../apps/common/config/ci_transport_uart.c \
	../../../../apps/common/debug/debug.c \
	../../../../apps/common/debug/debug_lite.c \
	../../../../apps/common/dev_manager/dev_manager.c \
	../../../../apps/common/dev_manager/dev_reg.c \
	../../../../apps/common/dev_manager/dev_update.c \
	../../../../apps/common/device/key/adkey.c \
	../../../../apps/common/device/key/adkey_rtcvdd.c \
	../../../../apps/common/device/key/ctmu_touch_key.c \
	../../../../apps/common/device/key/iokey.c \
	../../../../apps/common/device/key/irkey.c \
	../../../../apps/common/device/key/key_driver.c \
	../../../../apps/common/device/key/rdec_key.c \
	../../../../apps/common/device/key/slidekey.c \
	../../../../apps/common/device/key/touch_key.c \
	../../../../apps/common/device/nandflash/nandflash.c \
	../../../../apps/common/device/ntc/ntc_det.c \
	../../../../apps/common/device/usb/device/cdc.c \
	../../../../apps/common/device/usb/device/custom_hid.c \
	../../../../apps/common/device/usb/device/descriptor.c \
	../../../../apps/common/device/usb/device/hid.c \
	../../../../apps/common/device/usb/device/msd.c \
	../../../../apps/common/device/usb/device/msd_upgrade.c \
	../../../../apps/common/device/usb/device/task_pc.c \
	../../../../apps/common/device/usb/device/uac1.c \
	../../../../apps/common/device/usb/device/uac_stream.c \
	../../../../apps/common/device/usb/device/usb_device.c \
	../../../../apps/common/device/usb/device/user_setup.c \
	../../../../apps/common/device/usb/host/adb.c \
	../../../../apps/common/device/usb/host/aoa.c \
	../../../../apps/common/device/usb/host/audio.c \
	../../../../apps/common/device/usb/host/audio_demo.c \
	../../../../apps/common/device/usb/host/hid.c \
	../../../../apps/common/device/usb/host/usb_bulk_transfer.c \
	../../../../apps/common/device/usb/host/usb_ctrl_transfer.c \
	../../../../apps/common/device/usb/host/usb_host.c \
	../../../../apps/common/device/usb/host/usb_storage.c \
	../../../../apps/common/device/usb/usb_config.c \
	../../../../apps/common/device/usb/usb_host_config.c \
	../../../../apps/common/fat_nor/nor_fs.c \
	../../../../apps/common/fat_nor/phone_rec_fs.c \
	../../../../apps/common/fat_nor/virfat_flash.c \
	../../../../apps/common/file_operate/file_api.c \
	../../../../apps/common/file_operate/file_bs_deal.c \
	../../../../apps/common/file_operate/file_manager.c \
	../../../../apps/common/iap/iAP_des.c \
	../../../../apps/common/iap/iAP_device.c \
	../../../../apps/common/iap/iAP_iic.c \
	../../../../apps/common/rec_nor/nor_interface.c \
	../../../../apps/common/rec_nor/nor_rec_fs.c \
	../../../../apps/common/recorder/recorder.c \
	../../../../apps/common/recorder/recorder_coder.c \
	../../../../apps/common/recorder/recorder_file.c \
	../../../../apps/common/test/board_test.c \
	../../../../apps/common/third_party_profile/jieli/le_client_demo.c \
	../../../../apps/common/third_party_profile/jieli/multi_demo/le_multi_client.c \
	../../../../apps/common/third_party_profile/jieli/multi_demo/le_multi_common.c \
	../../../../apps/common/third_party_profile/jieli/multi_demo/le_multi_trans.c \
	../../../../apps/common/third_party_profile/jieli/trans_data_demo/le_trans_data.c \
	../../../../apps/common/third_party_profile/jieli/wireless_mic/le_wireless_mic_client.c \
	../../../../apps/common/third_party_profile/jieli/wireless_mic/le_wireless_mic_server.c \
	../../../../apps/common/update/norflash_ufw_update.c \
	../../../../apps/common/update/norflash_update.c \
	../../../../apps/common/update/testbox_update.c \
	../../../../apps/common/update/uart_update.c \
	../../../../apps/common/update/uart_update_master.c \
	../../../../apps/common/update/update.c \
	../../../../cpu/br28/adc_api.c \
	../../../../cpu/br28/audio_codec_clock.c \
	../../../../cpu/br28/audio_common/app_audio.c \
	../../../../cpu/br28/audio_common/sound_pcm_demo.c \
	../../../../cpu/br28/audio_effect/audio_dynamic_eq_demo.c \
	../../../../cpu/br28/audio_effect/audio_eff_default_parm.c \
	../../../../cpu/br28/audio_effect/audio_eq_drc_demo.c \
	../../../../cpu/br28/audio_effect/audio_gain_process_demo.c \
	../../../../cpu/br28/audio_effect/audio_noise_gate_demo.c \
	../../../../cpu/br28/audio_effect/audio_sound_track_2_p_x.c \
	../../../../cpu/br28/audio_effect/audio_surround_demo.c \
	../../../../cpu/br28/audio_effect/audio_vbass_demo.c \
	../../../../cpu/br28/audio_effect/audio_voice_changer_demo.c \
	../../../../cpu/br28/audio_effect/effects_adj.c \
	../../../../cpu/br28/audio_effect/eq_config.c \
	../../../../cpu/br28/audio_sound_dac.c \
	../../../../cpu/br28/audio_way/audio_app_stream.c \
	../../../../cpu/br28/audio_way/audio_way.c \
	../../../../cpu/br28/audio_way/audio_way_dac.c \
	../../../../cpu/br28/charge.c \
	../../../../cpu/br28/chargestore.c \
	../../../../cpu/br28/clock_manager.c \
	../../../../cpu/br28/hw_fft.c \
	../../../../cpu/br28/iic_hw.c \
	../../../../cpu/br28/iic_soft.c \
	../../../../cpu/br28/irflt.c \
	../../../../cpu/br28/lp_touch_key.c \
	../../../../cpu/br28/lp_touch_key_tool.c \
	../../../../cpu/br28/lua_port_api.c \
	../../../../cpu/br28/mcpwm.c \
	../../../../cpu/br28/overlay_code.c \
	../../../../cpu/br28/plcnt.c \
	../../../../cpu/br28/power_api.c \
	../../../../cpu/br28/pwm_led.c \
	../../../../cpu/br28/rdec.c \
	../../../../cpu/br28/setup.c \
	../../../../cpu/br28/spi.c \
	../../../../cpu/br28/uart_dev.c \
	../../../../cpu/br28/ui_driver/LED_HBS117/led_hbs117.c \
	../../../../cpu/br28/umidigi_chargestore.c \


# 需要编译的 .S 文件
S_SRC_FILES :=


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--plugin-opt=-warn-stack-size=256 \
	--sort-common \
	--plugin-opt=-used-symbol-file=../../../../cpu/br28/sdk_used_list.used \
	--plugin-opt=-pi32v2-large-program=true \
	--gc-sections \
	--start-group \
	--start-group  \
	../../../../cpu/br28/liba/cpu.a \
	../../../../cpu/br28/liba/system.a \
	../../../../cpu/br28/liba/libepmotion.a \
	../../../../cpu/br28/liba/wtg_dec_lib.a \
	../../../../cpu/br28/liba/sbc_eng_lib.a \
	../../../../cpu/br28/liba/mp3tsy_dec_lib.a \
	../../../../cpu/br28/liba/wtgv2_dec_lib.a \
	../../../../cpu/br28/liba/flac_dec_lib.a \
	../../../../cpu/br28/liba/ape_dec_lib.a \
	../../../../cpu/br28/liba/m4a_tws_lib.a \
	../../../../cpu/br28/liba/amr_dec_lib.a \
	../../../../cpu/br28/liba/dts_dec_lib.a \
	../../../../cpu/br28/liba/mp3_decstream_lib.a \
	../../../../cpu/br28/liba/wma_decstream_lib.a \
	../../../../cpu/br28/liba/libAptFilt_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/libEchoSuppress_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/libNoiseSuppress_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/libSplittingFilter_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/libDelayEstimate_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/libOpcore_maskrom_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/libFFT_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/compressor.a \
	../../../../cpu/br28/liba/crossover_coff.a \
	../../../../cpu/br28/liba/limiter.a \
	../../../../cpu/br28/liba/lib_pitchshifter.a \
	../../../../cpu/br28/liba/lib_pitch_speed.a \
	../../../../cpu/br28/liba/lib_reverb_cal.a \
	../../../../cpu/br28/liba/lib_mp2_code.a \
	../../../../cpu/br28/liba/lib_adpcm_code.a \
	../../../../cpu/br28/liba/mp3_enc_lib.a \
	../../../../cpu/br28/liba/opus_enc_lib.a \
	../../../../cpu/br28/liba/speex_enc_lib.a \
	../../../../cpu/br28/liba/lib_rmono2stereo.a \
	../../../../cpu/br28/liba/limiter_noiseGate.a \
	../../../../cpu/br28/liba/howling.a \
	../../../../cpu/br28/liba/noisegate.a \
	../../../../cpu/br28/liba/shiftPhase.a \
	../../../../cpu/br28/liba/pemafrow.a \
	../../../../cpu/br28/liba/lib_howlings_phf.a \
	../../../../cpu/br28/liba/lib_resample_cal.a \
	../../../../cpu/br28/liba/lib_resample_fast_cal.a \
	../../../../cpu/br28/liba/mp3_dec_lib.a \
	../../../../cpu/br28/liba/wma_dec_lib.a \
	../../../../cpu/br28/liba/wav_dec_lib.a \
	../../../../cpu/br28/liba/agreement.a \
	../../../../cpu/br28/liba/lib_sur_cal.a \
	../../../../cpu/br28/liba/lib_vbass_cal.a \
	../../../../cpu/br28/liba/loudness.a \
	../../../../cpu/br28/liba/filtparam_cal_lib.a \
	../../../../cpu/br28/liba/media_app.a \
	../../../../cpu/br28/liba/aec.a \
	../../../../cpu/br28/liba/bt_hash_enc.a \
	../../../../cpu/br28/liba/rcsp_stack.a \
	../../../../cpu/br28/liba/lib_usbaudio_plc.a \
	../../../../cpu/br28/liba/coeff_calculate.a \
	../../../../cpu/br28/liba/rms.a \
	../../../../cpu/br28/liba/SpectrumShow.a \
	../../../../cpu/br28/liba/gma_stack.a \
	../../../../cpu/br28/liba/dueros_stack.a \
	../../../../cpu/br28/liba/lib_adpcm_ima_code.a \
	../../../../cpu/br28/liba/lib_midi_dec.a \
	../../../../cpu/br28/liba/jla_codec_lib.a \
	../../../../cpu/br28/liba/libjlsp.a \
	../../../../cpu/br28/liba/libllns.a \
	../../../../cpu/br28/liba/libHighSampleRateSingleMicSystem_pi32v2_OnChip.a \
	../../../../cpu/br28/liba/btstack.a \
	../../../../cpu/br28/liba/btctrler.a \
	../../../../cpu/br28/liba/media.a \
	../../../../cpu/br28/liba/update.a \
	../../../../cpu/br28/liba/res.a \
	../../../../cpu/br28/liba/ui_dot.a \
	../../../../cpu/br28/liba/font.a \
	../../../../cpu/br28/liba/crypto_toolbox_Ospeed.a \
	--end-group \
	--end-group  \
	-T../../../../cpu/br28/sdk.ld \
	-M=../../../../cpu/br28/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS:$(ROOT_PREFIX)/%=%))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES:$(ROOT_PREFIX)/%=%))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br28/sdk_used_list.c -o ../../../../cpu/br28/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br28/sdk_ld.c -o ../../../../cpu/br28/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br28/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br28/tools/isd_config_rule.c -o ../../../../cpu/br28/tools/isd_config.ini

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)



ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : $(ROOT_PREFIX)/%.c
	$(info +CC $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.S.o : $(ROOT_PREFIX)/%.S
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.s.o : $(ROOT_PREFIX)/%.s
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : $(ROOT_PREFIX)/%.cpp
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : $(ROOT_PREFIX)/%.cxx
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cc.o : $(ROOT_PREFIX)/%.cc
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

-include $(DEP_FILES)
