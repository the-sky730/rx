#include "adapter_wireless_dec.h"
#include "adapter_decoder.h"
#include "asm/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "clock_cfg.h"
#include "adapter_decoder.h"
#include "adapter_process.h"
#include "key_event_deal.h"
#include "adapter_wireless_command.h"
#include "btcontroller_config.h"

#include "asm/dac.h"

extern u32 config_vendor_le_bb;
extern struct audio_dac_hdl dac_hdl;
#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)
#define ADAPTER_WIRELESS_FRAME_LBUF_SIZE		(150*5)
#elif (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)
#define ADAPTER_WIRELESS_FRAME_LBUF_SIZE		(200)
#elif (APP_MAIN == APP_WIRELESS_MIC_2T1)
#define ADAPTER_WIRELESS_FRAME_LBUF_SIZE		(150*6)
#else
/* lbuff_head + hfree(12bytes) + (hentry(20bytes) + frame_len + user_len) * numbers */
#if	(WIRELESS_MIC_STEREO_EN)
#define JLA_5MS_FRAME_LEN       42
#else
#define JLA_5MS_FRAME_LEN      (42 * 2)
#endif
#define LBUF_RX_CACHE_NUMS      2
#define ADAPTER_WIRELESS_FRAME_LBUF_SIZE \
    ( \
      sizeof(struct lbuff_head) + 12 + \
      (20 + JLA_5MS_FRAME_LEN + sizeof(struct __adapter_wireless_rx_bulk)) * LBUF_RX_CACHE_NUMS \
    )
#endif

#define WIRELESS_PACKET_HEADER_LEN  			(2 + 1)//(2)/*包头Max Length*/

#define WIRELESS_OUTPUT_DATA_TIME_REMAIN        (10)//单位ms

#define WIRELESS_DEC_DATA_INPUT_MIN             (5)//输入音频数据最少限制

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
#define WIRELESS_DEC_MIXER_CH_NUM               2//音频声道数
#else
#define WIRELESS_DEC_MIXER_CH_NUM               1//音频声道数
#endif
#define WIRELESS_DEC_MIXER_CH_MS_UINIT          ((WIRELESS_DECODE_SAMPLERATE/1000)*WIRELESS_DEC_MIXER_CH_NUM*2)
#define WIRELESS_DEC_MIXER_CH_OUTPUT_BUF_SIZE   ((WIRELESS_CODING_FRAME_LEN*WIRELESS_DEC_MIXER_CH_MS_UINIT)/10)//(480*1)

struct __adapter_wireless_rx_bulk {
    struct list_head entry;
    int data_len;
    u8 data[0];
};

struct __adapter_wireless_command_cbk {
    void *priv;
    int (*callback)(void *priv, u8 *data);
};

struct __adapter_wireless_rx {
    u8								        buf[ADAPTER_WIRELESS_FRAME_LBUF_SIZE];
#if (WIRELESS_TRANSFER_EXPAND_EN)
    u8								        seqn;
#else
    u16								        seqn;
#endif
    volatile struct wl_expand		        expand_data;//扩展内容
    volatile u8						        w_busy;
    struct list_head 				        list;
    struct __adapter_wireless_command_cbk   command_cbk;
    u8 id;
};

struct __adapter_wireless_dec {
    struct adapter_media_parm 		*media_parm;    // 动态参数
    struct adapter_decoder_fmt  	*fmt;		    // 解码参数配置
    struct adapter_audio_stream		*adapter_stream;// 音频流
    struct audio_stream             *audio_stream;
    struct audio_decoder 			decoder;	// 解码器
    struct audio_res_wait 			wait;		// 资源等待句柄
    u8 								start;		// 解码开始
    int 							coding_type;// 解码类型
    void 							*priv_parm;

    struct __adapter_wireless_rx 	*rx;

    struct audio_mixer              *mixer;
    struct audio_mixer_ch           mix_ch;    // 叠加句柄
    u8                              mixer_en;
    u8                              mixer_ch_index;
    u8                              mixer_ch_flag;
    u8                              mixer_ch_limit;//通道数据超出了限定标志
    volatile u8                     mixer_ch_switch;//声道转换，0:输出假立体声 1:输出到各自声道
    u8                              mixer_ch_buf[WIRELESS_DEC_MIXER_CH_OUTPUT_BUF_SIZE];
    u8 								dec_output_type;
    u8                              suspend;
    volatile u8                     wait_resume;
    int                             total_pcm_frames;
    s16                             total_num;
    u16                             packet_ts;
    u16                             prev_ts;
    s16                             ref_delay_time;
    u16                             ref_delay_cnt;
    s16                             delay_time;
    s16                             interval[5];
    u32                             suspend_timeout;
    struct wl_stream_delay_filter   *delay_filter;
    int                             frame_data[128];
};

extern void adapter_audio_stream_echo_run(struct adapter_audio_stream *audio, u8 echo_en);

static struct __adapter_wireless_rx *adapter_wireless_rx_open(void)
{
    struct __adapter_wireless_rx *rx = zalloc(sizeof(struct __adapter_wireless_rx));
    ASSERT(rx);

    lbuf_init(rx->buf, (sizeof(rx->buf) / sizeof(rx->buf[0])), 4, 0);

    INIT_LIST_HEAD(&rx->list);

    rx->command_cbk.priv = NULL;
    rx->command_cbk.callback = NULL;

    return rx;
}

static void adapter_wireless_rx_close(struct __adapter_wireless_rx **hdl)
{
    if (hdl && *hdl)	{
        struct __adapter_wireless_rx *rx = *hdl;
        while (rx->w_busy) {
            os_time_dly(1);
        }
        printf("adapter_wireless_rx_close wait ok!\n");
        local_irq_disable();
        free(rx);
        *hdl = NULL;
        local_irq_enable();
    }
}

static void adapter_wireless_rx_command_callback_register(struct __adapter_wireless_rx *rx, void (*callback)(void *priv, u8 *data, u16 len), void *priv)
{
    if (rx) {
        rx->command_cbk.priv = priv;
        rx->command_cbk.callback = callback;
    }
}

const unsigned char slience_5ms_44k_2ch[] = {
    0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x24,
    0xf9, 0x4a, 0x0d, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x38, 0x24, 0xf9, 0x4a, 0x0d, 0x00, 0x00, 0x04
};

static const unsigned char errpacket[] = {
    0x02, 0x00
};



static int adapter_wireless_rx_get_packet_num(struct __adapter_wireless_rx *rx);
//extern u8 g_cur_chl;
static int adapter_wireless_rx_write(struct __adapter_wireless_rx *rx, void *data, u16 len)
{
    if (len == 0) {
        return 0;
    }
    int ret = -1;
    int cmd_ret = 0;
    local_irq_disable();
    if (rx) {
        struct __adapter_wireless_rx_bulk *p;
        rx->w_busy = 1;
//        if (len < WIRELESS_PACKET_HEADER_LEN) {
//            goto __exit;
//        }
//
        u8 *buf = data;
#if (WIRELESS_TRANSFER_EXPAND_EN)
        u8 rx_seqn = buf[0];
        memcpy(&rx->expand_data, &buf[1], 1);
        //rx->expand_data = (struct wl_expand)buf[1];
        //解析expand_data
#else
        u16 rx_seqn = 0;
        rx_seqn |= (u16)(buf[0] << 8);
        rx_seqn |= buf[1];
#endif
#if (!WIRELESS_MICROPHONE_MODE)
        if (rx->seqn == rx_seqn) {
            goto __exit;
        }
#endif

        rx->seqn ++;
        if (rx_seqn != rx->seqn) {
//           if (g_cur_chl) {
//               putchar('1');
//           } else {
//               putchar('0');
//           }
            putchar('U');
            rx->seqn = rx_seqn;
        }

        len -= (2 + rx->expand_data.ex_len);
        buf += (2 + rx->expand_data.ex_len);
        if (rx->command_cbk.callback) {
            if ((config_vendor_le_bb & VENDOR_BB_PIS_EN)) {
                cmd_ret = 0;
            } else {
                cmd_ret = rx->command_cbk.callback((void *)rx->expand_data.channel, &buf[0]);
            }
            //printf("c=%d",cmd_ret);
        }

        len -= cmd_ret;
        buf += cmd_ret;
#if (WIRELESS_MICROPHONE_MODE || TCFG_2T1_RX_PRODUCT_TEST_EN)
        if (len <= 1 && (config_vendor_le_bb & VENDOR_BB_PIS_EN)) {
            len = sizeof(errpacket);//sizeof(slience_5ms_44k_2ch);
            buf = errpacket;//slience_5ms_44k_2ch;
            //len = sizeof(slience_5ms_44k_2ch);
            //buf = slience_5ms_44k_2ch;
            putchar('N');
        } else
#endif
        {
            if (len <= WIRELESS_DEC_DATA_INPUT_MIN) {
                //printf("no media data!!\n");
                goto __exit;
            }
        }

        p = lbuf_alloc((struct lbuff_head *)rx->buf, sizeof(*p) + len);
        //printf("alloc = %x\n", p);
        if (p == NULL) {
            ret = 1;
            putchar(rx->id == 0 ? 'F' : 'f');
            //adapter_wireless_media_rx_notice_to_decode();
            goto __exit;
        }


        // 填数
        p->data_len = len;
        memcpy(p->data, buf, len);
        list_add_tail(&p->entry, &rx->list);
        // 告诉上层有数据
        //adapter_wireless_media_rx_notice_to_decode();

        //printf("p:%d", adapter_wireless_rx_get_packet_num(rx));

        ret = 0;
    }
__exit:
    if (rx) {
        rx->w_busy = 0;
    }
    local_irq_enable();
    return ret;
}


static int adapter_wireless_rx_get_packet_num(struct __adapter_wireless_rx *rx)
{
    if (rx == 0) {
        return 0;
    }
    struct __adapter_wireless_rx_bulk *p;
    u32 num = 0;
    local_irq_disable();
    if (list_empty(&rx->list)) {
        local_irq_enable();
        return 0;
    }
    list_for_each_entry(p, &rx->list, entry) {
        num++;
    }
    local_irq_enable();
    return num;
}


static void *adapter_wireless_rx_fetch_packet(struct __adapter_wireless_rx *rx, int *len, void *prev_packet)
{
    if (rx == 0) {
        return NULL;
    }
    struct __adapter_wireless_rx_bulk *p;
    local_irq_disable();
    if (rx->list.next != &rx->list) {
        if (prev_packet) {
            p = container_of(prev_packet, struct __adapter_wireless_rx_bulk, data);
            if (p->entry.next != &rx->list) {
                p = list_entry(p->entry.next, typeof(*p), entry);
                *len = p->data_len;
                local_irq_enable();
                return p->data;
            }
        } else {
            p = list_entry((&rx->list)->next, typeof(*p), entry);
            *len = p->data_len;
            local_irq_enable();
            return p->data;
        }
    }
    local_irq_enable();

    return NULL;
}


void adapter_wireless_rx_free_packet(void *data)
{
    struct __adapter_wireless_rx_bulk *bulk = container_of(data, struct __adapter_wireless_rx_bulk, data);

    local_irq_disable();
    list_del(&bulk->entry);
    local_irq_enable();

    //printf("free = %x\n", bulk);
    lbuf_free(bulk);
}


int adapter_wireless_rx_get_packet(struct __adapter_wireless_rx *rx, u8 **frame)
{
    if (rx == NULL) {
        return 0;
    }

    struct __adapter_wireless_rx_bulk *p;
    local_irq_disable();
    if (!list_empty(&rx->list)) {
        p = list_first_entry(&rx->list, typeof(*p), entry);
        list_del(&p->entry);
        *frame = p->data;
        local_irq_enable();
        return p->data_len;
    }
    local_irq_enable();

    return 0;
}

//解码数据输入写接口
int adapter_wireless_dec_rx_write(struct __adapter_wireless_dec *dec, void *data, u16 len)
{
    int ret = -1;
    if (dec && dec->start) {
        local_irq_disable();
        int ret = adapter_wireless_rx_write(dec->rx, data, len);
        if (ret == 1) {
            audio_decoder_resume(&dec->decoder);
        }

        //if (ret >= 0) {
#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
    )
        if (dec->wait_resume)
#endif
        {
            dec->wait_resume = 0;
            audio_decoder_resume(&dec->decoder);
        }
        local_irq_enable();
        //}
    }
    return ret;
}
int adapter_wireless_dec_get_frame_packet_num(struct __adapter_wireless_dec *dec)
{
    if (dec == NULL) {
        return 0;
    }
    return adapter_wireless_rx_get_packet_num(dec->rx) ;
}

#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
    )


#define WL_ILLEGAL_INTERVAL         50



struct wl_stream_delay_filter {
    struct audio_stream_entry entry;
    u32 frame_counter;
    u8  sync_start;
    u8  channel;
    u16 process_len;
    u32 sync_off_time;
    int sample_rate;
    void *resample;
    void *resample_mem;
    struct __adapter_wireless_dec *dec;
};


static void wireless_dec_mixer_event_handler(void *priv, int event, int arg)
{
    struct __adapter_wireless_dec *dec = (struct __adapter_wireless_dec *)priv;

    switch (event) {
    case MIXER_EVENT_CH_SUSPEND:
        local_irq_disable();
        dec->suspend = 1;
        local_irq_enable();
#if NEW_AUDIO_W_MIC
        printf("%d, %d, %dp\n", dec->mixer_ch_index, adapter_wireless_dec_get_frame_packet_num(dec), audio_sound_dac_buffered_frames(dec->adapter_stream->dac1));
#else
        printf("%d, %d, %dp\n", dec->mixer_ch_index, adapter_wireless_dec_get_frame_packet_num(dec), audio_dac_channel_buffered_frames(dec->adapter_stream->dac));
#endif
        break;
    default:
        break;
    }
}

static int wl_stream_delay_data_handler(struct audio_stream_entry *entry, struct audio_data_frame *in,
                                        struct audio_data_frame *out)
{
    struct wl_stream_delay_filter *f = container_of(entry, struct wl_stream_delay_filter, entry);

    if (f->resample) {
        if (f->sync_start && time_after(jiffies, f->sync_off_time)) {
            f->sync_start = 0;
            f->dec->delay_time = f->dec->ref_delay_time;
            audio_resample_hw_change_config(f->resample, f->sample_rate, f->sample_rate);
        }

        if (!f->sync_start) {
            int delay_diff = f->dec->delay_time - f->dec->ref_delay_time;
            int diff_abs = __builtin_abs(delay_diff);
            int sample_offset = 0;

            if (diff_abs < 1) {
                sample_offset = 0;
            } else if (diff_abs < 3) {
                sample_offset = f->sample_rate / 2000;
                f->sync_off_time = jiffies + msecs_to_jiffies(diff_abs * 1000);
            } else if (diff_abs < 6) {
                sample_offset = f->sample_rate / 1000;
                f->sync_off_time = jiffies + msecs_to_jiffies(diff_abs * 1000 / 2);
            } else {
                sample_offset = f->sample_rate / 1000 * 2;
                f->sync_off_time = jiffies + msecs_to_jiffies(diff_abs * 1000 / 3);
            }

            sample_offset = delay_diff > 0 ? sample_offset : -sample_offset;
            if (sample_offset) {
                /*printf("diff : %d, %d\n", delay_diff, sample_offset);*/
                audio_resample_hw_change_config(f->resample, f->sample_rate + sample_offset, f->sample_rate);
                f->sync_start = 1;
            }
        }
        int wlen = audio_resample_hw_write(f->resample, in->data, in->data_len);
        if (wlen < in->data_len) {
            audio_resample_hw_trigger_interrupt(f->resample, (void *)&f->entry, (void (*)(void *))audio_stream_resume);
        }
        return wlen;
    }

    return in->data_len;
}

static int wl_stream_delay_probe_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    return 0;
}

static void wl_stream_data_process_len(struct audio_stream_entry *entry, int len)
{
    struct wl_stream_delay_filter *f = container_of(entry, struct wl_stream_delay_filter, entry);

    f->process_len = len;
}

static int wl_stream_resample_output_handler(void *priv, void *data, int len)
{
    struct wl_stream_delay_filter *f = (struct wl_stream_delay_filter *)priv;
    struct audio_data_frame frame = {0};
    frame.channel = f->channel;
    frame.sample_rate = f->sample_rate;
    frame.data = data;
    frame.data_len = len;

    audio_stream_run(&f->entry, &frame);

    return f->process_len;
}


static struct audio_stream_entry *wl_stream_delay_filter_setup(struct __adapter_wireless_dec *dec, struct audio_mixer_ch *mix_ch)
{
    struct wl_stream_delay_filter *f = (struct wl_stream_delay_filter *)zalloc(sizeof(struct wl_stream_delay_filter));

    if (!f) {
        return NULL;
    }

    dec->delay_filter = f;
    f->entry.prob_handler = wl_stream_delay_probe_handler;
    f->entry.data_handler = wl_stream_delay_data_handler;
    f->entry.data_process_len = wl_stream_data_process_len;
    f->sample_rate = dec->decoder.fmt.sample_rate;
    f->sync_start = 0;
    f->channel = dec->decoder.fmt.channel;
#if 0
    int resample_mem_size = 40 * 2 * f->channel;
    f->resample_mem = zalloc(40 * 2 * f->channel);
    f->resample = audio_resample_hw_open(f->channel, f->sample_rate, f->sample_rate, AUDIO_ONLY_RESAMPLE);
#else
    int resample_mem_size = 64 * 2 * f->channel;
    f->resample_mem = zalloc(64 * 2 * f->channel);
    /*f->resample = audio_resample_hw_open(f->channel, f->sample_rate, f->sample_rate, AUDIO_RESAMPLE_SYNC_OUTPUT);*/
    f->resample = audio_resample_hw_open(f->channel, f->sample_rate, f->sample_rate, AUDIO_ONLY_RESAMPLE);
#endif
    if (f->resample) {
        audio_resample_hw_set_input_buffer(f->resample, f->resample_mem, resample_mem_size);
        audio_resample_hw_set_output_handler(f->resample, (void *)f, wl_stream_resample_output_handler);
    }
    f->dec = dec;
    audio_mixer_ch_set_event_handler(mix_ch, dec, wireless_dec_mixer_event_handler);

    return &f->entry;
}

static void wl_stream_delay_filter_free(struct __adapter_wireless_dec *dec)
{
    if (dec->delay_filter) {
        local_irq_disable();
        audio_stream_del_entry(&dec->delay_filter->entry);
        if (dec->delay_filter->resample) {
            audio_resample_hw_close(dec->delay_filter->resample);
        }
        if (dec->delay_filter->resample_mem) {
            free(dec->delay_filter->resample_mem);
        }
        free(dec->delay_filter);
        dec->delay_filter = NULL;
        local_irq_enable();
    }
}


extern void bb_pkt_info_get_by_handle(int handle, u8 *miss_cnt, u8 *err_cnt, u8 *total_cnt);
extern u16 dualmic_dec_ch_2_conn_handle(u8 dec_ch);
extern void bb_pkt_info_clear_by_handle(int handle);
static int adapter_wireless_timestamp_filter(struct __adapter_wireless_dec *dec, u8 *packet, int len)
{
    if (dec->mixer == NULL) {
        return 0;
    }
    if (!(config_vendor_le_bb & VENDOR_BB_TS)) {
        return 0;
    }

    u16 ts = 0;
    u8 interval_case = 0;
    memcpy(&ts, packet + len - 2, 2);
    u16 interval = ts - dec->prev_ts;
    dec->prev_ts = ts;

#if (WIRELESS_BLE_CONNECT_INTERVAL == 2)
#if (WIRELESS_CODING_FRAME_LEN == 50)
    if (interval <= 4) {
        interval_case = 0;
    } else if (interval <= 6) {
        interval_case = 1;
    } else if (interval <= 8) {
        interval_case = 2;
    } else if (interval <= 10) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#elif (WIRELESS_CODING_FRAME_LEN == 75)
    if (interval <= 8) {
        interval_case = 0;
    } else if (interval <= 12) {
        interval_case = 1;
    } else if (interval <= 16) {
        interval_case = 2;
    } else if (interval <= 18) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#elif (WIRELESS_CODING_FRAME_LEN == 100)
    if (interval <= 8) {
        interval_case = 0;
    } else if (interval <= 12) {
        interval_case = 1;
    } else if (interval <= 16) {
        interval_case = 2;
    } else if (interval <= 18) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#else
#error "inerval error!!!! no support 111"
#endif
#elif (WIRELESS_BLE_CONNECT_INTERVAL == 4)
#if (WIRELESS_CODING_FRAME_LEN == 50)
    if (interval <= 2) {
        interval_case = 0;
    } else if (interval <= 3) {
        interval_case = 1;
    } else if (interval <= 4) {
        interval_case = 2;
    } else if (interval <= 5) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#elif (WIRELESS_CODING_FRAME_LEN == 75)
    if (interval <= 3) {
        interval_case = 0;
    } else if (interval <= 5) {
        interval_case = 1;
    } else if (interval <= 6) {
        interval_case = 2;
    } else if (interval <= 7) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#elif (WIRELESS_CODING_FRAME_LEN == 100)
    if (interval <= 4) {
        interval_case = 0;
    } else if (interval <= 6) {
        interval_case = 1;
    } else if (interval <= 8) {
        interval_case = 2;
    } else if (interval <= 10) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#else
#error "inerval error!!!! no support"
#endif

#elif (WIRELESS_BLE_CONNECT_INTERVAL == 6)

#if (WIRELESS_CODING_FRAME_LEN == 100)
    if (interval <= 3) {
        interval_case = 0;
    } else if (interval <= 4) {
        interval_case = 1;
    } else if (interval <= 5) {
        interval_case = 2;
    } else if (interval <= 6) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#else
#error "inerval error   3333!!!! no support"
#endif

#elif (WIRELESS_BLE_CONNECT_INTERVAL == 8)

#if (WIRELESS_CODING_FRAME_LEN == 100)
    if (interval <= 2) {
        interval_case = 0;
    } else if (interval <= 3) {
        interval_case = 1;
    } else if (interval <= 4) {
        interval_case = 2;
    } else if (interval <= 5) {
        interval_case = 3;
    } else {
        interval_case = 4;
    }
#else
#error "inerval error   4444!!!! no support"
#endif
#endif

#if (WIRELESS_HIGH_BW_EN && (WIRELESS_CODING_FRAME_LEN == 50))
#else
#endif

    dec->interval[interval_case]++;
    int sample_rate = dec->decoder.fmt.sample_rate;
#if NEW_AUDIO_W_MIC
    int dac_frames = audio_sound_dac_buffered_frames(dec->adapter_stream->dac1);
#else
    int dac_frames = audio_dac_channel_buffered_frames(dec->adapter_stream->dac);
#endif
    int mix_frames = audio_mixer_ch_data_len(&dec->mix_ch);
    int pcm_frames = dac_frames + mix_frames + (adapter_wireless_rx_get_packet_num(dec->rx) + 1) * WIRELESS_CODING_FRAME_LEN * sample_rate / 10000;
    if (dec->delay_filter && dec->delay_filter->resample) {
        pcm_frames += audio_resample_hw_buffered_frames(dec->delay_filter->resample);
    }
    dec->total_pcm_frames += pcm_frames;
    dec->total_num++;
#if WIRELESS_HIGH_BW_EN
    u16 ts_counter = 800;
#else
    u16 ts_counter = 400;
#endif


    if ((u16)(ts - dec->packet_ts) >= ts_counter) {

        int delay_time = (dec->total_pcm_frames / dec->total_num) * 1000 / sample_rate;
#if 0
        printf("[CH%d]: (0):%d ,(1):%d ,(2):%d ,(3):%d ,(4):%d , delay: %dms\n",
               dec->mixer_ch_index,
               dec->interval[0],
               dec->interval[1],
               dec->interval[2],
               dec->interval[3],
               dec->interval[4],
               delay_time);

        /* u16 curr_conn_hdl = dualmic_dec_ch_2_conn_handle(dec->mixer_ch_index); */
        /* u32 miss_cnt, err_cnt, total_cnt; */
        /* bb_pkt_info_get_by_handle(curr_conn_hdl, &miss_cnt, &err_cnt, &total_cnt); */
        /* printf("Miss:%d Err:%d Total:%d\n", miss_cnt, err_cnt, total_cnt); */
        /* bb_pkt_info_clear_by_handle(curr_conn_hdl); */
#endif

        s16 target_time = dec->ref_delay_time;
#if (WIRELESS_CODING_FRAME_LEN == 25)
        if ((dec->interval[2] > 100) || dec->interval[3] || dec->interval[4]) {
#else
        if ((dec->interval[2] > 10) || dec->interval[3] || dec->interval[4]) {
#endif
            target_time = WL_HIGH_DELAY_LIMIT;	//高延时下需要实时更新
            dec->ref_delay_cnt = 0;
        } else {
            if (dec->ref_delay_cnt < 15) {
                dec->ref_delay_cnt ++;
            } else {
                //低延时下更新可以慢一些
                target_time = WL_LOW_DELAY_LIMIT;
            }
        }
        dec->ref_delay_time = target_time;
        dec->delay_time = delay_time;
        /*printf("[CH%d] : %d, %d\n", dec->mixer_ch_index, dec->ref_delay_time, dec->delay_time);*/

        if (dec->total_num < (ts_counter / 4)) {
            //printf("counter fail!!!!!!!!! dec->total_num = %d, ts = %d, pts = %d\n", dec->total_num, ts, dec->packet_ts);
            dec->total_pcm_frames = 0;
            dec->total_num = 0;
            dec->packet_ts = ts;
            memset(dec->interval, 0x0, sizeof(dec->interval));
            return 0;
        }

        dec->total_pcm_frames = 0;
        dec->total_num = 0;
        /*情况1 ：10ms的间隔以及超过10ms的间隔超过阈值*/
        /*情况2 : 存在偶尔的10ms间隔情况*/
        /*情况3 : 正常的收数情况*/
        memset(dec->interval, 0x0, sizeof(dec->interval));
        dec->packet_ts = ts;
    }

    return 0;
}
#endif

// 解码获取数据
static int adapter_wireless_dec_get_frame(struct audio_decoder *decoder, u8 **frame)
{

    struct __adapter_wireless_dec *dec = container_of(decoder, struct __adapter_wireless_dec, decoder);
    u8 *packet = NULL;
    int len = 0;
    // 获取数据

    local_irq_disable();
    u8 frame_num = adapter_wireless_rx_get_packet_num(dec->rx);
#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
    )
    u8 frame_num_limit = ((WIRELESS_CODING_FRAME_LEN == 25) ? 4 : 2);
    if (!(config_vendor_le_bb & VENDOR_BB_PIS_EN) && (dec->suspend && frame_num < frame_num_limit) || frame_num < 1)
#else
    if (frame_num < 1)
#endif
    {
        dec->wait_resume = 1;
        /*putchar(dec->mixer_ch_index == 0 ? 'W' : 'w');*/
        local_irq_enable();
        return 0;
    }
    local_irq_enable();



    len = adapter_wireless_rx_get_packet(dec->rx, &packet);
    if (len <= 0) {
        //if(dec->mixer_ch_index){

        //    putchar('X');
        //}else{

        //    putchar('x');
        //}
        return len;
    }

#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
    )
    if (!(config_vendor_le_bb & VENDOR_BB_PIS_EN)) {
        adapter_wireless_timestamp_filter(dec, packet, len);
    }
#endif
    //printf("[0x%x - %d\n", (u32)packet, len);
    memcpy(dec->frame_data, packet, len);
    adapter_wireless_rx_free_packet(packet);
    local_irq_disable();
    if (dec->suspend == 1 && frame_num < 1) {
        dec->suspend = 2;
        dec->suspend_timeout = jiffies + msecs_to_jiffies(30);
        audio_mixer_ch_pause(&dec->mix_ch, 1);
    } else {
        dec->suspend = 0;
    }

    if (dec->suspend == 2) {
        if (time_before(jiffies, dec->suspend_timeout)) {
            dec->wait_resume = 1;
            local_irq_enable();
            return 0;
        }
        /*printf("--resume--\n");*/
        audio_mixer_ch_pause(&dec->mix_ch, 0);
        dec->suspend = 0;
    }
    local_irq_enable();
    *frame = (void *)dec->frame_data;//packet;

    if (config_vendor_le_bb & VENDOR_BB_TS) {
        return len - 2;
    }
    return len;
}

// 解码释放数据空间
static void adapter_wireless_dec_put_frame(struct audio_decoder *decoder, u8 *frame)
{
    //struct __adapter_wireless_dec *dec = container_of(decoder, struct __adapter_wireless_dec, decoder);

    //if (frame) {
    //    adapter_wireless_rx_free_packet((void *)(frame));
    //}
}

// 解码查询数据
static int adapter_wireless_dec_fetch_frame(struct audio_decoder *decoder, u8 **frame)
{
    struct __adapter_wireless_dec *dec = container_of(decoder, struct __adapter_wireless_dec, decoder);
    u8 *packet = NULL;
    int len = 0;
    u32 wait_timeout = 0;

    if (!dec->start) {
        wait_timeout = jiffies + msecs_to_jiffies(500);
    }

__retry_fetch:
    packet = adapter_wireless_rx_fetch_packet(dec->rx, &len, NULL);
    if (packet) {
        *frame = packet;
    } else if (!dec->start) {
        // 解码启动前获取数据来做格式信息获取等
        if (time_before(jiffies, wait_timeout)) {
            os_time_dly(1);
            goto __retry_fetch;
        }
    }
    printf("adapter_wireless_dec_fetch_frame ok\n");
    return len;
}

static const struct audio_dec_input adapter_wireless_input = {
    .coding_type = AUDIO_CODING_JLA,
    .data_type   = AUDIO_INPUT_FRAME,
    .ops = {
        .frame = {
            .fget = adapter_wireless_dec_get_frame,
            .fput = adapter_wireless_dec_put_frame,
            .ffetch = adapter_wireless_dec_fetch_frame,
        }
    }
};


// 解码预处理
static int adapter_wireless_dec_probe_handler(struct audio_decoder *decoder)
{
    return 0;
}

// 解码后处理
static int adapter_wireless_dec_post_handler(struct audio_decoder *decoder)
{
    return 0;
}
static const struct audio_dec_handler adapter_wireless_dec_handler = {
    .dec_probe  = adapter_wireless_dec_probe_handler,
    .dec_post   = adapter_wireless_dec_post_handler,
};



// 解码关闭
static void adapter_wireless_dec_res_close(struct __adapter_wireless_dec *dec)
{
    if ((dec == NULL) || (dec->start == 0)) {
        printf("adapter_wireless_dec->start == 0");
        return ;
    }

    // 关闭数据流节点
    dec->start = 0;

    audio_decoder_close(&dec->decoder);

    if (dec->mixer) {
        if (dec->audio_stream) {
#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
    )
            wl_stream_delay_filter_free(dec);
#endif
            audio_mixer_ch_close(&dec->mix_ch);
            audio_stream_close((struct audio_stream *)dec->audio_stream);
            dec->audio_stream = NULL;
        }
    } else {
        adapter_audio_stream_close((struct adapter_audio_stream **)&dec->adapter_stream);
    }

    //app_audio_state_exit(APP_AUDIO_STATE_MUSIC);
}

// 解码事件处理
static void adapter_wireless_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    struct __adapter_wireless_dec *dec = container_of(decoder, struct __adapter_wireless_dec, decoder);
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        printf("AUDIO_DEC_EVENT_END\n");
        //adapter_wireless_dec_close();
        break;
    }
}

static void adapter_audio_stream_resume(void *p)
{
    struct audio_decoder *decoder = (struct audio_decoder *)p;
    struct __adapter_wireless_dec *dec = container_of(decoder, struct __adapter_wireless_dec, decoder);

    local_irq_disable();
    if (!dec->wait_resume) {
        audio_decoder_resume(decoder);
    }
    local_irq_enable();
}


// 解码start
static int adapter_wireless_dec_start(struct __adapter_wireless_dec *dec)
{
    int err;
    struct audio_fmt *fmt;

    if (!dec) {
        return -EINVAL;
    }

    printf("adapter_wireless_dec_start: in\n");

    // 打开adapter_wireless解码
    err = audio_decoder_open(&dec->decoder, &adapter_wireless_input, adapter_decoder_get_task_handle());
    if (err) {
        goto __err1;
    }

    // 设置运行句柄
    audio_decoder_set_handler(&dec->decoder, &adapter_wireless_dec_handler);

    struct audio_fmt f = {0};
    f.coding_type = AUDIO_CODING_JLA;
    f.channel = WIRELESS_DECODE_CHANNEL_NUM;
    f.sample_rate = WIRELESS_DECODE_SAMPLERATE;//44100;
    f.bit_rate = 64000;//解码码率可以不设置， 跟编码
    f.frame_len = WIRELESS_CODING_FRAME_LEN;

    dec->decoder.fmt.channel = f.channel;
    dec->decoder.fmt.sample_rate = f.sample_rate;
    dec->decoder.fmt.bit_rate = f.bit_rate;
    dec->decoder.fmt.frame_len = f.frame_len;

#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
        )
    dec->ref_delay_time = WL_LOW_DELAY_LIMIT;
    dec->ref_delay_cnt = 10;
#endif

    err = audio_decoder_set_fmt(&dec->decoder, &f);
    if (err) {
        goto __err2;
    }

    // 使能事件回调
    audio_decoder_set_event_handler(&dec->decoder, adapter_wireless_dec_event_handler, 0);

    // 设置输出声道类型
//#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
//    audio_decoder_set_output_channel(&dec->decoder, AUDIO_CH_DIFF);
//#else
    audio_decoder_set_output_channel(&dec->decoder, dec->dec_output_type);

//#endif//#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)


    if (dec->mixer) {
        audio_mixer_ch_open(&dec->mix_ch, dec->mixer);

#if TCFG_MIXER_CYCLIC_TASK_EN
        audio_mixer_ch_set_output_buf(&dec->mix_ch, dec->mixer_ch_buf, sizeof(dec->mixer_ch_buf));
#endif

        dec->mixer_ch_index = dec->media_parm->ch_index;

#if WIRELESS_TX_MIC_STEREO_OUTPUT
        if (dec->media_parm->mixer_output_select) {
            //真立体声
            audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 0,  BIT(dec->mixer_ch_index));
        } else {
            //假立体声
            audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 0,  BIT(0) | BIT(1));
        }
#endif
        //audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 18); // 超时自动丢数

        struct audio_stream_entry *entries[8] = {NULL};
        u8 entry_cnt = 0;
        entries[entry_cnt++] = &dec->decoder.entry;
#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
    )
        entries[entry_cnt++] = wl_stream_delay_filter_setup(dec, &dec->mix_ch);
#endif
        entries[entry_cnt++] = &dec->mix_ch.entry;
        dec->audio_stream = (void *)audio_stream_open(&dec->decoder, adapter_audio_stream_resume);
        audio_stream_add_list(dec->audio_stream, entries, entry_cnt);


        printf("%s output  mixer\n", __FUNCTION__);
    } else {
        //数据流初始化
        dec->adapter_stream = (void *)adapter_audio_stream_open(
                                  dec->fmt->list,
                                  dec->fmt->list_num,
                                  &dec->decoder,
                                  adapter_audio_stream_resume,
                                  dec->media_parm
                              );
        if (dec->adapter_stream == NULL) {
            goto __err3;
        }
    }

#if (\
        (APP_MAIN == APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)\
        ||(APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER && WIRELESS_EARPHONE_MIC_EN)\
    )
    // 开始解码
    dec->suspend = 0;
    dec->prev_ts = -WL_ILLEGAL_INTERVAL;
#endif
    dec->start = 1;
    err = audio_decoder_start(&dec->decoder);
    if (err) {
        goto __err3;
    }
    //clock_set_cur();


    return 0;

__err3:
    dec->start = 0;
    if (dec->mixer) {
        if (dec->audio_stream) {
            audio_mixer_ch_close(&dec->mix_ch);
            audio_stream_close((struct audio_stream *)dec->audio_stream);
            dec->audio_stream = NULL;
        }
    } else {
        adapter_audio_stream_close((struct adapter_audio_stream **)&dec->adapter_stream);
    }
__err2:
    audio_decoder_close(&dec->decoder);
__err1:
    //发事件释放数据流
    adapter_process_event_notify(ADAPTER_EVENT_MEDIA_DECODE_ERR, 0);

    return err;
}

static int adapter_wireless_dec_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    struct __adapter_wireless_dec *hdl = container_of(wait, struct __adapter_wireless_dec, wait);

    y_printf("adapter_wireless_dec_wait_res_handler: %d\n", event);

    if (event == AUDIO_RES_GET) {
        // 可以开始解码
        err = adapter_wireless_dec_start(hdl);
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        adapter_wireless_dec_res_close(hdl);
    }
    return err;
}

static void adapter_wireless_dec_timer_resume(void *p)
{
    struct __adapter_wireless_dec *dec = (struct __adapter_wireless_dec *)p;
    if (dec->start) {
        audio_decoder_resume(&dec->decoder);
        //putchar('B');
    }
}

struct __adapter_wireless_dec *adapter_wireless_dec_open_base(
    struct adapter_decoder_fmt *fmt,
    struct adapter_media_parm *media_parm,
    struct audio_mixer *mixer,
    struct adapter_audio_stream	*adapter_stream)
{
    ASSERT(media_parm);

    struct __adapter_wireless_dec *hdl = zalloc(sizeof(struct __adapter_wireless_dec));
    ASSERT(hdl);

    //数据接收配置
    hdl->rx = adapter_wireless_rx_open();
    hdl->rx->id = media_parm->ch_index;

    hdl->mixer = mixer;
    hdl->adapter_stream = adapter_stream;


    media_parm->ex_output = 0;
    //解码初始化
    hdl->fmt = fmt;
    hdl->media_parm = media_parm;
    hdl->dec_output_type = fmt->dec_output_type;

    hdl->coding_type = AUDIO_CODING_JLA;	// 解码类型
    hdl->wait.priority = 1;		// 解码优先级
    hdl->wait.preemption = 0;	// 不使能直接抢断解码

#if APP_MAIN == APP_WIRELESS_DUPLEX	//双工会使用电脑发下来的音量
    hdl->wait.protect = 0;	    // 叠加
#else
    hdl->wait.protect = 1;	    // 叠加
#endif
    hdl->wait.snatch_same_prio = 1;	// 可抢断同优先级解码
    hdl->wait.handler = adapter_wireless_dec_wait_res_handler;

    audio_decoder_task_add_wait(adapter_decoder_get_task_handle(), &hdl->wait);

    return hdl;
}




struct __adapter_wireless_dec *adapter_wireless_dec_open(
    struct adapter_decoder_fmt *fmt,
    struct adapter_media_parm *media_parm)
{
    return adapter_wireless_dec_open_base(fmt, media_parm, NULL, NULL);
}


// 解码释放
void adapter_wireless_dec_close(struct __adapter_wireless_dec **hdl)
{
    if (hdl && (*hdl)) {
        struct __adapter_wireless_dec *dec = *hdl;
        adapter_wireless_dec_res_close(dec);
        audio_decoder_task_del_wait(adapter_decoder_get_task_handle(), &dec->wait);
        adapter_wireless_rx_close(&dec->rx);

        // 释放空间
        local_irq_disable();
        free(dec);
        *hdl = NULL;
        local_irq_enable();
    }
}

void adapter_wireless_dec_set_vol(struct __adapter_wireless_dec *dec, u32 channel, u8 vol)
{
    if (dec && (dec->mixer == NULL)) {
        adapter_audio_stream_set_vol(dec->adapter_stream, channel, vol);
    }
}

int adapter_wireless_dec_ioctrl(struct __adapter_wireless_dec *dec, int cmd, int argc, int *argv)
{
    int value = 0;
    u8 echo_value = 0;
    if (dec) {
        switch (cmd) {
        case ADAPTER_DEC_IOCTRL_CMD_DENOISE_SET_PARM:
            if (dec->mixer == NULL) {
                adapter_audio_stream_denoise_set_parm(dec->adapter_stream, (struct __stream_HighSampleRateSingleMicSystem_parm *)argv);
            }
            break;
        case ADAPTER_DEC_IOCTRL_CMD_DENOISE_SWITCH:
            if (dec->mixer == NULL) {
                adapter_audio_stream_denoise_switch(dec->adapter_stream, (u8)argv);
            }
            break;
        case ADAPTER_DEC_IOCTRL_CMD_ECHO_EN:

            if (dec->mixer == NULL) {
                value = (int)argv;
                if (value == WIRELESS_MIC_ECHO_ON) {
                    echo_value = 0;
                } else {
                    echo_value = 1;
                }
                adapter_audio_stream_echo_run(dec->adapter_stream, echo_value);
            }
            break;
        case ADAPTER_DEC_IOCTRL_CMD_REGISTER_COMMAND_CALLBACK:
            if (argc != 2) {
                printf("%s, param err!!\n", __FUNCTION__);
                break;
            }
            adapter_wireless_rx_command_callback_register(dec->rx, (void *)argv[1], (void *)argv[0]);
            break;
        case ADAPTER_DEC_IOCTRL_CMD_GET_RX_PACKET_NUM:
            return adapter_wireless_dec_get_frame_packet_num(dec);

        case ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_SWITCH:
            if (dec->mixer) {
                dec->mixer_ch_switch = (u8)argv;
                dec->media_parm->mixer_output_select = dec->mixer_ch_switch;
                if (dec->mixer_ch_switch) {
                    audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 0,  BIT(dec->mixer_ch_index));
                } else {
                    audio_mixer_ch_set_aud_ch_out(&dec->mix_ch, 0,  BIT(0) | BIT(1));
                }
            }
            break;
        case ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_PAUSE:
            value = (int)argv;

            //printf("pause = %d,ch = %d\n", value, dec->mixer_ch_index);
            if (dec->mixer) {
                //printf("pause = %d,ch = %d\n", value, dec->mixer_ch_index);
                if (dec->mixer_ch_flag == 0) {
                    if (value == 0) {
                        dec->mixer_ch_flag = 1;
                        //printf("first pause = %d,ch = %d\n", value, dec->mixer_ch_index);
                        audio_mixer_ch_pause(&dec->mix_ch, 0);
                    }
                }
                if (value) {
                    dec->mixer_ch_flag = 0;
                    //printf("__pause = %d,ch = %d\n", value, dec->mixer_ch_index);
                    audio_mixer_ch_pause(&dec->mix_ch, 1);
                }
            }
            break;
        case ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_PAUSE_GET_STATUS:
            if (dec->mixer) {
                return dec->mix_ch.pause;
            }
            break;
        case ADAPTER_DEC_IOCTRL_CMD_DAC_GAIN_SET:
            adapter_audio_stream_dac_gain_set(dec->adapter_stream, (u8)argv);
            break;
        }
    }
    return 0;
}

