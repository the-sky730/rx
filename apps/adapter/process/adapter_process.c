#include "adapter_process.h"
#include "adapter_media.h"
#include "app_task.h"
#include "clock_cfg.h"
#include "app_online_cfg.h"
#include "update.h"

#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
u8 flag_adapter_sd_online_status = 0;
void adapter_set_sd_online_status(u8 status)
{
    flag_adapter_sd_online_status = status;
}
u8 adapter_get_sd_online_status(void)
{
    return flag_adapter_sd_online_status;
}
#endif
#if TCFG_OTG_USB_DEV_EN
u8 flag_adapter_otg_online_status = 0;
void adapter_set_otg_online_status(u8 status)
{
    flag_adapter_otg_online_status = status;
}
u8 adapter_get_otg_online_status(void)
{
    return flag_adapter_otg_online_status;
}
#endif
static int adapter_process_media_start_callback(void *priv, u8 mode, u8 status, void *parm)
{
    struct adapter_pro *pro = (struct adapter_pro *)priv;


    if (pro == NULL || pro->media == NULL)	{
        return 0;
    }
    if (pro->mode == mode) {
        g_f_printf("media not  change ~~~~~~~~~~~~~~~~~~~~%d\n", pro->mode);
        if (status) {
            struct adapter_media_parm *downstream_parm = adapter_media_get_downstream_parm_handle(pro->media);
            struct adapter_media_parm *upstream_parm = adapter_media_get_upstream_parm_handle(pro->media);
            if (downstream_parm) {
                downstream_parm->priv_data = parm;
            }
            if (upstream_parm) {
                upstream_parm->priv_data = parm;
            }
            pro->media->media_sel = mode;

            adapter_media_stop(pro->media);
            adapter_media_start(pro->media);
        }
    } else {
        if (pro->mode != 0xff) {
            g_f_printf("media mode change ~~~~~~~~~~~~~~~~~~~~%d,%d\n", pro->mode, mode);
            adapter_odev_media_prepare(pro->out, pro->mode, adapter_process_media_start_callback, (void *)pro);
            return 0;
        }
    }
    pro->media_lock = 0;
    return 0;
}

struct adapter_pro *adapter_process_open(struct idev *in, struct odev *out, struct adapter_media *media, int (*event)(struct sys_event *event))
{
    struct adapter_pro *pro = zalloc(sizeof(struct adapter_pro));
    if (pro == NULL) {
        return NULL;
    }

    pro->in = in;
    pro->out = out;
    pro->media = media;
    pro->event_handle = event;

//    clock_add_set(ADAPTER_PROCESS_CLK);

    return pro;
}

void adapter_process_close(struct adapter_pro **hdl)
{
    if (hdl == NULL || *hdl == NULL)		{
        return ;
    }
    struct adapter_pro *pro = *hdl;

    //其他模块关闭
    adapter_media_stop(pro->media);

    adapter_idev_stop(pro->in);
    adapter_odev_stop(pro->out);

    //释放句柄
    local_irq_disable();
    free(pro);
    *hdl = NULL;
    local_irq_enable();
    clock_remove_set(ADAPTER_PROCESS_CLK);
}


static int adapter_device_event_parse(struct adapter_pro *pro, struct sys_event *e)
{
    u8 event = e->u.dev.event;
    int value = e->u.dev.value;

    switch (event) {
    //初始化完成
    case ADAPTER_EVENT_IDEV_INIT_OK:
        printf("ADAPTER_EVENT_IDEV_INIT_OK\n");
        adapter_idev_start(pro->in, pro->media);
        break;
    case ADAPTER_EVENT_ODEV_INIT_OK:
        printf("ADAPTER_EVENT_ODEV_INIT_OK\n");
        adapter_odev_start(pro->out, pro->media);
        break;

    //媒体相关事件
    case ADAPTER_EVENT_IDEV_MEDIA_OPEN:
        printf("ADAPTER_EVENT_IDEV_MEDIA_OPEN\n");
        if ((pro->dev_status & 0x3) == 0x3) {
            printf("idev odev had opened,break");
            break;
        }
        pro->mode = (u8)value;
        pro->dev_status |= BIT(ADAPTER_INDEX_IDEV);
#if ALWAYS_RUN_STREAM
        pro->dev_status |= BIT(ADAPTER_INDEX_ODEV);
#endif
        if ((pro->dev_status & 0x3) == 0x3) {
            //prepare media
            if (pro->media_lock == 0) {
                pro->media_lock = 1;
                if (adapter_odev_media_prepare(pro->out, pro->mode, adapter_process_media_start_callback, (void *)pro)) {
                    //prepare 返回值为非0， 需要主动启动媒体
                    printf("adapter_odev_media_prepare null\n");
                    adapter_process_media_start_callback(pro, pro->mode, 1, NULL);
                }
            } else {
                g_f_printf("%s, %d, pro->media_lock!!!!!\n", __FUNCTION__, __LINE__);
            }
        }
        break;
    case ADAPTER_EVENT_IDEV_MEDIA_CLOSE:
        printf("ADAPTER_EVENT_IDEV_MEDIA_CLOSE\n");
#if ALWAYS_RUN_STREAM
        printf("ALWAYS_RUN_STREAM,break");
        break;
#endif
        pro->mode = 0xff;
        pro->dev_status &= ~BIT(ADAPTER_INDEX_IDEV);
        adapter_odev_media_pp(pro->out, 0);
        adapter_media_stop(pro->media);
        break;
    case ADAPTER_EVENT_ODEV_MEDIA_OPEN:
        printf("ADAPTER_EVENT_ODEV_MEDIA_OPEN\n");
        if ((pro->dev_status & 0x3) == 0x3) {
            printf("idev odev had opened,break");
            break;
        }
        pro->dev_status |= BIT(ADAPTER_INDEX_ODEV);
#if ALWAYS_RUN_STREAM
        pro->dev_status |= BIT(ADAPTER_INDEX_IDEV);
#endif
        if ((pro->dev_status & 0x3) == 0x3) {
            //prepare media
            if (pro->media_lock == 0) {
                pro->media_lock = 1;
                if (adapter_odev_media_prepare(pro->out, pro->mode, adapter_process_media_start_callback, (void *)pro)) {
                    //prepare 返回值为非0， 需要主动启动媒体
                    printf("adapter_odev_media_prepare null\n");
                    adapter_process_media_start_callback(pro, pro->mode, 1, NULL);
                }
            } else {
                g_f_printf("%s, %d, pro->media_lock!!!!!\n", __FUNCTION__, __LINE__);
            }
        }
        break;
    case ADAPTER_EVENT_ODEV_MEDIA_CLOSE:
        printf("ADAPTER_EVENT_ODEV_MEDIA_CLOSE\n");
#if ALWAYS_RUN_STREAM
        printf("ALWAYS_RUN_STREAM,break");
        break;
#endif
        pro->media_lock = 0;
        pro->dev_status &= ~BIT(ADAPTER_INDEX_ODEV);
        adapter_media_stop(pro->media);
        break;
    case ADAPTER_EVENT_MEDIA_DECODE_ERR:
    case ADAPTER_EVENT_MEDIA_CLOSE:
        printf("ADAPTER_EVENT_MEDIA_STOP\n");
        adapter_media_stop(pro->media);
        break;
    default:
        break;
    }
    return 0;
}
u8 flag_key_sw_recorder;
extern void board_test_uart_event_handler();
static int adapter_process_event_parse(struct adapter_pro *pro, struct sys_event *e)
{
    int ret = 0;

    const char *usb_msg = (const char *)e->u.dev.value;
    //输入设备事件处理
    ret = adapter_idev_event_deal(pro->in, e);
    if (ret) {
        return ret;
    }

    //输出设备事件处理
    ret = adapter_odev_event_deal(pro->out, e);
    if (ret) {
        return ret;
    }

    //公共事件处理
    switch (e->type) {
    case SYS_KEY_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        switch ((u32)e->arg) {
        case DEVICE_EVENT_FROM_ADAPTER:
            ret = adapter_device_event_parse(pro, e);
            break;
#if (TCFG_ONLINE_ENABLE || TCFG_SOUNDBOX_TOOL_ENABLE || TCFG_SHARE_OSC_EN)
        case DEVICE_EVENT_FROM_SOUNDBOX_TOOL:
            extern int app_soundbox_tool_event_handler(struct soundbox_tool_event * soundbox_tool_dev);
            app_soundbox_tool_event_handler(&e->u.soundbox_tool);
            break;
        case DEVICE_EVENT_FROM_CI_UART:
            ci_data_rx_handler(CI_UART);
            break;
#endif
#if BOARD_TEST_EN
        case DEVICE_EVENT_FROM_BOARD_UART:
            board_test_uart_event_handler();
            break;
#endif
#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE
        case DRIVER_EVENT_FROM_SD0:
        case DRIVER_EVENT_FROM_SD1:
        case DRIVER_EVENT_FROM_SD2:
            if (e->u.dev.event == DEVICE_EVENT_IN) {
                printf("DEVICE_EVENT_IN\n");
                adapter_set_sd_online_status(1);
            } else {
                printf("DEVICE_EVENT_OUT\n");
                adapter_set_sd_online_status(0);
#if WIRELESS_MIC_RECORDER_ENABLE
                flag_key_sw_recorder = 0;
                if (wl_mic_get_recorder_status()) {
                    wireless_mic_recorder_stop();
                }
#endif
            }
            break;
#endif
#if TCFG_OTG_USB_DEV_EN
        case DEVICE_EVENT_FROM_OTG:
            if (usb_msg[0] == 's') {
                if (e->u.dev.event == DEVICE_EVENT_IN) {
                    printf("DEVICE_EVENT_IN\n");
#if WIRELESS_MIC_RECORDER_ENABLE
                    if (wl_mic_get_recorder_status()) {
                        wireless_mic_recorder_stop();
                    }
                    flag_key_sw_recorder = 0;
#endif
                    adapter_set_otg_online_status(1);
                } else if (e->u.dev.event == DEVICE_EVENT_OUT) {
                    printf("DEVICE_EVENT_OUT\n");
                    adapter_set_otg_online_status(0);
                }
            }
            break;
#endif
        default:
            break;
        }
        break;
    default:
        break;
    }
    return ret;
}

int adapter_process_run(struct adapter_pro *pro)
{
    if (pro == NULL) {
        return false;
    }

    int msg[32];
    int ret = 0;
    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);
        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (adapter_process_event_parse(pro, (struct sys_event *)&msg[1]) == 0) {
                if (pro->event_handle) {
                    ret = pro->event_handle((struct sys_event *)(&msg[1]));
                }
            }
            break;
        default:
            break;
        }
        if (ret) {
            break;
        }
    }
    return ret;
}

void adapter_process_event_notify(u8 event, int value)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg = (void *)DEVICE_EVENT_FROM_ADAPTER;
    e.u.dev.event = event;
    e.u.dev.value = value;

    sys_event_notify(&e);
}

#if TCFG_TEST_BOX_ENABLE
void adapter_testbox_update_cbk(int type, u32 state, void *priv)
{
    switch (state) {
    case UPDATE_CH_SUCESS_REPORT:
        adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
        break;
    }
}
#endif
