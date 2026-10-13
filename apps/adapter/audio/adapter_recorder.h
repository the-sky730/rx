#ifndef __ADAPTER_RECORDER_H__
#define __ADAPTER_RECORDER_H__

#include "generic/typedef.h"


u8 wl_mic_get_recorder_online_status(void);
void wireless_mic_recorder_start(void);
void wireless_mic_recorder_stop(void);
void wireless_mic_recorder_pcm_data_write(void *data, u16 len);

#endif

