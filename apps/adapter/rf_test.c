
#include "app_config.h"
#include "generic/typedef.h"
#include "asm/gpio.h"
#include "app_task.h"
#include "asm/pwm_led.h"

#if TCFG_RF_TEST_EN

extern u32 handshake_1wire(u32 io, u32 timer_id, u32 timeout_ms, u32 pattern);
extern int lmp_hci_write_scan_enable(u8 enable);

void enter_dut_test()
{
    bt_init();
    bredr_set_dut_enble(1, 0);
    lmp_hci_write_scan_enable(3);
}
void rf_test_process()
{

    struct sys_event *event = NULL;
    if (handshake_1wire(IO_PORT_DP, 5, 100, 0x68af)) {
        printf("check uart key ok");
        sys_key_event_enable();
        enter_dut_test();
        pwm_led_mode_set(PWM_LED_ALL_ON);
        int msg[32];
        while (1) {
            app_task_get_msg(msg, ARRAY_SIZE(msg), 1);
            switch (msg[0]) {
            case APP_MSG_SYS_EVENT:
                event = (struct sys_event *)(&msg[1]);
                switch (event->type) {
                case SYS_KEY_EVENT:
                    printf("test end,enter softoff");
                    power_set_soft_poweroff();
                    break;
                }
                break;
            default:
                break;
            }
        }
    } else {
        printf("check uart key error");
    }

}

#endif
