#include "system/includes.h"
#include "asm/dac.h"

__attribute__((weak))
void pwm_led_mode_set(u8 display)
{
}
__attribute__((weak))
u8 get_ldo5v_pulldown_res(void)
{
    return 0;
}
__attribute__((weak))
u8 get_charge_online_flag(void)
{
    return 0;
}
__attribute__((weak))
u8 get_ldo5v_pulldown_en(void)
{
    return 0;
}

/* void update_module_init(void (*handle)(int type, u32 state, u32 code))
{
}
int app_active_update_task_init(void *hdl)
{
    return 0;
} */

__attribute__((weak))
int audio_dac_channel_get_attr(struct audio_dac_channel *ch, struct audio_dac_channel_attr *attr)
{
    return 0;
}
__attribute__((weak))
int audio_src_base_filt_init(void *filt, int len)
{
    return 0;
}
__attribute__((weak))
int audio_dac_start(struct audio_dac_hdl *dac)
{
    return 0;
}

__attribute__((weak))
int audio_dac_stop(struct audio_dac_hdl *dac)
{
    return 0;
}
__attribute__((weak))
void audio_dac_free_channel(struct audio_dac_channel *ch)
{
}
__attribute__((weak))
int audio_dac_new_channel(struct audio_dac_hdl *dac, struct audio_dac_channel *ch)
{
    return 0;
}
__attribute__((weak))
int audio_dac_channel_set_attr(struct audio_dac_channel *ch, struct audio_dac_channel_attr *attr)
{
    return 0;
}
__attribute__((weak))
u8 audio_dac_init_status(void)
{
    return 0;
}


#if 0
int audio_wireless_sync_sound_reset(void *c, int time)
{
    return 0;
}


int audio_wireless_sync_with_stream(void *c, void *info)
{
    return 0;
}

int audio_wireless_sync_stop(void *c)
{
    return 0;
}
#endif
