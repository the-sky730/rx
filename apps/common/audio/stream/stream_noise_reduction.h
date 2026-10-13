#ifndef __STREAM_NOISE_REDUCTION_H__
#define __STREAM_NOISE_REDUCTION_H__

#include "system/includes.h"
#include "media/includes.h"

#ifdef __cplusplus
extern "C"
{
#endif


int RecordingPenSystem_GetMiniFrameSize(int sampleRate);
int RecordingPenSystem_QueryProcessDelay(int sampleRate);
int RecordingPenSystem_QueryBufSize(int sampleRate);
int RecordingPenSystem_QueryTempBufSize(int sampleRate);
void RecordingPenSystem_Init(void *runbuf,
                             void *tempbuf,
                             float AggressFactor,
                             float minSuppress,
                             int sampleRate,
                             float init_noise_lvl);
void RecordingPenSystem_Process(void *runbuf, void *tempbuf, short *mic_data, short *output, int nMiniFrame);

#ifdef __cplusplus
}
#endif


struct __noise_reduction_parm {
    float 			AggressFactor;	//噪声动态压制因子
    float 			minSuppress;	//最小压制
    //int 			sampleRate;		//采样率
    float 			init_noise_lvl;	//噪声初始值
};

struct __noise_reduction {
    void *run_buf;						//运行buf
    void *temp_buf;						//临时运行buf
    int sampleRate;

    struct audio_stream_entry entry;	// 音频流入口
    int out_len;
    int process_len;
    u8 run_en;

    u8 remain;

    int frame_buf_offset;
    int frame_buf_size;
    u8 *frame_buf;

    volatile u8 busy;
    volatile u8 status;
    OS_MUTEX 					mutex;	/*!< 混合器互斥量 */

};

//降噪初始化
struct __noise_reduction *noise_reduction_open(struct __noise_reduction_parm *parm, int	sampleRate);
//降噪关闭
void noise_reduction_close(struct __noise_reduction **hdl);

#endif//__STREAM_NOISE_REDUCTION_H__

