
#include "adapter_audio_miceq.h"
#include "asm/includes.h"
#include "media/includes.h"
#include "app_config.h"
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)
#include "media/effects_adj.h"
#include "audio_effect/audio_eff_default_parm.h"
#endif


#if TCFG_EQ_ENABLE&&TCFG_PHONE_EQ_ENABLE
#include "clock_cfg.h"
/*----------------------------------------------------------------------------*/
/**@brief    通话模式 eq drc 打开
     @param    sample_rate:采样率
	    @param    ch_num:通道个数
		   @return   句柄
		      @note
			  */
/*----------------------------------------------------------------------------*/
void *adapter_audio_miceq_open(u16 sample_rate, u8 ch_num)
{
    //mix_out_high_bass_dis(AUDIO_EQ_HIGH_BASS_DIS, 1);
#if TCFG_EQ_ENABLE
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)
    u8 mode = 2;
    if (sample_rate == 8000) {
        mode = 3;
    }
    struct audio_eq_param ul_eq_param = {0};
    struct audio_eq *ul_eq = NULL;
    ul_eq_param.sr = sample_rate;
    ul_eq_param.channels = ch_num;
    ul_eq_param.max_nsection = phone_mode[mode].eq_parm.seg_num;
    ul_eq_param.nsection = phone_mode[mode].eq_parm.seg_num;
    ul_eq_param.seg = phone_mode[mode].eq_parm.seg;
    ul_eq_param.global_gain = phone_mode[mode].eq_parm.global_gain;
    ul_eq_param.cb = eq_get_filter_info;
    ul_eq_param.eq_name = AEID_ESCO_UL_EQ;
    ul_eq = audio_dec_eq_open(&ul_eq_param);

    return ul_eq;
#else
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_PHONE_EQ_ENABLE
    effect_parm.eq_en = 1;

    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;
        effect_parm.online_en = 1;
        effect_parm.mode_en = 0;
    }

    if (sample_rate > 8000) {
        effect_parm.eq_name = aec_eq_mode;
    } else {
        effect_parm.eq_name = aec_narrow_eq_mode;
    }

    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = aec_ul_eq_filter;

    eq_drc = audio_eq_drc_open(&effect_parm);

#endif
    return eq_drc;
#endif//TCFG_EQ_ENABLE
#endif
    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    通话模式 eq drc 关闭
     @param    句柄
	    @return
		   @note
		   */
/*----------------------------------------------------------------------------*/
void adapter_audio_miceq_close(void *eq)
{
#if TCFG_PHONE_EQ_ENABLE
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)
    struct audio_eq *ul_eq = (struct audio_eq_drc *)eq;
    if (ul_eq) {
        audio_dec_eq_close(ul_eq);
        ul_eq = NULL;
    }
#else

    struct audio_eq_drc *eq_drc = (struct audio_eq_drc *)eq;
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
    }
#endif
#endif
    //mix_out_high_bass_dis(AUDIO_EQ_HIGH_BASS_DIS, 0);
    return;
}

#endif
