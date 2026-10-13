#include "adapter_odev.h"
#include "adapter_odev_ble.h"
#include "app_config.h"
#include "le/att.h"
#include "le/le_user.h"
#include "ble_user.h"
#include "btcontroller_modules.h"
#include "adapter_process.h"
#include "asm/pwm_led.h"
#include "ui_manage.h"

#define  LOG_TAG_CONST       BT
#define  LOG_TAG             "[ODEV_BLE]"
#define  LOG_ERROR_ENABLE
#define  LOG_DEBUG_ENABLE
#define  LOG_INFO_ENABLE
#define  LOG_DUMP_ENABLE
#define  LOG_CLI_ENABLE
#include "debug.h"



#if (APP_MAIN == APP_WIRELESS_MIC_1T1 || APP_MAIN == APP_WIRELESS_MIC_2T1||APP_MAIN == APP_WIRELESS_MIC_1TN || APP_MAIN == APP_WIRELESS_DUPLEX)

#define __this  (&odev_ble)

#define CLIENT_SENT_TEST    0

static u16 ble_client_timer = 0;

static void client_test_write(void)
{
    static u8 count = 0;

    u32 maxVaildAttValue = 141;

    u8 test_buf[145];

    printf(" %x ", count);
    memset(test_buf, count, sizeof(test_buf));
    int ret = __this->u.client.opt->opt_comm_send(0x0006, &test_buf, maxVaildAttValue + 4, ATT_OP_WRITE_WITHOUT_RESPOND);

    if (APP_BLE_BUFF_FULL != ret) {
        count++;
    }
}

void wireless_mic_ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{
    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION:
        //notify
        //y_printf("value_handle:%x conn_handle:%x uuid:%x\n", report_data->value_handle, report_data->conn_handle, search_uuid->services_uuid16);
        //extern int adapter_wireless_dec_frame_write(void *priv, void *data, u16 len);
        //adapter_wireless_dec_frame_write(NULL,report_data->blob,report_data->blob_length);

#if (APP_MAIN == APP_WIRELESS_DUPLEX)
        extern int adapter_wireless_dec_frame_write(void *data, u16 len);
        adapter_wireless_dec_frame_write(report_data->blob, report_data->blob_length);
#endif
        break;

    case GATT_EVENT_INDICATION:                                 //indicate
        log_info("indication handle:%04x,len= %d\n", report_data->value_handle, report_data->blob_length);
        put_buf(report_data->blob, report_data->blob_length);
        break;

    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:          //read
        log_info("read handle:%04x,len= %d\n", report_data->value_handle, report_data->blob_length);
        put_buf(report_data->blob, report_data->blob_length);
        break;

    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT:     //read long
        log_info("read_long handle:%04x,len= %d\n", report_data->value_handle, report_data->blob_length);
        put_buf(report_data->blob, report_data->blob_length);
        break;

    default:
        break;
    }
}

extern void sys_auto_shut_down_enable(void);
extern void sys_auto_shut_down_disable(void);
void wireless_adapter_event_callback(le_client_event_e event, u8 *packet, int size)
{
    switch (event) {
    case CLI_EVENT_MATCH_DEV:
        client_match_cfg_t *match_dev = packet;
        log_info("match_name:%s\n", match_dev->compare_data);
        break;

    case CLI_EVENT_MATCH_UUID:
        r_printf("CLI_EVENT_MATCH_UUID\n");
        opt_handle_t *opt_hdl = packet;
        log_info("match_uuid handle : %x\n", opt_hdl->value_handle);
        break;

    case CLI_EVENT_SEARCH_PROFILE_COMPLETE:
        log_info("search profile commplete");
        __this->ble_connected = 2;
        if (__this->u.client.match_type == MATCH_NULL) {
            if (__this->u.client.opt) {
                u16 ccc_val = 1;
                __this->u.client.opt->opt_comm_send(0x0009, &ccc_val, 2, ATT_OP_WRITE);
            }
        }
        break;

    case CLI_EVENT_CONNECTED:
        log_info("bt connected");
        __this->ble_connected = 1;
        adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 0);

        break;

    case CLI_EVENT_CONNECTION_UPDATE:
#if CLIENT_SENT_TEST
        ble_client_timer = sys_timer_add(NULL, client_test_write, 100);
#endif

        adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_OPEN, 0);
        break;

    case CLI_EVENT_DISCONNECT:
        __this->ble_connected = 0;
#if CLIENT_SENT_TEST
        if (ble_client_timer) {
            sys_timer_del(ble_client_timer);
        }
        ble_client_timer = 0;
#endif
        adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
        adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 0);
        log_info("bt disconnect");
        break;

    default:
        break;
    }
}
extern u8 wireless_conn_status;
int wireless_mic_odev_ble_send(void *priv, u8 *buf, u16 len)
{
    int ret = -1;
#if ALWAYS_RUN_STREAM
    if (!wireless_conn_status) {	//no connect,no send data
        return 0;
    }
#endif
#if BLE_CLIENT_EN || TRANS_MULTI_BLE_EN || BLE_WIRELESS_CLIENT_EN
    if (__this->u.client.opt) {
        ret = __this->u.client.opt->opt_comm_send(0x0006, buf, len, ATT_OP_WRITE_WITHOUT_RESPOND);
    }
#endif

#if TRANS_DATA_EN || BLE_WIRELESS_SERVER_EN
    if (__this->u.server.opt) {
        u32 vaild_len = __this->u.server.opt->get_buffer_vaild(0);
        if (vaild_len >= len) {
            ret = __this->u.server.opt->send_data(NULL, buf, len);
        }
    }
#endif

#if BLE_WIRELESS_1T1_TX_EN
    ret = wlm_1t1_tx_iso(buf, len);
#elif BLE_WIRELESS_1TN_TX_EN
    ret = wlm_1tN_tx_iso(buf, len);
#endif
    return ret;
}

#else
void wireless_mic_ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{

}
void wireless_adapter_event_callback(le_client_event_e event, u8 *packet, int size)
{

}
int wireless_mic_odev_ble_send(void *priv, u8 *buf, u16 len)
{
    return 0;
}
#endif
