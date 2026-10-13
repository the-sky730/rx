#ifndef __STREAM_LLNS_H__
#define __STREAM_LLNS_H__

#include "system/includes.h"
#include "media/includes.h"

struct __stream_llns_parm {
    float gainfloor;
    float suppress_level;
    u32   frame_len;      /*!< 数据流帧长*/
};

struct __stream_llns {
    char *private_buffer;
    char *share_buffer;
    int samplerate;
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
    void *llns_hdl;
    u16 frame_size;//unit:point
    //OS_SEM r_sem;
    volatile u8 w_busy: 1;
        volatile u8 r_busy: 1;
        volatile u8 busy: 1;
        volatile u8 status: 3;
        volatile u8 fill_flag: 1;

    };

//降噪初始化
    struct __stream_llns *stream_llns_open(struct __stream_llns_parm *parm, int	sampleRate, u8 onoff);
//降噪关闭
    void stream_llns_close(struct __stream_llns **hdl);

    void stream_llns_set_parm(struct __stream_llns *llns, struct __stream_llns_parm *parm);

    void stream_llns_switch(struct __stream_llns *llns, u8 onoff);

#endif//__STREAM_LLNS_H__

