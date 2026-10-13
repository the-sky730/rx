
#include "ui/io_led.h"
#include "gpio.h"
#include "timer.h"

#if TCFG_IOLED_ENABLE


static void led_set_output_value(u32 gpio, u8 value)
{

#if TCFG_IOLED_USE_MICIN_IO
    extern void audio_adc_mic_ctl(u8 en);
    if (gpio == TCFG_MICIN_IO0 || gpio == TCFG_MICIN_IO1) {
        audio_adc_mic_ctl(value);
    }
#endif
    gpio_set_output_value(gpio, value);
}

static u16 ioled_timer_id = 0;
u16 ioled_mode = 0;
void ioled_timer()
{
    static u8 fast_cnt = 0;
    static u8 slow_cnt = 0;
    static u8 flag_fast = 0;
    static u8 flag_slow = 0;
    if (ioled_timer_id) {

        fast_cnt++;
        slow_cnt++;

        if (fast_cnt > FAST_TIME) {
            fast_cnt = 0;
            flag_fast = !flag_fast;
            if (ioled_mode & LED0_FAST_FLASH) {
                led_set_output_value(TCFG_IOLED_PIN0, flag_fast);
            }
            if (ioled_mode & LED1_FAST_FLASH) {
                led_set_output_value(TCFG_IOLED_PIN1, flag_fast);
            }
        }
        if (slow_cnt > SLOW_TIME) {
            slow_cnt = 0;
            flag_slow = !flag_slow;
            if (ioled_mode & LED0_SLOW_FLASH) {
                led_set_output_value(TCFG_IOLED_PIN0, flag_slow);
            }
            if (ioled_mode & LED1_SLOW_FLASH) {
                led_set_output_value(TCFG_IOLED_PIN1, flag_slow);
            }
        }
    }

}
void ioled_ui_init(void)
{
#if TCFG_IOLED_USE_MICIN_IO
    audio_adc_mic_ctl(0);
#endif
    gpio_set_direction(TCFG_IOLED_PIN0, 0);
    gpio_set_direction(TCFG_IOLED_PIN1, 0);
    gpio_set_output_value(TCFG_IOLED_PIN0, 0);
    gpio_set_output_value(TCFG_IOLED_PIN1, 0);
    if (!ioled_timer_id) {
        ioled_timer_id = sys_timer_add(NULL, ioled_timer, 100);
    }
}

void ioled_update_status(u16 status)
{
    printf(" 1t2 led_status = %d", status);

    if (status & 0x00ff) {
        ioled_mode &= 0xff00;
    } else if (status & 0xff00) {
        ioled_mode &= 0x00ff;
    } else {
        printf("error mode");
        return ;
    }
    ioled_mode |= status;

    printf("ioled_mode = 0x%x", ioled_mode);

    //处理常亮常灭状态
    if (ioled_mode & LED0_ON) {
        led_set_output_value(TCFG_IOLED_PIN0, LED_ON_LEVEL);
    } else if (ioled_mode & LED0_OFF) {
        led_set_output_value(TCFG_IOLED_PIN0, !LED_ON_LEVEL);
    }
    if (ioled_mode & LED1_ON) {
        led_set_output_value(TCFG_IOLED_PIN1, LED_ON_LEVEL);
    } else if (ioled_mode & LED1_OFF) {
        led_set_output_value(TCFG_IOLED_PIN1, !LED_ON_LEVEL);
    }

}

#endif

