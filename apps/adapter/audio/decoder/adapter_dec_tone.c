#include "adapter_dec_tone.h"
#include "system/includes.h"
#include "media/includes.h"
#include "adapter_audio_stream.h"
#include "media/mixer.h"
#include "media/audio_decoder.h"
#include "audio_sound_dac.h"
#include "tone_player.h"

struct tone_audio_stream {
    u8	vol_limit;												/*!< 音量范围，如：vol_limit = 100, 音量调节的范围为0~100,设置音量时候会转换成本地音量表的音量范围 */
    struct audio_stream *stream;								/*!< 音频流控制句柄 */
#if NEW_AUDIO_W_MIC
    struct audio_sound_dac *dac;
#else
    struct audio_dac_channel *dac;								/*!< dac节点控制句柄 */
#endif
    struct __stream_src 	*src;
    void *dvol;													/*!< 数字音量节点控制句柄 */
};

struct tone_dec_handle {
    FILE *file;
    char *list;
    enum audio_channel channel;
    struct audio_decoder decoder;
    struct audio_mixer_ch mix_ch;
    u32 magic;
    u8 start;
    u8 ch_num;
    u8 preemption;
    struct audio_res_wait wait;

    struct tone_audio_stream *audio;
#if NEW_AUDIO_W_MIC
    struct audio_sound_dac *dac;
#else
    struct audio_dac_channel *dac;								/*!< dac节点控制句柄 */
#endif
    void *priv;
    void (*cbk)(void *p);
};

static const u16 tone_dvol_table[] = {
    0	, //0
    93	, //1
    111	, //2
    132	, //3
    158	, //4
    189	, //5
    226	, //6
    270	, //7
    323	, //8
    386	, //9
    462	, //10
    552	, //11
    660	, //12
    789	, //13
    943	, //14
    1127, //15
    1347, //16
    1610, //17
    1925, //18
    2301, //19
    2751, //20
    3288, //21
    3930, //22
    4698, //23
    5616, //24
    6713, //25
    8025, //26
    9592, //27
    11466,//28
    15200,//29
    16000,//30
    16384 //31
};

static const audio_dig_vol_param tone_digital_vol_parm = {
    .vol_start = ARRAY_SIZE(tone_dvol_table) - 1,
    .vol_max = ARRAY_SIZE(tone_dvol_table) - 1,
    .ch_total = 2,
    .fade_en = 1,
    .fade_points_step = 2,
    .fade_gain_step = 2,
    .vol_list = (void *)tone_dvol_table,
};


static struct tone_dec_handle *tone_dec = NULL;
extern struct audio_mixer mixer;
extern struct audio_decoder_task decode_task;
void wl_mic_tone_play_stop(void);
void tone_audio_stream_close(struct tone_audio_stream **hdl);

struct tone_format {
    const char *fmt;
    u32 coding_type;
};
const struct tone_format tone_fmt_support_list[] = {
    {"wtg", AUDIO_CODING_G729},
    {"msbc", AUDIO_CODING_MSBC},
    {"sbc", AUDIO_CODING_SBC},
    {"mty", AUDIO_CODING_MTY},
    {"aac", AUDIO_CODING_AAC},
    {"wts", AUDIO_CODING_WTGV2},
    {"mp3", AUDIO_CODING_MP3},
    {"wav", AUDIO_CODING_WAV},
};

static int tone_fread(struct audio_decoder *decoder, void *buf, u32 len)
{
    int rlen = 0;

    if (!tone_dec->file) {
        return 0;
    }

    /* printf("read ~~~~~~~~~~~~~~~~~~\n"); */
    rlen = fread(tone_dec->file, buf, len);
    if (rlen < len) {
        mem_stats();
        fclose(tone_dec->file);
        tone_dec->file = NULL;
    }
    return rlen;
}

static int tone_fseek(struct audio_decoder *decoder, u32 offset, int seek_mode)
{
    if (!tone_dec->file) {
        return 0;
    }
    /* printf("seek ~~~~~~~~~~~~~~~~~~\n"); */
    return fseek(tone_dec->file, offset, seek_mode);
}

static int tone_flen(struct audio_decoder *decoder)
{
    void *tone_file = NULL;
    int len = 0;

    /* printf("flen ~~~~~~~~~~~~~~~~~~\n"); */
    if (tone_dec->file) {
        len = flen(tone_dec->file);
        return len;
    }

    tone_file = fopen(tone_dec->list, "r");
    if (tone_file) {
        len = flen(tone_file);
        fclose(tone_file);
        tone_file = NULL;
    }
    return len;
}

struct audio_dec_input tone_input = {
    .coding_type = AUDIO_CODING_G729,
    .data_type   = AUDIO_INPUT_FILE,
    .ops = {
        .file = {
            .fread = tone_fread,
            .fseek = tone_fseek,
            .flen  = tone_flen,
        }
    }
};
static u32 tone_file_format_match(char *fmt)
{
    int list_num = ARRAY_SIZE(tone_fmt_support_list);
    int i = 0;

    if (fmt == NULL) {
        return AUDIO_CODING_UNKNOW;
    }

    for (i = 0; i < list_num; i++) {
        if (ASCII_StrCmpNoCase(fmt, tone_fmt_support_list[i].fmt, 4) == 0) {
            return tone_fmt_support_list[i].coding_type;
        }
    }
    return AUDIO_CODING_UNKNOW;
}


static int tone_dec_probe_handler(struct audio_decoder *decoder)
{
    return 0;
}

static int tone_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    /* struct tone_dec_handle *dec = container_of(decoder, struct tone_dec_handle, decoder); */
    /* return audio_mixer_ch_write(&dec->mix_ch, data, len); */
    return 0;
}

static int tone_dec_post_handler(struct audio_decoder *decoder)
{
    return 0;
}

const struct audio_dec_handler tone_dec_handler = {
    .dec_probe  = tone_dec_probe_handler,
    .dec_output = tone_dec_output_handler,
    .dec_post   = tone_dec_post_handler,
};

static void tone_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    if (tone_dec == NULL) {
        return ;
    }
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        if (argv[1] != tone_dec->magic) {
            break;
        }
        printf("AUDIO_DEC_EVENT_END\n");
        wl_mic_tone_play_stop();
        break;
    }
}

static void adapter_audio_stream_resume(void *p)
{
    struct audio_decoder *decoder = (struct audio_decoder *)p;
    audio_decoder_resume(decoder);
}

static char *get_file_ext_name(char *name)
{
    int len = strlen(name);
    char *ext = (char *)name;

    while (len--) {
        if (*ext++ == '.') {
            break;
        }
    }

    return ext;
}
#define TONE_MAX_VOL		100
#define TONE_DEFAULT_VOL	50
static u8 tone_vol = TONE_DEFAULT_VOL;
void adapter_set_tone_vol(u8 vol)
{
    tone_vol = vol;
}
extern struct audio_dac_hdl dac_hdl;
static int tone_dec_start()
{
    int err;
    u8 file_name[16];
    struct audio_fmt *fmt;
    struct audio_fmt f = {0};
    tone_dec->audio = zalloc(sizeof(struct tone_audio_stream));
    if (tone_dec->audio == NULL) {
        return 0;
    }

    if (!tone_dec || !tone_dec->file) {
        return -EINVAL;
    }

    if (tone_dec->start) {
        return 0;
    }

    printf("tone_dec_start !\n");

    err = audio_decoder_open(&tone_dec->decoder, &tone_input, adapter_decoder_get_task_handle());
    if (err) {
        return err;
    }

    err = audio_decoder_get_fmt(&tone_dec->decoder, &fmt);
    if (err) {
        goto __err1;
    }

    tone_dec->magic = rand32();
    audio_decoder_set_event_handler(&tone_dec->decoder, tone_dec_event_handler, tone_dec->magic);

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
    audio_decoder_set_output_channel(&tone_dec->decoder, AUDIO_CH_LR);
#else
    audio_decoder_set_output_channel(&tone_dec->decoder, AUDIO_CH_DIFF);
#endif//#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)


    /* app_audio_state_switch(APP_AUDIO_STATE_MUSIC, 15); */
    /* app_audio_set_volume(APP_AUDIO_STATE_MUSIC, 15, 0); */



    //数据流初始化
    int j = 0;
    struct audio_stream_entry *entries[10] = {NULL};
    entries[j++] = &tone_dec->decoder.entry;


    tone_dec->audio->src = stream_src_open(WIRELESS_DECODE_SAMPLERATE, 1);
    if (tone_dec->audio->src) {
        entries[j++] = &tone_dec->audio->src->entry;
    }
#if 1
    tone_dec->audio->dvol = audio_dig_vol_open((audio_dig_vol_param *)&tone_digital_vol_parm);
    if (tone_dec->audio->dvol) {
        printf("tone dvol open succ");
        tone_dec->audio->vol_limit = TONE_MAX_VOL;
        //设置初始音量
        adapter_audio_stream_set_vol(tone_dec->audio, BIT(0), tone_vol);
        adapter_audio_stream_set_vol(tone_dec->audio, BIT(1), tone_vol);
        entries[j++] = audio_dig_vol_entry_get(tone_dec->audio->dvol);
    } else {
        printf("tone dvol open fail");
    }

#endif
#if NEW_AUDIO_W_MIC
    struct dac_param dac_param_t = {0};
    dac_param_t.sample_rate = WIRELESS_DECODE_SAMPLERATE;
    dac_param_t.write_mode = WRITE_MODE_BLOCK;
    dac_param_t.delay_ms = 50;
    dac_param_t.protect_time = 0;
    dac_param_t.start_delay = 6;
    dac_param_t.underrun_time = 0;
    tone_dec->audio->dac = audio_sound_dac_open(&dac_param_t);
#else
    if (audio_dac_init_status() == 0) {
        app_audio_output_init();//如果DAC没有开， 这里打开一下
    }
    tone_dec->audio->dac = (struct audio_dac_channel *)zalloc(sizeof(struct audio_dac_channel));
    if (tone_dec->audio->dac) {
        extern struct audio_dac_hdl dac_hdl;
        audio_dac_new_channel(&dac_hdl, tone_dec->audio->dac);
        struct audio_dac_channel_attr dac_attr = {0};
        dac_attr.write_mode = WRITE_MODE_BLOCK;
        dac_attr.delay_time = 50;
        dac_attr.protect_time = 0;
        dac_attr.start_delay = 6;
        dac_attr.underrun_time = 0;
        audio_dac_channel_set_attr(tone_dec->audio->dac, &dac_attr);
    }
#endif
    audio_sound_dac_set_analog_gain(tone_dec->audio->dac, 0xf, MAX_ANA_VOL);
    audio_sound_dac_set_digital_gain(tone_dec->audio->dac, 0xf, 16384);
    entries[j++] = &tone_dec->audio->dac->entry;

    tone_dec->audio->stream = audio_stream_open(&tone_dec->decoder, adapter_audio_stream_resume);
    audio_stream_add_list(tone_dec->audio->stream, entries, j);

__dec_start:

    tone_dec->start = 1;
    err = audio_decoder_start(&tone_dec->decoder);
    if (err) {
        goto __err2;
    }

    return 0;


__err2:
    tone_audio_stream_close(&tone_dec->audio);
    audio_decoder_close(&tone_dec->decoder);
__err1:
    free(tone_dec);
    tone_dec = NULL;
    return err;
}

void tone_audio_stream_close(struct tone_audio_stream **hdl)
{
    if (hdl == NULL || *hdl == NULL) {
        return ;
    }
    struct tone_audio_stream *audio = *hdl;
    //关闭其他的数据流节点
    if (audio->dac) {
#if NEW_AUDIO_W_MIC
        audio_stream_del_entry(&audio->dac->entry);
        audio_sound_dac_close(audio->dac);
        audio->dac = NULL;
#else
        audio_stream_del_entry(&audio->dac->entry);
        audio_dac_free_channel(audio->dac);
        free(audio->dac);
        audio->dac = NULL;
#endif
    }

    if (audio->src) {
        stream_src_close(&audio->src);
    }
    if (audio->dvol) {
        printf("close tone dvol");
        audio_stream_del_entry(audio_dig_vol_entry_get(audio->dvol));
        audio_dig_vol_close(audio->dvol);
    }

    //audio->stream必须在最后释放
    if (audio->stream) {
        audio_stream_close(audio->stream);
    }
    local_irq_disable();
    free(audio);
    *hdl = NULL;
    local_irq_enable();
}

static void tone_dec_stop()
{
    if (tone_dec->start == 0) {
        printf("tone_dec->start == 0");
        return ;
    }

    // 关闭数据流节点
    tone_dec->start = 0;
    audio_decoder_close(&tone_dec->decoder);

    tone_audio_stream_close(&tone_dec->audio);

    //app_audio_state_exit(APP_AUDIO_STATE_MUSIC);

    if (tone_dec->file) {
        fclose(tone_dec->file);
    }

    if (tone_dec->cbk) {
        tone_dec->cbk(tone_dec->priv);
    }
}

static int tone_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    if (event == AUDIO_RES_GET) {
        err = tone_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        tone_dec_stop();
    }
    return err;
}

extern int audio_codec_clock_set(u8 mode, u32 coding_type, u8 preemption);
int wl_mic_tone_play(const char *list, u8 preemption, void (*cbk)(void *priv), void *priv)
{
    int err;
    u8 file_name[16];
    char *format = NULL;
    FILE *file = NULL;
    struct tone_dec_handle *dec;

    printf("wl_mic_tone_play \n");
    if (tone_dec) {
        return 0;
    }

    dec = zalloc(sizeof(*dec));
    ASSERT(dec);


    file = fopen(list, "r");
    if (!file) {
        return -EINVAL;
    }
    fget_name(file, file_name, 16);
    format = get_file_ext_name((char *)file_name);
    printf("file name == %s", file_name);

    dec->list = list;
    dec->file = file;
    dec->cbk = cbk;
    dec->priv = priv;

    tone_dec = dec;

    tone_input.coding_type = tone_file_format_match((char *)format);
    if (tone_input.coding_type == AUDIO_CODING_UNKNOW) {
        log_e("unknow tone file format\n");
        fclose(file);
        free(dec);
        tone_dec = NULL;
        return -EINVAL;
    }
    //暂时只支持wtg和msbc格式
    if (tone_input.coding_type != AUDIO_CODING_MSBC && tone_input.coding_type != AUDIO_CODING_G729) {
        printf("only support WTG and MSBC \n");
        fclose(file);
        free(dec);
        tone_dec = NULL;
        return -EINVAL;
    }
    tone_dec->wait.priority = 3;
    tone_dec->wait.preemption = preemption;
    tone_dec->wait.protect = 0;
    tone_dec->wait.handler = tone_wait_res_handler;

    err = audio_decoder_task_add_wait(adapter_decoder_get_task_handle(), &tone_dec->wait);
    return err;
}

static void tone_dec_release(void)
{
    if (tone_dec) {
        // 删除解码资源等待
        audio_decoder_task_del_wait(adapter_decoder_get_task_handle(), &tone_dec->wait);

        // 释放空间
        local_irq_disable();
        free(tone_dec);
        tone_dec = NULL;
        local_irq_enable();
    }
}

// 关闭解码
void wl_mic_tone_play_stop(void)
{
    if (!tone_dec) {
        return ;
    }
    printf("wl_mic_tone_stop \n");

    if (tone_dec->start) {
        tone_dec_stop();
    }
    tone_dec_release();


    return ;
}

int wl_mic_tone_get_dec_status()
{
    if (tone_dec  && (tone_dec->decoder.state != DEC_STA_WAIT_STOP)) {
        return TONE_START;
    }
    return 	TONE_STOP;
}

