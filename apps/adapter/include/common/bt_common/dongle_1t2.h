#ifndef _DONGLE_1T2_
#define _DONGLE_1T2_


#include "generic/typedef.h"
#include "event.h"
#include "app_config.h"
#include "adapter_odev_edr.h"

#if USER_SUPPORT_DUAL_A2DP_SOURCE

#define SEARCH_TIME 	   60000//60s

enum de_ch {
    DE_CH1 = 1,
    DE_CH2 = 2,
};
enum de_status {
    DB_EM_NOSEARCHING = 0,
    DB_EM_SEARCHING,
    DB_EM_RECONNECT,
    DB_EM_CONNECTED,

};
struct led_mode_t {
    u16 poweron;
    u16 connect;
    u16 disconn;
    u16 search;
};
struct de_role {
    u8 status;
    u8 mac_addr[6];
    u16 search_timer_id;
    struct led_mode_t ledmode;
};


extern struct de_role de_role1;
extern struct de_role de_role2;

void dongle_1t2_recon(void);
void dongle_1t2_bt_disconn(struct bt_event *bt);
void dongle_1t2_connect(struct bt_event *bt);
void dongle_1t2_hci_disconn(void);
void dongle_1t2_init(struct _odev_edr *edr);
void dongle_1t2_discon_ch(u8 ch);
void dongle_1t2_ch_discon_search(u8 ch);
#endif
#endif
