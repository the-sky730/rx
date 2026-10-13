#include "adapter_enc_uac_mic.h"
#include "adapter_encoder.h"
#include "uac_stream.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "app_config.h"


#if (TCFG_PC_ENABLE && USB_DEVICE_CLASS_CONFIG & MIC_CLASS)
#if APP_MAIN == APP_WIRELESS_DUPLEX
#define PCM_ENC2USB_OUTBUF_LEN			(25 * ((MIC_AUDIO_RATE/1000)*MIC_CHANNEL*2))
#else
#define PCM_ENC2USB_OUTBUF_LEN			(30 * ((MIC_AUDIO_RATE/1000)*MIC_CHANNEL*2))
#endif

struct __adapter_enc_uac_mic {
    struct audio_stream_entry entry;	// 音频流入口
    cbuffer_t output_cbuf;
    u8 *output_buf;
    u8 tx_chs;
    volatile u8 stream_chs;
    u8 data_ok;							//mic数据等到一定长度再开始发送
    u8 drop_data; 						//用来丢掉刚开mic的前几帧数据
    u8 status;
    volatile u8 wr_busy;
    volatile u8 tx_busy;
    //u32 w_data_max;
};

static struct __adapter_enc_uac_mic *uac_mic = NULL;
#define __this uac_mic

u32 adapter_usb_mic_get_output_total_size(void *priv)
{
    return PCM_ENC2USB_OUTBUF_LEN;
}

u32 adapter_usb_mic_get_output_size(void *priv)
{
    if (__this)	{
        return cbuf_get_data_size(&__this->output_cbuf);
    }
    return 0;
}


static int adapter_enc_uac_mic_tx_handler(int event, void *data, int len)
{
    //putchar('T');
    int i = 0;
    int r_len = 0;
    u8 ch = 0;
    u8 double_read = 0;
    if (__this == NULL) {
        memset(data, 0, len);
        return len;
    }
    if (__this->status == 0) {
        memset(data, 0, len);
        return len;
    }
    __this->tx_busy = 1;
#if 0
    if ((__this->tx_chs == 1) && (__this->stream_chs == 2)) {
        putchar('1');
        r_len = len << 1;
        if (r_len > cbuf_get_data_size(&__this->output_cbuf)) {
            putchar('#');
            memset(data, 0, len);
            __this->tx_busy = 0;
            return len;
        }
        int temp_len0, temp_len1;
        u8 *ptr = cbuf_read_alloc(&__this->output_cbuf, &temp_len0);
        if (temp_len0 >= r_len) {
            pcm_dual_to_single(data, ptr, r_len);
            cbuf_read_updata(&__this->output_cbuf, r_len);
        } else {
            pcm_dual_to_single(data, ptr, temp_len0);
            cbuf_read_updata(&__this->output_cbuf, temp_len0);

            ptr = cbuf_read_alloc(&__this->output_cbuf, &temp_len1);
            pcm_dual_to_single(data + temp_len0 / 2, ptr, r_len - temp_len0);
            cbuf_read_updata(&__this->output_cbuf, r_len - temp_len0);
        }
    } else if ((__this->tx_chs == 2) && (__this->stream_chs == 1)) {
        putchar('2');
        r_len = len >> 1;
        if (r_len > cbuf_get_data_size(&__this->output_cbuf)) {
            memset(data, 0, len);
            __this->tx_busy = 0;
            return len;
        }
        u8 *temp_buf = (u8 *)data;
        cbuf_read(&__this->output_cbuf, temp_buf + r_len, r_len);
        pcm_single_to_dual(temp_buf, temp_buf + r_len, r_len);
    } else
#endif
    {
        //putchar('3');
        r_len = len;
        if (r_len > cbuf_get_data_size(&__this->output_cbuf)) {
            memset(data, 0, len);
            __this->tx_busy = 0;
            return len;
        }
        len = cbuf_read(&__this->output_cbuf, data, len);
    }
    __this->tx_busy = 0;
    return len;
}


static int adapter_enc_uac_mic_write(struct audio_data_frame *in)
{
    int wlen = in->data_len;
    //putchar('5');
    if (__this && __this->status) {
        //putchar('6');
        __this->wr_busy = 1;
        __this->stream_chs = in->channel;;
        wlen = cbuf_write(&__this->output_cbuf, in->data, in->data_len);
        // if (__this->w_data_max < __this->output_cbuf.data_len) {
        //     __this->w_data_max = __this->output_cbuf.data_len;
        //     /* game_printf("usb_mic_max:%d", __this->w_data_max); */
        // }
        if (wlen != in->data_len) {
            //putchar('L');
            //game_printf("__this write full:%d-%d\n", wlen, len);
        }
        __this->wr_busy = 0;
    }
    return wlen;
}


extern u8 uac_get_mic_stream_status(void);
static int adapter_enc_uac_mic_data_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    if (uac_get_mic_stream_status() == 0) {
        return in->data_len;
    }
    int wlen = adapter_enc_uac_mic_write(in);
    if (wlen == in->data_len) {
        //putchar('W');
    } else {
        //putchar('M');
    }
    return in->data_len;
}

static void adapter_enc_uac_mic_data_process_len(struct audio_stream_entry *entry,  int len)
{
}
static int adapter_enc_uac_mic_open(struct adapter_encoder_fmt *encoder_fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        g_printf(" encoder_uac_mic_open aready, fail\n");
        return -1;
    }
    g_printf(" encoder_uac_mic_open !!!!!!!!!!!!!!!!!!\n");
    struct __adapter_enc_uac_mic *hdl = NULL;
    hdl = zalloc(sizeof(struct __adapter_enc_uac_mic));
    if (!hdl) {
        return -EFAULT;
    }

    hdl->output_buf = zalloc(PCM_ENC2USB_OUTBUF_LEN);
    if (!hdl->output_buf) {
        g_f_printf("output_buf NULL\n");
        free(hdl);
        return -EFAULT;
    }

    u32 sample_rate = media_parm->sample_rate;
    hdl->tx_chs = media_parm->ch_num;
    g_printf("usb mic sr:%d ch:%d\n", sample_rate, hdl->tx_chs);

    hdl->drop_data = 2; //用来丢掉前几帧数据
    cbuf_init(&hdl->output_cbuf, hdl->output_buf, PCM_ENC2USB_OUTBUF_LEN);
    set_uac_mic_tx_handler(NULL, adapter_enc_uac_mic_tx_handler);

    hdl->entry.data_process_len = adapter_enc_uac_mic_data_process_len;
    hdl->entry.data_handler = adapter_enc_uac_mic_data_handler;

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    __this->status = 1;

    return 0;
}
static void adapter_enc_uac_mic_close(void)
{
    printf("adapter_enc_uac_mic_close!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    if (__this) {
        __this->status = 0;
        while (__this->tx_busy) {
            os_time_dly(1);
        }
        while (__this->wr_busy) {
            os_time_dly(1);
        }
        printf("adapter_enc_uac_mic_close wait ok!!\n");
        local_irq_disable();
        if (__this->output_buf) {
            free(__this->output_buf);
        }
        free(__this);
        __this = NULL;
        local_irq_enable();
    }
}

static struct audio_stream_entry *adapter_mic_get_stream_entry(void)
{
    if (__this) {
        return &__this->entry;
    }
    return NULL;
}

REGISTER_ADAPTER_ENCODER(enc_uac_mic) = {
    .enc_type = ADAPTER_ENC_TYPE_UAC_MIC,
    .open = adapter_enc_uac_mic_open,
    .close = adapter_enc_uac_mic_close,
    .get_stream_entry = adapter_mic_get_stream_entry,
};

#endif

