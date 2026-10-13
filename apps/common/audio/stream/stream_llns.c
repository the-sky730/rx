#include "stream_llns.h"
#include "audio_llns.h"
#include "app_config.h"
#include "stream_jlsp_llns.h"

#if (WIRELESS_LLNS_ENABLE)


#define DEMO_TASK_NAME   "demo_task1"

#define LLNS_STATUS_STOP     0
#define LLNS_STATUS_START    1
#define LLNS_STATUS_PAUSE    2

const u16 stream_llns_frame_size[3][3] = {
    {16000, 80},
    {32000, 160},
    {48000, 240},
};

static void stream_llns_output_data_process_len(struct audio_stream_entry *entry,  int len)
{
}

static void stream_llns_task(void *p)
{
    u32 r_len, w_len;
    struct __stream_llns *llns = (struct __stream_llns *)p;
    while (1) {
        os_sem_pend(&llns->sem, 0);
        if (llns->status != LLNS_STATUS_STOP) {
            llns->w_busy = 1;
            while (1) {
                r_len = cbuf_read(&llns->in_cbuf, llns->frame_buf, llns->frame_buf_size);
                if (r_len == llns->frame_buf_size) {
                    if (llns->status == LLNS_STATUS_START) {
#if 1
                        os_mutex_pend(&llns->mutex, 0);
                        int outbuf_size = 0;
                        JLSP_llns_process(llns->llns_hdl, llns->frame_buf, llns->frame_buf, &outbuf_size);
                        os_mutex_post(&llns->mutex);
#endif
                    }
                    w_len = cbuf_write(&llns->out_cbuf, llns->frame_buf, llns->frame_buf_size);
                    if (w_len != llns->frame_buf_size) {
                        putchar('A');
                    } else {
                        //putchar('C');
                    }

                    //os_sem_set(&llns->r_sem, 0);
                    //os_sem_post(&llns->r_sem);
                } else {
                    //putchar('a');
                    //如果inbuf还有数据足够做一次算法的话，则继续做，否则break
                    break;
                }
            }
            llns->w_busy = 0;
        } else {
            break;
        }
    }

    printf("task wait kill!!\n");
    while (1) {
        os_time_dly(10000);
    }

}

static int stream_llns_data_handler(struct audio_stream_entry *entry,
                                    struct audio_data_frame *in,
                                    struct audio_data_frame *out)
{
    struct __stream_llns *llns = container_of(entry, struct __stream_llns, entry);
    if (in->offset) {
        out->data = in->data;
        out->data_len = in->data_len;
        out->channel = in->channel;
        out->sample_rate = in->sample_rate;
        return in->data_len;
    }
    u8 ready = 1;
    if (in->data_len && llns->status == LLNS_STATUS_START) {
        u32 r_len, w_len;
        llns->r_busy = 1;
        //先写数据流过来的数据到in_cbuf做运算准备
        w_len = cbuf_write(&llns->in_cbuf, in->data, in->data_len);
        if (w_len != in->data_len) {
            putchar('b');
        }
        os_sem_set(&llns->sem, 0);
        os_sem_post(&llns->sem);
        //从out_cbuf读取已经算法处理完的数据
#if 0
        if (cbuf_get_data_len(&llns->out_cbuf) < in->data_len) {
            //printf("r_len = %d\n", cbuf_get_data_len(&llns->out_cbuf));
            //os_sem_pend(&llns->r_sem, 0);
        }
#endif
        if (llns->fill_flag) {
            if (cbuf_get_data_len(&llns->out_cbuf) >= (llns->frame_buf_size)) {
                llns->fill_flag = 0;
            } else {
                memset(in->data, 0, in->data_len);
                ready = 0;
            }
        }

        if (ready) {
            //putchar('1');
            r_len = cbuf_read(&llns->out_cbuf, in->data, in->data_len);
            //putchar('2');
            if (r_len != in->data_len) {
                putchar('#');
                ready = 0;
            } else {
                //putchar('D');
                if (llns->skip_counter) {
                    llns->skip_counter--;
                    memset(in->data, 0, in->data_len);
                    /* putchar('$'); */
                }
            }
        }
        //通知算法处理线程， 有新的数据写入， 是否需要做运算
        llns->r_busy = 0;
    }

    out->data = in->data;
    out->data_len = in->data_len;
    out->channel = in->channel;
    out->sample_rate = in->sample_rate;



    return in->data_len;
}

void stream_llns_init(struct __stream_llns *llns, struct __stream_llns_parm *llns_parm)
{
    int private_size = 0, share_size = 0;

    JLSP_llns_get_heap_size(&share_size, &private_size, llns->samplerate);
    printf("llns share_size:%d,private_size:%d\n", share_size, private_size);

    llns->private_buffer = (char *)zalloc(private_size);
    ASSERT(llns->private_buffer);
    llns->share_buffer = (char *)zalloc(share_size);
    ASSERT(llns->share_buffer);
    for (int i = 0; i < ARRAY_SIZE(stream_llns_frame_size); i++) {
        if (stream_llns_frame_size[i][0] == llns->samplerate) {
            llns->frame_size = stream_llns_frame_size[i][1];
            printf("llns frame_size:%d\n", llns->frame_size);
            llns->llns_hdl = (void *)JLSP_llns_init(llns->private_buffer, private_size, llns->share_buffer, share_size, llns->samplerate, llns_parm->gainfloor, llns_parm->suppress_level);
        }
    }
    printf("llns init ok\n");

}
struct __stream_llns *stream_llns_open(
    struct __stream_llns_parm *parm,
    int	samplerate,
    u8 onoff)
{

    struct __stream_llns *llns = zalloc(sizeof(struct __stream_llns));
    os_mutex_create(&llns->mutex);
    os_sem_create(&llns->sem, 0);
    //os_sem_create(&llns->r_sem, 0);


    llns->busy = 0;
    llns->status = (onoff ? LLNS_STATUS_START : LLNS_STATUS_PAUSE);
    llns->fill_flag = 1;
    llns->skip_counter = (1500 * 10) / parm->frame_len; //刚启动，mic的数据有异常。主要是解决ADC与算法结合引入的噪声问题
    llns->samplerate = samplerate;
    stream_llns_init(llns, parm);


    llns->entry.data_process_len = stream_llns_output_data_process_len;
    llns->entry.data_handler = stream_llns_data_handler;

    llns->frame_buf_size = (llns->samplerate * 5) / 1000;
    llns->frame_buf_size <<= 1;
    //printf("llns->frame_buf_size = %d\n", llns->frame_buf_size);
    llns->frame_buf = (s16 *)zalloc(llns->frame_buf_size);
    ASSERT(llns->frame_buf);

    llns->in_buf = zalloc(llns->frame_buf_size * 2);
    ASSERT(llns->in_buf);
    llns->out_buf = zalloc(llns->frame_buf_size * 2);
    ASSERT(llns->out_buf);
    cbuf_init(&llns->in_cbuf, llns->in_buf, llns->frame_buf_size * 2);
    cbuf_init(&llns->out_cbuf, llns->out_buf, llns->frame_buf_size * 2);

    //u16 unit_len = (u16)((parm->frame_len * llns->sampleRate / 10000) * 2);


    //llns->start_fill_len = llns->frame_buf_size + llns->frame_buf_size/2;

    //llns->in_start_fill_len = unit_len * (llns->frame_buf_size / unit_len);
    //llns->out_start_fill_len = unit_len * ((llns->frame_buf_size / unit_len) + 1) - llns->frame_buf_size;
    //printf("in_start_fill_len = %d, out_start_fill_len = %d\n", llns->in_start_fill_len, llns->out_start_fill_len);

    memset(llns->frame_buf, 0, llns->frame_buf_size);
    /* cbuf_write(&llns->in_cbuf, llns->frame_buf, 640); */
    /* cbuf_write(&llns->out_cbuf, llns->frame_buf, 192); */
    //cbuf_write(&llns->in_cbuf, llns->frame_buf, llns->in_start_fill_len);
    //cbuf_write(&llns->out_cbuf, llns->frame_buf, llns->frame_buf_size/2);

    int err = task_create(stream_llns_task, llns, DEMO_TASK_NAME);
    if (err) {
        printf("func = %s, %d, fail\n", __FUNCTION__, __LINE__);
        if (llns) {
            if (llns->private_buffer) {
                free(llns->private_buffer);
            }
            if (llns->share_buffer) {
                free(llns->share_buffer);
            }
            if (llns->frame_buf) {
                free(llns->frame_buf);
            }
            if (llns->in_buf) {
                free(llns->in_buf);
            }
            if (llns->out_buf) {
                free(llns->out_buf);
            }
            free(llns);
            llns = NULL;
        }
    }



    return llns;
}

void stream_llns_close(struct __stream_llns **hdl)
{
    if (hdl == NULL || *hdl == NULL) {
        return ;
    }
    struct __stream_llns *llns = *hdl;

    llns->status = LLNS_STATUS_STOP;


    while (llns->busy) {
        os_time_dly(2);
    }
    while (llns->r_busy) {
        //    os_sem_set(&llns->r_sem, 0);
        //    os_sem_post(&llns->r_sem);
        os_time_dly(2);
    }
    while (llns->w_busy) {
        os_sem_set(&llns->sem, 0);
        os_sem_post(&llns->sem);
        os_time_dly(2);
    }


    task_kill(DEMO_TASK_NAME);


    audio_stream_del_entry(&llns->entry);

    local_irq_disable();
    if (llns->private_buffer) {
        free(llns->private_buffer);
    }
    if (llns->share_buffer) {
        free(llns->share_buffer);
    }
    if (llns->frame_buf) {
        free(llns->frame_buf);
    }
    if (llns->in_buf) {
        free(llns->in_buf);
    }
    if (llns->out_buf) {
        free(llns->out_buf);
    }
    free(llns);
    *hdl = NULL;
    local_irq_enable();
    printf("func = %s, %d\n", __FUNCTION__, __LINE__);
}


void stream_llns_set_parm(struct __stream_llns *llns, struct __stream_llns_parm *parm)
{
    if (llns == NULL || parm == NULL) {
        return ;
    }
    if (llns->status != LLNS_STATUS_STOP) {
        llns->busy = 1;
        os_mutex_pend(&llns->mutex, 0);

        llns_param_t param;
        if (llns->llns_hdl) {
            param.gainfloor = parm->gainfloor;
            param.suppress_level = parm->suppress_level;
            JLSP_llns_set_parameters(llns->llns_hdl, &param);
        }

        os_mutex_post(&llns->mutex);
        llns->busy = 0;
    }
}

void stream_llns_switch(struct __stream_llns *llns, u8 onoff)
{
    if (llns) {
        local_irq_disable();
        if (llns->status != LLNS_STATUS_STOP) {
            if (llns->status == LLNS_STATUS_PAUSE) {
                cbuf_clear(&llns->in_cbuf);
                cbuf_clear(&llns->out_cbuf);
                memset(llns->frame_buf, 0, llns->frame_buf_size);
                llns->fill_flag = 1;
                llns->skip_counter = 0;//通过按键切换的话，认为mic数据已经正常了,不然会有一段静音
                //cbuf_write(&llns->in_cbuf, llns->frame_buf, llns->in_start_fill_len);
                //cbuf_write(&llns->out_cbuf, llns->frame_buf, llns->frame_buf_size/2 );
            }
            llns->status = (onoff ? LLNS_STATUS_START : LLNS_STATUS_PAUSE);
        }
        local_irq_enable();
        //llns->status = onoff;
        //printf("llns->status = %d\n", llns->status);
    }
}

void stream_llns_set_winsize(struct __stream_llns *llns, int winsize)
{
    JLSP_llns_set_winsize(llns->llns_hdl, winsize);
}

#endif//WIRELESS_LLNS_ENABLE




