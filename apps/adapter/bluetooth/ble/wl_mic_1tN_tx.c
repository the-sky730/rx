#include "system/app_core.h"
#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "user_cfg.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "le_common.h"
#include "adapter_process.h"
#include "third_party/wireless_mic/wl_mic_api.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1TN_TX)

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)    printf("[wlm_1tN_tx]" x " ", ## __VA_ARGS__)
#define log_info_hexdump    put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

/* user config */
#define WL_MIC_PAIR_NAME    "WL_MIC_V1"

/* 内部函数声明 */
static void wlm_1tN_tx_connect_succ_callback(void);
static void wlm_1tN_tx_disconnect_callback(void);
static void wlm_1tN_tx_set_pair_trigger(uint32_t param, uint32_t type);
void wlm_1tN_tx_set_pair(uint32_t param);

/* 全局常量 */
static wlm_param tx_param = {
    .pair_name = WL_MIC_PAIR_NAME,
    .param = {
#if LOWPOWER_PAIR_MODE
        [1] = 64,
#else
        [1] = 10,
#endif
        [7] = 1,
    },
};

static const wlm_lib_callback wlm_1tN_tx_cb = {
    .wlm_connect_succ     = wlm_1tN_tx_connect_succ_callback,
    .wlm_disconnect       = wlm_1tN_tx_disconnect_callback,
    .wlm_set_pair_trigger = wlm_1tN_tx_set_pair_trigger,
};

static const wlm_lib_ops *const wlm_op = &wlm_1tN_tx_op;

#define __this              wlm_op

/* 函数实体 */
static void wlm_1tN_tx_connect_succ_callback(void)
{
    log_info("--func=%s", __FUNCTION__);

    adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_OPEN, 0);
    adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 0);
}

static void wlm_1tN_tx_disconnect_callback(void)
{
    log_info("--func=%s", __FUNCTION__);

    adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
    adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 0);
}

static void wlm_1tN_tx_set_pair_trigger(uint32_t param, uint32_t type)
{
    log_info("--func=%s", __FUNCTION__);

    if (type) {
        adapter_process_event_notify(ADAPTER_EVENT_SET_PAIR, param);
    } else {
        wlm_1tN_tx_set_pair(param);
    }
}

void wlm_enter_pair_mode(void)
{
    log_info("--func=%s", __FUNCTION__);

    adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);

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

void bt_ble_init(void)
{
    log_info("***** wl_mic_1tN_tx init ******\n");

    if (__this && __this->wlm_open) {
        __this->wlm_open();
    }
}

void bt_ble_exit(void)
{
    log_info("***** wl_mic_1tN_tx exit ******\n");

    if (__this && __this->wlm_close) {
        __this->wlm_close();
    }
}

void ble_profile_init(void)
{
    log_info("wl_mic_1tN_tx profile init");

    if (__this && __this->wlm_init) {
#if WIRELESS_TOOL_BLE_NAME_EN	//使用配置工具的蓝牙名
        extern const char *bt_get_local_name();
        uint8_t *config_name ;
        config_name = (uint8_t *)(bt_get_local_name());
        memset(tx_param.pair_name, 0, strlen(tx_param.pair_name));
        memcpy(tx_param.pair_name, config_name, strlen(config_name));
        printf("bt_name = %s", tx_param.pair_name);
#endif
        __this->wlm_init(&tx_param, &wlm_1tN_tx_cb);
    }
}

void wlm_1tN_tx_set_pair(uint32_t param)
{
    log_info("--func=%s", __FUNCTION__);

    if (__this && __this->wlm_set_pair) {
        __this->wlm_set_pair(param);
    }
}

int wlm_1tN_tx_iso(const void *const buf, size_t len)
{
    int res = -1;

    if (__this && __this->wlm_iso_tx) {
        res = __this->wlm_iso_tx(buf, len);
    }

    return res;
}

void ble_module_enable(u8 en)
{
}

#endif /* (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1TN_TX) */
