#include "system/app_core.h"
#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "user_cfg.h"
#include "bt_common.h"
#include "le_common.h"
#include "adapter_process.h"
#include "third_party/wireless_mic/wl_mic_api.h"
#include "ble/ll_config.h"

#if TCFG_2T1_RX_PRODUCT_TEST_EN

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)    printf("[wlm_1t1_rx]" x " ", ## __VA_ARGS__)
#define log_info_hexdump    put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define RX_ADAPTIVE_PARAM  \
    ((WIRELESS_CODING_SAMPLERATE == 44100)? \
    RX_FRAME_LEN_INDEX : (RX_FRAME_LEN_INDEX + 1))

#define WL_MIC_PAIR_NAME        "WL_MIC_V0"

#if (WIRELESS_CODING_FRAME_LEN == 50)
#define RX_FRAME_LEN_INDEX  0
#elif (WIRELESS_CODING_FRAME_LEN == 75)
#define RX_FRAME_LEN_INDEX  2
#elif (WIRELESS_CODING_FRAME_LEN == 100)
#define RX_FRAME_LEN_INDEX  4
#else
#error "wl_mic_1t1_rx WIRELESS_CODING_FRAME_LEN not allow !!!"
#endif

/* 全局常量 */
static wlm_param rx_param = {
    .pair_name = WL_MIC_PAIR_NAME,
    .spec_name = SPECIFIC_STRING,

    .param = {
#if LOWPOWER_PAIR_MODE
        [0] = 70,
        [1] = 320,
        [2] = TCFG_WIRELESS_RSSI, // BIT(14)/BIT(15) | 50,
        [3] = RX_ADAPTIVE_PARAM,
        [4] = 154 * 5,
        [5] = (40 << 8) | 200,
        [6] = 3000,
#else
        [0] = 28,
        [1] = 28,
        [2] = TCFG_WIRELESS_RSSI, // BIT(14)/BIT(15) | 50,
        [3] = RX_ADAPTIVE_PARAM,
        [4] = 50 * 5,
        [5] = (40 << 8) | 20,
        [6] = 1000,
#endif
        [7] = 0,
    },

};
/* 函数实体 */
static void wlm_1t1_rx_connect_succ_callback(void)
{
    log_info("--func=%s", __FUNCTION__);

    adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_OPEN, 0);
    adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 0);
}

extern u32 config_vendor_le_bb;
static void wlm_1t1_rx_disconnect_callback(void)
{
    log_info("--func=%s", __FUNCTION__);

    config_vendor_le_bb = VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_CONNECT_SLOT |
                          VENDOR_BB_ADV_PDU_INT(3) |
                          VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                          VENDOR_BB_NEW_SCAN_STRATEGY |
                          VENDOR_BB_TS |
#if WIRELESS_HIGH_BW_EN
                          VENDOR_BB_HIGH_BW |
#endif /* WIRELESS_HIGH_BW_EN */
                          0;
    adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_CLOSE, 0);
    adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 0);


    extern void ll_hci_reset(void);
    ll_hci_reset();
    extern void ble_multi_client_evt_handler_reset(void);
    ble_multi_client_evt_handler_reset();
    extern int ble_enable_new_dev_scan(void);
    ble_enable_new_dev_scan();
}

static void wlm_1t1_rx_iso_callback(const void *const buf, size_t length)
{
    //adapter_wireless_dec_frame_write((u8 *)buf, length);

    u8 dec_channel = 0;
    adapter_dec_dual_jla_frame_write(dec_channel, buf, length);
    adapter_dec_dual_jla_frame_write_pause(dec_channel, 0);
}

static const wlm_lib_callback wlm_1t1_rx_cb = {
    .wlm_connect_succ = wlm_1t1_rx_connect_succ_callback,
    .wlm_disconnect   = wlm_1t1_rx_disconnect_callback,
    .wlm_iso_rx       = wlm_1t1_rx_iso_callback,
};

static const wlm_lib_ops *const wlm_op = &wlm_1t1_rx_op;

#define __this              wlm_op

static const u8 product_tx_name_l[] = "MIC_TEST_2T1";
#define WRITE_LIT_U32(a,src)   {*((u8*)(a)+3) = (u8)((src)>>24);  *((u8*)(a)+2) = (u8)(((src)>>16)&0xff);*((u8*)(a)+1) = (u8)(((src)>>8)&0xff);*((u8*)(a)+0) = (u8)((src)&0xff);}
#define READ_LIT_U32(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8) + (*((u8*)(a)+2)<<16) + (*((u8*)(a)+3)<<24))

void wlm_2t1_test_rx_init(void)
{
    if (__this && __this->wlm_init) {
#if WIRELESS_TOOL_BLE_NAME_EN	//使用配置工具的蓝牙名
        extern const char *bt_get_local_name();
        uint8_t *config_name ;
        config_name = (uint8_t *)(bt_get_local_name());
        memset(rx_param.pair_name, 0, strlen(rx_param.pair_name));
        memcpy(rx_param.pair_name, config_name, strlen(config_name));
        printf("bt_name = %s", rx_param.pair_name);
#endif
        WRITE_LIT_U32(&rx_param.param[8], (u32)&product_tx_name_l);
#if (WIRELESS_24G_ENABLE)
        rx_param.param[10] = WIRELESS_24G_CODE_ID;
#endif
        printf("sel name:%s %x\n", READ_LIT_U32(&rx_param.param[8]), rx_param.param[10]);

        __this->wlm_init(&rx_param, &wlm_1t1_rx_cb);
    }

    if (__this && __this->wlm_open) {
        __this->wlm_open();
    }

}

#endif

