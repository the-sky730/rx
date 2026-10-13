#include "system/includes.h"
#include "app_config.h"
#include "asm/pwm_led.h"
#include "tone_player.h"
#include "ui_manage.h"
#include "app_main.h"
#include "app_task.h"
#include "asm/charge.h"
#include "app_power_manage.h"
#include "app_charge.h"
#include "user_cfg.h"
#include "power_on.h"
#include "audio.h"
#include "vm.h"
#include "adapter_app_status.h"
#include "key_event_deal.h"

#define LOG_TAG_CONST       APP
#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

APP_VAR app_var;


#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"

#if TCFG_USB_SLAVE_HID_ENABLE
#include "usb/device/hid.h"
#endif

#if TCFG_USB_SLAVE_MSD_ENABLE
#include "usb/device/msd.h"
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
#include "usb/device/cdc.h"
#endif
usb_dev usb_id = 0;
extern void app_charge_run(void);
extern void app_main_run(void);
u8 app_curr_status = 0;


u8 app_common_device_event_deal(struct sys_event *event)
{
    u8 ret = 0;
    switch ((u32)event->arg) {
#if TCFG_CHARGE_ENABLE
    case DEVICE_EVENT_FROM_CHARGE:
        ret = app_charge_event_handler(&event->u.dev);
        break;
#endif
    case DEVICE_EVENT_FROM_POWER:
        ret = app_power_event_handler(&event->u.dev);
        break;
    }
    return ret;
}

extern u8 wireless_conn_status;
void led_mode_set()
{
    static u8 flash_flag = 0;
    static u8 last_status = 0xff;
    static u8 wait_cnt = 0;         //等待第二个TX连接的闪烁计时, 250ms一次
    u8 status = wireless_conn_status;

    if (status != last_status) {
        flash_flag = 0;
        wait_cnt = 0;               //状态变化时重新计时10秒
        last_status = status;
    }

    if (status == 0) {
        //未连接：双灯闪烁
        flash_flag = !flash_flag;
        gpio_direction_output(IO_PORTB_07, flash_flag);
        gpio_direction_output(IO_PORTB_06, flash_flag);
        return;
    }

    flash_flag = !flash_flag;
    if ((status & BIT(0)) && (status & BIT(1))) {
        //连接两个TX：双灯常亮
        gpio_direction_output(IO_PORTB_07, 1);
        gpio_direction_output(IO_PORTB_06, 1);
        return;
    }

    //只连接一个TX：已连接的灯常亮，未连接的灯闪10秒后熄灭
    if (wait_cnt < 40) {            //40 * 250ms = 10s
        wait_cnt++;
        gpio_direction_output(IO_PORTB_07, (status & BIT(0)) ? 1 : flash_flag);
        gpio_direction_output(IO_PORTB_06, (status & BIT(1)) ? 1 : flash_flag);
    } else {
        gpio_direction_output(IO_PORTB_07, !!(status & BIT(0)));
        gpio_direction_output(IO_PORTB_06, !!(status & BIT(1)));
    }
}


void app_main()
{
    log_info("app_main\n");

    app_var.start_time = timer_get_ms();

    sys_s_hi_timer_add(NULL, led_mode_set, 250);

    printf("get_charge_online_flag() = %d", get_charge_online_flag());
    if (get_charge_online_flag()) {
#if(TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif
        app_curr_status = APP_CHARGE_STATUS;
    } else {
        app_curr_status = APP_NORMAL_STATUS;
    }
#if TCFG_CHARGE_ENABLE
    printf("set_charge_event_flag\n");
#ifdef CONFIG_CPU_BR28
    set_charge_event_flag(1);
#endif
#endif
    printf("app_curr_status = %d", app_curr_status);
    while (1) {
        switch (app_curr_status) {
        case APP_CHARGE_STATUS:
#if TCFG_CHARGE_ENABLE
            app_charge_run();
#endif
            break;
        default:
            break;
        }

        if (app_curr_status == APP_NORMAL_STATUS) {
            break;
        }
    }
    app_main_run();

}

