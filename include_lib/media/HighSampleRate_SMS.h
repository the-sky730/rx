#ifndef _HIGH_SAMPLERATE_SMS_H_
#define _HIGH_SAMPLERATE_SMS_H_

#include "generic/typedef.h"

int HighSampleRate_SMS_Init(void *runbuf,
                            void *tempbuf,
                            float AggressFactor,
                            float minSuppress,
                            int sampleRate,
                            float init_noise_lvl);

void HighSampleRate_SMS_Update(void *runbuf, float AggressFactor, float minSuppress);
void HighSampleRate_SMS_Process(void *runbuf, void *tempbuf, short *mic_data, short *output, int nMiniFrame);
int HighSampleRate_SMS_GetMiniFrameSize(int sampleRate);
int HighSampleRate_SMS_QueryBufSize(int sampleRate);
int HighSampleRate_SMS_QueryTempBufSize(int sampleRate);
void HighSampleRate_SMS_Close(void *runbuf, void *tempbuf, short *mic_data, short *output, int nMiniFrame);

#endif/*_HIGH_SAMPLERATE_SMS_H_*/
