#include "clock_cfg.h"
#include "media/sbc_enc.h"
#include "avctp_user.h"
#include "adapter_odev.h"
#include "adapter_process.h"
#include "adapter_idev.h"

#define SBC_ENC_IN_SIZE			512
#define SBC_ENC_OUT_SIZE		256

#define SBC_ENC_IN_CBUF_SIZE	(SBC_ENC_IN_SIZE  * 8)
#define SBC_ENC_OUT_CBUF_SIZE	(SBC_ENC_OUT_SIZE * 8)

struct sbc_enc_hdl {
    struct audio_stream_entry entry;		// 音频流入口

    struct audio_encoder encoder;
    OS_SEM pcm_frame_sem;
    u8 output_frame[SBC_ENC_OUT_SIZE];
    u8 pcm_frame[SBC_ENC_IN_SIZE];
    u8 frame_size;
    cbuffer_t output_cbuf;
    u8 out_cbuf_buf[SBC_ENC_OUT_CBUF_SIZE];
    cbuffer_t pcm_in_cbuf;                  /*要注意要跟DAC的buf大小一致*/
    u8 in_cbuf_buf[SBC_ENC_IN_CBUF_SIZE];
};

static struct sbc_enc_hdl *sbc_enc = NULL;
#define __this sbc_enc

enum {
    SBC_RUN_STEP_NOR = 0,
    SBC_RUN_STEP_WAIT_STOP,
    SBC_RUN_STEP_STOP,
    SBC_RUN_STEP_WAIT_NOR,
};

#define SBC_ENC_WRITING                 BIT(1)
#define SBC_ENC_START                   BIT(2)
#define SBC_ENC_WAKEUP_SEND             BIT(3)  /*减少唤醒问题*/
#define SBC_ENC_PCM_MUTE_DATA           BIT(4)  /*发一包静音数据*/

static u8 sbc_buf_flag = 0;

sbc_t sbc_param = {
    .frequency = SBC_FREQ_44100,
    .blocks = SBC_BLK_16,
    .subbands = SBC_SB_8,
    .mode = SBC_MODE_STEREO,
    .allocation = 0,
    .endian = SBC_LE,
    .bitpool = 53,
};

int adapter_audio_sbc_enc_is_work(void)
{
    if ((sbc_buf_flag & SBC_ENC_START) &&  __this) {
        return true;
    }
    return false;
}

///库里面直接调用
int a2dp_sbc_encoder_get_data(u8 *packet, u16 buf_len, int *frame_size)
{
    if (__this == NULL) {
        return 0;
    }

    if (false == adapter_audio_sbc_enc_is_work()) {
        return 0;
    }

    if (cbuf_get_data_size(&__this->output_cbuf) < buf_len) {
        return 0;
    }
    putchar('o');

    int debug_frame_size = __this->frame_size;
    int number = buf_len / debug_frame_size;  /*取整数包*/
    *frame_size = debug_frame_size;
    u16 rlen = cbuf_read(&__this->output_cbuf, packet, *frame_size * number);
    if (rlen == 0) {
    }

    return rlen;
}


static int audio_sbc_enc_get_channel_num(void)
{
    return sbc_param.mode == SBC_MODE_MONO ? 1 : 2;
}

int audio_sbc_enc_get_rate(void)
{
    int sr = 44100;
    switch (sbc_param.frequency) {
    case SBC_FREQ_16000:
        sr = 16000;
        break;
    case SBC_FREQ_32000:
        sr = 32000;
        break;
    case SBC_FREQ_44100:
        sr = 44100;
        break;
    case SBC_FREQ_48000:
        sr = 48000;
        break;
    }
    return sr;
}

int adapter_audio_enc_get_channel_num(void)
{
    g_printf("adapter_audio_enc_get_channel_num:%d\n", audio_sbc_enc_get_channel_num());
    return audio_sbc_enc_get_channel_num();
}

int adapter_audio_enc_get_rate(void)
{
    g_printf("adapter_audio_enc_get_rate:%d\n", audio_sbc_enc_get_rate());
    return audio_sbc_enc_get_rate();
}


static int sbc_enc_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}

static int sbc_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    if (__this == NULL || encoder == NULL) {
        return 0;
    }
    struct sbc_enc_hdl *enc = container_of(encoder, struct sbc_enc_hdl, encoder);
    enc->frame_size = len;

    //putchar('O');


    u16 wlen = cbuf_write(&enc->output_cbuf, frame, len);
    if (wlen != len) {
        return len;
    }
    if (cbuf_get_data_size(&__this->output_cbuf) >= enc->frame_size * 5) {
        //达到基本的数据量，唤醒蓝牙发数
        user_send_cmd_prepare(USER_CTRL_CMD_RESUME_STACK, 0, NULL);
    }
    return len;
}

const static struct audio_enc_handler sbc_enc_handler = {
    .enc_probe  = sbc_enc_probe_handler,
    .enc_output = sbc_enc_output_handler,
};

static int sbc_enc_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    u8  read_flag = 1;
    int pcm_len = 0;

    if (__this == NULL || encoder == NULL) {
        return 0;
    }

    struct sbc_enc_hdl *enc = container_of(encoder, struct sbc_enc_hdl, encoder);
    if (enc == NULL) {
        g_f_printf("enc NULL");
    }

    int limit_size = SBC_ENC_IN_SIZE;
    if (cbuf_get_data_size(&enc->pcm_in_cbuf) < limit_size) {
        if (!(sbc_buf_flag & SBC_ENC_PCM_MUTE_DATA)) {
            return 0;
        } else {
            read_flag = 0;
        }
    }

    //putchar('R');
    if (read_flag) {
        pcm_len = cbuf_read(&enc->pcm_in_cbuf, enc->pcm_frame, SBC_ENC_IN_SIZE);
        audio_stream_resume(&__this->entry);
    } else {
        memset(enc->pcm_frame, 0, SBC_ENC_IN_SIZE);
        pcm_len = SBC_ENC_IN_SIZE;
    }

    *frame = enc->pcm_frame;
    return pcm_len;
}

static void sbc_enc_pcm_put(struct audio_encoder *encoder, s16 *frame)
{

}

static const struct audio_enc_input sbc_enc_input = {
    .fget = sbc_enc_pcm_get,
    .fput = sbc_enc_pcm_put,
};

static void sbc_enc_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_ENC_EVENT_END:
        puts("AUDIO_ENC_EVENT_END\n");
        break;
    }
}



static int audio_sbc_enc_write_base(s16 *data, int len, u8 ch_i, u8 ch_o)
{
    if (!(sbc_buf_flag & SBC_ENC_START)) {
        return 0;
    }
    if (!__this) {
        return 0;
    }
    //putchar('B');
    sbc_buf_flag |= SBC_ENC_WRITING;

    int wlen = 0;
    int ilen = 0;
    int remain_len = len;
    while (remain_len) {
        wlen = 0;
        void *obuf = cbuf_write_alloc(&__this->pcm_in_cbuf, &wlen);
        if (!wlen) {
            break;
        }
        if (ch_o == ch_i) {
            ilen = wlen;
            if (ilen > remain_len) {
                ilen = remain_len;
                wlen = ilen;
            }
            memcpy(obuf, data, wlen);
        } else if ((ch_o == 2) && (ch_i == 1)) {
            ilen = (wlen >> 2) << 1;
            if (ilen > remain_len) {
                ilen = remain_len;
                wlen = ilen << 1;
            }
            s16 *outbuf = obuf;
            s16 *inbuf = data;
            int run_len = ilen >> 1;
            while (run_len --) {
                *outbuf++ = *inbuf;
                *outbuf++ = *inbuf;
                inbuf++;
            }
        } else if ((ch_o == 1) && (ch_i == 2)) {
            ilen = wlen << 1;
            if (ilen > remain_len) {
                ilen = remain_len;
                wlen = ilen >> 1;
            }
            s16 *outbuf = obuf;
            s16 *inbuf = data;
            int run_len = ilen >> 2;
            while (run_len--) {
                *outbuf++ = data_sat_s16(inbuf[0] + inbuf[1]);
                inbuf += 2;
            }
        }
        cbuf_write_updata(&__this->pcm_in_cbuf, wlen);
        remain_len -= ilen;
        data += ilen / 2;
    };
    wlen = len - remain_len;

    if (cbuf_get_data_size(&__this->pcm_in_cbuf) >= SBC_ENC_IN_SIZE) {
        audio_encoder_resume(&__this->encoder);
    }
    sbc_buf_flag &= ~SBC_ENC_WRITING;
    return wlen;
}

static int audio_stream_sbc_adapter_data_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    //putchar('E');
    //return in->data_len;

    local_irq_disable();
    if (false == adapter_audio_sbc_enc_is_work()) {
        local_irq_enable();
        return in->data_len;
    }
    local_irq_enable();

    if (in->data_len == 0) {
        return 0;
    }

    //putchar('X');
    return audio_sbc_enc_write_base(in->data, in->data_len, in->channel, audio_sbc_enc_get_channel_num());
}

static int adapter_enc_sbc_close(void)
{
    if (!__this) {
        return -1;
    }
    g_printf("adapter_enc_sbc_close\n");

    sbc_buf_flag &= ~SBC_ENC_START;

    while (sbc_buf_flag & SBC_ENC_WRITING) {
        os_time_dly(1);
    }

    audio_encoder_close(&__this->encoder);
    free(__this);
    __this = NULL;


    clock_remove(ENC_SBC_CLK);
    clock_set_cur();

    g_printf("adapter_enc_sbc_close end\n");
    return 0;
}
static void audio_stream_sbc_adapter_data_process_len(struct audio_stream_entry *entry,  int len)
{

}
static int adapter_enc_sbc_open(struct adapter_encoder_fmt *encoder_fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        printf("audio_sbc_enc_open aready, fail\n");
        return -1;
    }
    ASSERT(media_parm);

    int err;
    struct audio_fmt fmt;
    u16 frame_len = 512;
    memcpy(&sbc_param, media_parm->priv_data, sizeof(sbc_t));

    sbc_buf_flag &= ~SBC_ENC_START;

    fmt.channel = 1;
    fmt.frame_len = frame_len;
    fmt.coding_type = AUDIO_CODING_SBC;
    fmt.priv = &sbc_param;

    struct sbc_enc_hdl *hdl = zalloc(sizeof(struct sbc_enc_hdl));
    ASSERT(hdl, "audio_sbc_enc_open no mem!!\n");

    cbuf_init(&hdl->output_cbuf, hdl->out_cbuf_buf, SBC_ENC_OUT_CBUF_SIZE);
    cbuf_init(&hdl->pcm_in_cbuf, hdl->in_cbuf_buf,  SBC_ENC_IN_CBUF_SIZE);

    os_sem_create(&hdl->pcm_frame_sem, 0);

    audio_encoder_open(&hdl->encoder, &sbc_enc_input, adapter_encoder_get_task_handle());
    audio_encoder_set_handler(&hdl->encoder, &sbc_enc_handler);
    audio_encoder_set_fmt(&hdl->encoder, &fmt);
    audio_encoder_set_event_handler(&hdl->encoder, sbc_enc_event_handler, 0);
    audio_encoder_set_output_buffs(&hdl->encoder, hdl->output_frame,
                                   sizeof(hdl->output_frame), 1);

    if (!hdl->encoder.enc_priv) {
        log_e("encoder err, maybe coding(0x%x) disable \n", fmt.coding_type);
        err = -EINVAL;
        goto __err;
    }

    int start_err = audio_encoder_start(&hdl->encoder);
    hdl->entry.data_process_len = audio_stream_sbc_adapter_data_process_len;
    hdl->entry.data_handler = audio_stream_sbc_adapter_data_handler;

    clock_add(ENC_SBC_CLK);
    clock_set_cur();

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    sbc_buf_flag |= SBC_ENC_START;

    g_printf("sbc_enc_open ok %d\n", start_err);

    return 0;
__err:
    audio_encoder_close(&hdl->encoder);
    local_irq_disable();
    free(hdl);
    hdl = NULL;
    local_irq_enable();
    return err;
}


static struct audio_stream_entry *adapter_enc_sbc_get_stream_entry(void)
{
    if (__this) {
        return &__this->entry;
    }
    return NULL;
}



REGISTER_ADAPTER_ENCODER(adapter_enc_sbc) = {
    .enc_type     = ADAPTER_ENC_TYPE_SBC,
    .open   = adapter_enc_sbc_open,
    .close  = adapter_enc_sbc_close,
    .get_stream_entry = adapter_enc_sbc_get_stream_entry,
};

