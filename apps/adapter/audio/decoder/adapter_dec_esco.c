#include "adapter_dec_esco.h"
#include "adapter_media.h"
#include "clock_cfg.h"
#include "media/includes.h"
#include "media/esco_decoder.h"

//通话丢包修复
#define TCFG_ADAPTER_ESCO_PLC	0
#if TCFG_ADAPTER_ESCO_PLC
#include "PLC.h"
#define PLC_FRAME_LEN	60
#endif/*TCFG_ADAPTER_ESCO_PLC*/


struct __adapter_dec_esco {
    struct adapter_media_parm 		*media_parm;	// 动态参数
    struct adapter_decoder_fmt  	*fmt;			// 解码参数配置
    struct adapter_audio_stream 	*stream;		// 音频流
    struct esco_decoder 			dec; 			// esco解码句柄
    struct audio_res_wait 			wait;			// 资源等待句柄
    u8 								remain;			// 未输出完成
#if TCFG_ADAPTER_ESCO_PLC
    void *plc;				// 丢包修护
#endif


};

struct __adapter_dec_esco *esco_dec = NULL;
#define  __this  esco_dec

#if TCFG_ADAPTER_ESCO_PLC
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护初始化
   @param
   @return   丢包修护句柄
   @note
*/
/*----------------------------------------------------------------------------*/
static void *esco_plc_init(void)
{
    void *plc = malloc(PLC_query()); /*buf_size:1040*/
    //plc = zalloc_mux(PLC_query());
    log_i("PLC_buf:0x%x,size:%d\n", (int)plc, PLC_query());
    if (!plc) {
        return NULL;
    }
    int err = PLC_init(plc);
    if (err) {
        log_i("PLC_init err:%d", err);
        free(plc);
        plc = NULL;
    }
    return plc;
}
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护关闭
   @param    *plc: 丢包修护句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_plc_close(void *plc)
{
    free(plc);
}
/*----------------------------------------------------------------------------*/
/**@brief    esco丢包修护运行
   @param    *data: 数据
   @param    len: 数据长度
   @param    repair: 修护标记
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void esco_plc_run(s16 *data, u16 len, u8 repair)
{
    u16 repair_point, tmp_point;
    s16 *p_in, *p_out;
    p_in    = data;
    p_out   = data;
    tmp_point = len / 2;

#if 0	//debug
    static u16 repair_cnt = 0;
    if (repair) {
        repair_cnt++;
        y_printf("[E%d]", repair_cnt);
    } else {
        repair_cnt = 0;
    }
    //log_i("[%d]",point);
#endif/*debug*/

    while (tmp_point) {
        repair_point = (tmp_point > PLC_FRAME_LEN) ? PLC_FRAME_LEN : tmp_point;
        tmp_point = tmp_point - repair_point;
        PLC_run(p_in, p_out, repair_point, repair);
        p_in  += repair_point;
        p_out += repair_point;
    }
}
#endif


extern int lmp_private_esco_suspend_resume(int flag);

u16 adapter_esco_dec_get_samplerate(void *priv)
{
    u16 sample_rate = 0;
    if (__this) {
        sample_rate = __this->dec.sample_rate;
    }
    return sample_rate;
}

static void adapter_dec_esco_release()
{
    audio_decoder_task_del_wait(adapter_decoder_get_task_handle(), &__this->wait);
    if (__this->dec.coding_type == AUDIO_CODING_MSBC) {
        clock_remove(DEC_MSBC_CLK);
    } else if (__this->dec.coding_type == AUDIO_CODING_CVSD) {
        clock_remove(DEC_CVSD_CLK);
    }

    local_irq_disable();
    free(__this);
    __this = NULL;
    local_irq_enable();
}


static void adapter_dec_esco_res_close(void)
{
    /*
     *先关闭aec，里面有复用到enc的buff，再关闭enc，
     *如果没有buf复用，则没有先后顺序要求。
     */
    if (__this == NULL) {
        return ;
    }
    if (!__this->dec.start) {
        return ;
    }

    __this->dec.start = 0;
    __this->dec.enc_start = 0;
    esco_decoder_close(&__this->dec);
#if TCFG_ADAPTER_ESCO_PLC
    if (__this->plc) {
        esco_plc_close(__this->plc);
        __this->plc = NULL;
    }
#endif/*TCFG_ADAPTER_ESCO_PLC*/


    // 先关闭各个节点，最后才close数据流
    adapter_audio_stream_close(&__this->stream);
}

static void adapter_dec_esco_close()
{
    if (!__this) {
        return;
    }
    adapter_dec_esco_res_close();
    adapter_dec_esco_release();
    g_printf("adapter_dec_esco_close: exit\n");
    clock_set_cur();
}

static void adapter_dec_esco_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        g_printf("AUDIO_DEC_EVENT_END\n");
        adapter_dec_esco_close();
        break;
    }
}

static int adapter_dec_esco_data_handler(struct audio_stream_entry *entry,
        struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    struct audio_decoder *decoder = container_of(entry, struct audio_decoder, entry);
    struct esco_decoder *esco_dec = container_of(decoder, struct esco_decoder, decoder);
    struct __adapter_dec_esco *dec = container_of(esco_dec, struct __adapter_dec_esco, dec);
    if (dec->remain == 0) {
        ///run what you want
    }
#if TCFG_ADAPTER_ESCO_PLC
    if (dec->plc && out && out->data) {
        esco_plc_run(in->data, in->data_len, *(u8 *)out->data);
    }
#endif/*TCFG_ADAPTER_ESCO_PLC*/


    int wlen = esco_decoder_output_handler(&dec->dec, in);

    if (in->data_len != wlen) {
        dec->remain = 1;
    } else {
        dec->remain = 0;
    }
    return wlen;
}

void esco_rx_notice_to_decode(void)
{
    if (__this && __this->dec.start) {
        /* audio_decoder_resume(&__this->dec.decoder); */
        if (__this->dec.wait_resume) {
            __this->dec.wait_resume = 0;
            audio_decoder_resume(&__this->dec.decoder);
        }
    }
}

static int adapter_dec_esco_start()
{
    int err;
    struct __adapter_dec_esco *dec = __this;

    if (!__this) {
        return -EINVAL;
    }
    // 打开esco解码
    err = esco_decoder_open(&dec->dec, adapter_decoder_get_task_handle());
    if (err) {
        goto __err;
    }

    // 使能事件回调
    audio_decoder_set_event_handler(&dec->dec.decoder, adapter_dec_esco_event_handler, 0);
    dec->dec.decoder.entry.data_handler = adapter_dec_esco_data_handler;

    // 数据流串联
    __this->stream = adapter_audio_stream_open(
                         __this->fmt->list,
                         __this->fmt->list_num,
                         &__this->dec.decoder,
                         NULL,
                         __this->media_parm
                     );
    if (__this->stream == NULL) {
        goto __err;
    }

#if TCFG_ADAPTER_ESCO_PLC
    // 丢包修护
    __this->plc = esco_plc_init();
#endif

//#if TCFG_ESCO_LIMITER
//    // 限福器
//    dec->limiter = esco_limiter_init(dec->dec.sample_rate);
//#endif /* TCFG_ESCO_LIMITER */

    lmp_private_esco_suspend_resume(2);
    // 开始解码
    dec->dec.start = 1;
    err = audio_decoder_start(&dec->dec.decoder);
    if (err) {
        goto __err;
    }
    dec->dec.frame_get = 0;

    //数据流里面已经启动了编码
    dec->dec.enc_start = 1;

    g_printf("adapter_dec_esco_open ok~~~~~~~\n");
    clock_set_cur();
    return 0;

__err:
    dec->dec.start = 0;
    esco_decoder_close(&dec->dec);
    adapter_dec_esco_release();
    return err;
}


static int adapter_dec_esco_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    g_printf("adapter_dec_esco_wait_res_handler %d\n", event);
    if (event == AUDIO_RES_GET) {
        err = adapter_dec_esco_start();
    } else if (event == AUDIO_RES_PUT) {
        if (__this->dec.start) {
            lmp_private_esco_suspend_resume(1);
            adapter_dec_esco_res_close();
        }
    }

    return err;
}

int adapter_dec_esco_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm)
{
    if (__this) {
        printf("adapter_dec_esco_open aready, fail!!\n");
        return -1;
    }
    ASSERT(media_parm);

    int err;
    u32 esco_param = *(u32 *)media_parm->priv_data;
    int esco_len = esco_param >> 16;
    int codec_type = esco_param & 0x000000ff;

    g_printf("%s, type=%d,len=%d\n", __FUNCTION__, codec_type, esco_len);

    struct __adapter_dec_esco *hdl = zalloc(sizeof(struct __adapter_dec_esco));
    if (!hdl) {
        return -ENOMEM;
    }


    hdl->fmt = fmt;
    hdl->media_parm = media_parm;

    hdl->dec.esco_len = esco_len;
    hdl->dec.out_ch_num = 1;
    if (codec_type == 3) {
        g_printf("esco dec AUDIO_CODING_MSBC\n");
        hdl->dec.coding_type = AUDIO_CODING_MSBC;
        hdl->dec.sample_rate = 16000;
        hdl->dec.ch_num = 1;
        clock_add(DEC_MSBC_CLK);
    } else if (codec_type == 2) {
        g_printf("esco dec AUDIO_CODING_CVSD\n");
        hdl->dec.coding_type = AUDIO_CODING_CVSD;
        hdl->dec.sample_rate = 8000;
        hdl->dec.ch_num = 1;
        clock_add(DEC_CVSD_CLK);
    } else {
        g_f_printf("esco dec parm err!!!\n");
        return -1;
    }


    hdl->wait.protect = 1;
    hdl->wait.priority = 2;		// 解码优先级
    hdl->wait.preemption = 0;	// 不使能直接抢断解码
    hdl->wait.snatch_same_prio = 1;	// 可抢断同优先级解码
    hdl->wait.handler = adapter_dec_esco_wait_res_handler;

    local_irq_disable();
    __this = hdl;
    local_irq_enable();

    err = audio_decoder_task_add_wait(adapter_decoder_get_task_handle(), &hdl->wait);
    if (__this && (__this->dec.start == 0)) {
        lmp_private_esco_suspend_resume(1);
    }

    g_printf("adapter_dec_esco_open ok!!!!!!!\n");
    return 0;
}

static void adapter_dec_esco_set_vol(u32 channel, u8 vol)
{
    if (__this && __this->stream) {
        //printf("%s,channel = %d,vol = %d", __func__, channel, vol);
        adapter_audio_stream_set_vol(__this->stream, channel, vol);
    }
}

REGISTER_ADAPTER_DECODER(decoder_esco) = {
    .dec_type = ADAPTER_DEC_TYPE_ESCO,
    .open = adapter_dec_esco_open,
    .close = adapter_dec_esco_close,
    .set_vol = adapter_dec_esco_set_vol,
};

