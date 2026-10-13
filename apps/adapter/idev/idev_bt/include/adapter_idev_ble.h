#ifndef __ADAPTER_IDEV_BLE_H__
#define __ADAPTER_IDEV_BLE_H__

#include "generic/typedef.h"
#include "le_client_demo.h"

enum {
    IDEV_BLE_CLOSE,
    IDEV_BLE_OPEN,
    IDEV_BLE_START,
    IDEV_BLE_STOP,
};

struct _idev_ble_parm {
    client_conn_cfg_t *cfg_t;
};

struct idev_ble_client {
    u16 ble_timer_id;
    u8 match_type;
    struct _idev_ble_parm *parm;
    struct ble_client_operation_t *opt;
};

struct idev_ble_server {
    struct ble_server_operation_t *opt;
};

struct _idev_ble {
    u8 status;
    u16 ble_connected;
    union {
        struct idev_ble_client client;
        struct idev_ble_server server;
    } u;
};
int adapter_idev_ble_open(void *priv);
int adapter_idev_ble_start(void *priv);
int adapter_idev_ble_stop(void *priv);
int adapter_idev_ble_close(void *priv);
int adapter_idev_ble_config(int cmd, void *parm);

extern struct _idev_ble idev_ble;
extern client_conn_cfg_t idev_ble_conn_config;

#endif//__ADAPTER_IDEV_BLE_H__

