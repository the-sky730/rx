#ifndef __ADAPTER_ENCODER_H__
#define __ADAPTER_ENCODER_H__

#include "adapter_audio_stream.h"
#include "adapter_media.h"

enum adapter_enc_type {
    ADAPTER_ENC_TYPE_SBC = 0x0,
    ADAPTER_ENC_TYPE_ESCO,
    ADAPTER_ENC_TYPE_UAC_MIC,
    ADAPTER_ENC_TYPE_WIRELESS,

    //..
    ADAPTER_ENC_TYPE_MAX,
};



struct adapter_encoder_fmt {
    u8 enc_type;
    u8 *channel_type;//这里定义为指针类型， 目的是能够动态获取通道类型
    u8 command_len;
    void (*command_callback)(u8 *data, u8 len);
};

struct adapter_encoder {
    /*编码选择, 参考enum adapter_enc_type*/
    u8  enc_type;
    int (*open)(struct adapter_encoder_fmt *fmt, struct adapter_media_parm *media_parm);
    void (*close)(void);
    //其他操作
    struct audio_stream_entry *(*get_stream_entry)(void);
};

void adapter_encoder_init(void);
struct audio_encoder_task *adapter_encoder_get_task_handle(void);
struct adapter_encoder *adapter_encoder_open(struct adapter_encoder_fmt *fmt, struct adapter_media_parm *media_parm);
void adapter_encoder_close(struct adapter_encoder *enc);
void adapter_encoder_config(struct adapter_encoder *enc, int opt, void *parm);
struct audio_stream_entry *adapter_encoder_get_stream_entry(struct adapter_encoder *enc);


#define REGISTER_ADAPTER_ENCODER(ops) \
        const struct adapter_encoder ops sec(.adapter_encoder)

extern const struct adapter_encoder adapter_encoder_begin[];
extern const struct adapter_encoder adapter_encoder_end[];

#define list_for_each_adapter_encoder(p) \
    for (p = adapter_encoder_begin; p < adapter_encoder_end; p++)

#endif//__ADAPTER_ENCODER_H__


