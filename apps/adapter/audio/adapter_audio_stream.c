#include "adapter_audio_stream.h"
#include "adapter_encoder.h"
#include "adapter_media.h"
#include "media/audio_stream.h"
#include "stream_sync.h"
#include "adapter_audio_miceq.h"
#include "adapter_audio_musiceq.h"
#include "channel_switch.h"
#include "audio_config.h"
void test_st(void *p)
{
    mem_stats();
}
#define ADA_AUDIO_DEBUG_ENABLE
#ifdef ADA_AUDIO_DEBUG_ENABLE
#define log_info(x, ...)  printf("[ADA_AUDIO_STREAM]" x " ", ## __VA_ARGS__)
#define log_e(x, ...)     printf("[ADA_AUDIO_STREAM ERROR]" x " ", ## __VA_ARGS__)
#else
#define log_info(x, ...)
#define log_e(x, ...)
#endif//ADA_AUDIO_DEBUG_ENABLE

extern struct audio_dac_hdl dac_hdl;
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
#define DAC_OUTPUT_CHANNELS     2
#elif (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_LR_DIFF)
#define DAC_OUTPUT_CHANNELS     1
#endif

extern struct audio_adc_hdl adc_hdl;
extern struct adc_platform_data adc_data;

//int entry_debug_handle(void *priv, struct audio_data_frame *in)
//{
//    putchar('D');
//    return in->data_len;
//}

void *stream_drc_open(u32 sample_rate, u8 ch_num, u8 mode);
void stream_drc_close(struct audio_eq_drc *eq_drc);
//noisegate howling默认值设置，bin文件内有效果时会重新覆盖
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
void eff_cfg_default_init(EQ_CFG *eq_cfg)
{
    for (int i = 0; i < eq_cfg->mode_num; i++) {
        if (wirless_mic_mode == i) {
            //noisegate
            eq_cfg->cfg_parm[i].ns_parm.is_bypass = 0;
            eq_cfg->cfg_parm[i].ns_parm.parm.attackTime = 300;
            eq_cfg->cfg_parm[i].ns_parm.parm.releaseTime = 5;
            eq_cfg->cfg_parm[i].ns_parm.parm.threshold = -90300;//mdb -90.3dB
            eq_cfg->cfg_parm[i].ns_parm.parm.low_th_gain = 0;

            //freq_shift howling
            eq_cfg->cfg_parm[i].ps_howling_parm.is_bypass = 0;
            eq_cfg->cfg_parm[i].ps_howling_parm.parm.effect_v = EFFECT_HOWLING_FS;
            eq_cfg->cfg_parm[i].ps_howling_parm.parm.ps_parm = -50;
            eq_cfg->cfg_parm[i].ps_howling_parm.parm.fe_parm = 4;

            //notch howling
            eq_cfg->cfg_parm[i].notch_howling_parm.is_bypass = 0;
            eq_cfg->cfg_parm[i].notch_howling_parm.parm.Q = 2.0f;
            eq_cfg->cfg_parm[i].notch_howling_parm.parm.gain = -20;
            eq_cfg->cfg_parm[i].notch_howling_parm.parm.fade_n = 10;
            eq_cfg->cfg_parm[i].notch_howling_parm.parm.threshold = 25;
            for (int j = 0; j < 10 ; j++) {
                //voice changer
                eq_cfg->cfg_parm[i].voice_changer_parm.is_bypass = 1;
                eq_cfg->cfg_parm[i].voice_changer_parm.parm[j].effect_v = 0;
                eq_cfg->cfg_parm[i].voice_changer_parm.parm[j].shiftv = 56;
                eq_cfg->cfg_parm[i].voice_changer_parm.parm[j].formant_shift = 90;
            }

            //drc
            int th = 0;//db -60db~0db
            int threshold = roundf(powf(10.0f, th / 20.0f) * 32768); // 0db:32768, -60db:33
            eq_cfg->cfg_parm[i].drc_parm.parm.drc.nband = 1;
            eq_cfg->cfg_parm[i].drc_parm.parm.drc.type = 1;
            eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].attacktime = 5;
            eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].releasetime = 500;
            eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].threshold[0] = threshold;
            eq_cfg->cfg_parm[i].drc_parm.parm.drc._p.limiter[0].threshold[1] = 32768;

        }
    }
}
char eff_cfg_get_voice_changer_max_num(void)
{
    return (char)(WIRELESS_VOICE_CHANGER_MAX_NUM - 1);
}

#endif
struct __stream_sync *adapter_audio_stream_sync_open(struct adapter_sync_parm *parm)
{
    struct __stream_sync *sync = NULL;
    struct stream_sync_info info = {0};
    info.ch_num = parm->ch_num;
    info.begin_per = parm->begin_per;
    info.top_per = parm->top_per;
    info.bottom_per = parm->bottom_per;
    info.inc_step = parm->dec_step;
    info.max_step = parm->max_step;
    info.priv = parm->priv;
    ASSERT(parm->get_in_sr, "%s parm err!\n", __FUNCTION__);
    ASSERT(parm->get_out_sr, "%s parm err!\n", __FUNCTION__);
    info.i_sr = parm->get_in_sr(parm->priv);
    info.o_sr = parm->get_out_sr(parm->priv);
    info.get_total = parm->get_total;
    info.get_size = parm->get_size;
    log_info("info.i_sr = %d, info.o_sr = %d, parm->ch_num = %d, parm->always = %d\n", info.i_sr, info.o_sr, parm->ch_num, parm->always);
    sync = stream_sync_open(&info, parm->always);
    stream_sync_buf_alloc(sync, parm->ibuf_len, parm->obuf_len);
    return sync;
}


void adapter_audio_stream_echo_run(struct adapter_audio_stream *audio, u8 echo_en);
static struct audio_stream_entry *adapter_audio_stream_attr_open(struct adapter_audio_stream *audio, struct adapter_stream_fmt *fmt, struct adapter_media_parm *media_parm)
{
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
    EQ_CFG *eq_cfg = get_eq_cfg_hdl();
#endif
    struct audio_stream_entry *entry = NULL;
    switch (fmt->attr) {
    case ADAPTER_STREAM_ATTR_SYNC:
        audio->sync = adapter_audio_stream_sync_open((struct adapter_sync_parm *)&fmt->value.sync);
        if (audio->sync) {
            entry = &audio->sync->entry;
            entry->prob_handler = fmt->value.sync.data_handle;
        } else {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
        }
        break;
    case ADAPTER_STREAM_ATTR_SRC:
        log_info("src.target_sr = %d, src.always = %d\n", fmt->value.src.target_sr, fmt->value.src.always);
        audio->src = stream_src_open(fmt->value.src.target_sr, fmt->value.src.always);
        if (audio->src) {
            entry = &audio->src->entry;
            entry->prob_handler = fmt->value.src.data_handle;
        } else {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
        }
        break;
    case ADAPTER_STREAM_ATTR_DVOL:
        log_info("ADAPTER_STREAM_ATTR_DVOL\n");
        if (fmt->value.digital_vol.volume == NULL) {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
            break;
        }
        audio_dig_vol_param vol_parm = {0};
        ASSERT(media_parm, "ADAPTER_STREAM_ATTR_DVOL parm err!!\n");
        memcpy(&vol_parm, fmt->value.digital_vol.volume, sizeof(audio_dig_vol_param));
        audio->dvol = audio_dig_vol_open((audio_dig_vol_param *)&vol_parm);
#if !WIRELESS_DOWNSTREAM_ENABLE
        media_parm->vol_limit = 100;
        media_parm->start_vol_l = 100;
        media_parm->start_vol_r = 100;
#endif
        if (audio->dvol) {
            audio->vol_limit = media_parm->vol_limit;
            //设置初始音量
            adapter_audio_stream_set_vol(audio, BIT(0), media_parm->start_vol_l);
            adapter_audio_stream_set_vol(audio, BIT(1), media_parm->start_vol_r);
            entry = audio_dig_vol_entry_get(audio->dvol);
            entry->prob_handler = fmt->value.digital_vol.data_handle;
        } else {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
        }
        break;
    case ADAPTER_STREAM_ATTR_ENCODE:
        log_info("ADAPTER_STREAM_ATTR_ENCODE\n");
        audio->encoder = adapter_encoder_open((struct adapter_encoder_fmt *)fmt->value.encoder.parm, media_parm);
        if (audio->encoder) {
            entry = adapter_encoder_get_stream_entry(audio->encoder);
            entry->prob_handler = fmt->value.encoder.data_handle;
        } else {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
        }
        break;
    case ADAPTER_STREAM_ATTR_ENTRY_DEBUG:
        log_info("ADAPTER_STREAM_ATTR_ENTRY_DEBUG\n");
        audio->debug_entry = stream_entry_open(NULL, NULL, fmt->value.debug_entry.is_end);
        if (audio->debug_entry) {
            entry = &audio->debug_entry->entry;
            entry->prob_handler = fmt->value.debug_entry.data_handle;
        }
        break;
    case ADAPTER_STREAM_ATTR_STREAM_DEMO:
        log_info("ADAPTER_STREAM_ATTR_STREAM_DEMO\n");
        audio->demo = adapter_stream_demo_open();
        if (audio->demo) {
            entry = &audio->demo->stream->entry;
            entry->prob_handler = NULL;
        }
        break;
    case ADAPTER_STREAM_ATTR_AUTO_MUTE:
        log_info("ADAPTER_STREAM_ATTR_AUTO_MUTE");
        audio->auto_mute = audio_energy_detect_open(fmt->value.auto_mute.parm);
        if (audio->auto_mute) {
            entry = audio_energy_detect_entry_get(audio->auto_mute);
            entry->prob_handler = fmt->value.auto_mute.data_handle;
        } else {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
        }
        break;
    case ADAPTER_STREAM_ATTR_DAC:
        log_info("ADAPTER_STREAM_ATTR_DAC\n");
        audio_adc_init(&adc_hdl, &adc_data);//开dac之前需要初始化adc
#if NEW_AUDIO_W_MIC
        struct dac_param dac_param_t = {0};
        memcpy(&dac_param_t, &fmt->value.dac1.attr, sizeof(struct dac_param));
        audio->dac1 = audio_sound_dac_open(&dac_param_t);
        if (audio->dac1) {
            audio_sound_dac_set_analog_gain(audio->dac1, 0xF, MAX_ANA_VOL);//设置音量
            audio_sound_dac_set_digital_gain(audio->dac1, 0xF, 16384);
            entry = &(audio->dac1->entry);
            entry->prob_handler = fmt->value.dac1.data_handle;
        } else {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
        }


#else
        if (audio_dac_init_status() == 0) {
            app_audio_output_init();//如果DAC没有开， 这里打开一下
        }
        audio->dac = (struct audio_dac_channel *)zalloc(sizeof(struct audio_dac_channel));
        if (audio->dac) {
            extern struct audio_dac_hdl dac_hdl;
            struct audio_dac_channel_attr attr = {0};
            memcpy(&attr, &fmt->value.dac.attr, sizeof(struct audio_dac_channel_attr));
            if (fmt->value.dac.get_delay_time) {
                attr.delay_time = fmt->value.dac.get_delay_time();
            }
            log_info("dac delay_time = %d\n", attr.delay_time);
            audio_dac_new_channel(&dac_hdl, audio->dac);
            audio_dac_channel_set_attr(audio->dac, &attr);
            entry = &audio->dac->entry;
            entry->prob_handler = fmt->value.dac.data_handle;
            audio_sound_dac_set_analog_gain(audio->dac, 0xF, MAX_ANA_VOL);//设置音量
            audio_sound_dac_set_digital_gain(audio->dac, 0xF, 16384);
        } else {
            log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
        }

#endif
        break;
    case ADAPTER_STREAM_ATTR_NOISEGATE:
        log_info("ADAPTER_STREAM_ATTR_NOISEGATE\n");
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
        if (!eq_cfg) {
            break;
        }
        noisegate_update_param *parm = &eq_cfg->cfg_parm[wirless_mic_mode].ns_parm.parm;
        int bypass = eq_cfg->cfg_parm[wirless_mic_mode].ns_parm.is_bypass;

        NoiseGateParam noisegate_parm = {0};
        noisegate_parm.attackTime  = parm->attackTime;
        noisegate_parm.releaseTime = parm->releaseTime;
        noisegate_parm.threshold   = parm->threshold;
        noisegate_parm.low_th_gain = parm->low_th_gain;
        noisegate_parm.sampleRate  = WIRELESS_CODING_SAMPLERATE;
        noisegate_parm.channel = 1;
        audio->noisegate = audio_noisegate_open(AEID_MIC_NS_GATE,  &noisegate_parm);
        audio_noisegate_update(AEID_MIC_NS_GATE, parm);
        audio_noisegate_bypass(AEID_MIC_NS_GATE, bypass);
#else
        audio->noisegate = open_noisegate(fmt->value.noisegate.parm);
#endif
        if (audio->noisegate) {
            puts("-------------------noise gate----------------\n");
            entry = &audio->noisegate->entry;
            entry->prob_handler = fmt->value.noisegate.data_handle;
        }
        break;
#if (WIRELESS_DENOISE_ENABLE)
    case ADAPTER_STREAM_ATTR_DENOISE:
        log_info("ADAPTER_STREAM_ATTR_DONOISE\n");
        audio->denoise = stream_HighSampleRateSingleMicSystem_open(&fmt->value.denoise.parm, fmt->value.denoise.sample_rate, fmt->value.denoise.onoff);
        if (audio->denoise) {
            entry = &audio->denoise->entry;
            entry->prob_handler = fmt->value.denoise.data_handle;
        }
        break;
#endif//WIRELESS_DENOISE_ENABLE
#if TCFG_EQ_ENABLE
#if TCFG_PHONE_EQ_ENABLE
    case ADAPTER_STREAM_ATTR_MICEQ:
        log_info("ADAPTER_STREAM_ATTR_MICEQ");
        audio->miceq = adapter_audio_miceq_open(fmt->value.miceq.samplerate, fmt->value.miceq.ch_num);
        if (audio->miceq) {
            entry = &audio->miceq->entry;
            entry->prob_handler = fmt->value.miceq.data_handle;
        } else {
            log_e("adapter_audio_miceq_open err, line = %d\n", __LINE__);
        }
        break;
#endif
#if TCFG_MUSIC_EQ_ENABLE
    case ADAPTER_STREAM_ATTR_MUSICEQ:
        log_info("ADAPTER_STREAM_ATTR_MUSICEQ");
        printf("fmt->value.musiceq.samplerate=%d,fmt->value.musiceq.ch_num=%d", fmt->value.musiceq.samplerate, fmt->value.musiceq.ch_num);
        audio->musiceq = adapter_audio_music_eq_drc_open(fmt->value.musiceq.samplerate, fmt->value.musiceq.ch_num);
        if (audio->musiceq) {
            entry = &audio->musiceq->entry;
            entry->prob_handler = fmt->value.musiceq.data_handle;
        } else {
            log_e("adapter_audio_musiceq_open err, line = %d\n", __LINE__);
        }
        break;
#endif
#if TCFG_AEC_DCCS_EQ_ENABLE
    case ADAPTER_STREAM_ATTR_DCCS:
        log_info("ADAPTER_STREAM_ATTR_DCCS");
        audio->dccs = stream_eq_dccs_open(fmt->value.dccs.parm->sr);
        if (audio->dccs) {
            entry = &audio->dccs->entry;
            entry->prob_handler = fmt->value.dccs.data_handle;
        } else {
            log_e("adapter_audio_dccs_open err, line = %d\n", __LINE__);
        }
        break;
#endif
#endif
#if WIRELESS_PLATE_REVERB_ENABLE
    case ADAPTER_STREAM_ATTR_PLATE_REVERB:
        log_info("ADAPTER_STREAM_ATTR_PLATE_REVERB_ENABLE");
        audio->plate_reverb = open_plate_reverb(fmt->value.plate_reverb.parm, fmt->value.plate_reverb.samplerate);
        if (audio->plate_reverb) {
            entry = &audio->plate_reverb->entry;
            entry->prob_handler = fmt->value.plate_reverb.data_handle;
        } else {
            log_e("adapter_audio_plate_reverb err, line = %d\n", __LINE__);
        }
        adapter_audio_stream_echo_run(audio, 1);
        break;
#endif
#if WIRELESS_ECHO_ENABLE
    case ADAPTER_STREAM_ATTR_ECHO:
        log_info("ADAPTER_STREAM_ATTR_ECHO");
        audio->echo = open_echo(fmt->value.echo.parm, fmt->value.echo.fix_parm);
        if (audio->echo) {
            entry = &audio->echo->entry;
            entry->prob_handler = fmt->value.echo.data_handle;
        } else {
            log_e("adapter_audio_echo_open err, line = %d\n", __LINE__);
        }
        adapter_audio_stream_echo_run(audio, 1);
        break;
#endif
#if WIRELESS_LLNS_ENABLE
    case ADAPTER_STREAM_ATTR_LLNS:
        log_info("ADAPTER_STREAM_ATTR_LLNS\n");
        audio->llns = stream_llns_open(&fmt->value.llns.llns_parm, fmt->value.llns.samplerate, fmt->value.llns.onoff);
        if (audio->llns) {
            /* stream_llns_set_winsize(audio->llns, 300);//降噪会将连续的声音去掉，打开这里可以延迟处理连续声音的时间，不设置默认是50 */
            entry = &audio->llns->entry;
            entry->prob_handler = fmt->value.llns.data_handle;
        }
        break;
#endif
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
#if WIRELESS_HOWLING_ENABLE
    case ADAPTER_STREAM_ATTR_HOWLING:
        /* log_info("ADAPTER_STREAM_ATTR_HOWLING"); */
        if (!eq_cfg) {
            break;
        }

        audio->howling_ps = audio_open_howling(AEID_MIC_HOWLING_PS, NULL, WIRELESS_CODING_SAMPLERATE, 0, 1);//以频
        if (audio->howling_ps) {
            puts("-------------------howling_ps----------------\n");
            entry = &audio->howling_ps->entry;
            entry->prob_handler = NULL;

            int bypass = eq_cfg->cfg_parm[wirless_mic_mode].ps_howling_parm.is_bypass;
            audio_update_howling_parm(AEID_MIC_HOWLING_PS, &eq_cfg->cfg_parm[wirless_mic_mode].ps_howling_parm.parm);
            audio_howling_update_bypass(AEID_MIC_HOWLING_PS, bypass);
        }
        break;

    case ADAPTER_STREAM_ATTR_HOWLINGSUPPRESS:
        if (!eq_cfg) {
            break;
        }

        HOWLING_PARM_SET howling_param = {
            .threshold   = 25,
            .fade_time   = 10,
            .notch_Q     = 2.0f,
            .notch_gain  = -20,
            .sample_rate = WIRELESS_CODING_SAMPLERATE,
            .channel = 1,
        };
        audio->notch_howling = audio_open_howling(AEID_MIC_HOWLING_SUPRESS, &howling_param, WIRELESS_CODING_SAMPLERATE, 0, 0);//陷波
        if (audio->notch_howling) {
            puts("-------------------notch howling----------------\n");
            entry = &audio->notch_howling->entry;
            entry->prob_handler = NULL;

            int bypass = eq_cfg->cfg_parm[wirless_mic_mode].notch_howling_parm.is_bypass;
            audio_update_howling_parm(AEID_MIC_HOWLING_SUPRESS, &eq_cfg->cfg_parm[wirless_mic_mode].notch_howling_parm.parm);
            audio_howling_update_bypass(AEID_MIC_HOWLING_SUPRESS, bypass);
            static u16 test_tt = 0 ;
            if (!test_tt) {
                /* test_tt = sys_timer_add(NULL, test_st, 2000); */
            }
        }
        break;
#endif
#endif

#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
#if WIRELESS_VOICE_CHANGER_EN
    case ADAPTER_STREAM_ATTR_VOICE_CHANGER:
        if (!eq_cfg) {
            break;
        }
        VOICECHANGER_PARM vparm = {0};
        int user_mode = 0;
        memcpy(&vparm, &eq_cfg->cfg_parm[wirless_mic_mode].voice_changer_parm.parm[user_mode], sizeof(VOICECHANGER_PARM));
        audio->voice_changer = audio_voice_changer_open(&vparm, WIRELESS_CODING_SAMPLERATE, AEID_MIC_VOICE_CHANGER);
        if (audio->voice_changer) {
            puts("-------------------voice changer----------------\n");
            entry = &audio->voice_changer->entry;
            entry->prob_handler = NULL;
            int bypass = eq_cfg->cfg_parm[wirless_mic_mode].voice_changer_parm.is_bypass;
            audio_voice_changer_bypass(AEID_MIC_VOICE_CHANGER, bypass);
        }
        break;
#endif
#endif
#if WIRELESS_DRC_EN
    case ADAPTER_STREAM_ATTR_DRC:
        audio->mic_drc = stream_drc_open(WIRELESS_CODING_SAMPLERATE, 1, wirless_mic_mode);
        if (audio->mic_drc) {
            entry = &audio->mic_drc->entry;
            entry->prob_handler = NULL;
        }
        break;
#endif

    case ADAPTER_STREAM_ATTR_CH_SW:
        log_info("ADAPTER_STREAM_ATTR_CH_SW");
        audio->ch_sw = channel_switch_open(fmt->value.ch_sw.out_ch_type, fmt->value.ch_sw.buf_len);
        if (audio->ch_sw) {
            entry = &audio->ch_sw->entry;
            entry->prob_handler = fmt->value.ch_sw.data_handle;
        }
        break;
    default:
        log_info("%s attr err!!\n", __FUNCTION__);
        break;
    }

    return entry;
}

static void adapter_audio_stream_resume(void *p)
{
    struct audio_decoder *decoder = (struct audio_decoder *)p;

    //adapter_audio_try_resume(decoder);

    audio_decoder_resume(decoder);
}


void adapter_audio_stream_close(struct adapter_audio_stream **hdl)
{
    if (hdl == NULL || *hdl == NULL) {
        return ;
    }
    struct adapter_audio_stream *audio = *hdl;
    //关闭其他的数据流节点
    if (audio->sync) {
        log_info("close sync");
        stream_sync_close(&audio->sync);
    }
    if (audio->src) {
        log_info("close src");
        stream_src_close(&audio->src);
    }
    if (audio->dvol) {
        log_info("close dvol");
        audio_stream_del_entry(audio_dig_vol_entry_get(audio->dvol));
        audio_dig_vol_close(audio->dvol);
    }
#if WIRELESS_AUTO_MUTE
    if (audio->auto_mute) {
        log_info("close auto_mute");
        audio_stream_del_entry(audio_energy_detect_entry_get(audio->auto_mute));
        audio_energy_detect_close(audio->auto_mute);
    }
#endif
#if TCFG_EQ_ENABLE
#if TCFG_AEC_DCCS_EQ_ENABLE
    if (audio->dccs) {
        log_info("close dccs");
        stream_eq_dccs_close(&audio->dccs);
    }
#endif
#if TCFG_PHONE_EQ_ENABLE
    if (audio->miceq) {
        log_info("close miceq");
        adapter_audio_miceq_close((void *)audio->miceq);
    }
#endif
#if TCFG_MUSIC_EQ_ENABLE
    if (audio->musiceq) {
        log_info("close musiceq");
        adapter_audio_music_eq_drc_close(audio->musiceq);
    }
#endif
#endif
#if WIRELESS_DRC_EN
    if (audio->mic_drc) {
        log_info("close mic_drc");
        stream_drc_close(audio->mic_drc);
        audio->mic_drc = NULL;
    }
#endif

    if (audio->encoder) {
        log_info("close encoder");
        audio_stream_del_entry(adapter_encoder_get_stream_entry(audio->encoder));
        adapter_encoder_close(audio->encoder);
    }
    if (audio->debug_entry) {
        log_info("close debug_entry");
        stream_entry_close(&audio->debug_entry);
    }
#if WIRELESS_LLNS_ENABLE
    if (audio->llns) {
        log_info("close llns");
        stream_llns_close(&audio->llns);
    }
#endif
    if (audio->demo) {
        log_info("close demo");
        adapter_stream_demo_close(&audio->demo);
    }

    if (audio->dac) {
        log_info("close dac");
        audio_stream_del_entry(&audio->dac->entry);
        audio_dac_free_channel(audio->dac);
        free(audio->dac);
        audio->dac = NULL;
    }
    if (audio->dac1) {
        log_info("close dac1");
        audio_stream_del_entry(&audio->dac1->entry);
        audio_sound_dac_close(audio->dac1);
        audio->dac1 = NULL;
    }
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
    if (audio->noisegate) {
        audio_noisegate_close(audio->noisegate);
        audio->noisegate = NULL;
    }
#else
    if (audio->noisegate) {
        log_info("close noisegate");
        close_noisegate(audio->noisegate);
        audio->noisegate = NULL;
    }
#endif
#if (WIRELESS_DENOISE_ENABLE)
    if (audio->denoise) {
        log_info("close denoise");
        stream_HighSampleRateSingleMicSystem_close(&audio->denoise);
    }
#endif//WIRELESS_DENOISE_ENABLE
#if (WIRELESS_ECHO_ENABLE)
    if (audio->echo) {
        log_info("close echo");
        close_echo(audio->echo);
    }
#endif//WIRELESS_DENOISE_ENABLE
#if (WIRELESS_PLATE_REVERB_ENABLE)
    if (audio->plate_reverb) {
        log_info("close plate_reverb");
        close_plate_reverb(audio->plate_reverb);
    }
#endif//WIRELESS_PLATE_REVERB_ENABLE


    /* if (audio->howling) { */
    /* close_howling(audio->howling); */
    /* } */

    if (audio->ch_sw) {
        log_info("close ch_sw");
        channel_switch_close(&audio->ch_sw);
    }
#ifndef CONFIG_BOARD_WIRELESS_MIC_2T1_RX
#if (WIRELESS_MIC_OUTPUT_DAC_SAMETIME)
    //ex free
    if (audio->ex_dac) {
        audio_stream_del_entry(&audio->ex_dac->entry);
        audio_dac_free_channel(audio->ex_dac);
        free(audio->ex_dac);
        audio->ex_dac = NULL;
    }
    if (audio->ex_dac1) {
        log_info("close dac1");
        audio_stream_del_entry(&audio->ex_dac1->entry);
        audio_sound_dac_close(audio->ex_dac1);
        audio->ex_dac1 = NULL;
    }
    if (audio->ex_src) {
        stream_src_close(&audio->ex_src);
    }
    if (audio->ex_channel_zoom) {
        channel_switch_close(&audio->ex_channel_zoom);
    }
#endif

#if WIRELESS_HOWLING_ENABLE
    if (audio->howling_ps) {
        log_i("close_howling\n\n\n");
        audio_close_howling(audio->howling_ps);
        audio->howling_ps = NULL;
    }
    if (audio->notch_howling) {
        log_i("close_howling n\n\n\n");
        audio_close_howling(audio->notch_howling);
        audio->notch_howling = NULL;
    }
#endif
#if WIRELESS_VOICE_CHANGER_EN
    if (audio->voice_changer) {
        audio_voice_changer_close(audio->voice_changer);
        audio->voice_changer = NULL;
    }
#endif

#endif//CONFIG_BOARD_WIRELESS_MIC_2T1_RX

    //audio->stream必须在最后释放
    if (audio->stream) {
        log_info("close stream");
        audio_stream_close(audio->stream);
    }
    local_irq_disable();
    free(audio);
    *hdl = NULL;
    local_irq_enable();
}
extern u8 flag_mute_exdac;
void ex_dac_prohandler(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    if (flag_mute_exdac) {
        memset(in->data, 0x00, in->data_len);
    }
}
static void adapter_audio_stream_open_ex(struct adapter_audio_stream *audio, struct audio_stream_entry *start_entry)
{
    if (audio == NULL) {
        return ;
    }
#ifndef CONFIG_BOARD_WIRELESS_MIC_2T1_RX
#if (WIRELESS_MIC_OUTPUT_DAC_SAMETIME)
    printf("WIRELESS_MIC_OUTPUT_DAC_SAMETIME\n");

#if NEW_AUDIO_W_MIC

    struct dac_param dac_param_t = {0};
    dac_param_t.sample_rate = WIRELESS_DECODE_SAMPLERATE;
    dac_param_t.write_mode = WRITE_MODE_BLOCK;
    dac_param_t.delay_ms = 12;
    dac_param_t.protect_time = 0;
    dac_param_t.start_delay = 6;
    dac_param_t.underrun_time = 0;
    audio->ex_dac1 = audio_sound_dac_open(&dac_param_t);
    audio_sound_dac_set_analog_gain(audio->ex_dac1, 0xF, MAX_ANA_VOL); //设置音量
    audio_sound_dac_set_digital_gain(audio->ex_dac1, 0xF, 16384);
#else
    if (audio_dac_init_status() == 0) {
        app_audio_output_init();//如果DAC没有开， 这里打开一下
    }
    audio->ex_dac = (struct audio_dac_channel *)zalloc(sizeof(struct audio_dac_channel));
    if (audio->ex_dac) {
        extern struct audio_dac_hdl dac_hdl;
        audio_dac_new_channel(&dac_hdl, audio->ex_dac);
        struct audio_dac_channel_attr dac_attr;
        dac_attr.write_mode = WRITE_MODE_BLOCK;
        dac_attr.delay_time = 12;
        dac_attr.protect_time = 0;
        dac_attr.start_delay = 6;
        dac_attr.underrun_time = 0;
        audio_dac_channel_set_attr(audio->ex_dac, &dac_attr);
        audio_sound_dac_set_analog_gain(audio->ex_dac, 0xF, MAX_ANA_VOL); //设置音量
        audio_sound_dac_set_digital_gain(audio->ex_dac, 0xF, 16384);
    } else {
        log_e("adapter_audio_stream_attr_open err, line = %d\n", __LINE__);
    }
#endif
    int j = 0;
    struct audio_stream_entry *entries[15] = {NULL};

    struct audio_stream_entry *ex_entries_start = start_entry;
    entries[j++] = ex_entries_start;
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR || TCFG_AUDIO_MONO_LR_DIFF_CHANEL_SWITCH_ENABLE)

#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)
#if WIRELESS_CODING_SAMPLERATE != WIRELESS_DECODE_SAMPLERATE
    audio_sound_dac_set_sample_rate(audio->ex_dac1, WIRELESS_DECODE_SAMPLERATE);
    //audio_dac_set_sample_rate(&dac_hdl, WIRELESS_DECODE_SAMPLERATE);
    int dac_sr = audio_sound_dac_get_sample_rate(audio->ex_dac1);

    printf("dac_sr = %d>>>>>>>>>>>>>>>>>", dac_sr);
    audio->ex_src = stream_src_open(dac_sr, 1);
    entries[j++] = &audio->ex_src->entry;
#endif
#endif
#if (TCFG_AUDIO_MONO_LR_DIFF_CHANEL_SWITCH_ENABLE)
    audio->ex_channel_zoom = channel_switch_open(AUDIO_CH_DUAL_DIFF, 0);
#else
    audio->ex_channel_zoom = channel_switch_open(AUDIO_CH_LR, 0);
#endif
    entries[j++] = &audio->ex_channel_zoom->entry;
#endif

#if NEW_AUDIO_W_MIC
    struct audio_stream_entry *ex_entries_last = &(audio->ex_dac1->entry);
    audio->ex_dac1->entry.prob_handler = ex_dac_prohandler;
#else
    struct audio_stream_entry *ex_entries_last = &(audio->ex_dac->entry);
    audio->ex_dac->entry.prob_handler = ex_dac_prohandler;
#endif
    entries[j++] = ex_entries_last;
    //audio_stream_add_entry(entries[0], &(audio->ex_dac->entry));
    for (int i = 0; i < j - 1; i++) {
        audio_stream_add_entry(entries[i], entries[i + 1]);
    }
#endif
#endif//CONFIG_BOARD_WIRELESS_MIC_2T1_RX
}

struct adapter_audio_stream *adapter_audio_stream_open(
    struct adapter_stream_fmt *list,
    u8 list_num,
    struct audio_decoder *decoder,
    void (*resume)(void *priv),
    struct adapter_media_parm *media_parm)
{
    struct adapter_audio_stream *audio = zalloc(sizeof(struct adapter_audio_stream));
    if (audio == NULL) {
        return NULL;
    }

    struct audio_stream_entry *entries[15] = {NULL};
    if (list_num > (sizeof(entries) / sizeof(entries[0]) - 1)) {
        log_info("%s entry counter overlimit err!!\n", __FUNCTION__);
        adapter_audio_stream_close(&audio);
        return NULL;
    }
    int j = 0;
    struct audio_stream_entry *p = NULL;
    entries[j] = &decoder->entry;
    j++;
    for (int i = 0; i < list_num; i++) {
        p = adapter_audio_stream_attr_open(audio, list + i, media_parm);
        if (p) {
            entries[j] = p;
            j++;
        }
        log_info("%s, %d, i = %d, j = %d\n", __FUNCTION__, __LINE__, i, j);
    }
    if (j) {
        if (resume == NULL) {
            resume = adapter_audio_stream_resume;
        }
        audio->stream = audio_stream_open(decoder, resume);
        audio_stream_add_list(audio->stream, entries, j);

#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)
        if (media_parm->ex_output)
#endif
        {
#if (WIRELESS_DENOISE_ENABLE || WIRELESS_LLNS_ENABLE)
            adapter_audio_stream_open_ex(audio, entries[j - 2]);
#else
            adapter_audio_stream_open_ex(audio, entries[0]);
#endif
        }
    } else {
        adapter_audio_stream_close(&audio);
    }
    return audio;
}

static void audio_dac_stream_underrun_handler(void *priv, int arg)
{
    struct audio_mixer *mixer = (struct audio_mixer *)priv;

    audio_mixer_underrun_handler(mixer);
}

static void adapter_audio_dac_underrun_setup(struct adapter_audio_stream *audio, struct audio_mixer *mixer)
{
#if NEW_AUDIO_W_MIC
    if (audio->dac1) {
        sound_dac_set_underrun_handler(audio->dac1, mixer, audio_dac_stream_underrun_handler);
    }
#else
    if (audio->dac) {
        audio_dac_channel_set_underrun_handler(audio->dac, mixer, audio_dac_stream_underrun_handler);
    }
#endif
}

struct adapter_audio_stream *adapter_audio_mixer_stream_open(
    struct adapter_stream_fmt *list,
    u8 list_num,
    struct audio_mixer *mix,
    struct adapter_media_parm *media_parm)
{
    struct adapter_audio_stream *audio = zalloc(sizeof(struct adapter_audio_stream));
    if (audio == NULL) {
        return NULL;
    }

    struct audio_stream_entry *entries[15] = {NULL};
    if (list_num > (sizeof(entries) / sizeof(entries[0]) - 1)) {
        log_info("%s entry counter overlimit err!!\n", __FUNCTION__);
        adapter_audio_stream_close(&audio);
        return NULL;
    }
    int j = 0;
    struct audio_stream_entry *p = NULL;
    entries[j] = &mix->entry;
    j++;
    for (int i = 0; i < list_num; i++) {
        p = adapter_audio_stream_attr_open(audio, list + i, media_parm);
        if (p) {
            entries[j] = p;
            j++;
        }
        if (list[i].attr == ADAPTER_STREAM_ATTR_DAC) {
            adapter_audio_dac_underrun_setup(audio, mix);
        }
        log_info("%s, %d, i = %d, j = %d\n", __FUNCTION__, __LINE__, i, j);
    }
    if (j) {
        audio->stream = audio_stream_open(mix, audio_mixer_stream_resume);
        audio_stream_add_list(audio->stream, entries, j);
#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)
        if (media_parm->ex_output)
#endif
        {
            adapter_audio_stream_open_ex(audio, entries[0]);
        }
    } else {
        adapter_audio_stream_close(&audio);
    }
    return audio;
}

void adapter_audio_stream_set_vol(struct adapter_audio_stream *audio, u32 channel, u8 vol)
{
    if (audio && audio->dvol) {
        if (audio->vol_limit) {
            u16 vol_max = audio_dig_max_vol_get(audio->dvol);
            u16 valsum = vol * (vol_max + 1) / audio->vol_limit;
            vol = (u8)valsum;
        }
        if (channel & BIT(0)) {
            log_info("vol_l = %d\n", vol);
        }
        if (channel & BIT(1)) {
            log_info("vol_r = %d\n", vol);
        }
        audio_dig_vol_set(audio->dvol, channel, vol);
    }
}

void adapter_audio_stream_echo_run(struct adapter_audio_stream *audio, u8 echo_en)
{
#if WIRELESS_ECHO_ENABLE
    if (audio && audio->echo) {
        pause_echo(audio->echo, echo_en);
    }
#elif WIRELESS_PLATE_REVERB_ENABLE
    if (audio && audio->plate_reverb) {
        printf("echo_en = %d", echo_en);
        pause_plate_reverb(audio->plate_reverb, echo_en);
    }
#endif
}
void adapter_audio_stream_denoise_set_parm(struct adapter_audio_stream *audio, struct __stream_HighSampleRateSingleMicSystem_parm *parm)
{
#if (WIRELESS_DENOISE_ENABLE)
    if (audio && audio->denoise) {
        stream_HighSampleRateSingleMicSystem_set_parm(audio->denoise, parm);
    }
#endif
}

void adapter_audio_stream_denoise_switch(struct adapter_audio_stream *audio, u8 onoff)
{
#if (WIRELESS_DENOISE_ENABLE)
    if (audio && audio->denoise) {
        stream_HighSampleRateSingleMicSystem_switch(audio->denoise, onoff);
    }
#endif
}
#if (WIRELESS_LLNS_ENABLE)
void adapter_audio_stream_llns_set_parm(struct adapter_audio_stream *audio, struct __stream_llns_parm *parm)
{
    if (audio && audio->llns) {
        stream_llns_set_parm(audio->llns, parm);
    }
}

void adapter_audio_stream_llns_switch(struct adapter_audio_stream *audio, u8 onoff)
{
    if (audio && audio->llns) {
        stream_llns_switch(audio->llns, onoff);
    }
}
#endif

void adapter_audio_stream_dac_gain_set(struct adapter_audio_stream *audio, u8 dac_gain)
{
    if (audio && audio->dac1) {
        audio_sound_dac_set_analog_gain(audio->dac1, 0xF, dac_gain);//设置音量
    }
}
