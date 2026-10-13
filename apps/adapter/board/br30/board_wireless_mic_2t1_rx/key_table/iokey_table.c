#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "app_task.h"

#ifdef CONFIG_BOARD_WIRELESS_MIC_2T1_RX
const u16 key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX] = {
    //单击             //长按          //hold         //抬起            //双击                //三击
    [0] = {
        KEY_MUSIC_PP,	KEY_POWEROFF,	KEY_POWEROFF_HOLD,	KEY_NULL,	KEY_WIRELESS_MIC_ECHO_SET,	KEY_WIRELESS_MIC_CH_SW
    },
    [1] = {
        KEY_NULL,	KEY_NULL,			KEY_NULL,	KEY_NULL,	KEY_NULL, KEY_NULL
    },
    [2] = {
        KEY_NULL,	KEY_NULL,			KEY_NULL,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [3] = {
        KEY_NULL, KEY_NULL,			KEY_NULL,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [4] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
    [5] = {
        KEY_NULL,		KEY_NULL,			KEY_NULL,	KEY_NULL,	KEY_NULL,			KEY_NULL
    },
};
#endif
