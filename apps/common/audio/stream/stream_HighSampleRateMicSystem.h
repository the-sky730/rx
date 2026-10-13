#ifndef __STREAM_HIGHSAMPLERATEMICSYSTEM_H__
#define __STREAM_HIGHSAMPLERATEMICSYSTEM_H__

#include "system/includes.h"
#include "media/includes.h"

struct __stream_HighSampleRateSingleMicSystem_parm {
    float 			AggressFactor;	/*!< 噪声动态压制因子 */
    float 			minSuppress;	/*!< 最小压制,建议范围[0.04 - 0.5], 越大降得越少 */
    float 			init_noise_lvl;	/*!< 噪声初始值 */
    u32             frame_len;      /*!< 数据流帧长*/
};

struct __stream_HighSampleRateSingleMicSystem {
    void *run_buf;						//运行buf
    void *temp_buf;						//临时运行buf
    int sampleRate;
    struct audio_stream_entry entry;	// 音频流入口
    int frame_buf_size;
    u8 *frame_buf;
    u8 *in_buf;
    u8 *out_buf;
    u16 skip_counter;
    cbuffer_t in_cbuf;
    cbuffer_t out_cbuf;
    OS_MUTEX mutex;	/*!< 混合器互斥量 */
    OS_SEM sem;
    //OS_SEM r_sem;
    volatile u8 w_busy: 1;
        volatile u8 r_busy: 1;
        volatile u8 busy: 1;
        volatile u8 status: 3;
        volatile u8 fill_flag: 1;

    };

//降噪初始化
    struct __stream_HighSampleRateSingleMicSystem *stream_HighSampleRateSingleMicSystem_open(struct __stream_HighSampleRateSingleMicSystem_parm *parm, int	sampleRate, u8 onoff);
//降噪关闭
    void stream_HighSampleRateSingleMicSystem_close(struct __stream_HighSampleRateSingleMicSystem **hdl);

    void stream_HighSampleRateSingleMicSystem_set_parm(struct __stream_HighSampleRateSingleMicSystem *denoise, struct __stream_HighSampleRateSingleMicSystem_parm *parm);

    void stream_HighSampleRateSingleMicSystem_switch(struct __stream_HighSampleRateSingleMicSystem *denoise, u8 onoff);

#endif//__STREAM_HIGHSAMPLERATEMICSYSTEM_H__

