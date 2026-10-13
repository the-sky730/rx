#include "adapter_decoder.h"

#define ADAPTER_MAX_SRC_NUMBER      		4 // 最大支持src个数

static struct audio_decoder_task 	adapter_decode_task = {0};
static u8  adapter_src_hw_filt[SRC_FILT_POINTS * SRC_CHI * 2 * ADAPTER_MAX_SRC_NUMBER] ALIGNED(4); /*SRC的滤波器必须4个byte对齐*/

void adapter_decoder_init(void)
{
    int err = audio_decoder_task_create(&adapter_decode_task, "audio_dec");

    adapter_decode_task.occupy.pend_time = 1;
    adapter_decode_task.occupy.idle_expect = 4;
    adapter_decode_task.occupy.trace_period = 200;
    //adapter_decode_task.occupy.trace_hdl = audio_dec_occupy_trace_hdl;

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    audio_src_base_filt_init(adapter_src_hw_filt, sizeof(adapter_src_hw_filt));
}

struct audio_decoder_task *adapter_decoder_get_task_handle(void)
{
    return 	&adapter_decode_task;
}

struct adapter_decoder *adapter_decoder_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm)
{
    if (fmt == NULL) {
        return NULL;
    }
    struct adapter_decoder *dec = NULL;
    list_for_each_adapter_decoder(dec) {
        if (dec->dec_type == fmt->dec_type) {
            if (dec->open) {
                dec->open(fmt, media_parm);
                return dec;
            }
        }
    }
    return NULL;
}

void adapter_decoder_close(struct adapter_decoder *dec)
{
    if (dec && dec->close) {
        dec->close();
    }
}

void adapter_decoder_set_vol(struct adapter_decoder *dec, u32 channel, u8 vol)
{
    if (dec && dec->set_vol) {
        dec->set_vol(channel, vol);
    }
}

int adapter_decoder_ioctrl(struct adapter_decoder *dec, int cmd, int argc, int *argv)
{
    if (dec && dec->ioctrl) {
        return dec->ioctrl(cmd, argc, argv);
    }
    return 0;
}




