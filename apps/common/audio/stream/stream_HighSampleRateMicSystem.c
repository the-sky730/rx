#include "stream_HighSampleRateMicSystem.h"
#include "media/HighSampleRate_SMS.h"
#include "app_config.h"

#if (WIRELESS_DENOISE_ENABLE)

#define STREAM_HIGHSAMPLERATEMICSYSTEM_ALIN(var,al)     ((((var)+(al)-1)/(al))*(al))

#define DEMO_TASK_NAME   "demo_task1"

#define DENOISE_STATUS_STOP     0
#define DENOISE_STATUS_START    1
#define DENOISE_STATUS_PAUSE    2

extern void overlay_load_code(u32 type);

static void stream_HighSampleRateMicSystem_output_data_process_len(struct audio_stream_entry *entry,  int len)
{
}

static void stream_HighSampleRateSingleMicSystem_task(void *p)
{
    u32 r_len, w_len;
    struct __stream_HighSampleRateSingleMicSystem *denoise = (struct __stream_HighSampleRateSingleMicSystem *)p;
    while (1) {
        os_sem_pend(&denoise->sem, 0);
        if (denoise->status != DENOISE_STATUS_STOP) {
            denoise->w_busy = 1;
            r_len = cbuf_read(&denoise->in_cbuf, denoise->frame_buf, denoise->frame_buf_size);
            if (r_len == denoise->frame_buf_size) {
                if (denoise->status == DENOISE_STATUS_START) {
#if 1
                    os_mutex_pend(&denoise->mutex, 0);
                    HighSampleRate_SMS_Process(
                        denoise->run_buf,
                        denoise->temp_buf,
                        denoise->frame_buf,
                        denoise->frame_buf,
                        1
                    );
                    os_mutex_post(&denoise->mutex);
#endif
                }
                w_len = cbuf_write(&denoise->out_cbuf, denoise->frame_buf, denoise->frame_buf_size);
                if (w_len != denoise->frame_buf_size) {
                    putchar('A');
                } else {
                    //putchar('C');
                }

                //os_sem_set(&denoise->r_sem, 0);
                //os_sem_post(&denoise->r_sem);
            } else {
                //putchar('a');
            }
            denoise->w_busy = 0;
        } else {
            break;
        }
    }

    printf("task wait kill!!\n");
    while (1) {
        os_time_dly(10000);
    }

}

static int stream_HighSampleRateMicSystem_data_handler(struct audio_stream_entry *entry,
        struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    struct __stream_HighSampleRateSingleMicSystem *denoise = container_of(entry, struct __stream_HighSampleRateSingleMicSystem, entry);
    if (in->offset) {
        out->data = in->data;
        out->data_len = in->data_len;
        out->channel = in->channel;
        out->sample_rate = in->sample_rate;
        return in->data_len;
    }
    u8 ready = 1;
    if (in->data_len && denoise->status == DENOISE_STATUS_START) {
        u32 r_len, w_len;
        denoise->r_busy = 1;
        //先写数据流过来的数据到in_cbuf做运算准备
        w_len = cbuf_write(&denoise->in_cbuf, in->data, in->data_len);
        if (w_len != in->data_len) {
            putchar('b');
        }
        os_sem_set(&denoise->sem, 0);
        os_sem_post(&denoise->sem);
        //从out_cbuf读取已经算法处理完的数据
#if 0
        if (cbuf_get_data_len(&denoise->out_cbuf) < in->data_len) {
            //printf("r_len = %d\n", cbuf_get_data_len(&denoise->out_cbuf));
            //os_sem_pend(&denoise->r_sem, 0);
        }
#endif
        if (denoise->fill_flag) {
            if (cbuf_get_data_len(&denoise->out_cbuf) >= (denoise->frame_buf_size)) {
                denoise->fill_flag = 0;
            } else {
                memset(in->data, 0, in->data_len);
                ready = 0;
            }
        }

        if (ready) {
            r_len = cbuf_read(&denoise->out_cbuf, in->data, in->data_len);
            if (r_len != in->data_len) {
                putchar('#');
                ready = 0;
            } else {
                //putchar('D');
                if (denoise->skip_counter) {
                    denoise->skip_counter--;
                    memset(in->data, 0, in->data_len);
                    //putchar('$');
                }
            }
        }
        //通知算法处理线程， 有新的数据写入， 是否需要做运算
        denoise->r_busy = 0;
    }

    out->data = in->data;
    out->data_len = in->data_len;
    out->channel = in->channel;
    out->sample_rate = in->sample_rate;



    return in->data_len;
}

struct __stream_HighSampleRateSingleMicSystem *stream_HighSampleRateSingleMicSystem_open(
    struct __stream_HighSampleRateSingleMicSystem_parm *parm,
    int	sampleRate,
    u8 onoff)
{
    int run_bufsize = HighSampleRate_SMS_QueryBufSize(sampleRate);
    int temp_bufsize = HighSampleRate_SMS_QueryTempBufSize(sampleRate);
    mem_stats();
    int buf_size = STREAM_HIGHSAMPLERATEMICSYSTEM_ALIN(sizeof(struct __stream_HighSampleRateSingleMicSystem), 4)
                   + STREAM_HIGHSAMPLERATEMICSYSTEM_ALIN(run_bufsize, 4)
                   + STREAM_HIGHSAMPLERATEMICSYSTEM_ALIN(temp_bufsize, 4)
                   ;
    int offset = 0;

    printf("\n denoise OPEN:SampleRate:%d Need_buf_size: buf_size:%d , run_bufsize:%d + temp_bufsize:%d\n", sampleRate, buf_size, run_bufsize, temp_bufsize);

    u8 *buf = zalloc(buf_size);

    ASSERT(buf, "%s no mem!!\n", __FUNCTION__);

    struct __stream_HighSampleRateSingleMicSystem *denoise = (struct __stream_HighSampleRateSingleMicSystem *)buf;
    offset += STREAM_HIGHSAMPLERATEMICSYSTEM_ALIN(sizeof(struct __stream_HighSampleRateSingleMicSystem), 4);

    denoise->run_buf = (void *)(buf + offset);
    offset += STREAM_HIGHSAMPLERATEMICSYSTEM_ALIN(run_bufsize, 4);

    denoise->temp_buf = (void *)(buf + offset);
    offset += STREAM_HIGHSAMPLERATEMICSYSTEM_ALIN(temp_bufsize, 4);

    os_mutex_create(&denoise->mutex);
    os_sem_create(&denoise->sem, 0);
    //os_sem_create(&denoise->r_sem, 0);


    denoise->busy = 0;
    denoise->status = (onoff ? DENOISE_STATUS_START : DENOISE_STATUS_PAUSE);
    denoise->fill_flag = 1;
    denoise->skip_counter = (1500 * 10) / parm->frame_len; //刚启动，mic的数据有异常。主要是解决ADC与算法结合引入的噪声问题


    overlay_load_code(0);
    HighSampleRate_SMS_Init(denoise->run_buf,
                            denoise->temp_buf,
                            parm->AggressFactor,
                            parm->minSuppress,
                            sampleRate,
                            parm->init_noise_lvl
                           );
    denoise->sampleRate = sampleRate;

    denoise->entry.data_process_len = stream_HighSampleRateMicSystem_output_data_process_len;
    denoise->entry.data_handler = stream_HighSampleRateMicSystem_data_handler;

    denoise->frame_buf_size = HighSampleRate_SMS_GetMiniFrameSize(denoise->sampleRate);
    denoise->frame_buf_size <<= 1;
    //printf("denoise->frame_buf_size = %d\n", denoise->frame_buf_size);
    denoise->frame_buf = (s16 *)zalloc(denoise->frame_buf_size);
    ASSERT(denoise->frame_buf);

    denoise->in_buf = zalloc(denoise->frame_buf_size * 2);
    ASSERT(denoise->in_buf);
    denoise->out_buf = zalloc(denoise->frame_buf_size * 2);
    ASSERT(denoise->out_buf);
    cbuf_init(&denoise->in_cbuf, denoise->in_buf, denoise->frame_buf_size * 2);
    cbuf_init(&denoise->out_cbuf, denoise->out_buf, denoise->frame_buf_size * 2);

    //u16 unit_len = (u16)((parm->frame_len * denoise->sampleRate / 10000) * 2);


    //denoise->start_fill_len = denoise->frame_buf_size + denoise->frame_buf_size/2;

    //denoise->in_start_fill_len = unit_len * (denoise->frame_buf_size / unit_len);
    //denoise->out_start_fill_len = unit_len * ((denoise->frame_buf_size / unit_len) + 1) - denoise->frame_buf_size;
    //printf("in_start_fill_len = %d, out_start_fill_len = %d\n", denoise->in_start_fill_len, denoise->out_start_fill_len);

    memset(denoise->frame_buf, 0, denoise->frame_buf_size);
    /* cbuf_write(&denoise->in_cbuf, denoise->frame_buf, 640); */
    /* cbuf_write(&denoise->out_cbuf, denoise->frame_buf, 192); */
    //cbuf_write(&denoise->in_cbuf, denoise->frame_buf, denoise->in_start_fill_len);
    cbuf_write(&denoise->out_cbuf, denoise->frame_buf, denoise->frame_buf_size / 3);

    int err = task_create(stream_HighSampleRateSingleMicSystem_task, denoise, DEMO_TASK_NAME);
    if (err) {
        printf("func = %s, %d, fail\n", __FUNCTION__, __LINE__);
        if (denoise) {
            if (denoise->frame_buf) {
                free(denoise->frame_buf);
            }
            if (denoise->in_buf) {
                free(denoise->in_buf);
            }
            if (denoise->out_buf) {
                free(denoise->out_buf);
            }
            free(denoise);
            denoise = NULL;
        }
    }



    return denoise;
}

void stream_HighSampleRateSingleMicSystem_close(struct __stream_HighSampleRateSingleMicSystem **hdl)
{
    if (hdl == NULL || *hdl == NULL) {
        return ;
    }
    struct __stream_HighSampleRateSingleMicSystem *denoise = *hdl;

    denoise->status = DENOISE_STATUS_STOP;


    while (denoise->busy) {
        os_time_dly(2);
    }
    while (denoise->r_busy) {
        //    os_sem_set(&denoise->r_sem, 0);
        //    os_sem_post(&denoise->r_sem);
        os_time_dly(2);
    }
    while (denoise->w_busy) {
        os_sem_set(&denoise->sem, 0);
        os_sem_post(&denoise->sem);
        os_time_dly(2);
    }


    task_kill(DEMO_TASK_NAME);


    audio_stream_del_entry(&denoise->entry);

    local_irq_disable();
    if (denoise->frame_buf) {
        free(denoise->frame_buf);
    }
    if (denoise->in_buf) {
        free(denoise->in_buf);
    }
    if (denoise->out_buf) {
        free(denoise->out_buf);
    }
    free(denoise);
    *hdl = NULL;
    local_irq_enable();
    printf("func = %s, %d\n", __FUNCTION__, __LINE__);
}


void stream_HighSampleRateSingleMicSystem_set_parm(struct __stream_HighSampleRateSingleMicSystem *denoise, struct __stream_HighSampleRateSingleMicSystem_parm *parm)
{
    if (denoise == NULL || parm == NULL) {
        return ;
    }
    if (denoise->status != DENOISE_STATUS_STOP) {
        denoise->busy = 1;
        os_mutex_pend(&denoise->mutex, 0);
        HighSampleRate_SMS_Update(
            denoise->run_buf,
            parm->AggressFactor,
            parm->minSuppress
        );
        os_mutex_post(&denoise->mutex);
        denoise->busy = 0;
    }
}

void stream_HighSampleRateSingleMicSystem_switch(struct __stream_HighSampleRateSingleMicSystem *denoise, u8 onoff)
{
    if (denoise) {
        local_irq_disable();
        if (denoise->status != DENOISE_STATUS_STOP) {
            if (denoise->status == DENOISE_STATUS_PAUSE) {
                cbuf_clear(&denoise->in_cbuf);
                cbuf_clear(&denoise->out_cbuf);
                memset(denoise->frame_buf, 0, denoise->frame_buf_size);
                denoise->fill_flag = 1;
                denoise->skip_counter = 0;//通过按键切换的话，认为mic数据已经正常了,不然会有一段静音
                //cbuf_write(&denoise->in_cbuf, denoise->frame_buf, denoise->in_start_fill_len);
                cbuf_write(&denoise->out_cbuf, denoise->frame_buf, denoise->frame_buf_size / 3);
            }
            denoise->status = (onoff ? DENOISE_STATUS_START : DENOISE_STATUS_PAUSE);
        }
        local_irq_enable();
        //denoise->status = onoff;
        //printf("denoise->status = %d\n", denoise->status);
    }
}
#endif//WIRELESS_DENOISE_ENABLE




