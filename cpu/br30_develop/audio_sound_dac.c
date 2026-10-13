#include "app_config.h"
#include "media/includes.h"
#include "audio_sound_dac.h"
#include "audio_config.h"
#include "application/audio_output_dac.h"

extern struct audio_dac_hdl dac_hdl;

static int audio_sound_dac_status = 0;	// dac 是否已经初始化
static int audio_sound_dac_dma_status = 0;	// dac dma是否已经初始化
static int audio_sound_dac_cnt = 0;		// dac 打开的次数

struct sound_dac_struct {
    struct list_head sound_dac_head;/*DAC 子设备表头*/
};
static struct sound_dac_struct  sound_dac_list;

void audio_sound_dac_init(void)
{
    if (audio_sound_dac_status == 0) {
        audio_sound_dac_status = 1;
        app_audio_output_init();
        INIT_LIST_HEAD(&(sound_dac_list.sound_dac_head));
    }
}

void audio_sound_dac_uninit(void)
{
    if (audio_sound_dac_cnt != 0) {
        printf("!!! error: audio_sound_dac_cnt != 0\n");
    }
    if (audio_sound_dac_status == 1) {
    }
}

void audio_sound_dac_list_resume(void)
{
    struct audio_sound_dac *sound_dac;
    list_for_each_entry(sound_dac, &(sound_dac_list.sound_dac_head), sound_dac_entry) {
        if (sound_dac->wait_resume) {
            sound_dac->wait_resume = 0;
            audio_stream_resume(&sound_dac->entry);
        }
    }
}
static int audio_sound_dac_stream_data_handler(struct audio_stream_entry *entry,
        struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    struct audio_sound_dac *dac = container_of(entry, struct audio_sound_dac, entry);
    if (in->stop) {
        if (audio_sound_dac_dma_status) {
            audio_sound_dac_stop(dac);
        }
        return 0;
    } else {
        if (audio_sound_dac_dma_status == 0) {
            //audio_sound_dac_set_sample_rate(dac, in->sample_rate);
            u8 volume = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
            app_audio_set_volume(APP_AUDIO_CURRENT_STATE, volume, 1);
            audio_sound_dac_start(dac);
        }
    }
    if (in->data_len == 0) {
        return 0;
    }
    int wlen = audio_sound_dac_write(dac, in->data, in->data_len);
    if (wlen < in->data_len) {
        dac_hdl.need_resume = 1;
        dac->wait_resume = 1;
    }
    return wlen;
}

struct audio_sound_dac *audio_sound_dac_open(struct dac_param *param)
{
    int err = 0;
    struct audio_sound_dac *dac = zalloc(sizeof(struct audio_sound_dac));
    if (dac == NULL) {
        printf("audio_sound_dac_open malloc error\n");
        return NULL;
    }
    audio_sound_dac_init();
    audio_dac_set_resume_callback(&dac_hdl, audio_sound_dac_list_resume);
    // set sample_rate delay_ms
    audio_dac_new_channel(&dac_hdl, &dac->dac_channel);
    struct audio_dac_channel_attr attr = {0};
    audio_dac_channel_get_attr(&dac->dac_channel, &attr);
    attr.delay_time = param->delay_ms;
    attr.protect_time = param->protect_time;
    attr.write_mode = param->write_mode;
    attr.start_delay = param->start_delay;
    attr.underrun_time = param->underrun_time;
    audio_dac_channel_set_attr(&dac->dac_channel, &attr);

    audio_dac_set_sample_rate(&dac_hdl, param->sample_rate);

    audio_sound_dac_cnt++;
    dac->entry.data_handler = audio_sound_dac_stream_data_handler;
    list_add(&dac->sound_dac_entry, &(sound_dac_list.sound_dac_head));
    return dac;

_error:

    if (dac) {
        free(dac);
        return NULL;
    }
    return NULL;
}

int audio_sound_dac_close(struct audio_sound_dac *dac)
{
    if (dac == NULL) {
        return -1;
    }

    audio_sound_dac_cnt--;
    audio_dac_free_channel(&dac->dac_channel);
    list_del(&dac->sound_dac_entry);
    audio_sound_dac_stop(dac);
    free(dac);
    return 0;
}

int audio_sound_dac_start(struct audio_sound_dac *dac)
{
    if (dac == NULL) {
        return -1;
    }
    audio_dac_start(&dac_hdl);
    audio_sound_dac_dma_status = 1;
    return 0;
}

int audio_sound_dac_stop(struct audio_sound_dac *dac)
{
    if (dac == NULL) {
        return -1;
    }
    if (audio_sound_dac_cnt != 0) {
        return -1;
    }
    audio_dac_stop(&dac_hdl);
    audio_sound_dac_dma_status = 0;
    return 0;
}

int audio_sound_dac_write(struct audio_sound_dac *dac, s16 *data, u32 len)
{
    if (dac == NULL || data == NULL) {
        return 0;
    }
    int wlen = audio_dac_write(&dac->dac_channel, data, len);
    return wlen;
}

int audio_sound_dac_get_data_time(struct audio_sound_dac *dac)
{
    if (dac == NULL) {
        return -1;
    }
    return audio_dac_data_time(&dac_hdl);
}

int audio_sound_dac_get_sample_rate(struct audio_sound_dac *dac)
{
    if (dac == NULL) {
        return -1;
    }


    return dac_hdl.sample_rate;
}
int audio_sound_dac_set_sample_rate(struct audio_sound_dac *dac, u32 sample_rate)
{
    if (dac == NULL || sample_rate == 0) {
        return -1;
    }

    audio_dac_set_sample_rate(&dac_hdl, sample_rate);

    return 0;
}

int audio_sound_dac_buffered_frames(struct audio_sound_dac *dac)
{
    return audio_dac_channel_buffered_frames(&dac->dac_channel);
}

int audio_sound_dac_set_analog_gain(struct audio_sound_dac *dac, u32 channel, int gain)
{
    if (dac == NULL) {
        return -1;
    }

    audio_dac_vol_set(TYPE_DAC_AGAIN, channel, gain, 1);
    return 0;
}

int audio_sound_dac_set_digital_gain(struct audio_sound_dac *dac, u32 channel, int gain)
{
    if (dac == NULL) {
        return -1;
    }
    audio_dac_vol_set(TYPE_DAC_DGAIN, channel, gain, 1);
    return 0;
}

int audio_sound_dac_power_on(void)
{
    audio_dac_start(&dac_hdl);
    audio_dac_stop(&dac_hdl);
}

void sound_dac_set_underrun_handler(struct audio_sound_dac *dac, void *priv, void (*handler)(void *, int))
{
    audio_dac_channel_set_underrun_handler(&dac->dac_channel, priv, handler);
}


#if 1 // audio_sound_dac demo

const s16 data_sin44100[441] = {
    0x0000, 0x122d, 0x23fb, 0x350f, 0x450f, 0x53aa, 0x6092, 0x6b85, 0x744b, 0x7ab5, 0x7ea2, 0x7fff, 0x7ec3, 0x7af6, 0x74ab, 0x6c03,
    0x612a, 0x545a, 0x45d4, 0x35e3, 0x24db, 0x1314, 0x00e9, 0xeeba, 0xdce5, 0xcbc6, 0xbbb6, 0xad08, 0xa008, 0x94fa, 0x8c18, 0x858f,
    0x8181, 0x8003, 0x811d, 0x84ca, 0x8af5, 0x9380, 0x9e3e, 0xaaf7, 0xb969, 0xc94a, 0xda46, 0xec06, 0xfe2d, 0x105e, 0x223a, 0x3365,
    0x4385, 0x5246, 0x5f5d, 0x6a85, 0x7384, 0x7a2d, 0x7e5b, 0x7ffa, 0x7f01, 0x7b75, 0x7568, 0x6cfb, 0x6258, 0x55b7, 0x4759, 0x3789,
    0x2699, 0x14e1, 0x02bc, 0xf089, 0xdea7, 0xcd71, 0xbd42, 0xae6d, 0xa13f, 0x95fd, 0x8ce1, 0x861a, 0x81cb, 0x800b, 0x80e3, 0x844e,
    0x8a3c, 0x928c, 0x9d13, 0xa99c, 0xb7e6, 0xc7a5, 0xd889, 0xea39, 0xfc5a, 0x0e8f, 0x2077, 0x31b8, 0x41f6, 0x50de, 0x5e23, 0x697f,
    0x72b8, 0x799e, 0x7e0d, 0x7fee, 0x7f37, 0x7bed, 0x761f, 0x6ded, 0x6380, 0x570f, 0x48db, 0x392c, 0x2855, 0x16ad, 0x048f, 0xf259,
    0xe06b, 0xcf20, 0xbed2, 0xafd7, 0xa27c, 0x9705, 0x8db0, 0x86ab, 0x821c, 0x801a, 0x80b0, 0x83da, 0x8988, 0x919c, 0x9bee, 0xa846,
    0xb666, 0xc603, 0xd6ce, 0xe86e, 0xfa88, 0x0cbf, 0x1eb3, 0x3008, 0x4064, 0x4f73, 0x5ce4, 0x6874, 0x71e6, 0x790a, 0x7db9, 0x7fdc,
    0x7f68, 0x7c5e, 0x76d0, 0x6ed9, 0x64a3, 0x5863, 0x4a59, 0x3acc, 0x2a0f, 0x1878, 0x0661, 0xf42a, 0xe230, 0xd0d0, 0xc066, 0xb145,
    0xa3bd, 0x9813, 0x8e85, 0x8743, 0x8274, 0x8030, 0x8083, 0x836b, 0x88da, 0x90b3, 0x9acd, 0xa6f5, 0xb4ea, 0xc465, 0xd515, 0xe6a3,
    0xf8b6, 0x0aee, 0x1ced, 0x2e56, 0x3ecf, 0x4e02, 0x5ba1, 0x6764, 0x710e, 0x786f, 0x7d5e, 0x7fc3, 0x7f91, 0x7cc9, 0x777a, 0x6fc0,
    0x65c1, 0x59b3, 0x4bd3, 0x3c6a, 0x2bc7, 0x1a41, 0x0833, 0xf5fb, 0xe3f6, 0xd283, 0xc1fc, 0xb2b7, 0xa503, 0x9926, 0x8f60, 0x87e1,
    0x82d2, 0x804c, 0x805d, 0x8303, 0x8833, 0x8fcf, 0x99b2, 0xa5a8, 0xb372, 0xc2c9, 0xd35e, 0xe4da, 0xf6e4, 0x091c, 0x1b26, 0x2ca2,
    0x3d37, 0x4c8e, 0x5a58, 0x664e, 0x7031, 0x77cd, 0x7cfd, 0x7fa3, 0x7fb4, 0x7d2e, 0x781f, 0x70a0, 0x66da, 0x5afd, 0x4d49, 0x3e04,
    0x2d7d, 0x1c0a, 0x0a05, 0xf7cd, 0xe5bf, 0xd439, 0xc396, 0xb42d, 0xa64d, 0x9a3f, 0x9040, 0x8886, 0x8337, 0x806f, 0x803d, 0x82a2,
    0x8791, 0x8ef2, 0x989c, 0xa45f, 0xb1fe, 0xc131, 0xd1aa, 0xe313, 0xf512, 0x074a, 0x195d, 0x2aeb, 0x3b9b, 0x4b16, 0x590b, 0x6533,
    0x6f4d, 0x7726, 0x7c95, 0x7f7d, 0x7fd0, 0x7d8c, 0x78bd, 0x717b, 0x67ed, 0x5c43, 0x4ebb, 0x3f9a, 0x2f30, 0x1dd0, 0x0bd6, 0xf99f,
    0xe788, 0xd5f1, 0xc534, 0xb5a7, 0xa79d, 0x9b5d, 0x9127, 0x8930, 0x83a2, 0x8098, 0x8024, 0x8247, 0x86f6, 0x8e1a, 0x978c, 0xa31c,
    0xb08d, 0xbf9c, 0xcff8, 0xe14d, 0xf341, 0x0578, 0x1792, 0x2932, 0x39fd, 0x499a, 0x57ba, 0x6412, 0x6e64, 0x7678, 0x7c26, 0x7f50,
    0x7fe6, 0x7de4, 0x7955, 0x7250, 0x68fb, 0x5d84, 0x5029, 0x412e, 0x30e0, 0x1f95, 0x0da7, 0xfb71, 0xe953, 0xd7ab, 0xc6d4, 0xb725,
    0xa8f1, 0x9c80, 0x9213, 0x89e1, 0x8413, 0x80c9, 0x8012, 0x81f3, 0x8662, 0x8d48, 0x9681, 0xa1dd, 0xaf22, 0xbe0a, 0xce48, 0xdf89,
    0xf171, 0x03a6, 0x15c7, 0x2777, 0x385b, 0x481a, 0x5664, 0x62ed, 0x6d74, 0x75c4, 0x7bb2, 0x7f1d, 0x7ff5, 0x7e35, 0x79e6, 0x731f,
    0x6a03, 0x5ec1, 0x5193, 0x42be, 0x328f, 0x2159, 0x0f77, 0xfd44, 0xeb1f, 0xd967, 0xc877, 0xb8a7, 0xaa49, 0x9da8, 0x9305, 0x8a98,
    0x848b, 0x80ff, 0x8006, 0x81a5, 0x85d3, 0x8c7c, 0x957b, 0xa0a3, 0xadba, 0xbc7b, 0xcc9b, 0xddc6, 0xefa2, 0x01d3, 0x13fa, 0x25ba,
    0x36b6, 0x4697, 0x5509, 0x61c2, 0x6c80, 0x750b, 0x7b36, 0x7ee3, 0x7ffd, 0x7e7f, 0x7a71, 0x73e8, 0x6b06, 0x5ff8, 0x52f8, 0x444a,
    0x343a, 0x231b, 0x1146, 0xff17, 0xecec, 0xdb25, 0xca1d, 0xba2c, 0xaba6, 0x9ed6, 0x93fd, 0x8b55, 0x850a, 0x813d, 0x8001, 0x815e,
    0x854b, 0x8bb5, 0x947b, 0x9f6e, 0xac56, 0xbaf1, 0xcaf1, 0xdc05, 0xedd3
};

void audio_sound_dac_demo(void)
{
    int ret = 0;
    audio_sound_dac_init();

    struct dac_param demo_dac_param = {0};
    demo_dac_param.sample_rate = 44100;
    demo_dac_param.delay_ms = 50;
    struct audio_sound_dac *demo_dac = audio_sound_dac_open(&demo_dac_param);
    if (demo_dac == NULL) {
        goto _error;
    }
    ret = audio_sound_dac_start(demo_dac);
    if (ret) {
        goto _error;
    }
    audio_sound_dac_set_analog_gain(demo_dac, 0xF, 3);
    audio_sound_dac_set_digital_gain(demo_dac, 0xF, 16384);

    extern const s16 data_sin44100[441];
    int len = 0;
    int wlen = 0;
    s16 *ptr;
    s16 *data_addr;
    u32 data_len;
    int res = 0;
    int msg[16];
    data_addr = data_sin44100;
    data_len = 441 * 2;
    while (1) {
        res = os_taskq_accept(ARRAY_SIZE(msg), msg);
        ptr = data_addr;
        len = data_len;
        while (len) {
            wlen = audio_sound_dac_write(demo_dac, ptr, len);
            if (wlen != len) {	// dac缓存满了，延时 10ms 后再继续写
                os_time_dly(1);
            }
            ptr += wlen / 2;
            len -= wlen;
        }
    }
    return;
_error:
    audio_sound_dac_stop(demo_dac);
    audio_sound_dac_close(demo_dac);
    audio_sound_dac_uninit();
    return;
}

#endif // audio_sound_dac demo



