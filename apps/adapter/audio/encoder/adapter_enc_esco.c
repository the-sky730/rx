#include "adapter_enc_esco.h"
#include "adapter_media.h"
#include "clock_cfg.h"
#include "media/sbc_enc.h"
#include "avctp_user.h"
#include "classic/hci_lmp.h"
#include "clock_cfg.h"
#include "media/includes.h"
#include "audio_splicing.h"

#define ESCO_ENC_IN_SIZE   		60
#define ESCO_ENC_OUT_SIZE  		30

#define ESCO_ENC_IN_CBUF_SIZE	(ESCO_ENC_IN_SIZE  * 8)
#define ESCO_ENC_OUT_CBUF_SIZE	(ESCO_ENC_OUT_SIZE * 8)

struct esco_enc_hdl {
    struct audio_stream_entry entry;					// 音频流入口
    struct audio_encoder encoder;
    s16 output_frame[ESCO_ENC_OUT_SIZE];               	//align 4Bytes
    int pcm_frame[ESCO_ENC_IN_SIZE];                   	//align 4Bytes
    cbuffer_t pcm_in_cbuf;
    u8 in_cbuf_buf[ESCO_ENC_IN_CBUF_SIZE];
};

static struct esco_enc_hdl *esco_enc = NULL;
#define __this esco_enc

u32 adapter_enc_esco_get_output_total_size(void *priv)
{
    return ESCO_ENC_IN_CBUF_SIZE;
}

u32 adapter_enc_esco_get_output_size(void *priv)
{
    if (__this)	{
        return cbuf_get_data_size(&__this->pcm_in_cbuf);
    }
    return 0;
}

static int adapter_enc_esco_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    //putchar('G');
    int rlen = 0;
    if (__this == NULL || encoder == NULL) {
        return 0;
    }

    struct esco_enc_hdl *enc = container_of(encoder, struct esco_enc_hdl, encoder);

    if (enc == NULL) {
        r_printf("enc NULL");
    }

    while (1) {
        rlen = cbuf_read(&__this->pcm_in_cbuf, enc->pcm_frame, frame_len);
        if (rlen == frame_len) {
            break;
        } else if (rlen == 0) {
            /*esco编码读不到数,返回0*/
            //putchar('&');
            return 0;
        } else {
            /*通话结束，aec已经释放*/
            printf("audio_enc end:%d\n", rlen);
            rlen = 0;
            break;
        }
    }

    *frame = enc->pcm_frame;
    return rlen;
}
static void adapter_enc_esco_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input esco_enc_input = {
    .fget = adapter_enc_esco_pcm_get,
    .fput = adapter_enc_esco_pcm_put,
};

static int adapter_enc_esco_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}

static int adapter_enc_esco_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    //putchar('O');
    lmp_private_send_esco_packet(NULL, frame, len);
    return len;
}

const static struct audio_enc_handler esco_enc_handler = {
    .enc_probe = adapter_enc_esco_probe_handler,
    .enc_output = adapter_enc_esco_output_handler,
};

static void adapter_enc_esco_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_ENC_EVENT_END:
        puts("AUDIO_ENC_EVENT_END\n");
        break;
    }
}

static int adapter_enc_esco_data_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    u32 wlen = 0;
    if (__this) {
        pcm_dual_to_single(in->data, in->data, in->data_len);
        wlen = cbuf_write(&__this->pcm_in_cbuf, in->data, in->data_len / 2);
        if (wlen == in->data_len / 2) {
            //putchar('H');
            //printf("%d\n", in->data_len/2);
        } else {
            //putchar('K');
        }
        audio_encoder_resume(&__this->encoder);
    }

    return in->data_len;
}
static void adapter_enc_esco_data_process_len(struct audio_stream_entry *entry,  int len)
{

}
static int adapter_enc_esco_open(struct adapter_encoder_fmt *encoder_fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        printf("adapter_enc_esco_open fail\n");
        return 0;
    }

    ASSERT(media_parm);

    int err;
    struct audio_fmt fmt;

    u32 esco_param = *(u32 *)media_parm->priv_data;
    int esco_len = esco_param >> 16;
    int codec_type = esco_param & 0x000000ff;

    g_f_printf("%s, type=%d,len=%d\n", __FUNCTION__, codec_type, esco_len);

    fmt.channel = 1;
    fmt.frame_len = esco_len;
    if (codec_type == 3) {
        g_printf("AUDIO_CODING_MSBC \n");
        fmt.sample_rate = 16000;
        fmt.coding_type = AUDIO_CODING_MSBC;
        clock_add(ENC_MSBC_CLK);
    } else if (codec_type == 2) {
        g_printf("AUDIO_CODING_CVSD \n");
        fmt.sample_rate = 8000;
        fmt.coding_type = AUDIO_CODING_CVSD;
        clock_add(ENC_CVSD_CLK);
    } else {
        /*Unsupoport eSCO Air Mode*/
        g_f_printf("adapter_enc_esco_open parm err!!\n");
        return -1;
    }

    struct esco_enc_hdl *hdl = NULL;
    hdl = zalloc(sizeof(struct esco_enc_hdl));

    cbuf_init(&hdl->pcm_in_cbuf, hdl->in_cbuf_buf,  ESCO_ENC_IN_CBUF_SIZE);

    audio_encoder_open(&hdl->encoder, &esco_enc_input, adapter_encoder_get_task_handle());
    audio_encoder_set_handler(&hdl->encoder, &esco_enc_handler);
    audio_encoder_set_fmt(&hdl->encoder, &fmt);
    audio_encoder_set_event_handler(&hdl->encoder, adapter_enc_esco_event_handler, 0);
    audio_encoder_set_output_buffs(&hdl->encoder, hdl->output_frame,
                                   sizeof(hdl->output_frame), 1);

    if (!esco_enc->encoder.enc_priv) {
        log_e("encoder err, maybe coding(0x%x) disable \n", fmt.coding_type);
        err = -EINVAL;
        goto __err;
    }

    audio_encoder_start(&hdl->encoder);

    hdl->entry.data_process_len = adapter_enc_esco_data_process_len;
    hdl->entry.data_handler = adapter_enc_esco_data_handler;

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    clock_set_cur();

    g_printf("adapter_enc_esco_open ok~~~~~~~\n");
    return 0;

__err:
    audio_encoder_close(&esco_enc->encoder);

    local_irq_disable();
    free(esco_enc);
    esco_enc = NULL;
    local_irq_enable();

    return err;
}

static void adapter_enc_esco_close(void)
{
    g_printf("adapter_enc_esco_close\n");
    if (!__this) {
        return;
    }

    if (__this->encoder.fmt.coding_type == AUDIO_CODING_MSBC) {
        clock_remove(ENC_MSBC_CLK);
    } else if (__this->encoder.fmt.coding_type == AUDIO_CODING_CVSD) {
        clock_remove(ENC_CVSD_CLK);
    }

    audio_encoder_close(&__this->encoder);

    local_irq_disable();
    free(__this);
    __this = NULL;
    local_irq_enable();

    clock_set_cur();
}

static struct audio_stream_entry *adapter_enc_ecso_get_stream_entry(void)
{
    if (__this) {
        return &__this->entry;
    }
    return NULL;
}

REGISTER_ADAPTER_ENCODER(adapter_enc_esco) = {
    .enc_type     = ADAPTER_ENC_TYPE_ESCO,
    .open   = adapter_enc_esco_open,
    .close  = adapter_enc_esco_close,
    .get_stream_entry = adapter_enc_ecso_get_stream_entry,
};

