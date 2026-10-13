#include "system/includes.h"
#include "key_event_deal.h"
#include "adapter_app_status.h"
#include "app_task.h"
#include "app_config.h"

#if TCFG_CHARGE_ENABLE
//*----------------------------------------------------------------------------*/
/**@brief    idle 启动
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void idle_app_start()
{
#if (TCFG_CHARGE_POWERON_ENABLE)
    sys_key_event_enable();
#endif
}

u8 app_charge_key_event_deal(struct sys_event *event)
{
    int ret = 0;
    struct key_event *key = &event->u.key;

    u8 key_event = event->u.key.event;
    static u8 key_poweron_cnt = 0;
    static u8 flag_poweron = 0;
    printf("key_event:%d %d %d\n", key_event, key->value, key->event);
    switch (key_event) {
    case  KEY_POWEROFF:
        printf("KEY POWEROFF\n");
        key_poweron_cnt = 0;
        flag_poweron = 1;
        break;
    case  KEY_POWEROFF_HOLD:
        printf("KEY POWEROFF_HOLD\n");
        if (flag_poweron) {
#if TCFG_CHARGE_POWERON_ENABLE
            if (++key_poweron_cnt >= 15) {
                printf("power on from charge");
                key_poweron_cnt = 0;
                ret = 1;
            }
#endif
        }
    case  KEY_NULL:
        break;
    }
    return ret;
}

extern u8 app_common_device_event_deal(struct sys_event *event);
u8 charge_default_event_deal(struct sys_event *event)
{
    u8 ret = 0;
    switch (event->type) {
    case SYS_KEY_EVENT:
        ret = app_charge_key_event_deal(event);
        break;
    case SYS_DEVICE_EVENT:
        ret = app_common_device_event_deal(event);
        break;
    default:
        break;

    }
    return ret;

}
//*----------------------------------------------------------------------------*/
/**@brief    idle 主任务
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
extern u8 app_task_exitting();
void app_charge_run(void)
{
    int ret = 0;
    int msg[32];

    idle_app_start();

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            ret = charge_default_event_deal((struct sys_event *)(&msg[1]));    //由common统一处理
            break;
        default:
            break;
        }
#if TCFG_TEST_BOX_ENABLE
        extern u8 chargestore_get_testbox_status(void);
        if (chargestore_get_testbox_status()) {
            ret = 1;
        }
#endif
        if (ret) {
            app_curr_status = APP_NORMAL_STATUS;
            printf("app_charge_break");
            break;
        }
    }
    printf("exit charge_run");
}
#endif
