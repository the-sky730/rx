#include "system/includes.h"
#include "media/includes.h"

#include "app_config.h"
#include "app_action.h"
#include "app_main.h"

#include "audio_config.h"
#include "audio_sound_dac.h"

//#include "audio_dec.h"

#include "application/audio_output_dac.h"
#include "application/audio_dig_vol.h"
struct audio_dac_hdl dac_hdl;
extern struct audio_dac_channel default_dac;

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
#include "fm_emitter/fm_emitter_manage.h"
#endif

#include "update.h"

#if (defined(AUDIO_OUTPUT_WAY) && AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
extern void bt_emitter_set_vol(u8 vol);
#endif

#define LOG_TAG             "[APP_AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define DEFAULT_DIGTAL_VOLUME   16384

typedef short unaligned_u16 __attribute__((aligned(1)));
struct app_audio_config {
    u8 state;
    u8 prev_state;
    volatile u8 fade_gain_l;
    volatile u8 fade_gain_r;
    volatile s16 fade_dgain_l;
    volatile s16 fade_dgain_r;
    volatile s16 fade_dgain_step_l;
    volatile s16 fade_dgain_step_r;
    volatile int save_vol_timer;
    volatile u8  save_vol_cnt;
    s16 digital_volume;
    atomic_t ref;
    s16 max_volume[APP_AUDIO_MAX_STATE];
    u8 sys_cvol_max;
    u8 call_cvol_max;
    unaligned_u16 *sys_cvol;
    unaligned_u16 *call_cvol;
};
static const char *audio_state[] = {
    "idle",
    "music",
    "call",
    "tone",
    "linein",
    "err",
};

static struct app_audio_config app_audio_cfg = {0};

#define __this      (&app_audio_cfg)
extern struct dac_platform_data dac_data;
extern struct audio_dac_hdl dac_hdl;
extern struct audio_adc_hdl adc_hdl;
OS_SEM dac_sem;

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_DONGLE)
s16 dac_buff[1 * 1024] SEC(.dac_buff);
#elif (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
s16 dac_buff[1 * 1024] SEC(.dac_buff);
#elif AUDIO_OUTPUT_INCLUDE_DAC
s16 dac_buff[2 * 1024] SEC(.dac_buff);
#endif

/*从audio中申请一块空间给use使用*/
void *app_audio_alloc_use_buff(int use_len)
{
    void *buf = NULL;
#if AUDIO_OUTPUT_INCLUDE_DAC
    printf("uselen:%d, total:%d \n", use_len, sizeof(dac_buff));
    if (use_len + (2 * 1024) > sizeof(dac_buff)) {
        y_printf("dac buff < uselen\n");
        return NULL;
    }
    int dac_len = ((sizeof(dac_buff) - use_len) * 4) / 4;
    audio_dac_reset_buf(&dac_hdl, dac_buff, dac_len, 0);
    return (u8 *)dac_buff + dac_len;
#endif
    return buf;
}

/*释放从audio中申请的空间*/
void app_audio_release_use_buff(void *buf)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    printf("app_audio_release_use_buff \n");
    if (buf) {
        audio_dac_reset_buf(&dac_hdl, dac_buff, sizeof(dac_buff), 1);
    }
#endif
}

/*关闭audio相关模块使能*/
void audio_disable_all(void)
{
    //DAC:DACEN
    JL_AUDIO->DAC_CON &= ~BIT(4);
    //ADC:ADCEN
    JL_AUDIO->ADC_CON &= ~BIT(4);
    //EQ:
    JL_EQ->CON0 &= ~BIT(0);
    //FFT:
    JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};
/*
 *************************************************************
 *
 *	audio volume save
 *
 *************************************************************
 */

static void app_audio_volume_save_do(void *priv)
{
    return;
    /* log_info("app_audio_volume_save_do %d\n", __this->save_vol_cnt); */
    local_irq_disable();
    if (++__this->save_vol_cnt >= 5) {
        sys_timer_del(__this->save_vol_timer);
        __this->save_vol_timer = 0;
        __this->save_vol_cnt = 0;
        local_irq_enable();
        log_info("VOL_SAVE\n");
        syscfg_write(CFG_MUSIC_VOL, &app_var.music_volume, 1);//中断里不能操作vm 关中断不能操作vm
        return;
    }
    local_irq_enable();
}

static void app_audio_volume_change(void)
{
    local_irq_disable();
    __this->save_vol_cnt = 0;
    if (__this->save_vol_timer == 0) {
        __this->save_vol_timer = sys_timer_add(NULL, app_audio_volume_save_do, 1000);//中断里不能操作vm 关中断不能操作vm
    }
    local_irq_enable();
}

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
__attribute__((weak))
void bt_emitter_set_vol(u8 vol)
{

}
#endif

static int audio_vol_set(u8 gain_l, u8 gain_r, u8 fade)
{
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
    return 0;
#endif

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_IIS)
    extern void *iis_digvol_last;
    if (iis_digvol_last) {
        audio_dig_vol_set(iis_digvol_last, AUDIO_DIG_VOL_ALL_CH, gain_l);
    }
#endif

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)
    bt_emitter_set_vol(gain_l);
#endif
    local_irq_disable();
    __this->fade_gain_l = gain_l;
    __this->fade_gain_r = gain_r;

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_L)
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0), gain_l, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0), gain_l ? DEFAULT_DIGTAL_VOLUME : 0, fade);
#elif (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_R)
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(1), gain_r, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0), gain_r ? DEFAULT_DIGTAL_VOLUME : 0, fade);
#else
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0), gain_l, fade);
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(1), gain_r, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0), gain_l ? DEFAULT_DIGTAL_VOLUME : 0, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(1), gain_r ? DEFAULT_DIGTAL_VOLUME : 0, fade);
#endif
    local_irq_enable();

    return 0;
}


#if (SYS_VOL_TYPE == VOL_TYPE_AD)

#define DGAIN_SET_MAX_STEP (300)
#define DGAIN_SET_MIN_STEP (30)

static unsigned short combined_vol_list[31][2] = {
    { 0,     0}, //0: None
    { 0, 16384}, // 1:-40.92 db
    { 2, 14124}, // 2:-39.51 db
    { 3, 14240}, // 3:-38.10 db
    { 4, 14326}, // 4:-36.69 db
    { 5, 14427}, // 5:-35.28 db
    { 6, 14562}, // 6:-33.87 db
    { 7, 14681}, // 7:-32.46 db
    { 8, 14802}, // 8:-31.05 db
    { 9, 14960}, // 9:-29.64 db
    {10, 15117}, // 10:-28.22 db
    {11, 15276}, // 11:-26.81 db
    {12, 15366}, // 12:-25.40 db
    {13, 15528}, // 13:-23.99 db
    {14, 15675}, // 14:-22.58 db
    {15, 15731}, // 15:-21.17 db
    {16, 15535}, // 16:-19.76 db
    {17, 15609}, // 17:-18.35 db
    {18, 15684}, // 18:-16.93 db
    {19, 15777}, // 19:-15.52 db
    {20, 15851}, // 20:-14.11 db
    {21, 15945}, // 21:-12.70 db
    {22, 16002}, // 22:-11.29 db
    {23, 16006}, // 23:-9.88 db
    {24, 16050}, // 24:-8.47 db
    {25, 16089}, // 25:-7.06 db
    {26, 16154}, // 26:-5.64 db
    {27, 16230}, // 27:-4.23 db
    {28, 16279}, // 28:-2.82 db
    {29, 16328}, // 29:-1.41 db
    {30, 16384}, // 30:0.00 db
};
static unsigned short call_combined_vol_list[16][2] = {
    { 0,     0}, //0: None
    { 0, 16384}, // 1:-40.92 db
    { 2, 15345}, // 2:-38.79 db
    { 4, 14374}, // 3:-36.66 db
    { 5, 15726}, // 4:-34.53 db
    { 7, 14782}, // 5:-32.40 db
    { 8, 16191}, // 6:-30.27 db
    {10, 15271}, // 7:-28.14 db
    {12, 14336}, // 8:-26.00 db
    {13, 15739}, // 9:-23.87 db
    {15, 14725}, // 10:-21.74 db
    {16, 15799}, // 11:-19.61 db
    {18, 14731}, // 12:-17.48 db
    {19, 16098}, // 13:-15.35 db
    {21, 15027}, // 14:-13.22 db
    {22, 16384}, // 15:-11.09 db
};

void audio_combined_vol_init(u8 cfg_en)
{
    u16 sys_cvol_len = 0;
    u16 call_cvol_len = 0;
    u8 *sys_cvol  = NULL;
    u8 *call_cvol  = NULL;
    s16 *cvol;

    log_info("audio_combined_vol_init\n");
    __this->sys_cvol_max = ARRAY_SIZE(combined_vol_list) - 1;
    __this->sys_cvol = combined_vol_list;
    __this->call_cvol_max = ARRAY_SIZE(call_combined_vol_list) - 1;
    __this->call_cvol = call_combined_vol_list;

    if (cfg_en) {
        sys_cvol  = syscfg_ptr_read(CFG_COMBINE_SYS_VOL_ID, &sys_cvol_len);
        if (sys_cvol && sys_cvol_len) {
            __this->sys_cvol = (unaligned_u16 *)sys_cvol;
            __this->sys_cvol_max = sys_cvol_len / 4 - 1;
            printf("read sys_cvol ok\n");
        } else {
            printf("read sys_cvol false:%x,%x\n", sys_cvol, sys_cvol_len);
        }

        call_cvol  = syscfg_ptr_read(CFG_COMBINE_CALL_VOL_ID, &call_cvol_len);
        if (call_cvol && call_cvol_len) {
            __this->call_cvol = (unaligned_u16 *)call_cvol;
            __this->call_cvol_max = call_cvol_len / 4 - 1;
            printf("read call_cvol ok\n");
        } else {
            printf("read call_combine_vol false:%x,%x\n", call_cvol, call_cvol_len);
        }
    }

    log_info("sys_cvol_max:%d,call_cvol_max:%d\n", __this->sys_cvol_max, __this->call_cvol_max);
}

static int audio_combined_vol_set(u8 gain_l, u8 gain_r, u8 fade)
{
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
    return 0;
#endif
    u8  gain_max;
    u8  target_again_l = 0;
    u8  target_again_r = 0;
    u16 target_dgain_l = 0;
    u16 target_dgain_r = 0;

    if (__this->state == APP_AUDIO_STATE_CALL) {
        gain_max = __this->call_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->call_cvol[gain_l * 2]);
        target_again_r = *(&__this->call_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->call_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->call_cvol[gain_r * 2 + 1]);
    } else {
        gain_max = __this->sys_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->sys_cvol[gain_l * 2]);
        target_again_r = *(&__this->sys_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->sys_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->sys_cvol[gain_r * 2 + 1]);
    }
    y_printf("[l]v:%d,Av:%d,Dv:%d", gain_l, target_again_l, target_dgain_l);

    local_irq_disable();

    __this->fade_gain_l  = target_again_l;
    __this->fade_gain_r  = target_again_r;
    __this->fade_dgain_l = target_dgain_l;
    __this->fade_dgain_r = target_dgain_r;

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_L)
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0), __this->fade_gain_l, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0), __this->fade_dgain_l, fade);
#elif (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_R)
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(1), __this->fade_gain_r, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0), __this->fade_dgain_r, fade);
#else
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0), __this->fade_gain_l, fade);
    audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(1), __this->fade_gain_r, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0), __this->fade_dgain_l, fade);
    audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(1), __this->fade_dgain_r, fade);
#endif

    local_irq_enable();

    return 0;
}

#endif  // (SYS_VOL_TYPE == VOL_TYPE_AD)

void audio_volume_list_init(u8 cfg_en)
{
#if (SYS_VOL_TYPE == VOL_TYPE_AD)
    audio_combined_vol_init(cfg_en);
#elif (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    /* audio_hw_digital_vol_init(cfg_en); */
#endif/*SYS_VOL_TYPE*/
}

void volume_up_down_direct(s8 value)
{
    // reserve
}

void audio_fade_in_fade_out(u8 left_gain, u8 right_gain, u8 fade)
{
#if (SYS_VOL_TYPE == VOL_TYPE_AD)
    audio_combined_vol_set(left_gain, right_gain, fade);
#else
    audio_vol_set(left_gain, right_gain, fade);
#endif/*SYS_VOL_TYPE == VOL_TYPE_AD*/
}



extern char *music_dig_logo[];

extern void *sys_digvol_group;


void app_audio_set_volume(u8 state, s8 volume, u8 fade)
{

    char *digvol_type = NULL ;

#if (SMART_BOX_EN)
    extern bool smartbox_set_volume(s8 volume);
    if (smartbox_set_volume(volume)) {
        return;
    }
#endif/*SMART_BOX_EN*/

    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
    case APP_AUDIO_STATE_LINEIN:
#if SYS_DIGVOL_GROUP_EN
        digvol_type = "music_type";
#endif/*SYS_DIGVOL_GROUP_EN*/
        app_var.music_volume = volume;
        if (app_var.music_volume > get_max_sys_vol()) {
            app_var.music_volume = get_max_sys_vol();
        }
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
#if SYS_DIGVOL_GROUP_EN
        digvol_type = "call_esco";
#endif/*SYS_DIGVOL_GROUP_EN*/
        app_var.call_volume = volume;
        if (app_var.call_volume > 15) {
            app_var.call_volume = 15;
        }
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
        /*
         *SYS_VOL_TYPE == VOL_TYPE_ANALOG的时候，
         *将通话音量最大值按照手机通话的音量等级进行等分
         *由于等级不匹配，会出现对应有些等级没有一一对应
         *的情况，如果在意，建议使用以下其中一种：
         *#define TCFG_CALL_USE_DIGITAL_VOLUME	1
         *#define SYS_VOL_TYPE 	VOL_TYPE_AD
         *#define SYS_VOL_TYPE 	VOL_TYPE_DIGITAL
         */
        volume = app_var.aec_dac_gain * app_var.call_volume / 15;
#endif/*SYS_VOL_TYPE == VOL_TYPE_ANALOG*/

#if TCFG_CALL_USE_DIGITAL_VOLUME
        audio_dac_vol_set(TYPE_DAC_DGAIN, \
                          BIT(0) | BIT(1), \
                          16384L * (s32)app_var.call_volume / (s32)__this->max_volume[APP_AUDIO_STATE_CALL], \
                          1);
        return;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
        break;
    case APP_AUDIO_STATE_WTONE:
#if SYS_DIGVOL_GROUP_EN
        digvol_type = "tone_tone";
#endif/*SYS_DIGVOL_GROUP_EN*/

#if TONE_MODE_DEFAULE_VOLUME != 0
        app_var.wtone_volume = TONE_MODE_DEFAULE_VOLUME;
        volume = app_var.wtone_volume;
        break;
#endif

#if APP_AUDIO_STATE_WTONE_BY_MUSIC == 1


        app_var.wtone_volume = app_var.music_volume;
        if (app_var.wtone_volume < 5) {
            app_var.wtone_volume = 5;
        }
#else
        app_var.wtone_volume = volume;
#endif
        if (app_var.wtone_volume > get_max_sys_vol()) {
            app_var.wtone_volume = get_max_sys_vol();
        }
        volume = app_var.wtone_volume;
        break;
    default:
        return;
    }
    if (state == __this->state) {
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
        fm_emitter_manage_set_vol(volume);
#else
#if defined (VOL_TYPE_DIGGROUP) && defined (SYS_DIGVOL_GROUP_EN)
        if (state == APP_AUDIO_STATE_LINEIN) {
            //printf("linein analog vol:%d\n",volume);
            audio_fade_in_fade_out(volume, volume, fade);
            return;
        }
        if (SYS_VOL_TYPE == VOL_TYPE_DIGGROUP && SYS_DIGVOL_GROUP_EN) {
            if (sys_digvol_group == NULL) {
                /* printf("the sys_digvol_group is NULL\n-------------------------------------------------------------"); */
                return;
            }
            if (strcmp(digvol_type, "music_type") == 0) {
                for (int i = 0; music_dig_logo[i] != "NULL"; i++) {
                    /* printf("%s\n", music_dig_logo[i]); */
                    if (audio_dig_vol_group_hdl_get(sys_digvol_group, music_dig_logo[i]) == NULL) {
                        continue;
                    }
                    audio_dig_vol_group_vol_set(sys_digvol_group, music_dig_logo[i], AUDIO_DIG_VOL_ALL_CH, volume);
                }
            } else {
                if (audio_dig_vol_group_hdl_get(sys_digvol_group, digvol_type) == NULL) {
                    return;
                }
                audio_dig_vol_group_vol_set(sys_digvol_group, digvol_type, AUDIO_DIG_VOL_ALL_CH, volume);
            }
        }

        else {
            audio_fade_in_fade_out(volume, volume, fade);
        }

#else

        audio_fade_in_fade_out(volume, volume, fade);
#endif //vol_type_diggroup



#endif  //audio_output_way
    }
    app_audio_volume_change();
}

void app_audio_volume_init(void)
{
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
}

s8 app_audio_get_volume(u8 state)
{
    s8 volume = 0;
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
    case APP_AUDIO_STATE_LINEIN:
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
#if TONE_MODE_DEFAULE_VOLUME != 0
        app_var.wtone_volume = TONE_MODE_DEFAULE_VOLUME;
        volume = app_var.wtone_volume;
        break;
#endif
#if APP_AUDIO_STATE_WTONE_BY_MUSIC == 1
        volume = app_var.music_volume;
        break;
#else
        volume = app_var.wtone_volume;
        /* if (!volume) { */
        /* volume = app_var.music_volume; */
        /* } */
        break;
#endif
    case APP_AUDIO_CURRENT_STATE:
        volume = app_audio_get_volume(__this->state);
        break;
    default:
        break;
    }
    /* printf("app_audio_get_volume %d %d\n", state, volume); */
    return volume;
}


static const char *audio_mute_string[] = {
    "mute_default",
    "unmute_default",
    "mute_L",
    "unmute_L",
    "mute_R",
    "unmute_R",
};

void app_audio_mute(u8 value)
{
    u8 volume = 0;
    printf("audio_mute:%s", audio_mute_string[value]);
    switch (value) {
    case AUDIO_MUTE_DEFAULT:

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
        fm_emitter_manage_set_vol(0);
#else
        audio_dac_vol_mute(1, 1);
#endif
        break;
    case AUDIO_UNMUTE_DEFAULT:
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
        volume = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        fm_emitter_manage_set_vol(volume);
#else
        audio_dac_vol_mute(0, 1);
#endif
        break;
    }
}


void app_audio_volume_up(u8 value)
{
#if (SMART_BOX_EN)
    extern bool smartbox_key_volume_up(u8 value);
    if (smartbox_key_volume_up(value)) {
        return;
    }
#endif
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
    case APP_AUDIO_STATE_LINEIN:
        app_var.music_volume += value;
        if (app_var.music_volume > get_max_sys_vol()) {
            app_var.music_volume = get_max_sys_vol();
        }
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume += value;
        if (app_var.call_volume > 15) {
            app_var.call_volume = 15;
        }
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
#if TONE_MODE_DEFAULE_VOLUME != 0
        app_var.wtone_volume = TONE_MODE_DEFAULE_VOLUME;
        volume = app_var.wtone_volume;
        break;
#endif
#if APP_AUDIO_STATE_WTONE_BY_MUSIC == 1
        app_var.wtone_volume = app_var.music_volume;
#endif
        app_var.wtone_volume += value;
        if (app_var.wtone_volume > get_max_sys_vol()) {
            app_var.wtone_volume = get_max_sys_vol();
        }
        volume = app_var.wtone_volume;
#if APP_AUDIO_STATE_WTONE_BY_MUSIC == 1
        app_var.music_volume = app_var.wtone_volume;
#endif
        break;
    default:
        return;
    }

    app_audio_set_volume(__this->state, volume, 1);
}

void app_audio_volume_down(u8 value)
{
#if (SMART_BOX_EN)
    extern bool samrtbox_key_volume_down(u8 value);
    if (smartbox_key_volume_up(value)) {
        return;
    }
#endif
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
    case APP_AUDIO_STATE_LINEIN:
        app_var.music_volume -= value;
        if (app_var.music_volume < 0) {
            app_var.music_volume = 0;
        }
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume -= value;
        if (app_var.call_volume < 0) {
            app_var.call_volume = 0;
        }
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
#if TONE_MODE_DEFAULE_VOLUME != 0
        app_var.wtone_volume = TONE_MODE_DEFAULE_VOLUME;
        volume = app_var.wtone_volume;
        break;
#endif
#if APP_AUDIO_STATE_WTONE_BY_MUSIC == 1
        app_var.wtone_volume = app_var.music_volume;
#endif
        app_var.wtone_volume -= value;
        if (app_var.wtone_volume < 0) {
            app_var.wtone_volume = 0;
        }
        volume = app_var.wtone_volume;
#if APP_AUDIO_STATE_WTONE_BY_MUSIC == 1
        app_var.music_volume = app_var.wtone_volume;
#endif
        break;
    default:
        return;
    }

    app_audio_set_volume(__this->state, volume, 1);
}

void app_audio_state_switch(u8 state, s16 max_volume)
{
    r_printf("audio state old:%s,new:%s,vol:%d\n", audio_state[__this->state], audio_state[state], max_volume);

    __this->prev_state = __this->state;
    __this->state = state;
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        /*调数字音量的时候，模拟音量定最大*/
        audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0) | BIT(1), max_volume, 1);
        audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0) | BIT(1), 0, 1);
    }
#endif

    /*限制最大音量*/
    __this->digital_volume = DEFAULT_DIGTAL_VOLUME;
    __this->max_volume[state] = max_volume;

    if (__this->state == APP_AUDIO_STATE_CALL) {
        __this->max_volume[state] = 15;
    }



#if (SYS_VOL_TYPE ==VOL_TYPE_DIGGROUP)
    u8 dac_connect_mode =  app_audio_output_mode_get();
    switch (dac_connect_mode) {
    case DAC_OUTPUT_MONO_L :
        audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0), 30, 1);
        audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(1), 0, 1);
        audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0) | BIT(1), 16384, 1);
        break;
    case DAC_OUTPUT_MONO_R :
        audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(1), 30, 1);
        audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0), 0, 1);
        audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0) | BIT(1), 16384, 1);
        break;

    default :
        audio_dac_vol_set(TYPE_DAC_AGAIN, BIT(0) | BIT(1), 30, 1);
        audio_dac_vol_set(TYPE_DAC_DGAIN, BIT(0) | BIT(1), 16384, 1);

    }
#endif


    app_audio_set_volume(__this->state, app_audio_get_volume(__this->state), 1);
}

void app_audio_state_exit(u8 state)
{
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
    }
#endif

    r_printf("audio state now:%s,prev:%s\n", audio_state[__this->state], audio_state[__this->prev_state]);
    if (state == __this->state) {
        __this->state = __this->prev_state;
        __this->prev_state = APP_AUDIO_STATE_IDLE;
    } else if (state == __this->prev_state) {
        __this->prev_state = APP_AUDIO_STATE_IDLE;
    }
    app_audio_set_volume(__this->state, app_audio_get_volume(__this->state), 1);
}
u8 app_audio_get_state(void)
{
    return __this->state;
}

s16 app_audio_get_max_volume(void)
{
    if (__this->state == APP_AUDIO_STATE_IDLE) {
        return get_max_sys_vol();
    }
    return __this->max_volume[__this->state];
}

void dac_power_on(void)
{
    log_info(">>>dac_power_on:%d", __this->ref.counter);
    if (atomic_inc_return(&__this->ref) == 1) {
        audio_dac_open(&dac_hdl);
    }

}
void dac_sniff_power_off(void)
{
    audio_dac_close(&dac_hdl);
}

void dac_power_off(void)
{
    /* log_info(">>>dac_power_off:%d", __this->ref.counter); */
    /* if (atomic_dec_return(&__this->ref)) { */
    /*     return; */
    /* } */
#if 0
    app_audio_mute(AUDIO_MUTE_DEFAULT);
    if (dac_hdl.vol_l || dac_hdl.vol_r) {
        u8 fade_time = dac_hdl.vol_l * 2 / 10 + 1;
        os_time_dly(fade_time);
        printf("fade_time:%d ms", fade_time);
    }
#endif
    audio_dac_close(&dac_hdl);
}
/*
 *自定义dac上电延时时间，具体延时多久应通过示波器测量
 */
#if 1
void dac_power_on_delay()
{
    os_time_dly(50);
}
#endif

////////////////////////////////// audio_output_api //////////////////////////////////////////////
#if 1

void _audio_dac_irq_hook(void)
{
#if 0
    /* putbyte('d'); */
    extern struct audio_stream_dac_out *dac_last;
    audio_stream_resume(&dac_last->entry);
#else
    audio_sound_dac_list_resume();
#endif
}

void _audio_adc_irq_hook(void)
{
    /* putbyte('a'); */
    extern struct audio_adc_hdl adc_hdl;
    audio_adc_irq_handler(&adc_hdl);
}


/*******************************************************
* Function name	: app_audio_output_init
* Description	: 音频输出设备初始化
* Return        : None
********************* -HB ******************************/
void app_audio_output_init(void)
{
#if (AUDIO_OUTPUT_INCLUDE_DAC|| TCFG_AUDIO_ADC_ENABLE)
    audio_dac_init(&dac_hdl, &dac_data);
#endif
#if AUDIO_OUTPUT_INCLUDE_DAC

    audio_dac_set_buff(&dac_hdl, dac_buff, sizeof(dac_buff));

#if TCFG_MIC_CAPLESS_ENABLE
    extern void mic_trim_run();
    mic_trim_run();
#endif/*TCFG_MIC_CAPLESS_ENABLE*/

    struct audio_dac_trim dac_trim;
    int len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    if (len != sizeof(dac_trim) || dac_trim.left == 0 || dac_trim.right == 0) {
        audio_dac_do_trim(&dac_hdl, &dac_trim, 0);
        syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    }
    audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    audio_dac_set_delay_time(&dac_hdl, 30, 50);
#endif
}

/*******************************************************
* Function name	: app_audio_output_sync_buff_init
* Description	: 设置音频输出设备同步功能 buf
* Parameter		:
*   @sync_buff		buf 起始地址
*   @len       		buf 长度
* Return        : None
********************* -HB ******************************/
void app_audio_output_sync_buff_init(void *sync_buff, int len)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    /*音频同步DA端buffer设置*/
    /*audio_output_dac_sync_buff_init(sync_buff, len);*/
#endif
}


/*******************************************************
* Function name	: app_audio_output_samplerate_select
* Description	: 将输入采样率与输出采样率进行匹配对比
* Parameter		:
*   @sample_rate    输入采样率
*   @high:          0 - 低一级采样率，1 - 高一级采样率
* Return        : 匹配后的采样率
********************* -HB ******************************/
int app_audio_output_samplerate_select(u32 sample_rate, u8 high)
{
    return audio_dac_sample_rate_select(&dac_hdl, sample_rate, high);
}

/*******************************************************
* Function name	: app_audio_output_samplerate_set
* Description	: 设置音频输出设备的采样率
* Parameter		:
*   @sample_rate	采样率
* Return        : 0 success, other fail
********************* -HB ******************************/
int app_audio_output_samplerate_set(int sample_rate)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_set_sample_rate(&dac_hdl, sample_rate);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_samplerate_get
* Description	: 获取音频输出设备的采样率
* Return        : 音频输出设备的采样率
********************* -HB ******************************/
int app_audio_output_samplerate_get(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_get_sample_rate(&dac_hdl);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_mode_get
* Description	: 获取当前硬件输出模式
* Return        : 输出模式
********************* -HB ******************************/
int app_audio_output_mode_get(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_get_pd_output(&dac_hdl);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_mode_set
* Description	: 设置当前硬件输出模式
* Return        : 0 success, other fail
********************* -HB ******************************/
int app_audio_output_mode_set(u8 output)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_set_pd_output(&dac_hdl, output);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_channel_get
* Description	: 获取音频输出设备输出通道数
* Return        : 通道数
********************* -HB ******************************/
int app_audio_output_channel_get(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_get_channel(&dac_hdl);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_channel_set
* Description	: 设置音频输出设备输出通道数
* Parameter		:
*   @channel       	通道数
* Return        : 0 success, other fail
********************* -HB ******************************/
int app_audio_output_channel_set(u8 channel)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_set_channel(&dac_hdl, channel);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_write
* Description	: 向音频输出设备写入需要输出的音频数据
* Parameter		:
*   @buf			写入音频数据的起始地址
*   @len			写入音频数据的长度
* Return        : 成功写入的长度
********************* -HB ******************************/
int app_audio_output_write(void *buf, int len)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_write(&default_dac, buf, len);
#elif AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM
    return fm_emitter_cbuf_write(buf, len);
#endif
    return 0;
}


/*******************************************************
* Function name	: app_audio_output_start
* Description	: 音频输出设备输出打开
* Return        : 0 success, other fail
********************* -HB ******************************/
int app_audio_output_start(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    audio_dac_start(&dac_hdl);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_stop
* Description	: 音频输出设备输出停止
* Return        : 0 success, other fail
********************* -HB ******************************/
int app_audio_output_stop(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_channel_stop(&default_dac);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_reset
* Description	: 音频输出设备重启
* Parameter		:
*   @msecs       	重启时间 ms
* Return        : 0 success, other fail
********************* -HB ******************************/
int app_audio_output_reset(u32 msecs)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_sound_reset(&dac_hdl, msecs);
#endif
    return 0;
}

/*******************************************************
* Function name	: app_audio_output_get_cur_buf_points
* Description	: 获取当前音频输出buf还可以输出的点数
* Parameter		:
* Return        : 还可以输出的点数
********************* -HB ******************************/
int app_audio_output_get_cur_buf_points(void)
{
    return 0;
}

int app_audio_output_ch_analog_gain_set(u8 ch, u8 again)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_ch_analog_gain_set(&dac_hdl, ch, again);
#endif
    return 0;
}

int app_audio_output_ch_digital_gain_set(u8 ch, u32 dgain)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_ch_digital_gain_set(&dac_hdl, ch, dgain);
#endif
    return 0;
}

int app_audio_output_state_get(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_get_status(&dac_hdl);
#endif
    return 0;
}

void app_audio_output_ch_mute(u8 ch, u8 mute)
{
    audio_dac_ch_mute(&dac_hdl, ch, mute);
}

int audio_output_buf_time(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_data_time(&dac_hdl);
#endif
    return 0;
}

int audio_output_dev_is_working(void)
{
#if AUDIO_OUTPUT_INCLUDE_DAC
    return audio_dac_is_working(&dac_hdl);
#endif
    return 1;
}

int audio_output_sync_start(void)
{
    return 0;
}

int audio_output_sync_stop(void)
{
    return 0;
}

#endif

void audio_adda_dump(void) //打印所有的dac,adc寄存器
{
    printf("DAC_VL0:%x", JL_AUDIO->DAC_VL0);
    printf("DAC_TM0:%x", JL_AUDIO->DAC_TM0);
    printf("DAC_DTB:%x", JL_AUDIO->DAC_DTB);
    printf("DAC_CON:%x", JL_AUDIO->DAC_CON);
    printf("ADC_CON:%x", JL_AUDIO->ADC_CON);
    printf("DAC RES: DA0:0x%x DA1:0x%x DA2:0x%x DA3:0x%x DA7:0x%x,ADC RES:ADA0:0X%x ADA1:0X%x ADA2:0X%x  ADA3:0X%x ADA4:0X%x\\n", \
           JL_ADDA->DAA_CON0, JL_ADDA->DAA_CON1, JL_ADDA->DAA_CON2, JL_ADDA->DAA_CON3,  JL_ADDA->DAA_CON7, \
           JL_ADDA->ADA_CON0, JL_ADDA->ADA_CON1, JL_ADDA->ADA_CON2, JL_ADDA->ADA_CON3, JL_ADDA->ADA_CON4);
}


void audio_adda_gain_dump()
{
    u8 dac_again_l = JL_ADDA->DAA_CON1 & 0xF;
    u8 dac_again_r = (JL_ADDA->DAA_CON1 >> 4) & 0xF;
    u32 dac_dgain_l = JL_AUDIO->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDIO->DAC_VL0 >> 16) & 0xFFFF;

    u8 mic0_0_6 = JL_ADDA->ADA_CON1 & 0x1;
    u8 mic1_0_6 = JL_ADDA->ADA_CON2 & 0x1;
    u8 mic0_gain = JL_ADDA->ADA_CON0 & 0x1F;
    u8 mic1_gain = (JL_ADDA->ADA_CON0 >> 5) & 0x1F;
    short anc_gain = JL_ANC->CON5 & 0xFFFF;
    printf("MIC_G:%d,%d,MIC_0_6:%d,%d,DAC_AG:%d,%d,DAC_DG:%d,%d,ANC_G:%d\n", mic0_gain, mic1_gain, mic0_0_6, mic1_0_6, dac_again_l, dac_again_r, dac_dgain_l, dac_dgain_r, anc_gain);
}



