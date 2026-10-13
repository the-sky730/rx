#ifndef __ADAPTER_IDEV_USB_H__
#define __ADAPTER_IDEV_USB_H__

#include "generic/typedef.h"
#include "adapter_idev.h"

enum idev_usb_attr {
    IDEV_USB_ATTR_HID = 0x0,
    IDEV_USB_ATTR_SPEAKER,
    IDEV_USB_ATTR_MIC,
    IDEV_USB_ATTR_MSD,

    IDEV_USB_ATTR_END,
    IDEV_USB_ATTR_MAX,
};

struct report_map {
    u8 *map;
    int size;
};

struct idev_usb_hid {
    u8 report_map_num;
    struct report_map report_map[3];
};

struct idev_usb_speaker {
    u16 sameple_rate;
    u8 ch_num;
    u8 max_vol;
};

struct idev_usb_mic {
    u16 sameple_rate;
    u8 ch_num;
    u8 max_vol;
};

struct idev_usb_msd {

};

struct idev_usb_fmt {
    enum idev_usb_attr attr;
    union {
        struct idev_usb_hid 	hid;
        struct idev_usb_speaker spk;
        struct idev_usb_mic 	mic;
        struct idev_usb_msd 	msd;
    } value;
};

struct idev_usb_config {
    u8								 	class_list_num;
    u8								 	stream_list_num;
    const struct idev_usb_fmt 			*class_list;
    struct ada_stream_fmt 				*stream_list;
};

#endif//__ADAPTER_IDEV_USB_H__

