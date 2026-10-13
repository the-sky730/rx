#include "system/includes.h"

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

