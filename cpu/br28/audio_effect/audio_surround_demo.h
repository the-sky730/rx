#ifndef __AUDIO_SURROUND_DEMO__H
#define __AUDIO_SURROUND_DEMO__H
#include "media/effects_adj.h"
#include "media/audio_surround.h"
#include "app_config.h"
#include "clock_cfg.h"

enum {
    KARAOKE_SPK_OST,//原声
    KARAOKE_SPK_DBB,//重低音
    KARAOKE_SPK_SURROUND,//全景环绕
    KARAOKE_SPK_3D,//3d环绕
    KARAOKE_SPK_FLOW_VOICE,//流动人声
    KARAOKE_SPK_KING,//王者荣耀
    KARAOKE_SPK_WAR,//刺激战场
    KARAOKE_SPK_MAX,
};

surround_hdl *surround_open_demo(u32 sur_name, u8 ch_type);
void surround_close_demo(surround_hdl *surround);

void audio_surround_effect_switch_demo(u32 sur_name, u8 eff_mode);

#endif
