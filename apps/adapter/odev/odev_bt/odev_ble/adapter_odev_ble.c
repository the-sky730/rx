#include "adapter_odev.h"
#include "adapter_odev_ble.h"
#include "app_config.h"
#include "le/att.h"
#include "le/le_user.h"
#include "ble_user.h"
#include "btcontroller_modules.h"
#include "ui_manage.h"
#include "adapter_process.h"

#define  LOG_TAG_CONST       BT
#define  LOG_TAG             "[ODEV_BLE]"
#define  LOG_ERROR_ENABLE
#define  LOG_DEBUG_ENABLE
#define  LOG_INFO_ENABLE
#define  LOG_DUMP_ENABLE
#define  LOG_CLI_ENABLE
#include "debug.h"

struct _odev_ble odev_ble = {0};
#define __this  (&odev_ble)

extern void ble_module_enable(u8 en);
extern void bt_ble_init(void);
extern void (*app_testbox_update_cbk)(int type, u32 state, void *priv);
extern void adapter_testbox_update_cbk(int type, u32 state, void *priv);

static void ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{
    /* dongle_ble_report_data_deal(report_data,search_uuid); */
    /* wireless_mic_ble_report_data_deal(report_data,search_uuid); */
    BLE_REPORT_DATA_DEAL(report_data, search_uuid);
}

static void adapter_event_callback(le_client_event_e event, u8 *packet, int size)
{
    /* dongle_adapter_event_callback(event,packet,size); */
    /* wireless_adapter_event_callback(event,packet,size); */
    ADAPTER_EVENT_CALLBACK(event, packet, size);
}
static void adapter_odev_ble_send_wakeup(void)
{
    //putchar('W');
}
ble_state_e adapter_server_ble_status = 0;
ble_state_e get_adapter_server_ble_status(void)
{
    return adapter_server_ble_status;
}

static void adapter_odev_ble_status_callback(void *priv, ble_state_e status)
{
    adapter_server_ble_status = status;
    printf("update adapter_ble_status = %d", adapter_server_ble_status);
    switch (status) {
    case BLE_ST_IDLE:
        break;
    case BLE_ST_ADV:
        break;
    case BLE_ST_CONNECT:
        break;
    case BLE_ST_DISCONN:
    case BLE_ST_SEND_DISCONN:
        adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
        adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 0);
        break;
    case BLE_ST_NOTIFY_IDICATE:
        break;
    case BLE_ST_CONNECTION_UPDATE_OK:
        printf("BLE_ST_CONNECTION_UPDATE_OK!!!!!!!");
        adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_OPEN, 0);
        adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 0);
        break;
    default:
        break;
    }
}
//client default 信息
client_conn_cfg_t odev_ble_conn_config = {
    .match_dev_cfg[0]       = NULL,      //匹配指定的名字
    .match_dev_cfg[1]       = NULL,
    .match_dev_cfg[2]       = NULL,
    .search_uuid_cnt        = 0,
    .security_en            = 0,       //加密配对
    .report_data_callback   = ble_report_data_deal,
    .event_callback         = adapter_event_callback,
};

int adapter_odev_ble_open(void *priv)
{
    r_printf("adapter_odev_ble_open\n");

    if (__this->status == ODEV_BLE_OPEN) {
        r_printf("adapter_odev_ble_already_open\n");
        return 0;
    }

    memset(__this, 0, sizeof(struct _odev_ble));

    __this->status = ODEV_BLE_OPEN;

#if BLE_CLIENT_EN || TRANS_MULTI_BLE_EN || BLE_WIRELESS_CLIENT_EN
    __this->u.client.match_type = MATCH_NULL;

    if (priv) {
        __this->u.client.parm = (struct _odev_ble_parm *)priv;

        if (__this->u.client.parm->cfg_t->match_dev_cfg[0]) {
            odev_ble_conn_config.match_dev_cfg[0] = __this->u.client.parm->cfg_t->match_dev_cfg[0];
        }

        if (__this->u.client.parm->cfg_t->match_dev_cfg[1]) {
            odev_ble_conn_config.match_dev_cfg[1] = __this->u.client.parm->cfg_t->match_dev_cfg[1];
        }

        if (__this->u.client.parm->cfg_t->match_dev_cfg[2]) {
            odev_ble_conn_config.match_dev_cfg[2] = __this->u.client.parm->cfg_t->match_dev_cfg[2];
        }

        if (__this->u.client.parm->cfg_t->search_uuid_table) {
            odev_ble_conn_config.search_uuid_table = __this->u.client.parm->cfg_t->search_uuid_table;
        }

        odev_ble_conn_config.search_uuid_cnt = __this->u.client.parm->cfg_t->search_uuid_cnt;
        odev_ble_conn_config.security_en = __this->u.client.parm->cfg_t->security_en;

        if (odev_ble_conn_config.match_dev_cfg[0]) {
            y_printf("match_dev_cfg[0] : %s\n", odev_ble_conn_config.match_dev_cfg[0]->compare_data);
        }
        if (odev_ble_conn_config.match_dev_cfg[1]) {
            y_printf("match_dev_cfg[1] : %s\n", odev_ble_conn_config.match_dev_cfg[1]->compare_data);
        }
        if (odev_ble_conn_config.match_dev_cfg[2]) {
            y_printf("match_dev_cfg[2] : %s\n", odev_ble_conn_config.match_dev_cfg[2]->compare_data);
        }
    }

    __this->u.client.opt = ble_get_client_operation_table();
    ASSERT(__this->u.client.opt, "%s : %s : %d\n", __FUNCTION__, "ble_client_api == NULL", __LINE__);

    __this->u.client.opt->init_config(0, &odev_ble_conn_config);

#if TRANS_MULTI_BLE_MASTER_NUMS
    extern void ble_vendor_set_hold_prio(u8 role, u8 enable);
    ble_vendor_set_hold_prio(0, 1);

    le_multi_client_pair_reconnect_search_enable(0);

#ifndef CONFIG_NEW_BREDR_ENABLE
    //优化多连接,卡音
    bredr_link_vendor_support_packet_enable(PKT_TYPE_2DH5_EU, 0);
#endif //CONFIG_NEW_BREDR_ENABLE
#endif

#endif //#if BLE_CLIENT_EN || TRANS_MULTI_BLE_EN || BLE_WIRELESS_CLIENT_EN

#if TRANS_DATA_EN || BLE_WIRELESS_SERVER_EN
    printf("sel BLE_WIRELESS_SERVER_EN !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    ble_get_server_operation_table(&__this->u.server.opt);
#if (APP_MAIN ==  APP_WIRELESS_MIC_2T1)
    extern void wireless_mic_server_recieve_data(void *priv, void *data, u16 len);
    __this->u.server.opt->regist_recieve_cbk(0, wireless_mic_server_recieve_data);
#endif
    __this->u.server.opt->regist_state_cbk(0, adapter_odev_ble_status_callback);
    __this->u.server.opt->regist_wakeup_send(NULL, adapter_odev_ble_send_wakeup);
#endif

#if TCFG_TEST_BOX_ENABLE
    if (!app_testbox_update_cbk) {
        app_testbox_update_cbk = adapter_testbox_update_cbk;
    }
#endif
    return 0;
}

void adapter_odev_ble_search_device(void)
{
#if BLE_CLIENT_EN || TRANS_MULTI_BLE_EN || BLE_WIRELESS_CLIENT_EN
    ble_module_enable(1);
#endif
}
int adapter_odev_ble_start(void *priv)
{
    r_printf("adapter_odev_ble_start\n");

    if (__this->status == ODEV_BLE_START) {
        r_printf("adapter_odev_ble_already_start\n");
        return 0;
    }
    __this->status = ODEV_BLE_START;

#if TCFG_USER_BLE_ENABLE
#if TCFG_NORMAL_SET_DUT_MODE
    ble_standard_dut_test_init();
    r_printf("adapter_odev_enter_ble_dut\n");
#if TCFG_USER_BT_CLASSIC_ENABLE
    bt_init();
    bredr_set_dut_enble(1, 0);
    lmp_hci_write_scan_enable(3);
    r_printf("adapter_odev_enter_edr_dut\n");
#endif
#else
    bt_ble_init();
    r_printf("adapter_odev_enter_ble_normal\n");
#endif

#endif
#if BOARD_TEST_EN
    extern int board_test_uart_init();
    board_test_uart_init();
#endif

    return 0;
}

int adapter_odev_ble_stop(void *priv)
{
    r_printf("adapter_odev_ble_stop\n");
    if (__this->status == ODEV_BLE_STOP) {
        r_printf("adapter_odev_ble_already_stop\n");
        return 0;
    }

    __this->status = ODEV_BLE_STOP;

#if TCFG_USER_BLE_ENABLE
    ble_module_enable(0);
#endif

    return 0;
}

int adapter_odev_ble_close(void *priv)
{
    r_printf("adapter_odev_ble_close\n");

    if (__this->status == ODEV_BLE_CLOSE) {
        r_printf("adapter_odev_ble_already_close\n");
        return 0;
    }
    __this->status = ODEV_BLE_CLOSE;

#if TCFG_USER_BLE_ENABLE
    ble_module_enable(0);
#endif
    __this->u.client.opt = NULL;
    return 0;
}


int adapter_odev_ble_send(void *priv, u8 *buf, u16 len)
{
    int ret = 0;
    if (!__this->ble_connected) {
        r_printf("adapter_odev_ble not connect\n");
        return -1;
    }
#if TCFG_USER_BLE_ENABLE
    ret = ADAPTER_ODEV_BLE_SEND(priv, buf, len);
#endif
    return ret;
}

int adapter_odev_ble_config(int cmd, void *parm)
{
    switch (cmd) {
    case 0:
        break;
    default:
        break;
    }

    return 0;
}
