#ifndef _AUD_DEC_EFF_H
#define _AUD_DEC_EFF_H
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/eq_config.h"
#include "app_config.h"
#include "audio_config.h"

struct audio_eq *music_eq_open(u32 sample_rate, u8 ch_num);
void music_eq_close(struct audio_eq *eq);
struct audio_drc *music_drc_open(u32 sample_rate, u8 ch_num);
void music_drc_close(struct audio_drc *drc);
struct audio_eq *music_eq2_open(u32 sample_rate, u8 ch_num);
void music_eq2_close(struct audio_eq *eq);

struct audio_eq *music_eq_rl_rr_open(u32 sample_rate, u8 ch_num);
void music_eq_rl_rr_close(struct audio_eq *eq);
struct audio_drc *music_drc_rl_rr_open(u32 sample_rate, u8 ch_num);
void music_drc_rl_rr_close(struct audio_drc *drc);

struct audio_eq *esco_eq_open(u32 sample_rate, u8 ch_num, u8 bit_wide);
void esco_eq_close(struct audio_eq *eq);
struct audio_drc *esco_drc_open(u32 sample_rate, u8 ch_num, u8 bit_wide);
void esco_drc_close(struct audio_drc *drc);


struct audio_eq *linein_eq_open(u32 sample_rate, u8 ch_num);
void linein_eq_close(struct audio_eq *eq);
struct audio_drc *linein_drc_open(u32 sample_rate, u8 ch_num);
void linein_drc_close(struct audio_drc *drc);


struct audio_eq *high_bass_eq_open(u32 sample_rate, u8 ch_num);
void high_bass_eq_close(struct audio_eq *eq);
void high_bass_eq_udpate(u8 index, int freq, float gain);
void mix_out_high_bass(u32 cmd, struct high_bass *hb);

struct audio_drc *high_bass_drc_open(u32 sample_rate, u8 ch_num);
void high_bass_drc_close(struct audio_drc *drc);
int high_bass_drc_set_filter_info(int th);

#endif
