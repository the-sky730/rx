#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "app_task.h"

#ifdef CONFIG_BOARD_WIRELESS_MIC_2T1_RX
const u16 key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_CHANGE_MODE, KEY_POWEROFF,		KEY_POWEROFF_HOLD,	KEY_NULL,	    KEY_NULL,			KEY_NULL
    },
    [1] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [2] = {
        KEY_MUSIC_PP,	KEY_CALL_HANG_UP,	KEY_NULL,			KEY_NULL,		KEY_CALL_LAST_NO,	KEY_NULL
    },
    [3] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_MUSIC_PREV,	KEY_VOL_DOWN,		KEY_VOL_DOWN,		KEY_NULL,		KEY_ENC_START,			KEY_NULL
    },
    [5] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [6] = {
        KEY_MUSIC_NEXT,	KEY_VOL_UP,			KEY_VOL_UP,			KEY_NULL,		KEY_REVERB_OPEN,	KEY_NULL
    },
    [7] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [8] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
    [9] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,			KEY_NULL,		KEY_NULL,			KEY_NULL
    },
};
#endif
