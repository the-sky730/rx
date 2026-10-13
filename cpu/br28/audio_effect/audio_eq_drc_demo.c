
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "clock_cfg.h"
#include "media/audio_eq_drc_apply.h"
#include "audio_eq_drc_demo.h"
#include "media/effects_adj.h"
#include "audio_effect/audio_sound_track_2_p_x.h"
#include "audio_eff_default_parm.h"

struct music_parm_tool_set music_mode[mode_add];//音乐模式
struct music_eq2_tool music_eq2_parm[mode_add];//音乐模式下第二eq
struct phone_parm_tool_set phone_mode[4];//通话上下行模式 0:下行宽 1：下行窄  2：上行宽  3:上行窄
struct music_eq_tool rl_eq_parm[mode_add];//rl通道eq
struct nband_drc rl_drc_parm[mode_add];//rl通道drc


#if 0
//系数切换
void eq_sw_demo()
{
    eq_mode_sw();//7种默认系数切换
}

//获取当前eq系数表类型
void eq_mode_get_demo()
{
    u8 mode ;
    mode = eq_mode_get_cur();
}
//宏TCFG_USE_EQ_FILE配0
//自定义系数表动态更新
//本demo 示意更新中心截止频率，增益，总增益，如需设置更多参数，请查看eq_config.h头文件的demo
void eq_update_demo()
{
    eq_mode_set_custom_info(0, 200, 2);//第0段,200Hz中心截止频率，2db
    eq_mode_set_custom_info(5, 2000, 2);//第5段,2000Hz中心截止频率，2db

    cur_eq_set_global_gain(AEID_MUSIC_EQ, -1);//-1表示 -1dB
    eq_mode_set(EQ_MODE_CUSTOM);//设置系数、总增益更新
}

#endif

//用户自定义eq  drc系数
static const struct eq_seg_info your_audio_out_eq_tab[] = {//2段系数
    {0, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 12000, 0, 0.7f},
};
static float your_eq_coeff_tab[2][5];//2段eq系数表的长度
/*----------------------------------------------------------------------------*/
/**@brief    用户自定义eq的系数回调
   @param    eq:句柄
   @param    sr:采样率
   @param    info: 系数地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int eq_get_filter_info_demo(void *_eq, int sr, struct audio_eq_filter_info *info)
{
    if (!sr) {
        sr = 44100;
    }
#if TCFG_EQ_ENABLE
    local_irq_disable();
    u8 nsection = ARRAY_SIZE(your_audio_out_eq_tab);
    for (int i = 0; i < nsection; i++) {
        eq_seg_design((struct eq_seg_info *)&your_audio_out_eq_tab[i], sr, your_eq_coeff_tab[i]);//根据采样率对eq系数表，重计算，得出适用的系数表
    }
    local_irq_enable();
    info->L_coeff = info->R_coeff = (void *)your_eq_coeff_tab;//系数指针赋值
    info->L_gain = info->R_gain = 0;//总增益填写，用户可修改（-20~20db）,注意大于0db存在失真风险
    info->nsection = nsection;//eq段数，根据提供给的系数表来填写，例子是2
#endif//TCFG_EQ_ENABLE

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    更新自定义eq系数后，需要使用本函数将系数更新到eq模块
   @param    *_drc: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void eq_filter_info_update_demo(void *_eq)
{
#if TCFG_EQ_ENABLE
    struct audio_eq *eq = (struct audio_eq *)_eq;
    local_irq_disable();
    if (eq) {
        eq->updata = 1;
    }
    local_irq_enable();
#endif//TCFG_EQ_ENABLE

}

/*----------------------------------------------------------------------------*/
/**@brief    更新自定义限幅器系数后，需要使用本函数将系数更新到drc模块
   @param    *_drc: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void drc_filter_info_update_demo(void *_drc)
{
#if TCFG_DRC_ENABLE
    struct audio_drc *drc = (struct audio_drc *)_drc;
    local_irq_disable();
    if (drc) {
        drc->updata = 1;
    }
    local_irq_enable();
#endif//TCFG_DRC_ENABLE

}

struct audio_eq *music_eq_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
#ifdef CONFIG_ADAPTER_ENABLE
    parm.no_wait = 0;//同步eq，无线麦需要使用同步eq,否则影响延时统计
#else
    parm.no_wait = 1;//异步eq
#endif
    parm.cb = eq_get_filter_info;
    parm.sr = sample_rate;
    parm.eq_name = AEID_MUSIC_EQ;
    u8 tar = 0;
    parm.max_nsection = music_mode[tar].eq_parm.seg_num;
    parm.nsection = music_mode[tar].eq_parm.seg_num;
    parm.seg = music_mode[tar].eq_parm.seg;
    parm.global_gain = music_mode[tar].eq_parm.global_gain;
    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif //TCFG_EQ_ENABLE
    return NULL;
}

void music_eq_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}

struct audio_eq *music_eq2_open(u32 sample_rate, u8 ch_num)
{
#if TCFG_DYNAMIC_EQ_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
    parm.no_wait = 1;
    parm.cb = eq_get_filter_info;
    parm.sr = sample_rate;
    parm.eq_name = AEID_MUSIC_EQ2;
    u8 tar = 0;
    parm.max_nsection = music_eq2_parm[tar].seg_num;
    parm.nsection = music_eq2_parm[tar].seg_num;
    parm.seg = music_eq2_parm[tar].seg;
    parm.global_gain = music_eq2_parm[tar].global_gain;
    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif //TCFG_EQ_ENABLE
#endif
    return NULL;
}

void music_eq2_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}

struct audio_eq *linein_eq2_open(u32 sample_rate, u8 ch_num)
{
#if TCFG_DYNAMIC_EQ_ENABLE
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
    parm.no_wait = 1;
    parm.cb = eq_get_filter_info;
    parm.sr = sample_rate;
    parm.eq_name = AEID_AUX_EQ2;
    u8 tar = 1;
    parm.max_nsection = music_eq2_parm[tar].seg_num;
    parm.nsection = music_eq2_parm[tar].seg_num;
    parm.seg = music_eq2_parm[tar].seg;
    parm.global_gain = music_eq2_parm[tar].global_gain;
    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif //TCFG_EQ_ENABLE
#endif
    return NULL;
}

void linein_eq2_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}


struct audio_drc *music_drc_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    struct audio_drc_param parm = {0};
    parm.channels = ch_num;
    parm.sr = sample_rate;
    parm.out_32bit = 1;
    parm.cb = drc_get_filter_info;
    parm.drc_name = AEID_MUSIC_DRC;
#if defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 1)
    parm.nband = CROSSOVER_EN;//
#elif defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 2)
    parm.nband = CROSSOVER_EN | MORE_BAND_EN;//
#endif/*TCFG_AUDIO_MDRC_ENABLE*/
    u8 tar = 0;
    parm.crossover = &music_mode[tar].drc_parm.crossover;
    parm.wdrc = music_mode[tar].drc_parm.wdrc_parm;
    struct audio_drc *drc = audio_dec_drc_open(&parm);
    clock_add(EQ_DRC_CLK);
    return drc;
#endif/*TCFG_DRC_ENABLE*/
    return NULL;
}

void music_drc_close(struct audio_drc *drc)
{
    if (drc) {
        audio_dec_drc_close(drc);
        clock_remove(EQ_DRC_CLK);
    }
}



struct audio_eq *esco_eq_open(u32 sample_rate, u8 ch_num, u8 bit_wide)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    parm.out_32bit = bit_wide;//32bit位宽输出
#endif
    parm.no_wait = 1;
    parm.cb = eq_get_filter_info;
    parm.sr = sample_rate;
    parm.eq_name = AEID_ESCO_DL_EQ;
    u8 index = 0;
    if (sample_rate == 8000) { //窄频
        index = 1;
    }
    parm.max_nsection = phone_mode[index].eq_parm.seg_num;
    parm.nsection = phone_mode[index].eq_parm.seg_num;
    parm.seg = phone_mode[index].eq_parm.seg;
    parm.global_gain = phone_mode[index].eq_parm.global_gain;
    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif //TCFG_EQ_ENABLE
    return NULL;
}

void esco_eq_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}

struct audio_drc *esco_drc_open(u32 sample_rate, u8 ch_num, u8 bit_wide)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    struct audio_drc_param parm = {0};
    parm.channels = ch_num;
    parm.sr = sample_rate;
    parm.out_32bit = bit_wide;
    parm.cb = drc_get_filter_info;
    parm.drc_name = AEID_ESCO_DL_DRC;
    u8 index = 0;
    if (sample_rate == 8000) { //窄频
        index = 1;
    }
    parm.wdrc = &phone_mode[index].drc_parm;
    struct audio_drc *drc = audio_dec_drc_open(&parm);
    clock_add(EQ_DRC_CLK);
    return drc;
#endif/*TCFG_DRC_ENABLE*/
    return NULL;
}

void esco_drc_close(struct audio_drc *drc)
{
    if (drc) {
        audio_dec_drc_close(drc);
        clock_remove(EQ_DRC_CLK);
    }
}
//2.1/2.2声道，低音总增益调节
__attribute__((weak))
void low_bass_set_global_gain(float left_global_gain, float right_global_gain)
{
    cur_eq_set_global_gain(AEID_MUSIC_RL_EQ, left_global_gain);
    cur_eq_right_ch_set_global_gain(AEID_MUSIC_RL_EQ, right_global_gain);
}

struct audio_eq *music_eq_rl_rr_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
    parm.no_wait = 1;
#if defined(SOUND_TRACK_2_P_X_CH_CONFIG) &&SOUND_TRACK_2_P_X_CH_CONFIG && HIGH_GRADE_LOW_PASS_FILTER_EN
    parm.cb = low_bass_eq_get_filter_info_demo;
#else
    parm.cb = eq_get_filter_info;
#endif
    parm.sr = sample_rate;
    parm.eq_name = AEID_MUSIC_RL_EQ;
    u8 tar = 0;
    parm.max_nsection = rl_eq_parm[tar].seg_num;
    parm.nsection = rl_eq_parm[tar].seg_num;
    parm.seg = rl_eq_parm[tar].seg;
    parm.global_gain = rl_eq_parm[tar].global_gain;
    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif //TCFG_EQ_ENABLE
    return NULL;
}

void music_eq_rl_rr_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}

struct audio_eq *aux_eq_rl_rr_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
    parm.no_wait = 1;
#if defined(SOUND_TRACK_2_P_X_CH_CONFIG) &&SOUND_TRACK_2_P_X_CH_CONFIG && HIGH_GRADE_LOW_PASS_FILTER_EN
    parm.cb = low_bass_eq_get_filter_info_demo;
#else
    parm.cb = eq_get_filter_info;
#endif
    parm.sr = sample_rate;
    parm.eq_name = AEID_AUX_RL_EQ;
    u8 tar = 1;
    parm.max_nsection = rl_eq_parm[tar].seg_num;
    parm.nsection = rl_eq_parm[tar].seg_num;
    parm.seg = rl_eq_parm[tar].seg;
    parm.global_gain = rl_eq_parm[tar].global_gain;
    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif //TCFG_EQ_ENABLE
    return NULL;
}

void aux_eq_rl_rr_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}


struct audio_drc *music_drc_rl_rr_open(u32 sample_rate, u8 ch_num)
{
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_FRONT_LR_REAR_LR)
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    struct audio_drc_param parm = {0};
    parm.channels = ch_num;
    parm.sr = sample_rate;
    parm.out_32bit = 1;
    parm.cb = drc_get_filter_info;
    parm.drc_name = AEID_MUSIC_RL_DRC;
#if defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 1)
    parm.nband = CROSSOVER_EN;//
#elif defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 2)
    parm.nband = CROSSOVER_EN | MORE_BAND_EN;//
#endif/*TCFG_AUDIO_MDRC_ENABLE*/
    u8 tar = 0;
    parm.crossover = &rl_drc_parm[tar].crossover;//参数需要修改
    parm.wdrc = rl_drc_parm[tar].wdrc_parm;
    struct audio_drc *drc = audio_dec_drc_open(&parm);
    clock_add(EQ_DRC_CLK);
    return drc;
#endif/*TCFG_DRC_ENABLE*/
#endif
    return NULL;
}

void music_drc_rl_rr_close(struct audio_drc *drc)
{
    if (drc) {
        audio_dec_drc_close(drc);
        clock_remove(EQ_DRC_CLK);
    }
}

struct audio_drc *aux_drc_rl_rr_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    struct audio_drc_param parm = {0};
    parm.channels = ch_num;
    parm.sr = sample_rate;
    parm.out_32bit = 1;
    parm.cb = drc_get_filter_info;
    parm.drc_name = AEID_AUX_RL_DRC;
#if defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 1)
    parm.nband = CROSSOVER_EN;//
#elif defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 2)
    parm.nband = CROSSOVER_EN | MORE_BAND_EN;//
#endif/*TCFG_AUDIO_MDRC_ENABLE*/
    u8 tar = 1;
    parm.crossover = &rl_drc_parm[tar].crossover;//参数需要修改
    parm.wdrc = rl_drc_parm[tar].wdrc_parm;
    struct audio_drc *drc = audio_dec_drc_open(&parm);
    clock_add(EQ_DRC_CLK);
    return drc;
#endif/*TCFG_DRC_ENABLE*/
    return NULL;
}

void aux_drc_rl_rr_close(struct audio_drc *drc)
{
    if (drc) {
        audio_dec_drc_close(drc);
        clock_remove(EQ_DRC_CLK);
    }
}



#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
struct audio_eq *linein_eq_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
    parm.no_wait = 1;
    parm.cb = eq_get_filter_info;
    parm.sr = sample_rate;
    parm.eq_name = AEID_AUX_EQ;
    u8 tar = 1;
    parm.max_nsection = music_mode[tar].eq_parm.seg_num;
    parm.nsection = music_mode[tar].eq_parm.seg_num;
    parm.seg = music_mode[tar].eq_parm.seg;
    parm.global_gain = music_mode[tar].eq_parm.global_gain;
    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif //TCFG_EQ_ENABLE
    return NULL;
}

void linein_eq_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}

struct audio_drc *linein_drc_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE
    struct audio_drc_param parm = {0};
    parm.channels = ch_num;
    parm.sr = sample_rate;
    parm.out_32bit = 1;
    parm.cb = drc_get_filter_info;
    parm.drc_name = AEID_AUX_DRC;

#if defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 1)
    parm.nband = CROSSOVER_EN;//
#elif defined(TCFG_AUDIO_MDRC_ENABLE) && (TCFG_AUDIO_MDRC_ENABLE == 2)
    parm.nband = CROSSOVER_EN | MORE_BAND_EN;//
#endif/*TCFG_AUDIO_MDRC_ENABLE*/
    u8 tar = 1;
    parm.crossover = &music_mode[tar].drc_parm.crossover;
    parm.wdrc = music_mode[tar].drc_parm.wdrc_parm;
    struct audio_drc *drc = audio_dec_drc_open(&parm);
    clock_add(EQ_DRC_CLK);
    return drc;
#endif/*TCFG_DRC_ENABLE*/
    return NULL;
}

void linein_drc_close(struct audio_drc *drc)
{
    if (drc) {
        audio_dec_drc_close(drc);
        clock_remove(EQ_DRC_CLK);
    }
}
#endif

struct music_eq_tool high_bass_eq_parm[mode_add] = {0};

struct eq_seg_info high_bass_eq_seg[] = {//music模式的高低音,默认系数表
    {0, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 12000, 0, 0.3f},
};
struct eq_seg_info aux_high_bass_eq_seg[] = {//AUX模式的高低音,默认系数表
    {0, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 12000, 0, 0.3f},
};

struct audio_eq *high_bass_eq_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AUDIO_OUT_EQ_ENABLE
    u8 tar = 0;
    u16 seg_num = high_bass_eq_parm[tar].seg_num;
    for (int i = 0; i < ARRAY_SIZE(high_bass_eq_seg); i++) {
        high_bass_eq_seg[i].index = seg_num + i;
        memcpy(&high_bass_eq_parm[tar].seg[seg_num + i], &high_bass_eq_seg[i], sizeof(struct eq_seg_info));
    }

    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
    parm.no_wait = 1;
    parm.cb = eq_get_filter_info;
    parm.sr = sample_rate;
    parm.eq_name = AEID_HIGH_BASS_EQ;
    seg_num += ARRAY_SIZE(high_bass_eq_seg);
    parm.max_nsection = seg_num;
    parm.nsection = seg_num;
    parm.seg = high_bass_eq_parm[tar].seg;
    parm.global_gain = high_bass_eq_parm[tar].global_gain;

    parm.fade = 1;//高低音增益更新差异大，会引入哒哒音，此处使能系数淡入
    parm.fade_step = 0.4f;//淡入步进（0.1f~1.0f）

    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif /*TCFG_EQ_ENABLE*/
    return NULL;
}

void high_bass_eq_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AUDIO_OUT_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}
/*
 *index: 0, 更新低音中心频率freq,增益gain
 *index: 1, 更新高音中心频率freq,增益gain
 * */
void high_bass_eq_udpate(u8 index, int freq, float gain)
{
    if (freq) {
        high_bass_eq_seg[index].freq = freq;
    }
    high_bass_eq_seg[index].gain = gain;
    printf("index %d ,gain %d\n", index, (int)gain);

    struct audio_eq *hdl = get_cur_eq_hdl_by_name(AEID_HIGH_BASS_EQ);

    if (hdl) {
        if ((high_bass_eq_seg[index].index == 1) || (high_bass_eq_seg[index].index == 0)) {
            high_bass_eq_seg[index].index += high_bass_eq_parm[0].seg_num;
        }
        cur_eq_set_update(AEID_HIGH_BASS_EQ, &high_bass_eq_seg[index], high_bass_eq_parm[0].seg_num + ARRAY_SIZE(high_bass_eq_seg), 0);
    }


}



struct audio_eq *high_bass_aux_eq_open(u32 sample_rate, u8 ch_num)
{
    u8 tar = 1;
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AUDIO_OUT_EQ_ENABLE
    u16 seg_num = high_bass_eq_parm[tar].seg_num;
    for (int i = 0; i < ARRAY_SIZE(aux_high_bass_eq_seg); i++) {
        aux_high_bass_eq_seg[i].index = seg_num + i;
        memcpy(&high_bass_eq_parm[tar].seg[seg_num + i], &aux_high_bass_eq_seg[i], sizeof(struct eq_seg_info));
    }

    struct audio_eq_param parm = {0};
    parm.channels = ch_num;
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    parm.out_32bit = 1;//32bit位宽输出
#endif
    parm.no_wait = 1;
    parm.cb = eq_get_filter_info;
    parm.sr = sample_rate;
    parm.eq_name = AEID_AUX_HIGH_BASS_EQ;
    seg_num += ARRAY_SIZE(aux_high_bass_eq_seg);
    parm.max_nsection = seg_num;
    parm.nsection = seg_num;
    parm.seg = high_bass_eq_parm[tar].seg;
    parm.global_gain = high_bass_eq_parm[tar].global_gain;

    parm.fade = 1;//高低音增益更新差异大，会引入哒哒音，此处使能系数淡入
    parm.fade_step = 0.4f;//淡入步进（0.1f~1.0f）

    struct audio_eq *eq = audio_dec_eq_open(&parm);
    clock_add(EQ_CLK);
    return eq;
#endif /*TCFG_EQ_ENABLE*/
    return NULL;
}
void high_bass_aux_eq_close(struct audio_eq *eq)
{
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AUDIO_OUT_EQ_ENABLE
    if (eq) {
        audio_dec_eq_close(eq);
        clock_remove(EQ_CLK);
    }
#endif/*TCFG_EQ_ENABLE*/
}

void aux_high_bass_eq_udpate(u8 index, int freq, float gain)
{
    if (freq) {
        aux_high_bass_eq_seg[index].freq = freq;
    }
    aux_high_bass_eq_seg[index].gain = gain;
    printf("index %d ,gain %d\n", index, (int)gain);

    struct audio_eq *hdl = get_cur_eq_hdl_by_name(AEID_AUX_HIGH_BASS_EQ);

    if (hdl) {
        if ((aux_high_bass_eq_seg[index].index == 1) || (aux_high_bass_eq_seg[index].index == 0)) {
            aux_high_bass_eq_seg[index].index += high_bass_eq_parm[1].seg_num;
        }
        cur_eq_set_update(AEID_AUX_HIGH_BASS_EQ, &aux_high_bass_eq_seg[index], high_bass_eq_parm[1].seg_num + ARRAY_SIZE(aux_high_bass_eq_seg), 0);
    }
}


//兼容旧接口
void mix_out_high_bass(u32 cmd, struct high_bass *hb)
{
    if (cmd == AUDIO_EQ_HIGH) {
        high_bass_eq_udpate(1, hb->freq, hb->gain);
    } else if (cmd == AUDIO_EQ_BASS) {
        high_bass_eq_udpate(0, hb->freq, hb->gain);
    }
}

wdrc_struct_TOOL_SET high_bass_drc_parm[mode_add] = {0};
struct audio_drc *high_bass_drc_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    u8 tar = 0;
    struct audio_drc_param parm = {0};
    parm.channels = ch_num;
    parm.sr = sample_rate;
#if defined(TCFG_EQ_ENABLE) && TCFG_EQ_ENABLE && TCFG_AUDIO_OUT_EQ_ENABLE
    parm.out_32bit = 1;
#else
    parm.out_32bit = 0;
#endif
    parm.cb = drc_get_filter_info;
    parm.drc_name = AEID_HIGH_BASS_DRC;
    parm.wdrc = &high_bass_drc_parm[tar];
    struct audio_drc *drc = audio_dec_drc_open(&parm);
    clock_add(EQ_DRC_CLK);
    return drc;
#endif/*TCFG_DRC_ENABLE*/

    return NULL;
}

void high_bass_drc_close(struct audio_drc *drc)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    if (drc) {
        audio_dec_drc_close(drc);
        clock_remove(EQ_DRC_CLK);
    }
#endif
}

struct audio_drc *high_bass_aux_drc_open(u32 sample_rate, u8 ch_num)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    u8 tar = 1;
    struct audio_drc_param parm = {0};
    parm.channels = ch_num;
    parm.sr = sample_rate;
    parm.out_32bit = 1;
    parm.cb = drc_get_filter_info;
    parm.drc_name = AEID_AUX_HIGH_BASS_DRC;
    parm.wdrc = &high_bass_drc_parm[tar];
    struct audio_drc *drc = audio_dec_drc_open(&parm);
    clock_add(EQ_DRC_CLK);
    return drc;
#endif/*TCFG_DRC_ENABLE*/

    return NULL;
}

void high_bass_aux_drc_close(struct audio_drc *drc)
{
#if defined(TCFG_DRC_ENABLE) && TCFG_DRC_ENABLE && TCFG_AUDIO_OUT_DRC_ENABLE
    if (drc) {
        audio_dec_drc_close(drc);
        clock_remove(EQ_DRC_CLK);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    高低音限幅器系数回调
   @param    *drc: 句柄
   @param    *info: 系数结构地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int high_bass_drc_set_filter_info(int th)
{
    if (th < -60) {
        th = -60;
    }
    if (th > 0) {
        th = 0;
    }

    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    group[1].in_threshold = 90 + th;
    group[1].out_threshold = 90 + th;
    group[2].in_threshold = 90.3f;
    group[2].out_threshold = 90 + th;
    struct audio_drc *hdl = get_cur_drc_hdl_by_name(AEID_HIGH_BASS_DRC);
    if (hdl) {
        memcpy(high_bass_drc_parm[0].parm.threshold, group, sizeof(group));
        cur_drc_set_update(AEID_HIGH_BASS_DRC, 0, &high_bass_drc_parm[0]);
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    高低音限幅器系数回调
   @param    *drc: 句柄
   @param    *info: 系数结构地址
   @return
   @note     AUX的高低音drc
*/
/*----------------------------------------------------------------------------*/
int aux_high_bass_drc_set_filter_info(int th)
{
#if defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
    if (th < -60) {
        th = -60;
    }
    if (th > 0) {
        th = 0;
    }

    struct threshold_group group[] = {{0, 0}, {50, 50}, {90, 90}};
    group[1].in_threshold = 90 + th;
    group[1].out_threshold = 90 + th;
    group[2].in_threshold = 90.3f;
    group[2].out_threshold = 90 + th;
    struct audio_drc *hdl = get_cur_drc_hdl_by_name(AEID_AUX_HIGH_BASS_DRC);
    if (hdl) {
        memcpy(high_bass_drc_parm[1].parm.threshold, group, sizeof(group));
        cur_drc_set_update(AEID_AUX_HIGH_BASS_DRC, 0, &high_bass_drc_parm[1]);
    }
#endif
    return 0;
}

