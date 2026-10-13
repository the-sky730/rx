#ifndef ONLINE_CONFIG_H
#define ONLINE_CONFIG_H


#define CI_UART         0
#define CI_TWS          1

typedef struct _eq_online_gain {
    u8 mic_gain_boost;
    u8 mic_gain;
    u8 dac_gain;
    u8 eq_online_flag;
} eq_online_gain;
extern eq_online_gain var_gain;
void ci_data_rx_handler(u8 type);
u32 eq_cfg_sync(u8 priority);
u8 eq_online_get_adc_gain();
u8 eq_online_get_gain_boost();
u8 eq_online_get_dac_gain();
u8 eq_online_get_flag();
u8 eq_online_get_linein_gain();
#endif

