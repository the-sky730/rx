#include "app_config.h"
#include "app_task.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_error.h"
#include "btctrler/btctrler_task.h"
#include "classic/hci_lmp.h"
#include "bt_ble.h"
#include "bt_common.h"
#include "spp_user.h"
#include "app_main.h"
#include "user_cfg.h"
#include "asm/pwm_led.h"
#include "asm/timer.h"
#include "asm/hwi.h"
#include "cpu.h"
#include "ui/ui_api.h"
#include "clock_cfg.h"
#include "tone_player.h"
#include "bt_edr_fun.h"
#include "adapter_process.h"
#include "adapter_decoder.h"
#include "adapter_odev_bt.h"
#include "dongle_1t2.h"

#define LOG_TAG_CONST        BT
#define LOG_TAG             "[BT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define __this 	(&app_bt_hdl)

extern void ble_module_enable(u8 en);
/*************************************************************
  			蓝牙模式协议栈事件事件处理
**************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event     蓝牙协议栈事件过滤
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_hci_event_filter(struct bt_event *bt)
{
    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (0 == bt->value) {
            set_remote_test_flag(0);
            return 0;
        } else {

#if TCFG_USER_BLE_ENABLE
            //1:edr con;2:ble con;
            if (1 == bt->value) {
                /* extern void bt_ble_adv_enable(u8 enable); */
                /* bt_ble_adv_enable(0); */
                ble_module_enable(0);
            }
#endif
        }
    }

    if ((bt->event != HCI_EVENT_CONNECTION_COMPLETE) ||
        ((bt->event == HCI_EVENT_CONNECTION_COMPLETE) && (bt->value != ERROR_CODE_SUCCESS))) {
        if (get_remote_test_flag() \
            && !(HCI_EVENT_DISCONNECTION_COMPLETE == bt->event) \
            && !(HCI_EVENT_VENDOR_REMOTE_TEST == bt->event) \
            && !get_total_connect_dev()) {
            log_info("cpu reset\n");
            cpu_reset();
        }
    }

    if (__this->bt_close_bredr) {
        return 0;
    }

    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   搜索结束
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
__attribute__((weak))
void bt_hci_event_inquiry(struct bt_event *bt)
{
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接成功
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_hci_event_connection(struct bt_event *bt)
{
    log_info("-----------bt_hci_event_connection\n");

    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

    bt_user_priv_var.auto_connection_counter = 0;
    bt_wait_phone_connect_control(0);

    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}

void bt_recon_and_search_switch(void *p)
{
    u16 timeout = 4000;

    printf("counter = %d,p = %d", bt_user_priv_var.auto_connection_counter, (int)p);
    if (bt_user_priv_var.auto_connection_counter) {
        adapter_odev_edr_search_stop();
        if ((int)p == 0) {
            user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, bt_user_priv_var.auto_connection_addr);
        } else {
            user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            adapter_odev_edr_search_device();
        }

        bt_user_priv_var.auto_connection_timer = sys_timeout_add((void *)(!(int)p), bt_recon_and_search_switch, timeout);
    }

}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接断开
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_hci_event_disconnect(struct bt_event *bt)
{
    u8 local_addr[6];
    if (app_var.goto_poweroff_flag || __this->exiting) {
        return;
    }

    log_info("-----------bt_hci_event_disconnect\n");

    if (get_total_connect_dev() == 0) {    //已经没有设备连接
        sys_auto_shut_down_enable();
    }

    extern void adapter_process_event_notify(u8 event, int value);
#if USER_SUPPORT_DUAL_A2DP_SOURCE
    printf("get_total_connect_dev() = %d", get_total_connect_dev());
    if (get_total_connect_dev() == 0) {
        adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
    }
#else
    adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
#endif

#if (TCFG_AUTO_STOP_PAGE_SCAN_TIME && TCFG_BD_NUM == 2)
    if (get_total_connect_dev() == 1) {   //当前有一台连接上了
        if (app_var.auto_stop_page_scan_timer == 0) {
            app_var.auto_stop_page_scan_timer = sys_timeout_add(NULL, bt_close_page_scan, (TCFG_AUTO_STOP_PAGE_SCAN_TIME * 1000)); //2
        }
    } else {
        if (app_var.auto_stop_page_scan_timer) {
            sys_timeout_del(app_var.auto_stop_page_scan_timer);
            app_var.auto_stop_page_scan_timer = 0;
        }
    }
#endif   //endif AUTO_STOP_PAGE_SCAN_TIME

#if (TCFG_BD_NUM == 2)
    if ((bt->value == ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR) ||
        (bt->value == ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED)) {
        /*
         *连接接受超时
         *如果支持1t2，可以选择继续回连下一台，除非已经回连完毕
         */
        if (get_current_poweron_memory_search_index(NULL)) {
            user_send_cmd_prepare(USER_CTRL_START_CONNECTION, 0, NULL);
            return;
        }
    }
#endif

#if !USER_SUPPORT_DUAL_A2DP_SOURCE && !TCFG_USER_BLE_ENABLE
    bt_user_priv_var.auto_connection_counter = 8000;

    memcpy(bt_user_priv_var.auto_connection_addr, bt->args, 6);
    if (!bt_user_priv_var.auto_connection_timer) {
        bt_recon_and_search_switch(0);
    }
    //extern void adapter_odev_edr_search_device(void);
    //adapter_odev_edr_search_device();
#endif

}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接link key 错误
   @param
   @return
   @note   可能是取消配对
*/
/*----------------------------------------------------------------------------*/
static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   回链超时
   @param
   @return
   @note    回链超时内没有连接上设备
*/
/*----------------------------------------------------------------------------*/
static void bt_hci_event_page_timeout(struct bt_event *bt)
{
    log_info("-----------HCI_EVENT_PAGE_TIMEOUT conuter %d", bt_user_priv_var.auto_connection_counter);

    if (bt_user_priv_var.auto_connection_timer) {
        sys_timer_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

#if (TCFG_BD_NUM == 2)
    if (get_current_poweron_memory_search_index(NULL)) {
        user_send_cmd_prepare(USER_CTRL_START_CONNECTION, 0, NULL);
        return;
    }
#if USER_SUPPORT_DUAL_A2DP_SOURCE
    else {
        dongle_1t2_recon();
    }
#endif
#endif

#if !USER_SUPPORT_DUAL_A2DP_SOURCE
    //extern void adapter_odev_edr_search_device(void);
    //adapter_odev_edr_search_device();
    memcpy(bt_user_priv_var.auto_connection_addr, bt->args, 6);
    bt_user_priv_var.auto_connection_counter = 8000;
    r_f_printf("%s,%d", __func__, __LINE__);
    if (!bt_user_priv_var.auto_connection_timer) {
        bt_recon_and_search_switch(0);
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接超时
   @param
   @return
   @note    链接超时，设备拿远导致的超时等
*/
/*----------------------------------------------------------------------------*/
static void bt_hci_event_connection_timeout(struct bt_event *bt)
{
    if (!get_remote_test_flag() && !get_esco_busy_flag()) {
        bt_user_priv_var.auto_connection_counter = (TIMEOUT_CONN_TIME * 1000);
        memcpy(bt_user_priv_var.auto_connection_addr, bt->args, 6);

#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
        bt_wait_phone_connect_control(1);
#else
        //user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        //bt_wait_connect_and_phone_connect_switch(0);
#endif
        //user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, bt->args);
    } else {
        bt_wait_phone_connect_control(1);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接已经存在
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_hci_event_connection_exist(struct bt_event *bt)
{
    if (!get_remote_test_flag() && !get_esco_busy_flag()) {
        bt_user_priv_var.auto_connection_counter = (8 * 1000);

        memcpy(bt_user_priv_var.auto_connection_addr, bt->args, 6);

        if (bt_user_priv_var.auto_connection_timer) {
            sys_timer_del(bt_user_priv_var.auto_connection_timer);
            bt_user_priv_var.auto_connection_timer = 0;
        }
        bt_wait_connect_and_phone_connect_switch(0);
    } else {
        bt_wait_phone_connect_control(1);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙连接配置，提供app层用户可以输入配对鉴定key
   @param    key :配对需要输入的数字
   @return
   @note     配对需要输入6位数字的时候，按照顺序从左到右一个个输入
*/
/*----------------------------------------------------------------------------*/
static void bt_send_keypress(u8 key)
{
    user_send_cmd_prepare(USER_CTRL_KEYPRESS, 1, &key);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙连接配置，提供app层用户可以输入确定或者否定
   @param    en 0:否定   1:确定
   @return
   @note     在连接过程中类似手机弹出 确定和否定 按键，可以供用户界面设置
*/
/*----------------------------------------------------------------------------*/
static void bt_send_pair(u8 en)
{
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈事件
   @param    bt:事件
   @return
   @note     蓝牙底层事件,通过app层处理
*/
/*----------------------------------------------------------------------------*/
int bt_hci_event_handler(struct bt_event *bt, struct _odev_bt *odev_bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    g_printf("------------------------bt_hci_event_handler reason %x %x\n", bt->event, bt->value);

    if (bt_hci_event_filter(bt) == 0) {
        return 0;
    }

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        bt_hci_event_inquiry(bt);
        break;
    case HCI_EVENT_IO_CAPABILITY_REQUEST:
        log_info(" HCI_EVENT_IO_CAPABILITY_REQUEST \n");
        clock_add_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        clock_remove_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(1);
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        bt_hci_event_disconnect(bt) ;
        clock_remove_set(BT_CONN_CLK);
#if USER_SUPPORT_DUAL_A2DP_SOURCE
        dongle_1t2_hci_disconn();
#endif
        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            bt_hci_event_connection(bt);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;
        case ERROR_CODE_PAGE_TIMEOUT:
            log_info(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_hci_event_page_timeout(bt);
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_info("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_hci_event_connection_exist(bt);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}
