
#include "dongle_1t2.h"
#include "bt_edr_fun.h"
#include "btstack/avctp_user.h"
#include "timer.h"
#include "ui/io_led.h"

#if USER_SUPPORT_DUAL_A2DP_SOURCE


struct de_role de_role1;
struct de_role de_role2;

#if TCFG_IOLED_ENABLE
struct led_mode_t ch1_led_mode = {
    .poweron = LED0_SLOW_FLASH,
    .connect = LED0_ON,
    .disconn = LED0_SLOW_FLASH,
    .search  = LED0_FAST_FLASH
};
struct led_mode_t ch2_led_mode = {
    .poweron = LED1_SLOW_FLASH,
    .connect = LED1_ON,
    .disconn = LED1_SLOW_FLASH,
    .search  = LED1_FAST_FLASH
};
#endif


static u8 flag_sw_recon = 0;
static u8 zero_addr[6] = {0x00};
static u8 flag_recon_1t2 = 0;
extern void get_remote_device_info_from_vm(void);
extern u8 *get_mac_memory_by_index(u8 index);
void dongle_1t2_ch_search(u8 ch);

//poweron get vm bt_mac_addr
void dongle_1t2_init(struct _odev_edr *edr)
{
    get_remote_device_info_from_vm();
    memcpy(de_role1.mac_addr, get_mac_memory_by_index(1), 6);
    memcpy(de_role2.mac_addr, get_mac_memory_by_index(2), 6);
    printf("vm mac 1 = ");
    put_buf(de_role1.mac_addr, 6);
    printf("vm mac 2 = ");
    put_buf(de_role2.mac_addr, 6);

#if TCFG_IOLED_ENABLE
    de_role1.ledmode = ch1_led_mode;
    de_role2.ledmode = ch2_led_mode;
    ioled_update_status(de_role1.ledmode.poweron);
    ioled_update_status(de_role2.ledmode.poweron);
#endif

    if (memcmp(de_role1.mac_addr, zero_addr, 6) != 0) {
        de_role1.status = DB_EM_RECONNECT;
    } else if (edr->parm->poweron_start_search == 1) {
        dongle_1t2_ch_search(DE_CH1);
    }
    if (memcmp(de_role2.mac_addr, zero_addr, 6) != 0) {
        de_role2.status = DB_EM_RECONNECT;
    } else if (edr->parm->poweron_start_search == 1) {
        dongle_1t2_ch_search(DE_CH2);
    }
    edr->parm->poweron_start_search = 0;//clear status
    printf("de_role1.status = %d,de_role2.status = %d", de_role1.status, de_role2.status);
}
static void dongle_1t2_connect_ch(struct bt_event *bt, struct de_role *role)
{
    printf("%s,%d", __func__, __LINE__);
    if (role->search_timer_id) {
        sys_timeout_del(role->search_timer_id);
        role->search_timer_id = 0;
    }
    memset(role->mac_addr, 0x00, 6);
    memcpy(role->mac_addr, bt->args, 6);
    role->status = DB_EM_CONNECTED;
#if TCFG_IOLED_ENABLE
    ioled_update_status(role->ledmode.connect);
#endif
}
static void dongle_1t2_recon_ch(struct de_role *role)
{
    printf("%s,%d", __func__, __LINE__);
    if (role->status <= DB_EM_RECONNECT) {
        printf("%s,%d", __func__, __LINE__);
        if (memcmp(role->mac_addr, zero_addr, 6) != 0) {
            bt_user_priv_var.auto_connection_counter = 4000;
            printf("recon_1t2 mac = ");
            put_buf(role->mac_addr, 6);
            user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, role->mac_addr);
            role->status = DB_EM_RECONNECT;
            flag_recon_1t2 = 1;
        }
    }
}
void dongle_1t2_recon(void)
{

    u8 role_have_mac = 0;
    r_printf("%s,%d", __func__, __LINE__);
    printf("de_role1.status = %d,de_role2.status = %d", de_role1.status, de_role2.status);
    if (memcmp(de_role1.mac_addr, zero_addr, 6) != 0) {
        role_have_mac |= BIT(0);
    }
    if (memcmp(de_role2.mac_addr, zero_addr, 6) != 0) {
        role_have_mac |=  BIT(1);
    }
    if (de_role1.status <= DB_EM_RECONNECT && de_role2.status <= DB_EM_RECONNECT) {
        if ((role_have_mac & 0x03) == 0x03) {
            flag_sw_recon = !flag_sw_recon;//switch recon ch
        } else if (role_have_mac & BIT(0)) {
            flag_sw_recon = 0;
        } else if (role_have_mac & BIT(1)) {
            flag_sw_recon = 1;
        }
    } else if (de_role1.status <= DB_EM_RECONNECT && (role_have_mac & BIT(0))) {
        flag_sw_recon = 0;
    } else if (de_role2.status <= DB_EM_RECONNECT && (role_have_mac & BIT(1))) {
        flag_sw_recon = 1;
    }

    printf("flag_sw_recon = %d", flag_sw_recon);
    put_buf(de_role1.mac_addr, 6);
    put_buf(de_role2.mac_addr, 6);
    if (flag_sw_recon == 0) {
        dongle_1t2_recon_ch(&de_role1);
    } else if (flag_sw_recon == 1) {
        dongle_1t2_recon_ch(&de_role2);

    }
    printf("de_role1.status = %d,de_role2.status = %d", de_role1.status, de_role2.status);
}
//bt_connect ,ch update status and mac_addr
void dongle_1t2_connect(struct bt_event *bt)
{
    put_buf(bt->args, 6);
    put_buf(de_role1.mac_addr, 6);
    put_buf(de_role2.mac_addr, 6);
    flag_recon_1t2 = 0;
    printf("de_role1.status = %d,de_role2.status = %d", de_role1.status, de_role2.status);
    if (de_role1.status == DB_EM_RECONNECT && !memcmp(de_role1.mac_addr, bt->args, 6)) {
        printf("%s,%d", __func__, __LINE__);
        dongle_1t2_connect_ch(bt, &de_role1);
    } else if (de_role2.status == DB_EM_RECONNECT && !memcmp(de_role2.mac_addr, bt->args, 6)) {
        printf("%s,%d", __func__, __LINE__);
        dongle_1t2_connect_ch(bt, &de_role2);
    } else {
        printf("%s,%d", __func__, __LINE__);
        if (de_role1.status == DB_EM_SEARCHING && de_role2.status == DB_EM_SEARCHING) {
            if (!memcmp(de_role1.mac_addr, bt->args, 6)) { //一一对应
                printf("%s,%d", __func__, __LINE__);
                dongle_1t2_connect_ch(bt, &de_role1);
            } else {
                printf("%s,%d", __func__, __LINE__);
                dongle_1t2_connect_ch(bt, &de_role2);
            }
        } else if (de_role1.status == DB_EM_SEARCHING) {
            printf("%s,%d", __func__, __LINE__);
            dongle_1t2_connect_ch(bt, &de_role1);
        } else if (de_role2.status == DB_EM_SEARCHING) {
            printf("%s,%d", __func__, __LINE__);
            dongle_1t2_connect_ch(bt, &de_role2);

        }
    }
    printf("de_role1.search_timer_id = %d,de_role2.search_timer_id = %d", de_role1.search_timer_id, de_role2.search_timer_id);
    if (de_role1.search_timer_id || de_role2.search_timer_id) {
        adapter_odev_edr_search_stop();//先关闭再重新打开，有情况连接一个之后会无法搜索
        adapter_odev_edr_search_device();
    }
    printf("de_role1.status = %d,de_role2.status = %d", de_role1.status, de_role2.status);
    if (de_role1.status != DB_EM_CONNECTED || de_role2.status != DB_EM_CONNECTED) {
        dongle_1t2_recon();
    }
}
//统一处理超距和从机关机的回连
void dongle_1t2_hci_disconn(void)
{
    printf("%s", __func__);
    printf("de_role1.status = %d,de_role2.status = %d", de_role1.status, de_role2.status);
    if (de_role1.status == DB_EM_SEARCHING || de_role2.status == DB_EM_SEARCHING) {
        printf("search no recon");
        return;
    }
    if (!flag_recon_1t2) {
        dongle_1t2_recon();
    }
}
static void dongle_1t2_bt_ch_disconn(struct bt_event *bt, struct de_role *role)
{
    printf("%s,de_role1.status = %d,de_role2.status = %d", __func__, de_role1.status, de_role2.status);
    if (role->status == DB_EM_SEARCHING) {
        printf("searching,no update disconn UI and status");
    } else if (role->status == DB_EM_CONNECTED) {
        if (!memcmp(role->mac_addr, bt->args, 6)) {
            printf("role_disconn");
            role->status = DB_EM_NOSEARCHING;
#if TCFG_IOLED_ENABLE
            ioled_update_status(role->ledmode.disconn);
#endif
        }
    }
}
//disconn,ch update status
void dongle_1t2_bt_disconn(struct bt_event *bt)
{
    put_buf(bt->args, 6);
    put_buf(de_role1.mac_addr, 6);
    put_buf(de_role2.mac_addr, 6);
    dongle_1t2_bt_ch_disconn(bt, &de_role1);
    dongle_1t2_bt_ch_disconn(bt, &de_role2);
}


static void dongle_1t2_role_search_timer(void *role)
{
    struct de_role *role_tp = (struct doub_em_role *)role;
    if (role_tp->search_timer_id) {
        role_tp->search_timer_id = 0;
#if TCFG_IOLED_ENABLE
        ioled_update_status(role_tp->ledmode.disconn);
#endif
        role_tp->status = DB_EM_NOSEARCHING;
        if (!de_role1.search_timer_id && !de_role2.search_timer_id) {
            r_printf("STOP_SEARCH");
            adapter_odev_edr_search_stop();
            dongle_1t2_recon();
        }
    }

}
//主动断开通道
void dongle_1t2_discon_ch(u8 ch)
{
    struct de_role *role_tp;
    if (ch == DE_CH1) {
        role_tp = &de_role1;
    } else if (ch == DE_CH2) {
        role_tp = &de_role2;
    } else {
        printf("error ch");
        return;
    }
    if (role_tp->status == DB_EM_CONNECTED) {
        //断开连接
        printf("disconn mac addr");
        put_buf(role_tp->mac_addr, 6);
        user_emitter_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, role_tp->mac_addr);
    }
}
//选择通道搜索
void dongle_1t2_ch_search(u8 ch)
{
    struct de_role *role_tp;
    if (ch == DE_CH1) {
        role_tp = &de_role1;
    } else if (ch == DE_CH2) {
        role_tp = &de_role2;
    } else {
        printf("error ch");
        return;
    }
    printf("dongle_1t2_ch_search,role->status = %d", role_tp->status);
    role_tp->status = DB_EM_SEARCHING;
#if TCFG_IOLED_ENABLE
    ioled_update_status(role_tp->ledmode.search);
#endif
    adapter_odev_edr_search_device();
    if (!role_tp->search_timer_id) {
        printf("add role search_timer");
        role_tp->search_timer_id = sys_timeout_add((void *)role_tp, dongle_1t2_role_search_timer, SEARCH_TIME);
    } else {	//reset time
        sys_timeout_del(role_tp->search_timer_id);
        role_tp->search_timer_id = sys_timeout_add((void *)role_tp, dongle_1t2_role_search_timer, SEARCH_TIME);
    }
    r_printf("tid = %d,role status = %d", role_tp->search_timer_id, role_tp->status);



}
static u16 delay_search_tid = 0;
static void delay_search_timer(void *ch)
{
    if (delay_search_tid) {
        delay_search_tid = 0;
        u8 channel = (u8)ch;
        if (channel == DE_CH1) {
            dongle_1t2_ch_search(DE_CH1);
        } else if (channel == DE_CH2) {
            dongle_1t2_ch_search(DE_CH2);
        } else if (channel == DE_CH1 | DE_CH2) {
            dongle_1t2_ch_search(DE_CH1);
            dongle_1t2_ch_search(DE_CH2);
        }
    }
}
//选择ch断连和搜索
void dongle_1t2_ch_discon_search(u8 ch)
{
    printf("%s", __func__);
    user_send_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    adapter_odev_edr_search_stop();

    if (ch == DE_CH1) {
        dongle_1t2_discon_ch(DE_CH1);
    } else if (ch == DE_CH2) {
        dongle_1t2_discon_ch(DE_CH2);
    } else if (ch == DE_CH1 | DE_CH2) {
        dongle_1t2_discon_ch(DE_CH1);
        dongle_1t2_discon_ch(DE_CH2);
    }

    //断开之后不能马上去搜索，有可能会连不上
    if (!delay_search_tid) {
        delay_search_tid = sys_timeout_add((void *)ch, delay_search_timer, 1000);
    }

}

#endif
