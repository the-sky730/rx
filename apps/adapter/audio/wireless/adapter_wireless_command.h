#ifndef __ADAPTER_WIRELESS_COMMAND_H__
#define __ADAPTER_WIRELESS_COMMAND_H__

#include "generic/typedef.h"


enum adapter_wireless_cmd {
    WIRELESS_MIC_DENOISE_OFF 		 = 1,	//降噪
    WIRELESS_MIC_DENOISE_LEVEL_1	 = 2,
    WIRELESS_MIC_DENOISE_LEVEL_2     = 3,
    WIRELESS_MIC_DENOISE_LEVEL_MAX	 = 4,

    WIRELESS_MIC_ECHO_OFF			 = 5,	//混响
    WIRELESS_MIC_ECHO_ON			 = 6,

    WIRELESS_TRANSPORT_CMD_PP,
    WIRELESS_TRANSPORT_CMD_PREV,
    WIRELESS_TRANSPORT_CMD_NEXT,
    WIRELESS_TRANSPORT_CMD_VOL_DOWN,
    WIRELESS_TRANSPORT_CMD_VOL_UP,

};


struct wl_expand {
    u8 ex_len: 3; //头部数据长度， 0表示无扩展
    u8 channel: 1;//0:L, 1:R
    u8 reserved: 4;
};

#endif
