
#ifndef _ADAPTER_AUDIO_MUSIC_EQ_H_
#define _ADAPTER_AUDIO_MUSIC_EQ_H_


#include "generic/typedef.h"
#include "media/audio_eq_drc_apply.h"

void *adapter_audio_music_eq_drc_open(u16 sample_rate, u8 ch_num);
void adapter_audio_music_eq_drc_close(void *eq);

#endif
