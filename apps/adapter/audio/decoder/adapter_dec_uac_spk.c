#include "adapter_dec_uac_spk.h"
#include "adapter_decoder.h"
#include "usb/device/uac_audio.h"
#include "uac_stream.h"
#include "adapter_audio_stream.h"
#include "media/pcm_decoder.h"
#include "clock_cfg.h"
#include "adapter_media.h"

#if (USB_DEVICE_CLASS_CONFIG & SPEAKER_CLASS)
struct __adapter_dec_uac_spk {
    struct adapter_media_parm 		*media_parm;// 动态参数
    struct adapter_decoder_fmt  	*fmt;		// 解码参数配置
    struct adapter_audio_stream 	*stream;	// 音频流
    struct pcm_decoder 				pcm_dec;	// pcm解码句柄
    struct audio_res_wait 			wait;		// 资源等待句柄
    volatile u8 					status;		// 正在解码
    u16 							sample_rate;// 采样率
};
struct __adapter_dec_uac_spk *speaker = NULL;
#define __this speaker

u16 adapter_dec_uac_spk_get_samplerate(void *priv)
{
    u16 sample_rate = 0;
    if (__this) {
        sample_rate = __this->sample_rate;
    }
    return sample_rate;
}

static void adapter_dec_uac_spk_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    return ;
}

static int adapter_dec_uac_spk_stop(void)
{
    if (!__this || !__this->status) {
        return 0;
    }

    __this->status = 0;
    pcm_decoder_close(&__this->pcm_dec);
    //app_audio_state_exit(APP_AUDIO_STATE_MUSIC);

    //关闭数据流
    adapter_audio_stream_close(&__this->stream);

    //clock_set_cur();
    return 0;
}

static int adapter_dec_uac_spk_start(void)
{
    int err;
    if (!__this) {
        return -EINVAL;
    }

    // 打开pcm解码器
    err = pcm_decoder_open(&__this->pcm_dec, adapter_decoder_get_task_handle());
    if (err) {
        pcm_decoder_close(&__this->pcm_dec);
        return -1;
    }

    pcm_decoder_set_event_handler(&__this->pcm_dec, adapter_dec_uac_spk_event_handler, 0);
    pcm_decoder_set_read_data(&__this->pcm_dec, uac_speaker_read, NULL);

    //数据流初始化
    __this->stream = adapter_audio_stream_open(
                         __this->fmt->list,
                         __this->fmt->list_num,
                         &__this->pcm_dec.decoder,
                         NULL,
                         __this->media_parm
                     );
    if (__this->stream == NULL) {
        pcm_decoder_close(&__this->pcm_dec);
        return -1;
    }

    // 开始解码
    __this->status = 1;
    err = audio_decoder_start(&__this->pcm_dec.decoder);
    if (err) {
        __this->status = 0;
        pcm_decoder_close(&__this->pcm_dec);
        return -1;
    }
    //clock_set_cur();
    return 0;
}


static int adapter_dec_uac_spk_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    if (event == AUDIO_RES_GET) {
        // 启动解码
        err = adapter_dec_uac_spk_start();
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        adapter_dec_uac_spk_stop();
    }

    return err;
}


static void adapter_dec_uac_spk_rx_handler(int event, void *data, int len)
{
    if (__this && __this->status) {
        //putchar('1');
        audio_decoder_resume(&__this->pcm_dec.decoder);
    }
}

int adapter_dec_uac_spk_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        printf("adapter_dec_uac_spk_open aready, fail!!\n");
        return 0;
    }
    ASSERT(fmt, "%s, %d, fmt err!!\n", __FUNCTION__, __LINE__);
    struct __adapter_dec_uac_spk *hdl = NULL;
    hdl = zalloc(sizeof(struct __adapter_dec_uac_spk));
    if (hdl == NULL) {
        return -1;
    }

    g_printf("adapter_dec_uac_spk_open\n");

    hdl->fmt = fmt;
    hdl->media_parm = media_parm;

    ///以下参数需要通过外面配置
    hdl->pcm_dec.sample_rate = media_parm->sample_rate;
    hdl->pcm_dec.ch_num = media_parm->ch_num;
    g_printf("uac spk sample_rate = %d, ch_num = %d\n", media_parm->sample_rate, media_parm->ch_num);

    hdl->pcm_dec.output_ch_num = 2;
    hdl->pcm_dec.output_ch_type = AUDIO_CH_LR;

    set_uac_speaker_rx_handler(0, adapter_dec_uac_spk_rx_handler);

    hdl->sample_rate = hdl->pcm_dec.sample_rate;

    hdl->wait.protect = 1;

    hdl->wait.priority = 3;
    hdl->wait.preemption = 0;
    hdl->wait.snatch_same_prio = 1;
    hdl->wait.handler = adapter_dec_uac_spk_wait_res_handler;

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    audio_decoder_task_add_wait(adapter_decoder_get_task_handle(), &hdl->wait);

    g_printf("adapter_dec_uac_spk_open ok!!!!!!!\n");
    //clock_add(DEC_PCM_CLK);

    return 0;
}

void adapter_dec_uac_spk_close(void)
{
    g_printf("adapter_dec_uac_spk_close\n");

    if (!__this) {
        return ;
    }

    if (__this->status) {
        adapter_dec_uac_spk_stop();
    }

    audio_decoder_task_del_wait(adapter_decoder_get_task_handle(), &__this->wait);
    //clock_remove(DEC_PCM_CLK);

    local_irq_disable();
    free(__this);
    __this = NULL;
    local_irq_enable();
}

static void adapter_dec_uac_spk_set_vol(u32 channel, u8 vol)
{
    if (__this && __this->stream) {
        adapter_audio_stream_set_vol(__this->stream, channel, vol);
    }
}

REGISTER_ADAPTER_DECODER(decoder_uac_spk) = {
    .dec_type = ADAPTER_DEC_TYPE_UAC_SPK,
    .open = adapter_dec_uac_spk_open,
    .close = adapter_dec_uac_spk_close,
    .set_vol = adapter_dec_uac_spk_set_vol,
};

#endif//(USB_DEVICE_CLASS_CONFIG & SPEAKER_CLASS)

