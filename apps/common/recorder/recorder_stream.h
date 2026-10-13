#ifndef __RECORDER_STREAM_H__
#define __RECORDER_STREAM_H__

#include "generic/typedef.h"
#include "stream_noise_reduction.h"
#include "application/audio_energy_detect.h"


struct __recorder_stream_parm {
    u16 sample_rate;
    u8	ch_num;
    u8  mute;
    //降噪参数
    struct __noise_reduction_parm 	*reduction_parm;
    //能量检测参数
    audio_energy_detect_param *energy_parm;
    //能量检测状态处理
    void (*energy_mute_handler)(u8 mute_status);
    //能量检测mute状态时数据是否送至录音
    u8  energy_mute_data;

    void *data_handler_priv;
    void (*data_handler)(void *data_handler_priv, void *data, u16 len);
};


struct __recorder_stream;

int recorder_stream_pcm_data_write(struct __recorder_stream *stream, u8 *data, u16 len);
void recorder_stream_dec_close(struct __recorder_stream **hdl);
struct __recorder_stream *recorder_stream_dec_open(struct __recorder_stream_parm *parm);

extern int recorder_stream_energy_get(struct __recorder_stream *stream, u8 ch);

#endif//__RECORDER_STREAM_H__

