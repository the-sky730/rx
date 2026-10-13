#include "system/includes.h"
#include "media/includes.h"
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
#include "app_chargestore.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_power_manage.h"
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
#include "aec_user.h"

#define LOG_TAG_CONST        BT
#define LOG_TAG             "[BT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

struct app_bt_opr app_bt_hdl = {
    .exit_flag = 1,
    .replay_tone_flag = 1,
    .esco_dump_packet = ESCO_DUMP_PACKET_CALL,
    .hid_mode = 0,
    .wait_exit = 0,
};

#define __this 	(&app_bt_hdl)

BT_USER_PRIV_VAR bt_user_priv_var;

#if !TCFG_USER_BT_CLASSIC_ENABLE
//库函数重写，测ble dut需要return 0
u8 dut_get_dual_mode_flag(void)
{
    return 0;
}
#endif
u8 get_bt_init_status(void)
{
    return __this->init_ok;
}

u8 is_call_now(void)
{
    if (get_call_status() != BT_CALL_HANGUP) {
        return 1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式led状态更新
   @param
   @return   status : 蓝牙状态
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_set_led_status(u8 status)
{
    static u8 bt_status = STATUS_BT_INIT_OK;

    if (status) {
        bt_status = status;
    }

    pwm_led_mode_set(PWM_LED_ALL_OFF);
    ui_update_status(bt_status);
}

/*************************************************************
                开关可发现可连接的函数接口
**************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙可发现可连接使能
   @param    enable 1:开启   0：关闭
   @return
   @note     函数根据状态开启，开启tws后该函数不起作用
   			 1拖2的时候会限制开启
*/
/*----------------------------------------------------------------------------*/
void bt_wait_phone_connect_control(u8 enable)
{
#if TCFG_USER_TWS_ENABLE
    return;
#endif

    if (enable && app_var.goto_poweroff_flag && (__this->exiting)) {
        return;
    }

    if (enable) {
        log_debug("is_1t2_connection:%d \t total_conn_dev:%d\n", is_1t2_connection(), get_total_connect_dev());
        if (is_1t2_connection()) {
            /*达到最大连接数，可发现(0)可连接(0)*/
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        } else {
            if (get_total_connect_dev() == 1) {
                /*支持连接2台，只连接一台的情况下，可发现(0)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            } else {
                /*可发现(1)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            }
        }
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取vm连接记录信息
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_init_ok_search_index(void)
{
    if (!bt_user_priv_var.auto_connection_counter && get_current_poweron_memory_search_index(bt_user_priv_var.auto_connection_addr)) {
        /* log_info("bt_wait_connect_and_phone_connect_switch\n"); */
        clear_current_poweron_memory_search_index(1);
        bt_user_priv_var.auto_connection_counter = POWERON_AUTO_CONN_TIME * 1000; //8000ms
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙可发现可连接\回链轮询使能
   @param    p 1:开启轮询   0：不开启轮询,只开启可发现可连接
   @return
   @note     auto_connection_counter:回链设备的总时间
*/
/*----------------------------------------------------------------------------*/
int bt_wait_connect_and_phone_connect_switch(void *p)
{
    int ret = 0;
    int timeout = 0;
    s32 wait_timeout;

    if (__this->exiting) {
        return 0;
    }

    log_debug("connect_switch: %d, %d\n", (int)p, bt_user_priv_var.auto_connection_counter);

    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

    if (bt_user_priv_var.auto_connection_counter <= 0) {
        bt_user_priv_var.auto_connection_timer = 0;
        bt_user_priv_var.auto_connection_counter = 0;



        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        if (get_current_poweron_memory_search_index(NULL)) {
            bt_init_ok_search_index();
            return bt_wait_connect_and_phone_connect_switch(0);
        } else {
            bt_wait_phone_connect_control(1);
            return 0;
        }
    }
    /* log_info(">>>phone_connect_switch=%d\n",bt_user_priv_var.auto_connection_counter ); */
    if ((int)p == 0) {  //// 根据地址回链
        if (bt_user_priv_var.auto_connection_counter) {
            timeout = 6000;     ////设置回链6s
            bt_wait_phone_connect_control(0);
            user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, bt_user_priv_var.auto_connection_addr);
            ret = 1;
            bt_user_priv_var.auto_connection_counter -= timeout ;
        }
    } else {
        timeout = 2000;   ////设置开始被发现被连接2s
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        bt_wait_phone_connect_control(1);
    }

    bt_user_priv_var.auto_connection_timer = sys_timeout_add((void *)(!(int)p),
            bt_wait_connect_and_phone_connect_switch, timeout);

    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙关闭可发现可连接
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_close_page_scan(void *p)
{
    bt_wait_phone_connect_control(0);
    sys_timer_del(app_var.auto_stop_page_scan_timer);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙通话开始丢包数
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
#if ESCO_DUMP_PACKET_ADJUST
u8 get_esco_packet_dump(void)
{
    //log_info("esco_dump_packet:%d\n", esco_dump_packet);
    return __this->esco_dump_packet;
}
#endif

/*************************************************************
  	     蓝牙模式app层检测空闲是否可以进入sniff模式
**************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙退出sniff模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_check_exit_sniff()
{
    sys_timeout_del(__this->exit_sniff_timer);
    __this->exit_sniff_timer = 0;

    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙检测是否空闲，空闲就发起sniff
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_check_enter_sniff()
{
    struct sniff_ctrl_config_t sniff_ctrl_config;
    u8 addr[12];
    u8 conn_cnt = 0;
    u8 i = 0;
    /*putchar('H');*/
    conn_cnt = bt_api_enter_sniff_status_check(SNIFF_CNT_TIME, addr);

    ASSERT(conn_cnt <= 2);

    for (i = 0; i < conn_cnt; i++) {
        log_debug("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        sniff_ctrl_config.sniff_max_interval = SNIFF_MAX_INTERVALSLOT;
        sniff_ctrl_config.sniff_mix_interval = SNIFF_MIN_INTERVALSLOT;
        sniff_ctrl_config.sniff_attemp = SNIFF_ATTEMPT_SLOT;
        sniff_ctrl_config.sniff_timeout  = SNIFF_TIMEOUT_SLOT;
        memcpy(sniff_ctrl_config.sniff_addr, addr + i * 6, 6);

        user_send_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(struct sniff_ctrl_config_t), (u8 *)&sniff_ctrl_config);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙启动 检测空闲 发起sniff
   @param   enable : 1 启动  0:关闭
   			addr:要和对应设备进入sniff 状态的地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
#if (TCFG_BT_SNIFF_ENABLE == 0)
    return;
#endif

    if (addr) {
        if (bt_api_conn_mode_check(enable, addr) == 0) {
            log_debug("sniff ctr not change\n");
            return;
        }
    }

    if (enable) {
        if (get_total_connect_dev() == 0) {
            return;
        }

        if (addr) {
            log_debug("sniff cmd timer init\n");
            user_cmd_timer_init();
        }

        if (bt_user_priv_var.sniff_timer == 0) {
            log_debug("check_sniff_enable\n");
            bt_user_priv_var.sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, 1000);
        }
    } else {
        if (get_total_connect_dev() > 0) {
            return;
        }
        if (addr) {
            log_debug("sniff cmd timer remove\n");
            remove_user_cmd_timer();
        }

        if (bt_user_priv_var.sniff_timer) {
            log_info("check_sniff_disable\n");
            sys_timeout_del(bt_user_priv_var.sniff_timer);
            bt_user_priv_var.sniff_timer = 0;

            if (__this->exit_sniff_timer == 0) {
                /* exit_sniff_timer = sys_timer_add(NULL, bt_check_exit_sniff, 5000); */
            }
        }
    }
}

/*************************************************************
  蓝牙模式关机处理
 **************************************************************/
/*----------------------------------------------------------------------------*/
/**@brief    软关机蓝牙模式处理
   @param
   @return
   @note     断开蓝牙链接\关闭所有状态
*/
/*----------------------------------------------------------------------------*/
static void wait_exit_btstack_flag(void *priv)
{
    sys_timer_del(app_var.wait_exit_timer);
    app_var.wait_exit_timer = 0;
    if (priv == NULL) {
        //enter softpoweroff mode
    } else if (priv == (void *)1) {
        log_info("cpu_reset!!!\n");
        cpu_reset();
    }

    adapter_process_event_notify(ADAPTER_EVENT_POWEROFF, 0);
}

void sys_enter_soft_poweroff(void *priv)
{
    int detach_phone = 1;
    struct sys_event clear_key_event = {.type =  SYS_KEY_EVENT, .arg = "key"};

    log_info("%s", __func__);

    bt_user_priv_var.emitter_or_receiver = 0;

    if (app_var.goto_poweroff_flag) {
        return;
    }

    ui_update_status(STATUS_POWEROFF);

    app_var.goto_poweroff_flag = 1;
    app_var.goto_poweroff_cnt = 0;


    if (app_var.wait_exit_timer == 0) {
        app_var.wait_exit_timer = sys_timer_add(priv, wait_exit_btstack_flag, 50);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   自动关机开关
   @param
   @return
   @note    开启后，如果一段时间内没有连接等操作就会进入关机
*/
/*----------------------------------------------------------------------------*/
#if 0
void sys_auto_shut_down_enable(void)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    log_debug("sys_auto_shut_down_enable\n");

    if (app_var.auto_shut_down_timer == 0) {
        app_var.auto_shut_down_timer = sys_timeout_add(NULL, sys_enter_soft_poweroff, (app_var.auto_off_time * 1000));
    } else {//在切换到蓝牙任务APP_STA_START中，current_app为空
        sys_auto_shut_down_disable();
        app_var.auto_shut_down_timer = sys_timeout_add(NULL, sys_enter_soft_poweroff, (app_var.auto_off_time * 1000));
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief   关闭自动关机
   @param
   @return
   @note    链接上设备后会调用关闭
*/
/*----------------------------------------------------------------------------*/
void sys_auto_shut_down_disable(void)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    log_debug("sys_auto_shut_down_disable\n");
    if (app_var.auto_shut_down_timer) {
        sys_timeout_del(app_var.auto_shut_down_timer);
        app_var.auto_shut_down_timer = 0;
    }
#endif
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙spp 协议数据 回调
   @param    packet_type:数据类型
  			 ch         :
			 packet     :数据缓存
			size        ：数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size)
{
    switch (packet_type) {
    case 1:
        log_debug("spp connect\n");
        break;
    case 2:
        log_debug("spp disconnect\n");
        break;
    case 7:
        //log_info("spp_rx:");
        //put_buf(packet,size);
#if AEC_DEBUG_ONLINE
        aec_debug_online(packet, size);
#endif
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取在连接设备名字回调
   @param    status : 1：获取失败   0：获取成功
			 addr   : 配对设备地址
			 name   :配对设备名字
   @return
   @note     需要连接上设备后发起USER_CTRL_READ_REMOTE_NAME
   			 命令来
*/
/*----------------------------------------------------------------------------*/
__attribute__((weak))
void remote_name_speciali_deal(u8 status, u8 *addr, u8 *name)
{

}

static void bt_read_remote_name(u8 status, u8 *addr, u8 *name)
{
    if (status) {
        log_debug("remote_name fail \n");
    } else {
        log_debug("remote_name : %s \n", name);
    }

    log_debug_hexdump(addr, 6);

    remote_name_speciali_deal(status, addr, name);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙歌词信息获取回调
   @param
   @return
   @note
   const u8 more_avctp_cmd_support = 1;置上1
   需要在void bredr_handle_register()注册回调函数
   要动态获取播放时间的，可以发送USER_CTRL_AVCTP_OPID_GET_PLAY_TIME命令就可以了
   要半秒或者1秒获取就做个定时发这个命令
*/
/*----------------------------------------------------------------------------*/
static void user_get_bt_music_info(u8 type, u32 time, u8 *info, u16 len)
{
    //profile define type: 1-title 2-artist name 3-album names 4-track number 5-total number of tracks 6-genre  7-playing time
    //JL define 0x10-total time , 0x11 current play position
    u8  min, sec;
    //printf("type %d\n", type );
    if ((info != NULL) && (len != 0)) {
        log_debug(" %s \n", info);
    }
    if (time != 0) {
        min = time / 1000 / 60;
        sec = time / 1000 - (min * 60);
        log_debug(" time %d %d\n ", min, sec);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取样机当前电量
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_get_battery_value()
{
    //取消默认蓝牙定时发送电量给手机，需要更新电量给手机使用USER_CTRL_HFP_CMD_UPDATE_BATTARY命令
    /*电量协议的是0-9个等级，请比例换算*/
    return get_cur_battery_level();
}

//音量同步
static void vol_sys_tab_init(void)
{
}

static void bt_set_music_device_volume(int volume)
{
    r_printf("bt_set_music_device_volume : %d\n", volume);
}

static int phone_get_device_vol(void)
{
    //音量同步最大是127，请计数比例
#if 0
    return (app_var.sys_vol_l * get_max_sys_vol() / 127) ;
#else
    return app_var.opid_play_vol_sync;
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈功能配置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_function_select_init()
{
    set_idle_period_slot(1600);
    ////设置协议栈支持设备数
    __set_user_ctrl_conn_num(TCFG_BD_NUM);
    ////msbc功能使能
    __set_support_msbc_flag(1);
#if TCFG_BT_SUPPORT_AAC
    ////AAC功能使能
    __set_support_aac_flag(1);
#else
    __set_support_aac_flag(0);
#endif

#if BT_SUPPORT_DISPLAY_BAT
    ////设置更新电池电量的时间间隔
    __bt_set_update_battery_time(60);
#else
    __bt_set_update_battery_time(0);
#endif

    ////回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16
    __set_page_timeout_value(8000);

    ////回连时超时参数设置。ms单位。做主机有效
    __set_super_timeout_value(8000);

#if (TCFG_BD_NUM == 2)
    ////设置开机回链的设备个数
    __set_auto_conn_device_num(2);
#endif

#if BT_SUPPORT_MUSIC_VOL_SYNC
    ////设置音乐音量同步的表
    vol_sys_tab_init();
#endif

    ////设置蓝牙是否跑后台
    __set_user_background_goback(BACKGROUND_GOBACK); // 后台链接是否跳回蓝牙 1:跳回

    ////设置蓝牙加密的level
    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //authentication_requirements: 0:not protect  1 :protect
    __set_simple_pair_param(3, 0, 2);


#if (USER_SUPPORT_PROFILE_PBAP==1)
    ////设置蓝牙设备类型
    __change_hci_class_type(BD_CLASS_CAR_AUDIO);
#endif

#if (TCFG_BT_SNIFF_ENABLE == 0)
    void lmp_set_sniff_disable(void);
    lmp_set_sniff_disable();
#endif


    /*
                TX     RX
       AI800x   PA13   PA12
       AC692x   PA13   PA12
       AC693x   PA8    PA9
       AC695x   PA9    PA10
       AC696x   PA9    PA10
       AC694x   PB1    PB2
       AC697x   PC2    PC3
       AC631x   PA7    PA8

    */
    ////设置蓝牙接收状态io输出，可以外接pa
    /* bt_set_rxtx_status_enable(1); */

#if TCFG_USER_BLE_ENABLE
    {
        u8 tmp_ble_addr[6];
        lib_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
        le_controller_set_mac((void *)tmp_ble_addr);

        printf("\n-----edr + ble 's address-----");
        printf_buf((void *)bt_get_mac_addr(), 6);
        printf_buf((void *)tmp_ble_addr, 6);
    }
#endif // TCFG_USER_BLE_ENABLE

#if (CONFIG_BT_MODE != BT_NORMAL)
    set_bt_enhanced_power_control(1);
#endif
}
static void bt_dut_disconnect_api(u8 *addr, int reason)
{
    /* lmp_scan_enable(3);  */
    bt_wait_phone_connect_control(1);
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈回调函数
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void bredr_handle_register()
{
#if (USER_SUPPORT_PROFILE_SPP==1)
#if APP_ONLINE_DEBUG
    extern void online_spp_init(void);
    spp_data_deal_handle_register(user_spp_data_handler);
    online_spp_init();
#else
    spp_data_deal_handle_register(spp_data_handler);
#endif
#endif

#if BT_SUPPORT_MUSIC_VOL_SYNC
    ///蓝牙音乐和通话音量同步
    music_vol_change_handle_register(bt_set_music_device_volume, phone_get_device_vol);
#endif

#if BT_SUPPORT_DISPLAY_BAT
    ///电量显示获取电量的接口
    get_battery_value_register(bt_get_battery_value);
#endif

    extern void bt_fast_test_api(void);
    ///被测试盒链接上进入快速测试回调
    bt_fast_test_handle_register(bt_fast_test_api);

    extern void bt_dut_api(u8 value);
    ///样机进入dut被测试仪器链接上回调
    bt_dut_test_handle_register(bt_dut_api);

    ///获取远端设备蓝牙名字回调
    read_remote_name_handle_register(bt_read_remote_name);

    discon_complete_handle_register(bt_dut_disconnect_api);
    ////获取歌曲信息回调
    /* bt_music_info_handle_register(user_get_bt_music_info); */
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙直接开关
   @param
   @return
   @note   如果想后台开机不需要进蓝牙，可以在poweron模式
   			直接调用这个函数初始化蓝牙
*/
/*----------------------------------------------------------------------------*/

void bt_init()
{
    if (__this->bt_direct_init) {
        return;
    }

    log_info(" bt_direct_init  \n");

    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    __this->ignore_discon_tone = 0;
    __this->bt_direct_init = 1;

    bt_function_select_init();
    bredr_handle_register();
    btstack_init();

    /* 按键消息使能 */
    sys_key_event_enable();
    sys_auto_shut_down_enable();
    sys_auto_sniff_controle(1, NULL);
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙 退出蓝牙等待蓝牙状态可以退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_close_check(void *priv)
{
    if (bt_user_priv_var.auto_connection_timer) {
        sys_timeout_del(bt_user_priv_var.auto_connection_timer);
        bt_user_priv_var.auto_connection_timer = 0;
    }

    if (__this->init_ok == 0) {
        /* putchar('#'); */
        return;
    }

    // if (bt_audio_is_running()) {
    //     /* putchar('$'); */
    //     return;
    // }

#if TCFG_USER_BLE_ENABLE
    bt_ble_exit();
#endif

    btstack_exit();
    log_info(" bt_direct_close_check ok\n");
    sys_timer_del(__this->timer);
    __this->init_ok = 0;
    __this->init_start = 0;
    __this->timer = 0;
    __this->bt_direct_init = 0;
    set_stack_exiting(0);
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台直接关闭
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_close(void)
{
    if (__this->init_ok == 0) {
        /* putchar('#'); */
        return;
    }

    log_info(" bt_direct_close");

    __this->ignore_discon_tone = 1;
    sys_auto_shut_down_disable();

    set_stack_exiting(1);

    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (__this->timer == 0) {
        __this->tmr_cnt = 0;
        __this->timer = sys_timer_add(NULL, bt_close_check, 10);
        printf("set exit timer\n");
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙通话时候设置不让系统进入idle状态,不让系统进powerdown
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 check_phone_income_idle(void)
{
    if (bt_user_priv_var.phone_ring_flag) {
        return 0;
    }
    return 1;
}
REGISTER_LP_TARGET(phone_incom_lp_target) = {
    .name       = "phone_check",
    .is_idle    = check_phone_income_idle,
};
