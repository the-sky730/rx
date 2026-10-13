#ifndef ADAPTER_APP_STATUS_H
#define ADAPTER_APP_STATUS_H


#include "system/includes.h"

enum {
    APP_NORMAL_STATUS  = 1,
    APP_CHARGE_STATUS,
    APP_STATUS_MAX_INDEX,
};

extern u8 app_curr_status;
#define app_get_curr_status()		app_curr_status
#define app_switch_normal()			app_curr_status = APP_NORMAL_STATUS

#endif
