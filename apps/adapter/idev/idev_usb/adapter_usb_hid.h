#ifndef __ADAPTER_USB_HID_H__
#define __ADAPTER_USB_HID_H__

#include "generic/typedef.h"
#include "app_config.h"
#include "os/os_api.h"


#if TCFG_USB_SLAVE_AUDIO_ENABLE
#if USB_EP_PROTECT
#define     ADAPTER_AUDIO_DMA_SIZE  1024+192
#else
#define     ADAPTER_AUDIO_DMA_SIZE  256+192
#endif
#else
#define     ADAPTER_AUDIO_DMA_SIZE  0
#endif

#define     MOUSE_MAXP_SIZE_HIDIN     	        10//64//10
#define     KEYBOARD_MAXP_SIZE_HIDIN            9//64//9
#define     VENDOR_MAXP_SIZE_HIDOUT            	64
#define     VENDOR_MAXP_SIZE_HIDIN             	64

//键盘互斥使能
#define KEYBOARD_MUTEX_ENABLE		1
static OS_MUTEX keyboard_mutex;
#if KEYBOARD_MUTEX_ENABLE
#define	KEYBOARD_MUTEX_INIT()			os_mutex_create(&keyboard_mutex)
#define	KEYBOARD_MUTEX_PEND()			os_mutex_pend(&keyboard_mutex,0)
#define	KEYBOARD_MUTEX_POST()			os_mutex_post(&keyboard_mutex)
#else
#define	KEYBOARD_MUTEX_INIT()
#define	KEYBOARD_MUTEX_PEND()
#define	KEYBOARD_MUTEX_POST()
#endif


#define DMA_BUF_ALIGN	(8)

#define USB_DMA_BUF_MAX_SIZE ( \
		MOUSE_MAXP_SIZE_HIDIN +DMA_BUF_ALIGN + \
		KEYBOARD_MAXP_SIZE_HIDIN +DMA_BUF_ALIGN + \
		VENDOR_MAXP_SIZE_HIDOUT +DMA_BUF_ALIGN + \
		VENDOR_MAXP_SIZE_HIDIN +DMA_BUF_ALIGN + \
		ADAPTER_AUDIO_DMA_SIZE +DMA_BUF_ALIGN \
		+128 \
		)

int mouse_hid_send_data(const u8 *buf, u32 len);
int keyboard_hid_send_data(const u8 *buf, u32 len);
int vendor_hid_send_data(const u8 *buf, u32 len);
void adapter_avctp_key_handler(u32 hid_key);

#endif//__ADAPTER_USB_HID_H__

