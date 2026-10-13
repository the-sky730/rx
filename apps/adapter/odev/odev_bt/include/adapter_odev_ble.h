#ifndef __ADAPTER_ODEV_BLE_H__
#define __ADAPTER_ODEV_BLE_H__

#include "generic/typedef.h"
#include "le_client_demo.h"
#include "app_config.h"

#define DEVICE_RSSI_LEVEL        (-50)
#define POWER_ON_PAIR_TIME       (3500)//unit ms,切换搜索回连周期

struct _odev_ble_parm {
    client_conn_cfg_t *cfg_t;
};

enum {
    ODEV_BLE_CLOSE,
    ODEV_BLE_OPEN,
    ODEV_BLE_START,
    ODEV_BLE_STOP,
};

enum {
    MATCH_KEYBOARD,
    MATCH_MOUSE,
    MATCH_NULL,
};

struct odev_ble_client {
    u16 ble_timer_id;
    u8 match_type;
    struct _odev_ble_parm *parm;
    struct ble_client_operation_t *opt;
};

struct odev_ble_server {
    struct ble_server_operation_t *opt;
};

struct _odev_ble {
    u8 status;
    u16 ble_connected;
    union {
        struct odev_ble_client client;
        struct odev_ble_server server;
    } u;
};

int adapter_odev_ble_open(void *priv);
int adapter_odev_ble_start(void *priv);
int adapter_odev_ble_stop(void *priv);
int adapter_odev_ble_close(void *priv);
int adapter_odev_ble_config(int cmd, void *parm);
int adapter_odev_ble_send(void *priv, u8 *buf, u16 len);

void adapter_odev_ble_search_device(void);

//dongle
void dongle_ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid);
void dongle_adapter_event_callback(le_client_event_e event, u8 *packet, int size);
int dongle_ble_send(void *priv, u8 *buf, u16 len);

//wireless_mic
void wireless_mic_ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid);
void wireless_adapter_event_callback(le_client_event_e event, u8 *packet, int size);
int wireless_mic_odev_ble_send(void *priv, u8 *buf, u16 len);

#if (APP_MAIN == APP_DONGLE) || (APP_MAIN == APP_DONGLE_1T2)
#define BLE_REPORT_DATA_DEAL(a,b)       dongle_ble_report_data_deal(a,b)
#define ADAPTER_EVENT_CALLBACK(a,b,c)   dongle_adapter_event_callback(a,b,c)
#define ADAPTER_ODEV_BLE_SEND(a,b,c)    dongle_ble_send(a,b,c)
#elif ((APP_MAIN == APP_WIRELESS_MIC_1T1) || (APP_MAIN == APP_WIRELESS_MIC_2T1) || (APP_MAIN == APP_WIRELESS_DUPLEX))
#define BLE_REPORT_DATA_DEAL(a,b)       wireless_mic_ble_report_data_deal(a,b)
#define ADAPTER_EVENT_CALLBACK(a,b,c)   wireless_adapter_event_callback(a,b,c)
#define ADAPTER_ODEV_BLE_SEND(a,b,c)    wireless_mic_odev_ble_send(a,b,c)
#elif (APP_MAIN == APP_WIRELESS_MIC_1TN)
#define BLE_REPORT_DATA_DEAL(a,b)       //dongle_ble_report_data_deal(a,b)
#define ADAPTER_EVENT_CALLBACK(a,b,c)   //dongle_adapter_event_callback(a,b,c)
#define ADAPTER_ODEV_BLE_SEND(a,b,c)    wireless_mic_odev_ble_send(a,b,c)
#endif

extern struct _odev_ble odev_ble;
extern client_conn_cfg_t odev_ble_conn_config;

#endif//__ADAPTER_ODEV_BLE_H__

