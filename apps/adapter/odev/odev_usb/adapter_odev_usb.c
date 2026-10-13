#include "adapter_odev.h"
#include "adapter_process.h"
#include "adapter_odev_usb.h"
#include "uac_stream.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"

#if TCFG_USB_SLAVE_HID_ENABLE
#include "usb/device/hid.h"
#endif

#define ODEV_USB_DEBUG_ENABLE
#ifdef ODEV_USB_DEBUG_ENABLE
#define odev_usb_printf printf
#define odev_usb_putchar putchar
#define odev_usb_putbuf put_buf
#else
#define odev_usb_printf(...)
#define odev_usb_putchar(...)
#define odev_usb_putbuf(...)
#endif//ODEV_USB_DEBUG_ENABLE

#if (TCFG_PC_ENABLE&&TCFG_USB_SLAVE_AUDIO_ENABLE)

struct __odev_usb {
    struct adapter_media *media;
    u32 usbfd;
};
struct __odev_usb *odev_usb = NULL;
#define __this odev_usb


extern u16 uac_get_mic_vol(const usb_dev usb_id);

static int adapter_odev_usb_open(void *parm)
{
    __this = zalloc(sizeof(struct __odev_usb));
    if (__this == NULL) {
        return -1;
    }
    adapter_process_event_notify(ADAPTER_EVENT_ODEV_INIT_OK, 0);
    return 0;
}
static int adapter_odev_usb_start(void *priv, struct adapter_media *media)
{
    if (__this == NULL) {
        return -1;
    }
    __this->media = media;
#if !TCFG_OTG_USB_DEV_EN
#ifdef USB_DEVICE_CLASS_CONFIG
    usb_device_mode(__this->usbfd, USB_DEVICE_CLASS_CONFIG);
#endif
#endif
    //USB mic一直打开
    struct adapter_media_parm *downstream_parm = adapter_media_get_downstream_parm_handle(__this->media);
    if (downstream_parm) {
        downstream_parm->sample_rate = MIC_AUDIO_RATE;
        downstream_parm->ch_num = MIC_CHANNEL;
        downstream_parm->vol_limit = 100;
        downstream_parm->start_vol_l = 100;
        downstream_parm->start_vol_r = 100;
    }
    adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_OPEN, 0);
    return 0;
}


static int adapter_odev_usb_stop(void *priv)
{
    return 0;
}

static int adapter_odev_usb_close(void)
{
    if (__this) {
        free(__this);
        __this = NULL;
        adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
    }
    return 0;
}


static int adapter_odev_uac_event_prase(struct sys_event *e)
{
    /* if (__this == NULL) { */
    /* return 0; */
    /* } */
    int ret = 1;
    u8 event = e->u.dev.event;
    int value = e->u.dev.value;
    u16 mic_vol = 0;
    struct adapter_media_parm *downstream_parm = adapter_media_get_downstream_parm_handle(__this->media);
    switch (event) {
    case USB_AUDIO_PLAY_OPEN:
        odev_usb_printf("ODEV, USB_AUDIO_PLAY_OPEN\n");
        break;
    case USB_AUDIO_PLAY_CLOSE:
        odev_usb_printf("ODEV, USB_AUDIO_PLAY_CLOSE\n");
        break;
    case USB_AUDIO_MIC_OPEN:
        odev_usb_printf("ODEV, USB_AUDIO_MIC_OPEN\n");
        if (__this->media) {
            //因为解码提前开了， 这里mic开启的时候设置一下mic数字增益
            mic_vol = uac_get_mic_vol(0);
            adapter_decoder_set_vol(__this->media->downdecode, BIT(0), mic_vol);
            adapter_decoder_set_vol(__this->media->downdecode, BIT(1), mic_vol);
            if (downstream_parm) {
                downstream_parm->start_vol_l = mic_vol;
                downstream_parm->start_vol_r = mic_vol;
            }
        }
        //adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_OPEN, 0);
        break;
    case USB_AUDIO_MIC_CLOSE:
        odev_usb_printf("ODEV, USB_AUDIO_MIC_CLOSE\n");
        //adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
        break;
    case USB_AUDIO_SET_MIC_VOL:
        if (__this->media) {
            mic_vol = value & 0xffff;
            adapter_decoder_set_vol(__this->media->downdecode, BIT(0), mic_vol);
            adapter_decoder_set_vol(__this->media->downdecode, BIT(1), mic_vol);
            if (downstream_parm) {
                downstream_parm->start_vol_l = mic_vol;
                downstream_parm->start_vol_r = mic_vol;
            }
        }
        break;
    case USB_AUDIO_SET_PLAY_VOL:
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}


static int adapter_odev_usb_event_deal(struct sys_event *e)
{
    int ret = 0;

    switch (e->type) {
    case SYS_DEVICE_EVENT:
        switch ((u32)e->arg) {
        case DEVICE_EVENT_FROM_UAC:
            ret = adapter_odev_uac_event_prase(e);
            break;
        }
        break;
    default:
        break;
    }
    return ret;
}




static int adapter_odev_usb_config(int cmd, void *parm)
{
    return 0;
}

REGISTER_ADAPTER_ODEV(adapter_odev_usb) = {
    .id     = ADAPTER_ODEV_USB,
    .open   = adapter_odev_usb_open,
    .start  = adapter_odev_usb_start,
    .stop   = adapter_odev_usb_stop,
    .close  = adapter_odev_usb_close,
    .media_pp = NULL,
    .get_status = NULL,
    .media_prepare = NULL,
    .event_fun  = adapter_odev_usb_event_deal,
    .config = NULL,
};

#endif

