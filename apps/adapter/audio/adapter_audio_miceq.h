
#ifndef _ADAPTER_AUDIO_MICEQ_H_
#define _ADAPTER_AUDIO_MICEQ_H_


#include "generic/typedef.h"
#include "media/audio_eq_drc_apply.h"

void *adapter_audio_miceq_open(u16 sample_rate, u8 ch_num);
void adapter_audio_miceq_close(void *eq);

#endif
