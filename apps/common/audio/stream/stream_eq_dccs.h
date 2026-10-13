#ifndef _STREAM_EQ_DCCS_
#define _STREAM_EQ_DCCS_


#include "system/includes.h"
#include "media/includes.h"

struct stream_eq_dccs_parm {
    u16 sr;
};

struct stream_eq_dccs {
    struct audio_stream_entry entry;
};

struct stream_eq_dccs *stream_eq_dccs_open(u16 sample_rate);
void stream_eq_dccs_close(struct stream_eq_dccs **hdl);

#endif
