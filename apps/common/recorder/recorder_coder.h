#ifndef __RECORDER_CODER_H__
#define __RECORDER_CODER_H__

#include "generic/typedef.h"
#include "media/includes.h"

struct __recorder_coder;

enum {
    RECORDER_CODER_STATUS_IDLE = 0x0,
    RECORDER_CODER_STATUS_STOP,
    RECORDER_CODER_STATUS_PLAY,
    RECORDER_CODER_STATUS_PAUSE,
};

enum {
    RECORDER_CODER_CALLBACK_EVENT_OUTPUT = 0x0,
    RECORDER_CODER_CALLBACK_EVENT_SET_HEAD,
};



//编码唤醒
void recorder_coder_resume(struct __recorder_coder *coder);
//编码关闭
void recorder_coder_close(struct __recorder_coder **hdl);
//编码开启
struct __recorder_coder *recorder_coder_open(struct audio_fmt *fmt, struct audio_encoder_task *encode_task);
//编码/暂停
int recorder_coder_pp(struct __recorder_coder *coder);
//编码开始
int recorder_coder_start(struct __recorder_coder *coder);
//编码停止
void recorder_coder_stop(struct __recorder_coder *coder);
//设置编码回调
void recorder_coder_set_callback(struct __recorder_coder *coder, int (*callback)(void *priv, int event, int argc, void *argv), void *priv);
//编码器输入写
int recorder_coder_pcm_data_write(struct __recorder_coder *coder, void *buf, int len);
//获取编码状态
int recorder_coder_get_status(struct __recorder_coder *coder);
//获取录音标签
int recorder_coder_get_mark(struct __recorder_coder *coder, u8 *buf, u16 len);
//获取编码时间
int recorder_coder_get_time(struct __recorder_coder *coder);

#endif//__RECORDER_CODER_H__

