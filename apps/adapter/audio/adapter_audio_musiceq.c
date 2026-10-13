#include "adapter_audio_miceq.h"
#include "asm/includes.h"
#include "media/includes.h"
#include "app_config.h"
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)
#include "media/effects_adj.h"
#include "audio_effect/audio_eff_default_parm.h"
#endif
#if TCFG_EQ_ENABLE&&TCFG_MUSIC_EQ_ENABLE
#include "clock_cfg.h"

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 eq drc 打开
  @param    sample_rate:采样率
  @param    ch_num:通道个数
  @return   句柄
  @note
 */
/*----------------------------------------------------------------------------*/
void *adapter_audio_music_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE && TCFG_MUSIC_EQ_ENABLE
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)
    struct audio_eq *music_eq;
    music_eq = music_eq_open(sample_rate, ch_num);
    return music_eq;
#else
    struct audio_eq_drc *eq_drc = NULL;
    u8 drc_en = 0;
#if TCFG_DRC_ENABLE && TCFG_BT_MUSIC_DRC_ENABLE
    drc_en = 1;
#endif/*TCFG_DRC_ENABLE && TCFG_BT_MUSIC_DRC_ENABLE*/
    eq_drc = stream_eq_drc_open_demo(sample_rate, ch_num, drc_en, song_eq_mode);

    return eq_drc;
#endif
#endif/*TCFG_EQ_ENABLE && TCFG_BT_MUSIC_EQ_ENABLE*/
    return NULL;

}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式 eq drc 关闭
  @param    句柄
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
void adapter_audio_music_eq_drc_close(void *eq)
{
#if TCFG_EQ_ENABLE && TCFG_MUSIC_EQ_ENABLE
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)
    struct audio_eq *music_eq = (struct audio_eq *)eq;
    if (music_eq) {
        music_eq_close(music_eq);
    }
#else

    struct audio_eq_drc *eq_drc = (struct audio_eq_drc *)eq;
    if (eq_drc) {
        stream_eq_drc_close_demo(eq_drc);
    }
#endif
#endif/*TCFG_EQ_ENABLE && TCFG_BT_MUSIC_EQ_ENABLE*/

}

#endif
