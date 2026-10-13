#include "adapter_wireless_enc.h"
#include "adapter_encoder.h"
#include "media/sbc_enc.h"
#include "key_event_deal.h"
#include "adapter_wireless_command.h"


//#if (WIRELESS_CODING_FRAME_LEN == 50)
//#define WIRELESS_ENC_IN_SIZE			(320*2)//(480 *2)
//#else
//#define WIRELESS_ENC_IN_SIZE			(480)
//#endif

#if (WIRELESS_CODING_SAMPLERATE == 44100)
#define WIRELESS_ENC_IN_SIZE			(2*2*(48000*WIRELESS_CODING_FRAME_LEN/10000))
#else
#define WIRELESS_ENC_IN_SIZE			(2*2*(WIRELESS_CODING_SAMPLERATE*WIRELESS_CODING_FRAME_LEN/10000))
#endif


#define WIRELESS_ENC_OUT_SIZE			256
#define WIRELESS_FRAME_SUM          	(1)/*一包多少帧*/
#define WIRELESS_FRAME_COMMAND_MAX     	(16)/*附带命令最大长度*/
//#define WIRELESS_PACKET_HEADER_LEN  	(2 + 1)/*包头Max Length*/
#define WIRELESS_PACKET_REPEAT_SUM  	(1)/*包重发次数*/
#define WIRELESS_ENC_OUT_BUF_SIZE		(WIRELESS_ENC_OUT_SIZE * WIRELESS_FRAME_SUM)
#define WIRELESS_ENC_PCM_BUF_LEN		(WIRELESS_ENC_IN_SIZE * 4)

#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)
#define TRANSFERING_TIMER_ENABLE        1
#else
#define TRANSFERING_TIMER_ENABLE        0
#endif
#define TRANSFERING_TIMER_UNIT          (100)



struct __adapter_wireless_enc {
    struct audio_encoder 	        encoder;
    cbuffer_t 			 	        cbuf_pcm;
    s16 				 	        pcm_buf[WIRELESS_ENC_PCM_BUF_LEN / 2];
    s16 				 	        output_frame[WIRELESS_ENC_OUT_SIZE / 2];		// 编码输出帧
    int 				 	        input_frame[WIRELESS_ENC_IN_SIZE / 4];
    u8  				 	        *out_buf;//[WIRELESS_PACKET_HEADER_LEN + WIRELESS_ENC_OUT_BUF_SIZE];	// sbc帧缓存

    u16 				 	        frame_in_size;	// 帧输入大小
    u8  				 	        frame_cnt;		// 帧打包计数
    u16 				 	        frame_len;		// 帧长
#if (WIRELESS_TRANSFER_EXPAND_EN)
    u8 				 	            tx_seqn;		// 缩小seqn， 减少带宽占用
#else
    u16 				 	        tx_seqn;		// 传输计数,保留u16兼容之前的版本SDK
#endif//WIRELESS_TRANSFER_EXPAND_EN
    volatile u8  		 	        start; 			// 启动标志

    volatile u8 			        command;		//语音数据附带命令
    volatile u8 			        resume_cnt;
    volatile struct wl_expand	    expand_data;    //数据扩展
#if (TRANSFERING_TIMER_ENABLE)
    u16                             transfering_timer;//数据传输检查定时器
#endif
};

struct adapter_wireless_enc_parm {
    struct audio_stream_entry audio_entry;
    void (*command_callback)(u8 *buf, u8 len);
    u8 command_len;
    u8 channel_type;
};
static struct adapter_wireless_enc_parm  g_wireless_enc_parm = { 0 };
static struct __adapter_wireless_enc *wireless_enc = NULL;

static void adapter_wireless_enc_packet_pack(struct __adapter_wireless_enc *hdl, u16 len);

#if (TRANSFERING_TIMER_ENABLE)
void transfering_timer_check(void *p)
{
    if (wireless_enc && wireless_enc->start) {
        //printf("send head only!!!!\n");
        adapter_wireless_enc_packet_pack(wireless_enc, 0);
        adapter_wireless_enc_push(wireless_enc, wireless_enc->out_buf, wireless_enc->frame_len);
    }
}
#endif

static int adapter_wireless_enc_push(struct __adapter_wireless_enc *hdl, void *buf, int len)
{
    if (!hdl->start) {
        return 0;
    }

//    printf("len = %d", len);

    int ret = 0;
    extern int wireless_mic_odev_ble_send(void *priv, u8 * buf, u16 len);
    extern int wireless_mic_idev_ble_send(void *priv, u8 * buf, u16 len);
    for (int i = 0; i < WIRELESS_PACKET_REPEAT_SUM; i++) {
#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)
        ret = wireless_mic_idev_ble_send(NULL, buf, len);
#else
        ret = wireless_mic_odev_ble_send(NULL, buf, len);
#endif
        if (ret) {
            putchar('E');
        } else {
            //putchar('O');
        }
    }
    return len;
}

static int adapter_wireless_enc_write_pcm(struct __adapter_wireless_enc *hdl, void *buf, int len)
{
    if (!hdl->start) {
        return 0;
    }
    int wlen = cbuf_write(&hdl->cbuf_pcm, buf, len);
#if (WIRELESS_MIC_ADC_RESUME_CODING_EN == 0)
    if (cbuf_get_data_len(&hdl->cbuf_pcm) >= (hdl->frame_in_size)) {
        audio_encoder_resume(&hdl->encoder);
    }
#endif
    return wlen;
}

void adapter_wireless_enc_resume(void)
{
#if (WIRELESS_MIC_ADC_RESUME_CODING_EN)
    local_irq_disable();
    if (wireless_enc && wireless_enc->start) {
        wireless_enc->resume_cnt = 1;
        //putchar('r');
        audio_encoder_resume(&wireless_enc->encoder);
    }
    local_irq_enable();
#endif
}


static void adapter_wireless_enc_packet_pack(struct __adapter_wireless_enc *hdl, u16 len)
{
    //将包的序列号写到头部
    hdl->tx_seqn++;
#if (WIRELESS_TRANSFER_EXPAND_EN)
    hdl->out_buf[0] = hdl->tx_seqn;
    memcpy(&hdl->out_buf[1], (u8 *)&hdl->expand_data, 1);
    //hdl->out_buf[1] = (u8)hdl->expand_data;
#else
    hdl->out_buf[0] = hdl->tx_seqn >> 8;
    hdl->out_buf[1] = hdl->tx_seqn & 0xff;
#endif//WIRELESS_TRANSFER_EXPAND_EN
    if (g_wireless_enc_parm.command_callback) {
        u8 tmp_buf[WIRELESS_FRAME_COMMAND_MAX] = {0};
        g_wireless_enc_parm.command_callback(tmp_buf, g_wireless_enc_parm.command_len);
        memcpy(&hdl->out_buf[2], tmp_buf, g_wireless_enc_parm.command_len);
    } else {
        if (g_wireless_enc_parm.command_len) {
            hdl->out_buf[2] = hdl->command;//命令
        }
    }
    hdl->command = 0;//命令传递之后， 将命令清零防止重复发
    hdl->frame_len = 2 + hdl->expand_data.ex_len + g_wireless_enc_parm.command_len;//WIRELESS_PACKET_HEADER_LEN;
}

static int adapter_wireless_enc_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    struct __adapter_wireless_enc *hdl = container_of(encoder, struct __adapter_wireless_enc, encoder);
    int pcm_len = 0;
    //printf("frame_len = %d   ", frame_len);
#if (WIRELESS_MIC_ADC_RESUME_CODING_EN)
    local_irq_disable();
    if (hdl->resume_cnt == 0) {
        local_irq_enable();
        return 0;
    }
    hdl->resume_cnt = 0;
    local_irq_enable();
#endif
    //putchar('R');

    if (frame_len > hdl->frame_in_size) {
        printf("frame_len over limit !!!!, %d\n", frame_len);
        return 0;
    }

    //printf("frame_len = %d\n", frame_len);
    pcm_len = cbuf_read(&hdl->cbuf_pcm, hdl->input_frame, frame_len);
    if (pcm_len != frame_len) {
        //putchar('x');
    } else {
        /* put_u16hex(frame_len); */
    }

    /* put_u16hex(frame_len); */
    /* put_u16hex(pcm_len); */

    *frame = (s16 *)hdl->input_frame;
    return pcm_len;
}

static void adapter_wireless_enc_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input adapter_wireless_enc_input = {
    .fget = adapter_wireless_enc_pcm_get,
    .fput = adapter_wireless_enc_pcm_put,
};


static int adapter_wireless_enc_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}

static int adapter_wireless_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    struct __adapter_wireless_enc *enc = container_of(encoder, struct __adapter_wireless_enc, encoder);

#if (TRANSFERING_TIMER_ENABLE)
    if (enc->transfering_timer) {
        sys_timer_modify(enc->transfering_timer, TRANSFERING_TIMER_UNIT);
    }
#endif

    //检查上次是否还有数据没有发出
    if (enc->frame_cnt >= WIRELESS_FRAME_SUM) {
        int wlen = adapter_wireless_enc_push(enc, enc->out_buf, enc->frame_len);
        if (wlen != enc->frame_len) {
            return 0;
        }
        enc->frame_cnt = 0;
    }
    if (enc->frame_cnt == 0) {
        //重新打包， 加包头
        adapter_wireless_enc_packet_pack(enc, len);
        //enc->frame_len = 2 + g_wireless_enc_parm.command_len;//WIRELESS_PACKET_HEADER_LEN;
    }
    //拼接数据内容
    memcpy(&enc->out_buf[enc->frame_len], frame, len);
    enc->frame_len += len;
    enc->frame_cnt ++;
    //检查是否达到发送条件
    if (enc->frame_cnt >= WIRELESS_FRAME_SUM) {
        int wlen = adapter_wireless_enc_push(enc, enc->out_buf, enc->frame_len);
        if (wlen == enc->frame_len) {
            enc->frame_cnt = 0;
            //audio_encoder_resume(&enc->encoder);
        }
    }
    return len;
}

const static struct audio_enc_handler adapter_wireless_enc_handler = {
    .enc_probe = adapter_wireless_enc_probe_handler,
    .enc_output = adapter_wireless_enc_output_handler,
};


static void adapter_wireless_enc_stop(struct __adapter_wireless_enc *hdl)
{
    if (hdl->start) {
        hdl->start = 0;
        audio_encoder_close(&hdl->encoder);
    }
}

void adapter_wireless_enc_close(void)
{
    if (!wireless_enc) {
        return ;
    }



    adapter_wireless_enc_stop(wireless_enc);

#if (TRANSFERING_TIMER_ENABLE)
    if (wireless_enc->transfering_timer) {
        sys_timer_del(wireless_enc->transfering_timer);
        wireless_enc->transfering_timer = 0;
    }
#endif

    local_irq_disable();
    if (wireless_enc->out_buf) {
        free(wireless_enc->out_buf);
    }
    free(wireless_enc);
    wireless_enc = NULL;
    local_irq_enable();
}

#if 1
int adapter_wireless_enc_open_base(struct audio_fmt *fmt, u16 frame_in_size)
{
    if (fmt == NULL) {
        log_e("wireless_enc_open parm error\n");
        return -1;
    }
    struct __adapter_wireless_enc *hdl = zalloc(sizeof(struct __adapter_wireless_enc));
    if (!hdl) {
        return -1;
    }

    if (g_wireless_enc_parm.command_len > WIRELESS_FRAME_COMMAND_MAX) {
        g_wireless_enc_parm.command_len = WIRELESS_FRAME_COMMAND_MAX;
        log_i("command_len overlimit !!\n");
    }

    hdl->expand_data.ex_len = 0;//暂时没有数据要扩展
    hdl->expand_data.channel = g_wireless_enc_parm.channel_type;//通道类型,2t1用于通道区分做立体声切换用
    hdl->command = 0;//默认没有命令传输

    //u32 out_buf_len = 2 + g_wireless_enc_parm.command_len + WIRELESS_ENC_OUT_SIZE;
    u32 out_buf_len = 2 + hdl->expand_data.ex_len + g_wireless_enc_parm.command_len + WIRELESS_ENC_OUT_SIZE;
    hdl->out_buf = zalloc(out_buf_len);
    if (hdl->out_buf == NULL) {
        free(hdl);
        return -1;
    }

    hdl->frame_in_size = frame_in_size;


#if (TRANSFERING_TIMER_ENABLE)
    hdl->transfering_timer = sys_timer_add(NULL, transfering_timer_check, TRANSFERING_TIMER_UNIT);
#endif

    // 变量初始化
    cbuf_init(&hdl->cbuf_pcm, hdl->pcm_buf, sizeof(hdl->pcm_buf));

    audio_encoder_open(&hdl->encoder, &adapter_wireless_enc_input, adapter_encoder_get_task_handle());
    audio_encoder_set_handler(&hdl->encoder, &adapter_wireless_enc_handler);
    audio_encoder_set_fmt(&hdl->encoder, fmt);
    audio_encoder_set_output_buffs(&hdl->encoder, hdl->output_frame, sizeof(hdl->output_frame), 1);

    audio_encoder_start(&hdl->encoder);

    hdl->start = 1;

    local_irq_disable();
    wireless_enc = hdl;
    local_irq_enable();

    return 0;
}

#endif

int adapter_wireless_enc_write(void *buf, int len)
{
    if (!wireless_enc) {
        return 0;
    }
    int wlen = adapter_wireless_enc_write_pcm(wireless_enc, buf, len);
    if (wlen != len) {
        putchar('L');
    }
    return wlen;
}





static int adapter_wireless_enc_data_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    int wlen = adapter_wireless_enc_write(in->data, in->data_len);
    if (wlen == in->data_len) {
        //putchar('W');
    } else {
        //putchar('M');
    }
    return in->data_len;
}

static void adapter_wireless_enc_data_process_len(struct audio_stream_entry *entry,  int len)
{
}
static int adapter_wireless_enc_open(struct adapter_encoder_fmt *encoder_fmt, struct adapter_media_parm *media_parm)
{
    struct audio_fmt fmt = {0};
    u16 frame_in_size = 0;
    g_wireless_enc_parm.command_callback = encoder_fmt->command_callback;
    g_wireless_enc_parm.command_len = encoder_fmt->command_len;
    if (encoder_fmt->channel_type) {
        g_wireless_enc_parm.channel_type = *(encoder_fmt->channel_type);
    } else {
        g_wireless_enc_parm.channel_type = 0;
    }
    fmt.coding_type = AUDIO_CODING_JLA;
    fmt.sample_rate = WIRELESS_CODING_SAMPLERATE;
    fmt.channel = WIRELESS_CODING_CHANNEL_NUM;
    fmt.bit_rate = WIRELESS_CODING_BIT_RATE;
    fmt.frame_len = WIRELESS_CODING_FRAME_LEN;
    if (fmt.channel == 1) {
        frame_in_size = WIRELESS_ENC_IN_SIZE / 2;
    } else {
        frame_in_size = WIRELESS_ENC_IN_SIZE;
    }

#if !WIRELESS_ECHO_EMITTER
    app_task_put_key_msg(KEY_WIRELESS_MIC_ECHO_SYNC, 0);
#endif
    g_wireless_enc_parm.audio_entry.data_process_len = adapter_wireless_enc_data_process_len;
    g_wireless_enc_parm.audio_entry.data_handler = adapter_wireless_enc_data_handler;

    adapter_wireless_enc_open_base(&fmt, frame_in_size);
    return 0;
}

static struct audio_stream_entry *adapter_wireless_enc_get_stream_entry(void)
{
    return &g_wireless_enc_parm.audio_entry;
}

void adapter_wireless_enc_command_send(u8 command)
{
    if (!wireless_enc) {
        return ;
    }
    wireless_enc->command = command;
}


REGISTER_ADAPTER_ENCODER(adapter_enc_wireless) = {
    .enc_type     = ADAPTER_ENC_TYPE_WIRELESS,
    .open   = adapter_wireless_enc_open,
    .close  = adapter_wireless_enc_close,
    .get_stream_entry = adapter_wireless_enc_get_stream_entry,
};

