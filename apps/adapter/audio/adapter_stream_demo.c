
#include "adapter_stream_demo.h"
#include "stream_entry.h"


#define DEMO_TASK_NAME   "demo_task1"

#define DEMO_STREAM_ASYNC_EN        1//1:异步处理, 会增加一个处理单元的延时， 0:同步处理， 不增加延时， 但是要求在一个cpu核要可以做完算法处理

static int adapter_stream_demo_data_handle(void *priv, struct audio_data_frame *in)
{
    //putchar('$');
    struct __stream_demo_hdl *hdl = (struct __stream_demo_hdl *)priv;

    if (in->offset) {
        return in->data_len;
    }
#if (DEMO_STREAM_ASYNC_EN)
    u8 ready = 1;
    if (in->data_len && hdl->status) {
        u32 r_len, w_len;
        hdl->r_busy = 1;
        //先写数据流过来的数据到in_cbuf做运算准备
        w_len = cbuf_write(&hdl->in_cbuf, in->data, in->data_len);
        if (w_len != in->data_len) {
            putchar('b');
        }
        os_sem_set(&hdl->sem, 0);
        os_sem_post(&hdl->sem);
        //从out_cbuf读取已经算法处理完的数据
        if (hdl->fill_flag) {
            if (cbuf_get_data_len(&hdl->out_cbuf) >= (hdl->frame_buf_size)) {
                hdl->fill_flag = 0;
            } else {
                memset(in->data, 0, in->data_len);
                ready = 0;
            }
        }

        if (ready) {
            r_len = cbuf_read(&hdl->out_cbuf, in->data, in->data_len);
            if (r_len != in->data_len) {
                putchar('#');
                ready = 0;
            } else {
                if (hdl->skip_counter) {
                    hdl->skip_counter--;
                    memset(in->data, 0, in->data_len);
                }
            }
        }
        //通知算法处理线程， 有新的数据写入， 是否需要做运算
        hdl->r_busy = 0;
    }
#else
    //处理算法
    //拿data的数据来做算法运算
    //memset(in->data, 0, in->data_len);
#endif

    return in->data_len;
}


static void adapter_stream_demo_task(void *priv)
{
    u32 r_len, w_len;
    struct __stream_demo_hdl *hdl = (struct __stream_demo_hdl *)priv;
    ASSERT(hdl);
    while (1) {
        os_sem_pend(&hdl->sem, 0);
        if (hdl->status) {
            hdl->w_busy = 1;
            r_len = cbuf_read(&hdl->in_cbuf, hdl->frame_buf, hdl->frame_buf_size);
            if (r_len == hdl->frame_buf_size) {
                if (hdl->status) {
#if 1
                    os_mutex_pend(&hdl->mutex, 0);
                    //处理算法
                    //拿data的数据来做算法运算
                    //memset(hdl->data, 0, hdl->data_len);
                    os_mutex_post(&hdl->mutex);
#endif
                }
//                putchar('W');
                w_len = cbuf_write(&hdl->out_cbuf, hdl->frame_buf, hdl->frame_buf_size);
                if (w_len != hdl->frame_buf_size) {
                    putchar('A');
                } else {
                    //putchar('C');
                }
            } else {
                //putchar('a');
            }
            hdl->w_busy = 0;
        } else {
            break;
        }

    }
    printf("task wait kill!!\n");
    while (1) {
        os_time_dly(10000);
    }
}

#define PROCESS_POINT	WIRELESS_MIC_ADC_POINT_UNIT//480		//一次算法处理的点数

struct __stream_demo_hdl *adapter_stream_demo_open(void)
{
    struct __stream_demo_hdl *hdl = zalloc(sizeof(struct __stream_demo_hdl));
    ASSERT(hdl);

    hdl->stream = stream_entry_open(hdl, adapter_stream_demo_data_handle, 0);
    ASSERT(hdl->stream);

#if (DEMO_STREAM_ASYNC_EN)
    hdl->status = 1;
    hdl->fill_flag = 1;
    hdl->skip_counter = 300;
    os_mutex_create(&hdl->mutex);
    os_sem_create(&hdl->sem, 0);


    hdl->frame_buf_size = PROCESS_POINT;
    hdl->frame_buf_size <<= 1;
    hdl->frame_buf = (s16 *)zalloc(hdl->frame_buf_size);
    ASSERT(hdl->frame_buf);

    hdl->in_buf = zalloc(hdl->frame_buf_size * 2);
    ASSERT(hdl->in_buf);
    hdl->out_buf = zalloc(hdl->frame_buf_size * 2);
    ASSERT(hdl->out_buf);
    cbuf_init(&hdl->in_cbuf, hdl->in_buf, hdl->frame_buf_size * 2);
    cbuf_init(&hdl->out_cbuf, hdl->out_buf, hdl->frame_buf_size * 2);
    int err = task_create(adapter_stream_demo_task, hdl, DEMO_TASK_NAME);

    if (err) {
        stream_entry_close(&hdl->stream);
        if (hdl) {

            if (hdl->frame_buf) {
                free(hdl->frame_buf);
            }
            if (hdl->in_buf) {
                free(hdl->in_buf);
            }
            if (hdl->out_buf) {
                free(hdl->out_buf);
            }
            free(hdl);
            hdl = NULL;
        }
    }
#endif
    printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    return hdl;
}


void adapter_stream_demo_close(struct __stream_demo_hdl **p)
{
    struct __stream_demo_hdl *hdl = *p;
    if (p && hdl) {
#if (DEMO_STREAM_ASYNC_EN)
        hdl->status = 0;
        while (hdl->r_busy) {
            os_time_dly(2);
        }

        while (hdl->w_busy) {
            os_sem_set(&hdl->w_sem, 0);
            os_sem_post(&hdl->w_sem);
            os_time_dly(2);
        }

        printf("%s, wait busy ok\n", __FUNCTION__);

        stream_entry_close(&hdl->stream);


        task_kill(DEMO_TASK_NAME);
        if (hdl->frame_buf) {
            free(hdl->frame_buf);
        }
        if (hdl->in_buf) {
            free(hdl->in_buf);
        }
        if (hdl->out_buf) {
            free(hdl->out_buf);
        }

#else
        stream_entry_close(&hdl->stream);
#endif
        free(hdl);
        *p = NULL;
        mem_stats();
    }
}







