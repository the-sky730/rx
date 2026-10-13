
#ifndef __ADAPTER_WIRELESS_DEC_H__
#define __ADAPTER_WIRELESS_DEC_H__

#include "generic/typedef.h"
#include "media/includes.h"
#include "adapter_decoder.h"

struct __adapter_wireless_dec;

struct __adapter_wireless_dec *adapter_wireless_dec_open_base(
    struct adapter_decoder_fmt *fmt,
    struct adapter_media_parm *media_parm,
    struct audio_mixer *mixer,
    struct adapter_audio_stream	*adapter_stream);
struct __adapter_wireless_dec *adapter_wireless_dec_open(struct adapter_decoder_fmt *fmt, struct adapter_media_parm *media_parm);
int adapter_wireless_dec_rx_write(struct __adapter_wireless_dec *dec, void *data, u16 len);
int adapter_dec_dual_jla_frame_write_pause(u8 channel, u8 value);
int adapter_dec_dual_jla_frame_write_pause_get_status(u8 channel);
void adapter_wireless_dec_close(struct __adapter_wireless_dec **hdl);
void adapter_wireless_dec_set_vol(struct __adapter_wireless_dec *dec, u32 channel, u8 vol);
int adapter_wireless_dec_ioctrl(struct __adapter_wireless_dec *dec, int cmd, int argc, int *argv);

#endif//__ADAPTER_WIRELESS_DEC_H__

