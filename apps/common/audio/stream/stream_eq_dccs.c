
#include "app_config.h"
#include "stream_eq_dccs.h"
#include "application/audio_eq.h"
#include "asm/hw_eq.h"

#if TCFG_EQ_ENABLE

#define AEC_DCCS_EN				TCFG_AEC_DCCS_EQ_ENABLE
#if AEC_DCCS_EN
struct audio_aec_dccs_hdl {
#if AEC_DCCS_EN
    struct audio_eq dccs_eq;
    struct hw_eq_ch dccs_eq_ch;
#endif/*AEC_DCCS_EN*/

};
struct audio_aec_dccs_hdl *aec_hdl = NULL;
extern struct adc_platform_data adc_data;

const struct eq_seg_info dccs_eq_tab_44k1[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 200,   0 << 20, (int)(0.7f * (1 << 24))},
};
const struct eq_seg_info dccs_eq_tab_16k[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 100,   0 << 20, (int)(0.7f * (1 << 24))},
};
static float dccs_tab[5];
int aec_dccs_eq_filter(void *eq, int sr, struct audio_eq_filter_info *info)
{
    if (!sr) {
        sr = 16000;
    }
    u8 nsection = ARRAY_SIZE(dccs_eq_tab_44k1);
    local_irq_disable();
    if (sr < 32000) {
        for (int i = 0; i < nsection; i++) {
            eq_seg_design((struct eq_seg_info *)&dccs_eq_tab_16k[i], sr, &dccs_tab[5 * i]);
        }
    } else {
        for (int i = 0; i < nsection; i++) {
            eq_seg_design((struct eq_seg_info *)&dccs_eq_tab_44k1[i], sr, &dccs_tab[5 * i]);
        }
    }
    local_irq_enable();

    info->L_coeff = (float *)dccs_tab;
    info->R_coeff = (float *)dccs_tab;
    info->L_gain = 0;
    info->R_gain = 0;
    info->nsection = nsection;
    return 0;
}
static int stream_dccs_eq_output(void *priv, void *data, u32 len)
{
    return 0;
}

static void stream_dccs_output_data_process_len(struct audio_stream_entry *entry, int len)
{

}
static int stream_dccs_data_handler(struct audio_stream_entry *entry,
                                    struct audio_data_frame *in,
                                    struct audio_data_frame *out)
{

    if (aec_hdl->dccs_eq.start)	{
        out->data = in->data;
        out->data_len = in->data_len;
        out->channel = in->channel;
        out->sample_rate = in->sample_rate;
        audio_eq_run(&aec_hdl->dccs_eq, in->data, in->data_len);
    }
    return in->data_len;

}

struct stream_eq_dccs *stream_eq_dccs_open(u16 sample_rate)
{
    struct stream_eq_dccs *stream = zalloc(sizeof(struct stream_eq_dccs));
    ASSERT(stream);

    aec_hdl = zalloc(sizeof(struct audio_aec_dccs_hdl));
    if (aec_hdl == NULL) {
        printf("aec_hdl malloc failed");
        return -ENOMEM;
    }

#if AEC_DCCS_EN
    printf("adc_data.mic_capless = %d", adc_data.mic_capless);
    if (adc_data.mic_capless) {
        memset(&aec_hdl->dccs_eq, 0, sizeof(struct audio_eq));
        memset(&aec_hdl->dccs_eq_ch, 0, sizeof(struct hw_eq_ch));
        aec_hdl->dccs_eq.eq_ch = &aec_hdl->dccs_eq_ch;
        struct audio_eq_param dccs_eq_param;
        dccs_eq_param.channels = 1;
        dccs_eq_param.online_en = 0;
        dccs_eq_param.mode_en = 0;
        dccs_eq_param.remain_en = 0;
        dccs_eq_param.max_nsection = EQ_SECTION_MAX;
        dccs_eq_param.cb = aec_dccs_eq_filter;
        audio_eq_open(&aec_hdl->dccs_eq, &dccs_eq_param);
        audio_eq_set_samplerate(&aec_hdl->dccs_eq, sample_rate);
        audio_eq_set_output_handle(&aec_hdl->dccs_eq, stream_dccs_eq_output, NULL);
        audio_eq_start(&aec_hdl->dccs_eq);

        stream->entry.data_process_len = stream_dccs_output_data_process_len;
        stream->entry.data_handler = stream_dccs_data_handler;
        return stream;
    } else {
        return NULL;
    }
#endif
}
void stream_eq_dccs_close(struct stream_eq_dccs **hdl)
{
    if (hdl && (*hdl)) {
        struct stream_eq_dccs *stream = *hdl;
        audio_stream_del_entry(&stream->entry);
#if AEC_DCCS_EN
        if (aec_hdl) {
            if (aec_hdl->dccs_eq.start) {
                audio_eq_close(&aec_hdl->dccs_eq);
                local_irq_disable();
                free(aec_hdl);
                aec_hdl = NULL;
                local_irq_enable();
            }
        }
#endif
        local_irq_disable();
        free(stream);
        *hdl = NULL;
        local_irq_enable();
    }
}


#endif
#endif
