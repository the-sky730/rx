#include "adapter_idev.h"
#include "adapter_idev_ble.h"
#include "app_config.h"
#include "le/att.h"
#include "le/le_user.h"
#include "ble_user.h"
#include "btcontroller_modules.h"
#include "adapter_process.h"
#include "asm/pwm_led.h"
#include "ui_manage.h"
#include "adapter_wireless_command.h"

#define  LOG_TAG_CONST       BT
#define  LOG_TAG             "[IDEV_BLE]"
#define  LOG_ERROR_ENABLE
#define  LOG_DEBUG_ENABLE
#define  LOG_INFO_ENABLE
#define  LOG_DUMP_ENABLE
#define  LOG_CLI_ENABLE
#include "debug.h"



#if (APP_MAIN == APP_WIRELESS_MIC_1T1 || APP_MAIN == APP_WIRELESS_MIC_2T1 || APP_MAIN == APP_WIRELESS_DUPLEX)

#if(APP_MAIN == APP_WIRELESS_MIC_2T1)
struct __dualmic_handle_map {
    u16 conn_handle;
    u8  dec_channel;
    u8  dec_role;//L OR R
};
struct __dualmic_handle_map dualmic_handle_map[2] = {
    {0xffff, 0, 0xff},
    {0xffff, 1,	0xff},
};
static u8 dualmic_conn_2_dec(u16 conn_handle)
{
    for (int i = 0; i < 2; i++) {
        if (dualmic_handle_map[i].conn_handle == conn_handle) {
            return dualmic_handle_map[i].dec_channel;
        }
    }
    printf("not match !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    return 0xff;
}

u16 dualmic_dec_ch_2_conn_handle(u8 dec_ch)
{
    for (int i = 0; i < 2; i++) {
        if (dualmic_handle_map[i].dec_channel == dec_ch) {
            return dualmic_handle_map[i].conn_handle;
        }
    }

    return 0xffff;

}


//使能立体声输入的，channel = 0	发送给左声道	channel = 1 发送给右声道
//没有使能立体声输入的，按连接上的顺序发送给TX
void wireless_mic_client_send_data(u8 channel, u8 *data, u16 len)
{
    if (channel >= 2) {
        return;
    }
#if WIRELESS_TX_MIC_STEREO_OUTPUT && (!WIRELESS_TX_MIC_STEREO_OUTPUT_AUTO)
    for (u8 i = 0; i < 2; i++) {
        if (dualmic_handle_map[i].dec_role == channel) {
            client_2t1_write_send(dualmic_handle_map[i].conn_handle, data, len);
            return;
        }
    }
#else
    if (dualmic_handle_map[channel].conn_handle != 0xffff) {
        client_2t1_write_send(dualmic_handle_map[channel].conn_handle, data, len);
        return;
    }
#endif
    printf("disconn send err");
}

static void dualmic_set_dec_role(u16 conn_handle, u8 channel)
{
    for (int i = 0; i < 2; i++) {
        if (dualmic_handle_map[i].conn_handle == conn_handle) {
            if (dualmic_handle_map[i].dec_role == 0xff) {
                if (channel == 0) {
                    printf("LLLLLLLLLLLLLLL_CONN");
                } else if (channel == 1) {
                    printf("RRRRRRRRRRRRRRR_CONN");
                }
            }
            dualmic_handle_map[i].dec_role = channel;
        }
    }
}

static u8 dualmic_get_dec_role(u16 conn_handle)
{
    for (int i = 0; i < 2; i++) {
        if (dualmic_handle_map[i].conn_handle == conn_handle) {
            return dualmic_handle_map[i].dec_role;
        }
    }
    printf("dualmic_get_dec_role, not match !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    return 0xff;
}

static void dualmic_handle_map_update(u16 conn_handle, u8 flag)
{
    if (flag) {
        for (int i = 0; i < 2; i++) {
            if (dualmic_handle_map[i].conn_handle == conn_handle) {
                //disconnect 逻辑
                printf("warning conn channel exist!!!!!!!!!!!!!!!!!!!!!\n");
                printf("warning conn channel exist!!!!!!!!!!!!!!!!!!!!!\n");
                printf("warning conn channel exist!!!!!!!!!!!!!!!!!!!!!\n");
                return ;
            }
        }
        for (int i = 0; i < 2; i++) {
            if (dualmic_handle_map[i].conn_handle == 0xffff) {
                //connect 逻辑
                printf("func = %s, connect !!\n", __FUNCTION__);
                dualmic_handle_map[i].conn_handle = conn_handle;
                return ;
            }
        }
    } else {
        for (int i = 0; i < 2; i++) {
            if (dualmic_handle_map[i].conn_handle == conn_handle) {
                //disconnect 逻辑
                printf("func = %s, disconnect !!\n", __FUNCTION__);
                dualmic_handle_map[i].conn_handle = 0xffff;
                return ;
            }
        }
    }
}
static u8 dualmic_handle_map_get_connected_num(void)
{
    u8 cnt = 0;
    for (int i = 0; i < 2; i++) {
        if (dualmic_handle_map[i].conn_handle != 0xffff) {
            cnt ++;
        }
    }
    return cnt;
}
#endif

#define __this  (&idev_ble)

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

void wireless_mic_ble_idev_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{
    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION:
        //notify
        //y_printf("value_handle:%x conn_handle:%x uuid:%x\n", report_data->value_handle, report_data->conn_handle, search_uuid->services_uuid16);

#if (APP_MAIN == APP_WIRELESS_MIC_2T1)
        extern int adapter_dec_dual_jla_frame_write(u8 channel, void *data, u16 len);
        u8 dec_channel = dualmic_conn_2_dec(report_data->conn_handle);
#if WIRELESS_TX_MIC_STEREO_OUTPUT && (!WIRELESS_TX_MIC_STEREO_OUTPUT_AUTO)
        u8 *buf = (u8 *)(report_data->blob);
        u8 channle_type = 0xff;
        struct wl_expand *expand_data = (struct wl_expand *)&buf[1];
        if (expand_data->channel == 0) {
            //左声道的数据
            channle_type = 0;
            //putchar('L');
        } else {
            //右声道的数据
            channle_type = 1;
            //putchar('R');
        }
        dualmic_set_dec_role(report_data->conn_handle, channle_type);
        dec_channel = dualmic_get_dec_role(report_data->conn_handle);
#endif
        adapter_dec_dual_jla_frame_write(dec_channel, report_data->blob, report_data->blob_length);
        adapter_dec_dual_jla_frame_write_pause(dec_channel, 0);
#else
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
void wireless_adapter_idev_event_callback(le_client_event_e event, u8 *packet, int size)
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

#if(APP_MAIN == APP_WIRELESS_MIC_2T1)
        u16 *conn_handle = (u16 *)packet;
        if (dualmic_handle_map_get_connected_num() == 0) {
            printf("first connect !!, media open\n");
            adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 1);
            adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_OPEN, 0);
        } else {
            adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 2);
            printf("second connect !!\n");
        }
        dualmic_handle_map_update(*conn_handle, 1);
#else
        adapter_process_event_notify(ADAPTER_EVENT_CONNECT, 0);
        adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_OPEN, 0);
#endif
        break;

    case CLI_EVENT_CONNECTED:
        log_info("bt connected");
        __this->ble_connected = 1;

        break;
#if TCFG_2T1_RX_PRODUCT_TEST_EN
    case CLI_EVENT_ENTER_TEST_AUDIO_SYNC:
        adapter_process_event_notify(ADAPTER_EVENT_SWITCH_RX_TEST, 0);
        break;
#endif
    case CLI_EVENT_CONNECTION_UPDATE:
#if CLIENT_SENT_TEST
        ble_client_timer = sys_timer_add(NULL, client_test_write, 100);
#endif
        break;

    case CLI_EVENT_DISCONNECT:
        __this->ble_connected = 0;
#if CLIENT_SENT_TEST
        if (ble_client_timer) {
            sys_timer_del(ble_client_timer);
        }
        ble_client_timer = 0;
#endif
        log_info("bt disconnec");

#if(APP_MAIN == APP_WIRELESS_MIC_2T1)
        u16 tmp_handle = little_endian_read_16(packet, 3);
        u8 channel = dualmic_conn_2_dec(tmp_handle);
#if WIRELESS_TX_MIC_STEREO_OUTPUT && (!WIRELESS_TX_MIC_STEREO_OUTPUT_AUTO)
        channel = dualmic_get_dec_role(tmp_handle);
#endif
        adapter_dec_dual_jla_frame_write_pause(channel, 1);
        if (channel == 0) {
            printf("LLLLLLLLLLLLLLL_DISCON");
        } else if (channel == 1) {
            printf("RRRRRRRRRRRRRRR_DISCON");
        }
#if WIRELESS_TX_MIC_STEREO_OUTPUT && (!WIRELESS_TX_MIC_STEREO_OUTPUT_AUTO)
        dualmic_set_dec_role(tmp_handle, 0xff);
#endif


        dualmic_handle_map_update(tmp_handle, 0);
        if (dualmic_handle_map_get_connected_num() == 0) {
            printf("all disconnted !!, media close\n");
            adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_CLOSE, 0);
            adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 1);
        } else {
            adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 2);
            printf("not all disconnted !! \n");
        }
#else
        adapter_process_event_notify(ADAPTER_EVENT_DISCONN, 0);
        adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_CLOSE, 0);
#endif

        break;

    default:
        break;
    }
}
extern u8 wireless_conn_status ;
int wireless_mic_idev_ble_send(void *priv, u8 *buf, u16 len)
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
    return ret;
}

#else
void wireless_mic_ble_idev_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{

}
void wireless_adapter_idev_event_callback(le_client_event_e event, u8 *packet, int size)
{

}
int wireless_mic_idev_ble_send(void *priv, u8 *buf, u16 len)
{
    return 0;
}
/* int wireless_mic_ble_send(void *priv, u8 *buf, u16 len) */
/* { */
/* return 0; */
/* } */
#endif
