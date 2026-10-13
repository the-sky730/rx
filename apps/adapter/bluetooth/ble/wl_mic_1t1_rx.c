#include "system/app_core.h"
#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "user_cfg.h"
#include "bt_common.h"
#include "le_common.h"
#include "adapter_process.h"
#include "third_party/wireless_mic/wl_mic_api.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1T1_RX)

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)    printf("[wlm_1t1_rx]" x " ", ## __VA_ARGS__)
#define log_info_hexdump    put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

/* user config */
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

#define RX_ADAPTIVE_PARAM  \
    ((WIRELESS_CODING_SAMPLERATE == 44100)? \
    RX_FRAME_LEN_INDEX : (RX_FRAME_LEN_INDEX + 1))

/* 内部函数声明 */
static void wlm_1t1_rx_connect_succ_callback(void);
static void wlm_1t1_rx_disconnect_callback(void);
static void wlm_1t1_rx_iso_callback(const void *const buf, size_t length);
static void wlm_1t1_rx_set_pair_trigger(uint32_t param, uint32_t type);
void wlm_set_pair(uint32_t param);

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

static const wlm_lib_callback wlm_1t1_rx_cb = {
    .wlm_connect_succ     = wlm_1t1_rx_connect_succ_callback,
    .wlm_disconnect       = wlm_1t1_rx_disconnect_callback,
    .wlm_iso_rx           = wlm_1t1_rx_iso_callback,
    .wlm_set_pair_trigger = wlm_1t1_rx_set_pair_trigger,
};

static const wlm_lib_ops *const wlm_op = &wlm_1t1_rx_op;

#define __this              wlm_op

/* 函数实体 */
static void wlm_1t1_rx_connect_succ_callback(void)
{
    log_info("--func=%s", __FUNCTION__);

    adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_OPEN, 0);
    adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 0);
}

static void wlm_1t1_rx_disconnect_callback(void)
{
    log_info("--func=%s", __FUNCTION__);

    adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_CLOSE, 0);
    adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 0);
}

static void wlm_1t1_rx_iso_callback(const void *const buf, size_t length)
{
    adapter_wireless_dec_frame_write((u8 *)buf, length);
}

static void wlm_1t1_rx_set_pair_trigger(uint32_t param, uint32_t type)
{
    log_info("--func=%s", __FUNCTION__);

    if (type) {
        adapter_process_event_notify(ADAPTER_EVENT_SET_PAIR, param);
    } else {
        wlm_set_pair(param);
    }
}

void wlm_enter_pair_mode(void)
{
    log_info("--func=%s", __FUNCTION__);

    adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_CLOSE, 0);

    if (__this && __this->wlm_enter_pair) {
        __this->wlm_enter_pair();
    }
}

void wlm_exit_pair_mode(void)
{
    log_info("--func=%s", __FUNCTION__);

    if (__this && __this->wlm_exit_pair) {
        __this->wlm_exit_pair();
    }
}

void wlm_set_pair(uint32_t param)
{
    log_info("--func=%s", __FUNCTION__);

    if (__this && __this->wlm_set_pair) {
        __this->wlm_set_pair(WIRELESS_PAIR_BONDING);
    }
}

void bt_ble_init(void)
{
    log_info("***** wl_mic_1t1_rx init ******");

    if (__this && __this->wlm_open) {
        __this->wlm_open();
    }
}

void bt_ble_exit(void)
{
    log_info("***** wl_mic_1t1_rx exit ******");

    if (__this && __this->wlm_close) {
        __this->wlm_close();
    }
}

void ble_profile_init(void)
{
    log_info("wl_mic_1t1_rx profile init");

    if (__this && __this->wlm_init) {
#if WIRELESS_TOOL_BLE_NAME_EN	//使用配置工具的蓝牙名
        extern const char *bt_get_local_name();
        uint8_t *config_name ;
        config_name = (uint8_t *)(bt_get_local_name());
        memset(rx_param.pair_name, 0, strlen(rx_param.pair_name));
        memcpy(rx_param.pair_name, config_name, strlen(config_name));
        printf("bt_name = %s", rx_param.pair_name);
#endif
        __this->wlm_init(&rx_param, &wlm_1t1_rx_cb);
    }
}

void ble_module_enable(u8 en)
{
}

#endif /* (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1T1_RX) */
