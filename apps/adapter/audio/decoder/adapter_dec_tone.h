#ifndef _ADAPTER_DEC_TONE_H_
#define _ADAPTER_DEC_TONE_H_

#include "app_config.h"
#include "audio_config.h"

int wl_mic_tone_play(const char *list, u8 preemption, void (*cbk)(void *priv), void *priv);
void wl_mic_tone_play_stop(void);
int wl_mic_tone_get_dec_status();

#endif
