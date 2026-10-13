#include "recorder_coder.h"
#include "media/includes.h"
#include "system/includes.h"

#define PCM_BUF_SIZE			(3*1024)


struct __recorder_coder {
    struct audio_encoder 	encoder;
    s16 					output_frame[1152 / 2];        //align 4Bytes
    int 					pcm_frame[64];                 //align 4Bytes
    u8 						*pcm_buf;
    cbuffer_t 				pcm_cbuf;
    volatile u8 			status;
    volatile u32 			lost;

    //回调函数
    void *callback_priv;
    int (*callback)(void *callback_priv, int event, int argc, void *argv);//event查看recorder_encode.h回调事件枚举
};

extern struct audio_encoder_task *encode_task;
extern int audio_encoder_task_open(void);

void recorder_coder_resume(struct __recorder_coder *coder)
{
    if ((coder == NULL) || (coder->status != RECORDER_CODER_STATUS_PLAY)) {
        return ;
    }
    audio_encoder_resume(&coder->encoder);
}

// 写pcm数据
int recorder_coder_pcm_data_write(struct __recorder_coder *coder, void *buf, int len)
{
    int wlen = 0;
    if (coder == NULL) {
        return 0;
    }
    if (coder->status != RECORDER_CODER_STATUS_PLAY) {
        return 0;
    }
    wlen = cbuf_write(&coder->pcm_cbuf, buf, len);
    if (wlen != len) {
        putchar('~');
        coder->lost ++;
    }
    //唤醒编码器
    audio_encoder_resume(&coder->encoder);
    return wlen;
}

// 编码器获取数据
static int recorder_coder_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    struct __recorder_coder *coder = container_of(encoder, struct __recorder_coder, encoder);
    if (coder == NULL) {
        return 0;
    }
    if (coder->status != RECORDER_CODER_STATUS_PLAY) {
        return 0;
    }

    int dlen = cbuf_get_data_len(&coder->pcm_cbuf);
    if (dlen < frame_len) {
        return 0;
    }

    int rlen = cbuf_read(&coder->pcm_cbuf, coder->pcm_frame, frame_len);
    *frame = coder->pcm_frame;
    return rlen;
}

static void recorder_coder_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input recorder_coder_input = {
    .fget =  recorder_coder_pcm_get,
    .fput =  recorder_coder_pcm_put,
};


static int recorder_coder_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}
// 编码器输出
static int recorder_coder_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    struct __recorder_coder *coder = container_of(encoder, struct __recorder_coder, encoder);
    ///将数据回调出去给外部处理
    int wlen = 0;
    if (coder->callback) {
        wlen = coder->callback(coder->callback_priv, RECORDER_CODER_CALLBACK_EVENT_OUTPUT, len, frame);
    }
    return wlen;
}


static int  recorder_coder_close_handler(struct audio_encoder *encoder)
{
    struct __recorder_coder *coder = container_of(encoder, struct __recorder_coder, encoder);
    ///回调到外面处理,更新头部信息
    if (coder->callback) {
        int len = 0;
        u8 *ptr = (u8 *)audio_encoder_ioctrl(encoder, 2, AUDIO_ENCODER_IOCTRL_CMD_GET_HEAD_INFO, &len);
        coder->callback(coder->callback_priv, RECORDER_CODER_CALLBACK_EVENT_SET_HEAD, len, ptr);
    }
    return 0;
}


const static struct audio_enc_handler recorder_coder_handler = {
    .enc_probe =  recorder_coder_probe_handler,
    .enc_output = recorder_coder_output_handler,
    .enc_close =  recorder_coder_close_handler,
};


//编码关闭
void recorder_coder_close(struct __recorder_coder **hdl)
{
    if (hdl == NULL || (*hdl == NULL)) {
        return ;
    }

    struct __recorder_coder *coder = *hdl;
    audio_encoder_close(&coder->encoder);
    if (coder->pcm_buf) {
        free(coder->pcm_buf);
    }
    free(coder);
    *hdl = NULL;
    //audio_encoder_task_close();//add 0831
}

//编码开启
struct __recorder_coder *recorder_coder_open(struct audio_fmt *fmt, struct audio_encoder_task *encode_task)
{
    struct __recorder_coder *coder = zalloc(sizeof(struct __recorder_coder));
    if (coder == NULL) {
        return NULL;
    }
    //输出缓存初始化
    coder->pcm_buf = zalloc(PCM_BUF_SIZE);
    if (coder->pcm_buf == NULL) {
        goto __err;
    }
    cbuf_init(&coder->pcm_cbuf, coder->pcm_buf, PCM_BUF_SIZE);

#if 0
    //创建录音线程
    audio_encoder_task_open();
    //编码初始化
    audio_encoder_open(&coder->encoder, &recorder_coder_input, encode_task);
#else
    audio_encoder_open(&coder->encoder, &recorder_coder_input, encode_task);
#endif
    audio_encoder_set_handler(&coder->encoder, &recorder_coder_handler);
    audio_encoder_set_fmt(&coder->encoder, fmt);
    audio_encoder_set_output_buffs(&coder->encoder, coder->output_frame, sizeof(coder->output_frame), 1);

    coder->status = RECORDER_CODER_STATUS_IDLE;

    if (!coder->encoder.enc_priv) {
        log_e("encoder err, maybe coding(0x%x) disable \n", fmt->coding_type);
        goto __err;
    }

    return coder;
__err:
    recorder_coder_close(&coder);
    return NULL;
}

//编码开始
int recorder_coder_start(struct __recorder_coder *coder)
{
    if (coder == NULL)	{
        return -1;
    }
    audio_encoder_start(&coder->encoder);
    coder->status = RECORDER_CODER_STATUS_PLAY;
    return 0;
}

//编码停止
void recorder_coder_stop(struct __recorder_coder *coder)
{
    if (coder == NULL)	{
        return ;
    }

    coder->status = RECORDER_CODER_STATUS_STOP;
    audio_encoder_resume(&coder->encoder);
}

//编码/暂停
int recorder_coder_pp(struct __recorder_coder *coder)
{
    if (coder == NULL)	{
        return RECORDER_CODER_STATUS_STOP;
    }

    if (coder->status == RECORDER_CODER_STATUS_PLAY) {
        coder->status = RECORDER_CODER_STATUS_PAUSE;
    } else if (coder->status == RECORDER_CODER_STATUS_PAUSE) {
        coder->status = RECORDER_CODER_STATUS_PLAY;
    }

    return coder->status;
}

void recorder_coder_set_callback(struct __recorder_coder *coder, int (*callback)(void *priv, int event, int argc, void *argv), void *priv)
{
    if (coder == NULL)	{
        return ;
    }
    coder->callback_priv = priv;
    coder->callback = callback;
}

//获取编码状态
int recorder_coder_get_status(struct __recorder_coder *coder)
{
    if (coder == NULL)	{
        return RECORDER_CODER_STATUS_STOP;
    }
    return coder->status;
}
//获取录音标签
int recorder_coder_get_mark(struct __recorder_coder *coder, u8 *buf, u16 len)
{
    int ret = 0;
    if (coder == NULL)	{
        return 0;
    }
    if (len < sizeof(int)) {
        return 0;
    }
    int size = len - sizeof(int);
    memcpy(buf, &size, sizeof(int));
    ret = audio_encoder_ioctrl(&coder->encoder, 2, AUDIO_ENCODER_IOCTRL_CMD_GET_TMARK, buf);
    return ret;
}
//获取编码时间
int recorder_coder_get_time(struct __recorder_coder *coder)
{
    if (coder == NULL)	{
        return 0;
    }

    return audio_encoder_ioctrl(&coder->encoder, 1, AUDIO_ENCODER_IOCTRL_CMD_GET_TIME);
}


