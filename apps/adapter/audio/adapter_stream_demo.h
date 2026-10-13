#ifndef __ADAPTER_STREAM_DEMO_H__
#define __ADAPTER_STREAM_DEMO_H__

#include "app_config.h"
#include "generic/typedef.h"
#include "media/includes.h"

struct __stream_demo_hdl {
    struct __stream_entry  *stream;

    OS_MUTEX mutex; /*!< 混合器互斥量 */
    OS_SEM                  w_sem;
    OS_SEM                  sem;
    volatile u8             status;
    volatile u8             r_busy;
    volatile u8             w_busy;
    volatile u8             fill_flag;
    int                     data_len;
    u8                      *data;


    u16 skip_counter;
    u8 *in_buf;
    u8 *out_buf;
    int frame_buf_size;
    u8 *frame_buf;
    cbuffer_t in_cbuf;
    cbuffer_t out_cbuf;

};

struct __stream_demo_hdl *adapter_stream_demo_open(void);
void adapter_stream_demo_close(struct __stream_demo_hdl **p);

#endif//__ADAPTER_STREAM_DEMO_H__


