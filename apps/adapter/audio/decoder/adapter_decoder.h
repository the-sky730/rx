#ifndef __ADAPTER_DECODER_H__
#define __ADAPTER_DECODER_H__

#include "adapter_audio_stream.h"
#include "adapter_media.h"

enum adapter_dec_type {
    ADAPTER_DEC_TYPE_A2DP = 0x0,
    ADAPTER_DEC_TYPE_ESCO,
    ADAPTER_DEC_TYPE_UAC_SPK,
    ADAPTER_DEC_TYPE_MIC,
    ADAPTER_DEC_TYPE_JLA,
    ADAPTER_DEC_TYPE_JLA_MIXER,
    ADAPTER_DEC_TYPE_DUAL_JLA,
    ADAPTER_DEC_TYPE_WIRELESS,

    //..
    ADAPTER_DEC_TYPE_MAX,
};

enum adapter_dec_ioctrl_cmd {
    ADAPTER_DEC_IOCTRL_CMD_LLNS_SET_PARM,
    ADAPTER_DEC_IOCTRL_CMD_LLNS_SWITCH,
    ADAPTER_DEC_IOCTRL_CMD_DENOISE_SET_PARM,
    ADAPTER_DEC_IOCTRL_CMD_DENOISE_SWITCH,
    ADAPTER_DEC_IOCTRL_CMD_REGISTER_COMMAND_CALLBACK,
    ADAPTER_DEC_IOCTRL_CMD_ECHO_EN,
    ADAPTER_DEC_IOCTRL_CMD_GET_RX_PACKET_NUM,
    ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_SWITCH,
    ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_PAUSE,
    ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_PAUSE_GET_STATUS,
    ADAPTER_DEC_IOCTRL_CMD_GET_AUDIO_STREAM_HDL,
    ADAPTER_DEC_IOCTRL_CMD_DAC_GAIN_SET,
};

struct adapter_decoder_fmt {
    u8 dec_type;
    struct adapter_stream_fmt *list;
    u8 list_num;
    u8 dec_output_type;
    int (*cmd_parse_cbk)(void *priv, u8 *data);
};

struct adapter_decoder {
    /*解码选择, 参考enum adapter_dec_type*/
    u8  dec_type;
    int (*open)(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm);
    void (*close)(void);
    void (*set_vol)(u32 channel, u8 vol);
    //其他操作
    int (*ioctrl)(int cmd, int argc, int *argv);
};

void adapter_decoder_init(void);
struct audio_decoder_task *adapter_decoder_get_task_handle(void);

struct adapter_decoder *adapter_decoder_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm);
void adapter_decoder_close(struct adapter_decoder *dec);
void adapter_decoder_set_vol(struct adapter_decoder *dec, u32 channel, u8 vol);
int adapter_decoder_ioctrl(struct adapter_decoder *dec, int cmd, int argc, int *argv);


#define REGISTER_ADAPTER_DECODER(ops) \
        const struct adapter_decoder ops sec(.adapter_decoder)

extern const struct adapter_decoder adapter_decoder_begin[];
extern const struct adapter_decoder adapter_decoder_end[];

#define list_for_each_adapter_decoder(p) \
    for (p = adapter_decoder_begin; p < adapter_decoder_end; p++)

#endif//__ADAPTER_DECODER_H__


