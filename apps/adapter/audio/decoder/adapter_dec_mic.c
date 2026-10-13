#include "adapter_dec_mic.h"
#include "adapter_decoder.h"
#include "adapter_audio_stream.h"
#include "adapter_adc.h"
#include "media/pcm_decoder.h"
#include "clock_cfg.h"
#include "audio_config.h"
#include "asm/dac.h"
#include "adapter_wireless_command.h"

#define ADC_PCM_FOR_JLA_EN				1

#if (TCFG_AUDIO_ADC_ENABLE)
struct __adapter_dec_mic {
    struct adapter_media_parm 		*media_parm;// 动态参数
    struct adapter_decoder_fmt  	*fmt;		// 解码参数配置
    struct adapter_audio_stream 	*stream;	// 音频流
    struct pcm_decoder 				pcm_dec;	// pcm解码句柄
    struct audio_res_wait 			wait;		// 资源等待句柄
    volatile u8 					status;		// 正在解码
    u16 							sample_rate;// 采样率
};
struct __adapter_dec_mic *dec_mic = NULL;
#define __this dec_mic

extern void adapter_audio_stream_echo_run(struct adapter_audio_stream *audio, u8 echo_en);

u16 adapter_dec_mic_get_samplerate(void *priv)
{
    u16 sample_rate = 0;
    if (__this) {
        sample_rate = __this->sample_rate;
    }
    return sample_rate;
}

static void adapter_dec_mic_resume(void *priv)
{
    if (__this && __this->status) {
        //putchar('1');
        audio_decoder_resume(&__this->pcm_dec.decoder);
    }
}

static void adapter_dec_mic_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    return ;
}

static int adapter_dec_mic_stop(void)
{
    if (!__this || !__this->status) {
        return 0;
    }

    __this->status = 0;
    adapter_adc_mic_close();
    pcm_decoder_close(&__this->pcm_dec);
    //app_audio_state_exit(APP_AUDIO_STATE_MUSIC);

    //关闭数据流
    adapter_audio_stream_close(&__this->stream);

    //clock_set_cur();
    return 0;
}

static int adapter_dec_mic_start(void)
{
    int err;
    if (!__this) {
        return -EINVAL;
    }

#if(ADC_PCM_FOR_JLA_EN)
    adapter_adc_mic_open(WIRELESS_CODING_SAMPLERATE, WIRELESS_MIC_ADC_GAIN, WIRELESS_MIC_ADC_POINT_UNIT);
    //adapter_adc_mic_open(44100, 10, 120);
#else
    adapter_adc_mic_open(WIRELESS_CODING_SAMPLERATE, WIRELESS_MIC_ADC_GAIN, WIRELESS_MIC_ADC_POINT_UNIT);
#endif

    adapter_adc_mic_data_output_resume_register(adapter_dec_mic_resume, NULL);

    // 打开pcm解码器
    err = pcm_decoder_open(&__this->pcm_dec, adapter_decoder_get_task_handle());
    if (err) {
        adapter_adc_mic_close();
        pcm_decoder_close(&__this->pcm_dec);
        return -1;
    }

    pcm_decoder_set_event_handler(&__this->pcm_dec, adapter_dec_mic_event_handler, 0);
    pcm_decoder_set_read_data(&__this->pcm_dec, adapter_adc_mic_data_read, NULL);

    //数据流初始化
    __this->stream = adapter_audio_stream_open(
                         __this->fmt->list,
                         __this->fmt->list_num,
                         &__this->pcm_dec.decoder,
                         NULL,
                         __this->media_parm
                     );
    if (__this->stream == NULL) {
        adapter_adc_mic_close();
        pcm_decoder_close(&__this->pcm_dec);
        return -1;
    }

    // 开始解码
    __this->status = 1;
    err = audio_decoder_start(&__this->pcm_dec.decoder);
    if (err) {
        __this->status = 0;
        adapter_adc_mic_close();
        pcm_decoder_close(&__this->pcm_dec);
        return -1;
    }
    //启动mic
    adapter_adc_mic_start();


    //clock_set_cur();
    return 0;
}


static int adapter_dec_mic_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    if (event == AUDIO_RES_GET) {
        // 启动解码
        err = adapter_dec_mic_start();
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        adapter_dec_mic_stop();
    }

    return err;
}

int adapter_dec_mic_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        printf("adapter_dec_mic_open aready, fail!!\n");
        return 0;
    }
    ASSERT(fmt, "%s, %d, fmt err!!\n", __FUNCTION__, __LINE__);
    struct __adapter_dec_mic *hdl = zalloc(sizeof(struct __adapter_dec_mic));
    if (hdl == NULL) {
        return -1;
    }

    g_printf("adapter_dec_mic_open\n");
    media_parm->ex_output = 1;
#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)
    if (hdl->media_parm) {
        hdl->media_parm->start_vol_l = 100;
        hdl->media_parm->start_vol_r = 100;
        printf("start_vol_l = %d\n", hdl->media_parm->start_vol_l);
    }
#endif
    hdl->fmt = fmt;
    hdl->media_parm = media_parm;

    hdl->pcm_dec.sample_rate = WIRELESS_CODING_SAMPLERATE;//44100L;
    hdl->pcm_dec.ch_num = 1;
    hdl->pcm_dec.output_ch_num = 1;
    hdl->pcm_dec.output_ch_type = AUDIO_CH_DIFF;

    hdl->sample_rate = hdl->pcm_dec.sample_rate;

    hdl->wait.protect = 1;

    hdl->wait.priority = 3;
    hdl->wait.preemption = 0;
    hdl->wait.snatch_same_prio = 1;
    hdl->wait.handler = adapter_dec_mic_wait_res_handler;

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    audio_decoder_task_add_wait(adapter_decoder_get_task_handle(), &hdl->wait);

    g_printf("adapter_dec_mic_open ok!!!!!!!\n");
    //clock_add(DEC_PCM_CLK);

    return 0;
}

void adapter_dec_mic_close(void)
{
    g_printf("adapter_dec_mic_close\n");

    if (!__this) {
        return ;
    }

    if (__this->status) {
        adapter_dec_mic_stop();
    }

    audio_decoder_task_del_wait(adapter_decoder_get_task_handle(), &__this->wait);
    //clock_remove(DEC_PCM_CLK);

    local_irq_disable();
    free(__this);
    __this = NULL;
    local_irq_enable();
}

static void adapter_dec_mic_set_vol(u32 channel, u8 vol)
{
    if (__this && __this->stream) {
        adapter_audio_stream_set_vol(__this->stream, channel, vol);
    }
}
static int adapter_dec_mic_ioctrl(int cmd, int argc, int *argv)
{
    int value = 0;
    u8 echo_value = 0;
    if (__this == NULL) {
        return 0;
    };

    switch (cmd) {
#if WIRELESS_LLNS_ENABLE
    case ADAPTER_DEC_IOCTRL_CMD_LLNS_SET_PARM:
        adapter_audio_stream_llns_set_parm(__this->stream, (struct __stream_llns_parm *)argv);
        break;
    case ADAPTER_DEC_IOCTRL_CMD_LLNS_SWITCH:
        adapter_audio_stream_llns_switch(__this->stream, (u8)argv);
        break;
#endif
    case ADAPTER_DEC_IOCTRL_CMD_DENOISE_SET_PARM:
        adapter_audio_stream_denoise_set_parm(__this->stream, (struct __stream_HighSampleRateSingleMicSystem_parm *)argv);
        break;
    case ADAPTER_DEC_IOCTRL_CMD_DENOISE_SWITCH:
        adapter_audio_stream_denoise_switch(__this->stream, (u8)argv);
        break;
    case ADAPTER_DEC_IOCTRL_CMD_ECHO_EN:
        value = (int)argv;
        if (value == WIRELESS_MIC_ECHO_OFF) {
            echo_value = 1;
        } else {
            echo_value = 0;
        }
        adapter_audio_stream_echo_run(__this->stream, echo_value);
        break;
    }
    return 0;
}
REGISTER_ADAPTER_DECODER(decoder_mic) = {
    .dec_type = ADAPTER_DEC_TYPE_MIC,
    .open = adapter_dec_mic_open,
    .close = adapter_dec_mic_close,
    .set_vol = adapter_dec_mic_set_vol,
    .ioctrl = adapter_dec_mic_ioctrl,
};

#endif//TCFG_AUDIO_ADC_ENABLE

