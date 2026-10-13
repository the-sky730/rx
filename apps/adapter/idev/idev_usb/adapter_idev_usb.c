#include "adapter_idev_usb.h"
#include "uac_stream.h"
#include "adapter_process.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"

#if TCFG_USB_SLAVE_HID_ENABLE
#include "usb/device/hid.h"
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
#include "usb/device/custom_hid.h"
#endif


#define IDEV_USB_DEBUG_ENABLE
#ifdef IDEV_USB_DEBUG_ENABLE
#define idev_usb_printf printf
#define idev_usb_putchar putchar
#define idev_usb_putbuf put_buf
#else
#define idev_usb_printf(...)
#define idev_usb_putchar(...)
#define idev_usb_putbuf(...)
#endif//IDEV_USB_DEBUG_ENABLE

#if (TCFG_PC_ENABLE&&TCFG_USB_SLAVE_AUDIO_ENABLE)

#if (APP_MAIN == APP_DONGLE)
#define USB_AUDIO_NORMAL_EN         1//音频情景跟着mic和spk打开情况进行切换
#else
#define USB_AUDIO_NORMAL_EN         0//音频情景不随mic和spk打开情况进行切换
#endif

struct idev_usb {
    struct idev_usb_fmt 		 *parm;
    struct adapter_media		 *media;
    usb_dev 					  usbfd;
    u32							  class_mask;
    volatile u8 				  mic_status;
    volatile u8 				  spk_status;
};


static struct idev_usb *g_idev_usb = NULL;
#define __this	g_idev_usb

static struct idev_usb_fmt *adapter_idev_usb_get_fmt(int attr)
{
    if (__this == NULL || __this->parm == NULL) {
        return NULL;
    }

    int ret = 0;
    u8 i = 0;
    struct idev_usb_fmt *tmp = __this->parm;
    //__this->class_mask = 0;
    while (1) {
        if (tmp[i].attr == IDEV_USB_ATTR_END) {
            printf("usb_parm_list parse end!!\n");
            break;
        }

        if (attr == tmp[i].attr) {
            return tmp + i;
        }
        i++;
    }
    return NULL;
}


static void adapter_idev_usb_media_close_request(struct adapter_media *media)
{
    adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_CLOSE, 0);
}

extern u16 uac_get_mic_vol(const usb_dev usb_id);
static int adapter_idev_usb_media_open_request(struct adapter_media *media, u8 media_sel_mode)
{
    if (media == NULL) {
        return -1;
    }
    struct idev_usb_fmt *fmt = NULL;
    struct adapter_media_parm *downstream_parm = adapter_media_get_downstream_parm_handle(media);
    if (downstream_parm) {
        u16 vol_l = 0;
        u16 vol_r = 0;
        extern u16 uac_get_cur_vol(const u8 id, u16 * l_vol, u16 * r_vol);
        uac_get_cur_vol(0, &vol_l, &vol_r);

        downstream_parm->vol_limit = 100;
        downstream_parm->start_vol_l = vol_l;
        downstream_parm->start_vol_r = vol_r;
        if (downstream_parm->sample_rate == 0) {
            printf("warnning !!!\n");
            downstream_parm->sample_rate = SPK_AUDIO_RATE;
            downstream_parm->ch_num = SPK_CHANNEL;
        }
    }
    struct adapter_media_parm *upstream_parm = adapter_media_get_upstream_parm_handle(media);
    if (upstream_parm) {
        u16 mic_vol = uac_get_mic_vol(0);
        upstream_parm->vol_limit = 100;
        upstream_parm->start_vol_l = mic_vol;
        upstream_parm->start_vol_r = mic_vol;
        if (upstream_parm->sample_rate == 0) {
            upstream_parm->sample_rate = MIC_AUDIO_RATE;
            upstream_parm->ch_num = MIC_CHANNEL;
        }
    }
    adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_OPEN, media_sel_mode);
    return 0;
}

static int adapter_idev_uac_event_prase(struct sys_event *e)
{
    if (__this == NULL) {
        return 0;
    }
    u8 reset = 0;
    int ret = 1;
    u8 event = e->u.dev.event;
    int value = e->u.dev.value;
    struct adapter_media_parm *downstream_parm = adapter_media_get_downstream_parm_handle(__this->media);
    struct adapter_media_parm *upstream_parm = adapter_media_get_upstream_parm_handle(__this->media);
    struct idev_usb_fmt *fmt = NULL;
    switch (event) {
    case USB_AUDIO_PLAY_OPEN:
        idev_usb_printf("USB_AUDIO_PLAY_OPEN\n");
        //usb spk open
        //有以下两种情形
        //1:usb spk -> a2dp(usb mic 不开启的情况下)
        //2:usb spk -> esco(usb mic 开启情况下)
        __this->spk_status = 1;
        if (downstream_parm) {
            if ((downstream_parm->sample_rate != ((u16)(value & 0xFFFFFF))) || (downstream_parm->ch_num != (u8)(value >> 24))) {
                //参数发生改变， 音频需要重启
                downstream_parm->sample_rate = (u16)(value & 0xFFFFFF);
                downstream_parm->ch_num = (u8)(value >> 24);
                reset = 1;
            }
        }
        if (__this->mic_status == 0 || reset == 1) {
            adapter_idev_usb_media_open_request(__this->media, 0);
        }
        break;
    case USB_AUDIO_PLAY_CLOSE:
        idev_usb_printf("USB_AUDIO_PLAY_CLOSE\n");
        //usb spk close
        //关闭下行转发
        __this->spk_status = 0;
        if (__this->spk_status == 0 && __this->mic_status == 0) {
            adapter_idev_usb_media_close_request(__this->media);
        }
        break;
    case USB_AUDIO_MIC_OPEN:
#if USER_SUPPORT_DUAL_A2DP_SOURCE
        printf("DUAL_A2DP NO ACK MIC");
        break;
#endif
        idev_usb_printf("USB_AUDIO_MIC_OPEN\n");
        //切换转发通道 , 前提是usb spk已经打开
        //由usb spk -> a2dp切换成usb spk -> esco
        __this->mic_status = 1;
        if (upstream_parm) {
            upstream_parm->sample_rate = (u16)(value & 0xFFFFFF);
            upstream_parm->ch_num = (u8)(value >> 24);
        }
#if (USB_AUDIO_NORMAL_EN)
        adapter_idev_usb_media_close_request(__this->media);
        adapter_idev_usb_media_open_request(__this->media, 1);
#else
        if (__this->spk_status == 0) {
            adapter_idev_usb_media_open_request(__this->media, 0);
        }
#endif
        break;
    case USB_AUDIO_MIC_CLOSE:
#if USER_SUPPORT_DUAL_A2DP_SOURCE
        printf("DUAL_A2DP NO ACK MIC");
        break;
#endif
        idev_usb_printf("USB_AUDIO_MIC_CLOSE\n");
        //切换转发通道 , 前提是usb spk已经打开
        //由usb spk -> esco切换成usb spk -> a2dp
        __this->mic_status = 0;
#if (USB_AUDIO_NORMAL_EN)
        if (__this->spk_status) {
            adapter_idev_usb_media_open_request(__this->media, 0);
        } else {
            adapter_idev_usb_media_close_request(__this->media);
        }
#else
        if (__this->spk_status == 0 && __this->mic_status == 0) {
            adapter_idev_usb_media_close_request(__this->media);
        }
#endif
        break;
    case USB_AUDIO_SET_MIC_VOL:
        if (__this->media) {
            u16 mic_vol = value & 0xffff;
            adapter_decoder_set_vol(__this->media->updecode, BIT(0), mic_vol);
            adapter_decoder_set_vol(__this->media->updecode, BIT(1), mic_vol);
            if (upstream_parm) {
                upstream_parm->start_vol_l = mic_vol;
                upstream_parm->start_vol_r = mic_vol;
            }
        }
        break;
    case USB_AUDIO_SET_PLAY_VOL:
        if (__this->media) {
            u16 vol_l = value & 0xffff;
            u16 vol_r = (value >> 16) & 0xffff;
            adapter_decoder_set_vol(__this->media->downdecode, BIT(0), vol_l);
            adapter_decoder_set_vol(__this->media->downdecode, BIT(1), vol_r);
            if (downstream_parm) {
                downstream_parm->start_vol_l = vol_l;
                downstream_parm->start_vol_r = vol_r;
            }
        }
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}


static int adapter_idev_usb_event_handle(struct sys_event *e)
{
    int ret = 0;

    switch (e->type) {
    case SYS_DEVICE_EVENT:
        switch ((u32)e->arg) {
        case DEVICE_EVENT_FROM_UAC:
            ret = adapter_idev_uac_event_prase(e);
            break;
        }
        break;
    default:
        break;
    }
    return ret;
}


static int adapter_idev_usb_open(void *parm)
{
    if (__this) {
        return 0;
    }
    __this = zalloc(sizeof(struct idev_usb));
    ASSERT(__this, "g_idev_usb malloc fail\n");
    ASSERT(parm, "idev_usb parm err!!\n");
    __this->parm = parm;
    adapter_process_event_notify(ADAPTER_EVENT_IDEV_INIT_OK, 0);

    return 0;
}
static int adapter_idev_usb_close(void)
{
    if (__this) {
        free(__this);
        __this = NULL;
    }
    return 0;
}

#if TCFG_USB_CUSTOM_HID_ENABLE
static void custom_hid_rx_handler(void *priv, u8 *buf, u32 len)
{
    put_buf(buf, len);
}
#endif /* #if TCFG_USB_CUSTOM_HID_ENABLE */

static int adapter_idev_usb_start(struct adapter_media *media)
{
    if (__this == NULL) {
        return 0;
    }
    printf("adapter_idev_usb_start\n");

    __this->media = media;

    //CLASS init
#if !TCFG_OTG_USB_DEV_EN
#ifdef USB_DEVICE_CLASS_CONFIG
    usb_device_mode(__this->usbfd, USB_DEVICE_CLASS_CONFIG);
#endif
#endif
#if TCFG_USB_CUSTOM_HID_ENABLE
    custom_hid_set_rx_hook(NULL, custom_hid_rx_handler);
    printf("custom_hid rx_hook\n");
#endif

    //MSD init

    return 0;
}

static void adapter_idev_usb_stop(void)
{
    if (__this == NULL) {
        return ;
    }
    //usb模块关闭
    usb_sie_disable(__this->usbfd);
    usb_device_mode(__this->usbfd, 0);
    usb_sie_close(__this->usbfd);
}



REGISTER_ADAPTER_IDEV(adapter_idev_usb) = {
    .id = ADAPTER_IDEV_USB,
    .open = adapter_idev_usb_open,
    .close = adapter_idev_usb_close,
    .start = adapter_idev_usb_start,
    .stop = adapter_idev_usb_stop,
    .event_fun = adapter_idev_usb_event_handle,
};

#endif

