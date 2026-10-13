#include "adapter_odev.h"
#include "adapter_odev_ble.h"
#include "app_config.h"
#include "le/att.h"
#include "le/le_user.h"
#include "ble_user.h"
#include "btcontroller_modules.h"
#include "adapter_process.h"

#define  LOG_TAG_CONST       BT
#define  LOG_TAG             "[ODEV_BLE]"
#define  LOG_ERROR_ENABLE
#define  LOG_DEBUG_ENABLE
#define  LOG_INFO_ENABLE
#define  LOG_DUMP_ENABLE
#define  LOG_CLI_ENABLE
#include "debug.h"

#if (APP_MAIN == APP_DONGLE)

#define __this  (&odev_ble)

static const u16 mouse_notify_handle[1]     = {0x001C};
static const u16 keyboard_notify_handle[1]  = {0x0036};
static const u16 mouse_ccc_value            = 0x0001;

extern int keyboard_hid_send_data(const u8 *buf, u32 len);
extern int mouse_hid_send_data(const u8 *buf, u32 len);

static void mouse_enable_notify_ccc(void)
{
    ASSERT(__this->u.client.opt, "%s : %s : %d\n", __FUNCTION__, "ble_client_api == NULL", __LINE__);

    __this->u.client.opt->opt_comm_send(mouse_notify_handle[0] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
}

static void keyboard_enable_notify_ccc(void)
{
    ASSERT(__this->u.client.opt, "%s : %s : %d\n", __FUNCTION__, "ble_client_api == NULL", __LINE__);

    __this->u.client.opt->opt_comm_send(keyboard_notify_handle[0] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
}

void dongle_ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{
    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION: {
        //notify
        /* y_printf("value_handle:%x conn_handle:%x\n", report_data->value_handle, report_data->conn_handle); */
        //send hid data to pc
        if (report_data->value_handle == mouse_notify_handle[0]) {
            u8 mouse_packet[10];
            //g_printf("mouse key:");
            if ((report_data->blob_length + 1) > sizeof(mouse_packet)) {
                g_printf("blob_length len err!!\n");
                break;
            }
            mouse_packet[0] = 1;
            memcpy(&mouse_packet[1], report_data->blob, report_data->blob_length);
            //printf_buf(mouse_packet,sizeof(mouse_packet));
            mouse_hid_send_data(mouse_packet, sizeof(mouse_packet));
        } else if (report_data->value_handle == keyboard_notify_handle[0]) {
            u8 keyboard_packet[9];
            //g_printf("keyboard key:");
            if ((report_data->blob_length + 1) > sizeof(keyboard_packet)) {
                g_printf("blob_length len err!!\n");
                break;
            }
            keyboard_packet[0] = 1;
            memcpy(&keyboard_packet[1], report_data->blob, report_data->blob_length);
            //printf_buf(keyboard_packet,sizeof(keyboard_packet));
            keyboard_hid_send_data(keyboard_packet, sizeof(keyboard_packet));
        }
    }
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

void dongle_adapter_event_callback(le_client_event_e event, u8 *packet, int size)
{
    switch (event) {
    case CLI_EVENT_MATCH_DEV:
        client_match_cfg_t *match_dev = packet;
        log_info("match_name:%s\n", match_dev->compare_data);
        if (odev_ble_conn_config.match_dev_cfg[0]) {
            if (!memcmp(odev_ble_conn_config.match_dev_cfg[0]->compare_data, match_dev->compare_data, strlen(odev_ble_conn_config.match_dev_cfg[0]->compare_data))) {
                r_printf("match keyboard,bond=%d\n", odev_ble_conn_config.match_dev_cfg[0]->bonding_flag);
                __this->u.client.match_type = MATCH_KEYBOARD;
            }
        }

        if (odev_ble_conn_config.match_dev_cfg[1]) {
            if (!memcmp(odev_ble_conn_config.match_dev_cfg[1]->compare_data, match_dev->compare_data, strlen(odev_ble_conn_config.match_dev_cfg[0]->compare_data))) {
                r_printf("match mouse,bond=%d\n", odev_ble_conn_config.match_dev_cfg[0]->bonding_flag);
                __this->u.client.match_type = MATCH_MOUSE;
            }
        }
        break;

    case CLI_EVENT_MATCH_UUID:
        r_printf("CLI_EVENT_MATCH_UUID\n");
        opt_handle_t *opt_hdl = packet;
        log_info("match_uuid handle : %x\n", opt_hdl->value_handle);
        break;

    case CLI_EVENT_SEARCH_PROFILE_COMPLETE:
        log_info("search profile commplete");
        if (__this->u.client.match_type == MATCH_MOUSE) {
            mouse_enable_notify_ccc();
        } else if (__this->u.client.match_type == MATCH_KEYBOARD) {
            keyboard_enable_notify_ccc();
        }

        __this->ble_connected = 2;
        break;

    case CLI_EVENT_CONNECTED:
        log_info("bt connected");
        __this->ble_connected = 1;
        __this->u.client.match_type = MATCH_NULL;
        adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 0);
        break;

    case CLI_EVENT_DISCONNECT:
        __this->ble_connected = 0;
        log_info("bt disconnec");
        adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 0);
        break;

    default:
        break;
    }
}

int dongle_ble_send(void *priv, u8 *buf, u16 len)
{
    int ret = 0;
#if 0
    if (len > 145) {
        log_info("ble send max len is 145 !!!!!\n");
        return -1;
    }

    int ret = __this->u.client.opt->opt_comm_send(0x0006, buf, len, ATT_OP_WRITE_WITHOUT_RESPOND);
#endif

    return ret;
}

#else
void dongle_ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{

}
void dongle_adapter_event_callback(le_client_event_e event, u8 *packet, int size)
{

}
int dongle_ble_send(void *priv, u8 *buf, u16 len)
{
    return 0;
}
#endif
