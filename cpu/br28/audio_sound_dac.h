#ifndef AUDIO_SOUND_DAC_H
#define AUDIO_SOUND_DAC_H

#include "system/includes.h"
#include "audio_type.h"
#include "sound/sound.h"

struct audio_sound_dac {
    struct audio_way *way;
    struct audio_stream_entry entry;
    int dma_start;
    void *underrun_data;
    void (*underrun_handler)(void *priv, int event);
    u16 start_delay_points;
};

struct dac_param {
    u32 sample_rate;        /*采样率*/
    u16 delay_ms;         /*DAC通道延时*/
    u16 underrun_time;
    u16 protect_time;       /*DAC延时保护时间*/
    u16 start_delay;        /*DAC起始延时时间(ms)*/
    u8  write_mode;         /*DAC写入模式 WRITE_MODE_BLOCK or WRITE_MODE_FORCE*/
};

void audio_sound_dac_init(void);
void audio_sound_dac_uninit(void);
struct audio_sound_dac *audio_sound_dac_open(struct dac_param *param);
int audio_sound_dac_close(struct audio_sound_dac *dac);
int audio_sound_dac_start(struct audio_sound_dac *dac);
int audio_sound_dac_stop(struct audio_sound_dac *dac);
int audio_sound_dac_write(struct audio_sound_dac *dac, s16 *data, u32 len);
int audio_sound_dac_get_data_time(struct audio_sound_dac *dac);
int audio_sound_dac_get_sample_rate(struct audio_sound_dac *dac);
int audio_sound_dac_set_sample_rate(struct audio_sound_dac *dac, u32 sample_rate);
int audio_sound_dac_set_analog_gain(struct audio_sound_dac *dac, u32 channel, int gain);
int audio_sound_dac_set_digital_gain(struct audio_sound_dac *dac, u32 channel, int gain);
int audio_sound_dac_power_on(void);
void sound_dac_set_underrun_handler(struct audio_sound_dac *dac, void *priv, void (*handler)(void *, int));
int audio_sound_dac_buffered_frames(struct audio_sound_dac *dac);

#endif
