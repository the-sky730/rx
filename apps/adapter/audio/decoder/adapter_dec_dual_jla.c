#include "adapter_wireless_dec.h"
#include "adapter_wireless_command.h"
#include "app_config.h"

#define MIXER_CH_NUM            1//音频声道数
#define MIXER_MS_UINIT          ((WIRELESS_DECODE_SAMPLERATE/1000)*MIXER_CH_NUM*2)
#define MIXER_MIN_LEN		    (MIXER_MS_UINIT*3)
//#define AUDIO_MIXER_LEN			(MIXER_MS_UINIT*5)//480
#define AUDIO_MIXER_LEN			(MIXER_MS_UINIT*2)//480



struct __dual_jla {
    s16 mix_buff[AUDIO_MIXER_LEN / 2];
    struct audio_mixer_ch mix_ch;
    struct audio_mixer mixer;
    struct adapter_audio_stream *stream;
    struct __adapter_wireless_dec *handler[2];
};
struct __dual_jla *dual_jla = NULL;

#define __this dual_jla


u32 adapter_jla_dec_get_mixer_buf_total(void)
{
    return  AUDIO_MIXER_LEN;
}

u32 adapter_jla_dec_get_mixer_buf_cur(void)
{
    u32 len = 0;
    if (__this) {
        len = audio_mixer_data_len(&__this->mixer);
        //if(len > (adapter_jla_dec_get_mixer_buf_total()/2)){
        //   putchar('B');
        //}
        return len;
    }
    return 0;
}

extern int audio_dac_check_write_points(struct audio_dac_channel *ch);
static int mixer_test_check_out_len(void *priv, struct audio_mixer *mixer, int len)
{
    int wlen = 0;
    if (__this && __this->stream && __this->stream->dac) {
        int points = audio_dac_check_write_points(__this->stream->dac);
        wlen = points * 2 * MIXER_CH_NUM; // s16单声道
    }
    if (wlen == 0) {
        wlen = len;
    }
    return wlen;
}


static int adapter_dec_dual_jla_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        printf("adapter_dec_dual_jla_open error !!\n");
        return -1;
    }

    struct __dual_jla *hdl = (struct __dual_jla *)zalloc(sizeof(struct __dual_jla));
    if (hdl == NULL) {
        printf("no mem for %s\n", __FUNCTION__);
        return -1;
    }

    // 初始化mixer
    audio_mixer_open(&hdl->mixer);
    // 使能mixer事件回调
    audio_mixer_set_event_handler(&hdl->mixer, NULL);
    // 使能mixer采样率检测
    audio_mixer_set_check_sr_handler(&hdl->mixer, NULL);
    /*初始化mix_buf的长度*/
    audio_mixer_set_output_buf(&hdl->mixer, hdl->mix_buff, sizeof(hdl->mix_buff));
    u8 ch_num = 0;
//#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
//#if WIRELESS_TX_MIC_STEREO_OUTPUT
//    ch_num = 2;
//#else
//    ch_num = 1;
//#endif
//#else
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
    ch_num = 2;
#else
    ch_num = 1;
#endif//#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
//#endif//#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
    /*!<  初始化mix通道数*/
    audio_mixer_set_channel_num(&hdl->mixer, ch_num);

    hdl->stream = adapter_audio_mixer_stream_open(
                      fmt->list,
                      fmt->list_num,
                      &hdl->mixer,
                      media_parm
                  );
    ASSERT(hdl->stream);

    //media_parm->priv_data = hdl->stream;
    media_parm->ch_index = 0;
    hdl->handler[0] = adapter_wireless_dec_open_base(fmt, media_parm, &hdl->mixer, hdl->stream);
    ASSERT(hdl->handler[0]);
    media_parm->ch_index = 1;
    hdl->handler[1] = adapter_wireless_dec_open_base(fmt, media_parm, &hdl->mixer, hdl->stream);
    ASSERT(hdl->handler[1]);

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    int command_reg[2];
    command_reg[0] = (int)NULL;
    command_reg[1] = (int)(fmt->cmd_parse_cbk);
    adapter_wireless_dec_ioctrl(__this->handler[0], ADAPTER_DEC_IOCTRL_CMD_REGISTER_COMMAND_CALLBACK, 2, (int *)command_reg);
    adapter_wireless_dec_ioctrl(__this->handler[1], ADAPTER_DEC_IOCTRL_CMD_REGISTER_COMMAND_CALLBACK, 2, (int *)command_reg);

    printf("adapter_dec_dual_jla_open okn");
    return 0;
}
static void adapter_dec_dual_jla_close(void)
{
    if (__this == NULL) {
        return ;
    }


    adapter_wireless_dec_close(&__this->handler[0]);
    adapter_wireless_dec_close(&__this->handler[1]);

    adapter_audio_stream_close(&__this->stream);

    audio_mixer_close(&__this->mixer);

    local_irq_disable();
    free(__this);
    __this = NULL;
    local_irq_enable();
    mem_stats();
}
static void adapter_dec_dual_jla_set_vol(u32 channel, u8 vol)
{
    //adapter_wireless_dec_set_vol(__this, channel, vol);
    adapter_audio_stream_set_vol(__this->stream, channel, vol);
}
static int adapter_dec_dual_jla_ioctrl(int cmd, int argc, int *argv)
{

    int ch = (cmd >> 30) & 0x3;
    if (ch > 2) {
        printf("%s, channel err!!\n", __FUNCTION__);
        return -1;
    }
    cmd = cmd & 0x7FFFFFF;
    printf("cmd = %d,ch = %d", cmd, ch);
    return adapter_wireless_dec_ioctrl(__this->handler[ch], cmd, argc, argv);
}

#define     MY_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     MY_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

//u8 g_cur_chl = 0;
//static u8 ch0_debug_io = 0;
//static u8 ch1_debug_io = 0;
int adapter_dec_dual_jla_frame_write(u8 channel, void *data, u16 len)
{
    if (channel >= 2) {
        return -1;
    }
    if (__this == NULL) {
        return -1;
    }


#if 0
    if (channel == 0) {
        if (ch0_debug_io) {
            MY_IO_DEBUG_0(A, 3);
        } else {
            MY_IO_DEBUG_1(A, 3);
        }
        ch0_debug_io = !ch0_debug_io;
    } else if (channel == 1) {
        if (ch1_debug_io) {
            MY_IO_DEBUG_0(A, 4);
        } else {
            MY_IO_DEBUG_1(A, 4);
        }
        ch1_debug_io = !ch1_debug_io;
    }
    g_cur_chl = channel;
#endif
    //printf("%d ", channel);
    //putchar('@');
    adapter_wireless_dec_rx_write(__this->handler[channel], data, len);
    return 0;
}


int adapter_dec_dual_jla_frame_write_pause(u8 channel, u8 value)
{
    if (channel >= 2) {
        return -1;
    }
    if (__this) {
        adapter_wireless_dec_ioctrl(__this->handler[channel], ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_PAUSE, 1, (int *)value);
    }
    return 0;
}

int adapter_dec_dual_jla_frame_write_pause_get_status(u8 channel)
{
    if (channel >= 2) {
        return 0;
    }
    if (__this) {
        return adapter_wireless_dec_ioctrl(__this->handler[channel], ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_PAUSE_GET_STATUS, 0, 0);
    }
    return 0;
}


REGISTER_ADAPTER_DECODER(decoder_dual_jla) = {
    .dec_type = ADAPTER_DEC_TYPE_DUAL_JLA,
    .open = adapter_dec_dual_jla_open,
    .close = adapter_dec_dual_jla_close,
    .set_vol = adapter_dec_dual_jla_set_vol,
    .ioctrl = adapter_dec_dual_jla_ioctrl,
};
