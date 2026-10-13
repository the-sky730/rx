#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "app_task.h"
#include "event.h"

//ad key
extern const u16 key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];

/***********************************************************
 *				adkey table 映射管理
 ***********************************************************/
typedef const u16(*type_key_ad_table)[KEY_EVENT_MAX];

u16 adkey_event_to_msg(struct key_event *key)
{
    type_key_ad_table cur_task_ad_table = key_ad_table;
    return	cur_task_ad_table[key->value][key->event];
}

//io key
extern const u16 key_io_table[KEY_IO_NUM_MAX][KEY_EVENT_MAX];

/***********************************************************
 *				iokey table 映射管理
 ***********************************************************/
typedef const u16(*type_key_io_table)[KEY_EVENT_MAX];

u16 iokey_event_to_msg(struct key_event *key)
{
    type_key_io_table cur_task_io_table = key_io_table;
    return	cur_task_io_table[key->value][key->event];
}

//ir key
extern const u16 key_ir_table[KEY_IR_NUM_MAX][KEY_EVENT_MAX];

/***********************************************************
 *				irkey table 映射管理
 ***********************************************************/
typedef const u16(*type_key_ir_table)[KEY_EVENT_MAX];

u16 irkey_event_to_msg(struct key_event *key)
{

    type_key_ir_table cur_task_ir_table = key_ir_table;
    return	cur_task_ir_table[key->value][key->event];
}

//rdec key
extern const u16 key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX];

/***********************************************************
 *				rdec_key table 映射管理
 ***********************************************************/
typedef const u16(*type_key_rdec_table)[KEY_EVENT_MAX];

u16 rdec_key_event_to_msg(struct key_event *key)
{
    type_key_rdec_table cur_task_rdec_table = key_rdec_table;
    return	cur_task_rdec_table[key->value][key->event];
}

//touch key
extern const u16 key_touch_table[KEY_TOUCH_NUM_MAX][KEY_EVENT_MAX];

/***********************************************************
 *				touch_key table 映射管理
 ***********************************************************/
typedef const u16(*type_key_touch_table)[KEY_EVENT_MAX];

u16 touch_key_event_to_msg(struct key_event *key)
{
    type_key_touch_table cur_task_touch_table = key_touch_table;
    return	cur_task_touch_table[key->value][key->event];
}

//*----------------------------------------------------------------------------*/
/**@brief    模式按键映射处理
   @param    e:按键事件
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

int app_key_event_remap(struct sys_event *e)
{
    struct key_event *key = &e->u.key;
    int msg = KEY_NULL;
    switch (key->type) {
    case KEY_DRIVER_TYPE_IO:
#if TCFG_IOKEY_ENABLE
        msg = iokey_event_to_msg(key);
#endif
        break;

    case KEY_DRIVER_TYPE_AD:
    case KEY_DRIVER_TYPE_RTCVDD_AD:
#if TCFG_ADKEY_ENABLE
        msg = adkey_event_to_msg(key);
#endif
        break;

    case KEY_DRIVER_TYPE_IR:
#if TCFG_IRKEY_ENABLE
        msg = irkey_event_to_msg(key);
#endif
        break;

    case KEY_DRIVER_TYPE_TOUCH:
#if TCFG_TOUCH_KEY_ENABLE
        msg = touch_key_event_to_msg(key);
#endif
        break;

    case KEY_DRIVER_TYPE_RDEC:
#if TCFG_RDEC_KEY_ENABLE
        msg = rdec_key_event_to_msg(key);
#endif
        break;

    case KEY_DRIVER_TYPE_SOFTKEY:
        msg = key->event;
        break;
    default:
        break;
    }

    if (msg == KEY_NULL) {
        e->consumed = 1;//接管按键消息,app应用不会收到消息
        return FALSE;
    }

    e->u.key.event = msg;
    e->u.key.value = 0;

    return TRUE;//notify数据
}

SYS_EVENT_HANDLER(SYS_KEY_EVENT,  app_key_event_remap, 3);

