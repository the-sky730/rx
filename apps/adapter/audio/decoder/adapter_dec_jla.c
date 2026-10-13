#include "adapter_wireless_dec.h"
#include "key_event_deal.h"
#include "adapter_wireless_command.h"

#if (\
        (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE) \
        || (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN == 1)\
    )
#define ADAPTER_DEC_JLA_MIXER_EN    1
#else
#define ADAPTER_DEC_JLA_MIXER_EN    0
#endif

#define MIXER_CH_NUM            2//音频声道数
#define MIXER_MS_UINIT          ((WIRELESS_DECODE_SAMPLERATE/1000)*MIXER_CH_NUM*2)
#define MIXER_MIN_LEN		    (MIXER_MS_UINIT*3)
//#define AUDIO_MIXER_LEN			(MIXER_MS_UINIT*5)//480
#define AUDIO_MIXER_LEN			(MIXER_MS_UINIT*2)//480

struct __jla {
#if (ADAPTER_DEC_JLA_MIXER_EN)
    s16 mix_buff[AUDIO_MIXER_LEN / 2];
    struct audio_mixer_ch mix_ch;
    struct audio_mixer mixer;
    struct adapter_audio_stream *stream;
#endif
    struct __adapter_wireless_dec *handler;
};
struct __jla *jla_hdl = NULL;

#define __this jla_hdl

static int adapter_dec_jla_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        printf("adapter_dec_jla_open error !!\n");
        return -1;
    }

    struct __jla *hdl = (struct __jla *)zalloc(sizeof(struct __jla));
    if (hdl == NULL) {
        printf("no mem for %s\n", __FUNCTION__);
        return -1;
    }
#if (ADAPTER_DEC_JLA_MIXER_EN)
    // 初始化mixer
    audio_mixer_open(&hdl->mixer);
    // 使能mixer事件回调
    audio_mixer_set_event_handler(&hdl->mixer, NULL);
    // 使能mixer采样率检测
    audio_mixer_set_check_sr_handler(&hdl->mixer, NULL);
    /*初始化mix_buf的长度*/
    audio_mixer_set_output_buf(&hdl->mixer, hdl->mix_buff, sizeof(hdl->mix_buff));
    u8 ch_num = 0;
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
    ch_num = 2;
#else
    ch_num = 1;
#endif
    /*!<  初始化mix通道数*/
    audio_mixer_set_channel_num(&hdl->mixer, ch_num);

    hdl->stream = adapter_audio_mixer_stream_open(
                      fmt->list,
                      fmt->list_num,
                      &hdl->mixer,
                      media_parm
                  );
    ASSERT(hdl->stream);

    hdl->handler = adapter_wireless_dec_open_base(fmt, media_parm, &hdl->mixer, hdl->stream);
#else
    hdl->handler = adapter_wireless_dec_open(fmt, media_parm);
#endif
    ASSERT(hdl->handler);

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    int command_reg[2];
    command_reg[0] = (int)NULL;
    command_reg[1] = (int)(fmt->cmd_parse_cbk);
    adapter_wireless_dec_ioctrl(__this->handler, ADAPTER_DEC_IOCTRL_CMD_REGISTER_COMMAND_CALLBACK, 2, (int *)command_reg);
    printf("adapter_dec_jla_open ok\n");
    return 0;
}
static void adapter_dec_jla_close(void)
{
    if (__this == NULL) {
        return ;
    }

    adapter_wireless_dec_close(&__this->handler);

#if (ADAPTER_DEC_JLA_MIXER_EN)
    adapter_audio_stream_close(&__this->stream);
    audio_mixer_close(&__this->mixer);
#endif

    local_irq_disable();
    free(__this);
    __this = NULL;
    local_irq_enable();
    mem_stats();
}
static void adapter_dec_jla_set_vol(u32 channel, u8 vol)
{
    if (__this == NULL) {
        return ;
    }
#if (ADAPTER_DEC_JLA_MIXER_EN)
    adapter_audio_stream_set_vol(__this->stream, channel, vol);
#else
    adapter_wireless_dec_set_vol(__this->handler, channel, vol);
#endif
}
static int adapter_dec_jla_ioctrl(int cmd, int argc, int *argv)
{
    if (__this == NULL) {
        return 0;
    }
    return adapter_wireless_dec_ioctrl(__this->handler, cmd, argc, argv);
}

int adapter_wireless_dec_frame_write(void *data, u16 len)
{
    if (__this == NULL) {
        return 0;
    }
    adapter_wireless_dec_rx_write(__this->handler, data, len);
    return 0;
}

REGISTER_ADAPTER_DECODER(decoder_jla) = {
    .dec_type = ADAPTER_DEC_TYPE_JLA,
    .open = adapter_dec_jla_open,
    .close = adapter_dec_jla_close,
    .set_vol = adapter_dec_jla_set_vol,
    .ioctrl = adapter_dec_jla_ioctrl,
};
