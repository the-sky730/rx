#include "stream_sync.h"

static void stream_sync_irq_cb(void *hdl)
{
    struct __stream_sync *stream = hdl;
    audio_stream_resume(&stream->entry);
}

static int stream_sync_output_handler(void *hdl, void *buf, int len)
{
    struct __stream_sync *stream = hdl;
    if (!stream) {
        return 0;
    }

    if (stream->out_buf) {
        if (stream->out_points < stream->out_total) {
            return 0;
        } else {
            stream->out_buf = NULL;
            return len;
        }
    }

    stream->out_buf = buf;
    stream->out_points = 0;
    stream->out_total = len / 2;

    return 0;
}


extern int audio_src_base_data_flush_out(struct audio_src_base_handle *src);
static int stream_sync_run(struct __stream_sync *stream, s16 *data, int len)
{
    int data_size = stream->cb.get_size(stream->cb.priv);
//    if (stream->sync.start == 0) {
//        if (data_size >= stream->sync.begin_size) {
//            stream->sync.start = 1;
//        }
//    }
    if (stream->sync.start) {
        audio_buf_sync_adjust(&stream->sync, data_size);
    } else {
//        putchar('$');
    }

    int wlen = audio_src_resample_write(stream->sync.src_sync, data, len);
    if (wlen < len) {
        audio_resample_hw_trigger_interrupt(stream->sync.src_sync, (void *)&stream->entry, (void (*)(void *))audio_stream_resume);
    }
    //if (wlen) {
    //    if (stream->ibuf && stream->obuf) {
    //        //audio_src_base_data_flush_out(&stream->sync.src_sync->base);
    //    }
    //}
    return wlen;
}

static void stream_sync_output_data_process_len(struct audio_stream_entry *entry, int len)
{
    struct __stream_sync *stream = container_of(entry, struct __stream_sync, entry);
    if (stream) {
        stream->out_points += len / 2;
    }
}

static int stream_sync_data_handler(struct audio_stream_entry *entry,
                                    struct audio_data_frame *in,
                                    struct audio_data_frame *out)
{
    struct __stream_sync *stream = container_of(entry, struct __stream_sync, entry);
    out->sample_rate = stream->sample_rate;
    if (in->data_len) {
        out->no_subsequent = 1;
    }
    //输出剩余数据
    if (stream->out_buf && (stream->out_points < stream->out_total)) {
        struct audio_data_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.channel = in->channel;
        frame.sample_rate = stream->sample_rate;
        frame.data_sync = in->data_sync;
        while (1) {
            frame.data_len = (stream->out_total - stream->out_points) * 2;
            frame.data = &stream->out_buf[stream->out_points];
            int len = stream->out_points;
            audio_stream_run(entry, &frame);
#if 0
            if (len == stream->out_points) {
                break;
            }
            if (stream->out_points == stream->out_total) {
                break;
            }
#else
            break;
#endif
        }
        //未输出完退出
        if (stream->out_points < stream->out_total) {
            return 0;
        }
    } else {
        //printf("=========\n");
    }

    int len = 0;
    u8 cnt = 0;
    len = stream_sync_run(stream, in->data, in->data_len);

    if (stream->out_buf && (stream->out_points < stream->out_total)) {
        struct audio_data_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.channel = in->channel;
        frame.sample_rate = stream->sample_rate;
        frame.data_sync = in->data_sync;
        while (1) {
            frame.data_len = (stream->out_total - stream->out_points) * 2;
            frame.data = &stream->out_buf[stream->out_points];
            int len = stream->out_points;
            audio_stream_run(entry, &frame);
#if 0
            if (len == stream->out_points) {
                break;
            }
            if (stream->out_points == stream->out_total) {
                break;
            }
#else
            break;
#endif
        }
        if (stream->out_points < stream->out_total) {
            return len;
        }
        if (!len) {
            len = stream_sync_run(stream, in->data, in->data_len);
        }

    } else {
        out->data_len = 0;
        //printf("^^^^^^^^^^/n");
    }

    return len;
}

void stream_sync_buf_alloc(struct __stream_sync *stream, int ibuf_len, int obuf_len)
{
    if (stream) {
        if (ibuf_len) {
            printf("stream_sync_buf_alloc ibuf = %d\n", ibuf_len);
            stream->ibuf = zalloc(ibuf_len);
            ASSERT(stream->ibuf);
            audio_hw_src_set_input_buffer(stream->sync.src_sync, stream->ibuf, ibuf_len);
        }
        if (obuf_len) {
            printf("stream_sync_buf_alloc obuf = %d\n", obuf_len);
            stream->obuf = zalloc(obuf_len);
            ASSERT(stream->obuf);
            audio_hw_src_set_output_buffer(stream->sync.src_sync, stream->obuf, obuf_len);
        }
    }
}

void stream_sync_buf_free(struct __stream_sync *stream)
{
    if (stream) {
        if (stream->ibuf) {
            free(stream->ibuf);
            stream->ibuf = NULL;
        }
        if (stream->obuf) {
            free(stream->obuf);
            stream->obuf = NULL;
        }
    }
}


struct __stream_sync *stream_sync_open(struct stream_sync_info *info, u8 always)
{
    struct audio_buf_sync_open_param param = {0};

    struct __stream_sync *stream = zalloc(sizeof(struct __stream_sync));
    ASSERT(stream);

    param.total_len = info->get_total(info->priv);
    param.begin_per = info->begin_per;
    param.top_per = info->top_per;
    param.bottom_per = info->bottom_per;
    param.inc_step = info->inc_step;
    param.dec_step = info->dec_step;
    param.max_step = info->max_step;
    param.in_sr = info->i_sr;
    param.out_sr = info->o_sr;
    param.ch_num = info->ch_num;
    if (always) {
        param.check_in_out = 0;
    } else {
        param.check_in_out = 1;
    }
    param.output_hdl = stream;
    param.output_handler = stream_sync_output_handler;
    int ret = audio_buf_sync_open(&stream->sync, &param);
    ASSERT(ret == true);
    audio_src_set_rise_irq_handler(stream->sync.src_sync, stream, stream_sync_irq_cb);

    stream->sample_rate = info->o_sr;
    stream->cb.priv = info->priv;
    stream->cb.get_size = info->get_size;
    stream->cb.get_total = info->get_total;

    stream->sync.start = 1;

    stream->entry.data_process_len = stream_sync_output_data_process_len;
    stream->entry.data_handler = stream_sync_data_handler;
    r_f_printf("stream_sync_open ok !!!!!\n");

    return stream;
}

void stream_sync_close(struct __stream_sync **hdl)
{
    if (hdl && (*hdl)) {
        struct __stream_sync *stream = *hdl;
        audio_stream_del_entry(&stream->entry);
        stream->sync.start = 0;
        audio_buf_sync_close(&stream->sync);
        stream_sync_buf_free(stream);
        local_irq_disable();
        free(stream);
        *hdl = NULL;
        local_irq_enable();
    }
}

void stream_sync_resume(struct __stream_sync *hdl)
{
    if (hdl) {
        audio_stream_resume(&hdl->entry);
    }
}


