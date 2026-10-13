#include "adapter_usb_hid.h"
#include "usb/device/usb_stack.h"
#include "usb/device/hid.h"
#include "usb_config.h"


#if TCFG_USB_SLAVE_HID_ENABLE

//端点1~3可以用
#define MOUSE_HID_EP_IN				1
#define KEYBOARD_HID_EP_IN			2
#define VENDOR_HID_EP_IN			4
#define VENDOR_HID_EP_OUT			4


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
static const u8 mouse_sHIDDescriptor[] = {
//HID
    //InterfaceDeszcriptor:
    USB_DT_INTERFACE_SIZE,     // Length
    USB_DT_INTERFACE,          // DescriptorType
    /* 0x04,                      // bInterface number */
    0x00,                       // bInterface number
    0x00,                      // AlternateSetting
    0x01,                      // NumEndpoint
    /* 0x02,                        // NumEndpoint */
    USB_CLASS_HID,             // Class = Human Interface Device
    0x00,                      // Subclass, 0 No subclass, 1 Boot Interface subclass
    0x00,                      // Procotol, 0 None, 1 Keyboard, 2 Mouse
    0x00,                      // Interface Name

    //HIDDescriptor:
    0x09,                      // bLength
    USB_HID_DT_HID,            // bDescriptorType, HID Descriptor
    0x00, 0x01,                // bcdHID, HID Class Specification release NO.
    0x00,                      // bCuntryCode, Country localization (=none)
    0x01,                       // bNumDescriptors, Number of descriptors to follow
    0x22,                       // bDescriptorType, Report Desc. 0x22, Physical Desc. 0x23
    0,//LOW(ReportLength)
    0, //HIGH(ReportLength)

    //EndpointDescriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_IN | MOUSE_HID_EP_IN,     // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MOUSE_MAXP_SIZE_HIDIN), HIBYTE(MOUSE_MAXP_SIZE_HIDIN),// Maximum packet size
    1,     // Poll every 10msec seconds
    //10,     // Poll every 10msec seconds

//@Endpoint Descriptor:
    /* USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_OUT | MOUSE_HID_EP_IN,   // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_HIDOUT), HIBYTE(MAXP_SIZE_HIDOUT),// Maximum packet size
    0x01,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms */
};


static const u8 keyboard_sHIDDescriptor[] = {
//HID
    //InterfaceDeszcriptor:
    USB_DT_INTERFACE_SIZE,     // Length
    USB_DT_INTERFACE,          // DescriptorType
    /* 0x04,                      // bInterface number */
    0x00,                       // bInterface number
    0x00,                      // AlternateSetting
    0x01,                      // NumEndpoint
    /* 0x02,                        // NumEndpoint */
    USB_CLASS_HID,             // Class = Human Interface Device
    0x00,                      // Subclass, 0 No subclass, 1 Boot Interface subclass
    0x00,                      // Procotol, 0 None, 1 Keyboard, 2 Mouse
    0x00,                      // Interface Name

    //HIDDescriptor:
    0x09,                      // bLength
    USB_HID_DT_HID,            // bDescriptorType, HID Descriptor
    0x00, 0x01,                // bcdHID, HID Class Specification release NO.
    0x00,                      // bCuntryCode, Country localization (=none)
    0x01,                       // bNumDescriptors, Number of descriptors to follow
    0x22,                       // bDescriptorType, Report Desc. 0x22, Physical Desc. 0x23
    0,//LOW(ReportLength)
    0, //HIGH(ReportLength)

    //EndpointDescriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_IN | KEYBOARD_HID_EP_IN,     // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(KEYBOARD_MAXP_SIZE_HIDIN), HIBYTE(KEYBOARD_MAXP_SIZE_HIDIN),// Maximum packet size
    1,     // Poll every 10msec seconds
    //10,     // Poll every 10msec seconds

//@Endpoint Descriptor:
    /* USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_OUT | MOUSE_HID_EP_IN,   // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_HIDOUT), HIBYTE(MAXP_SIZE_HIDOUT),// Maximum packet size
    0x01,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms */
};

static const u8 vendor_sHIDDescriptor[] = {
//HID
    //InterfaceDeszcriptor:
    USB_DT_INTERFACE_SIZE,     // Length
    USB_DT_INTERFACE,          // DescriptorType
    /* 0x04,                      // bInterface number */
    0x00,                       // bInterface number
    0x00,                      // AlternateSetting
    0x02,                      // NumEndpoint
    /* 0x02,                        // NumEndpoint */
    USB_CLASS_HID,             // Class = Human Interface Device
    0x00,                      // Subclass, 0 No subclass, 1 Boot Interface subclass
    0x00,                      // Procotol, 0 None, 1 Keyboard, 2 Mouse
    0x00,                      // Interface Name

    //HIDDescriptor:
    0x09,                      // bLength
    USB_HID_DT_HID,            // bDescriptorType, HID Descriptor
    0x00, 0x01,                // bcdHID, HID Class Specification release NO.
    0x00,                      // bCuntryCode, Country localization (=none)
    0x01,                       // bNumDescriptors, Number of descriptors to follow
    0x22,                       // bDescriptorType, Report Desc. 0x22, Physical Desc. 0x23
    0,//LOW(ReportLength)
    0, //HIGH(ReportLength)

    //EndpointDescriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_IN | VENDOR_HID_EP_IN,     // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(VENDOR_MAXP_SIZE_HIDIN), HIBYTE(VENDOR_MAXP_SIZE_HIDIN),// Maximum packet size
    //10,     // Poll every 10msec seconds
    1,     // Poll every 10msec seconds

//@Endpoint Descriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_OUT | VENDOR_HID_EP_OUT,   // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(VENDOR_MAXP_SIZE_HIDOUT), HIBYTE(VENDOR_MAXP_SIZE_HIDOUT),// Maximum packet size
    0x01,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms
};

static const u8 mouse_sHIDReportDesc[] = {
#if 1
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x05,        //     Usage Maximum (0x05)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x05,        //     Report Count (5)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x16, 0x00, 0x80,  //     Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x38,        //     Usage (Wheel)
    0x16, 0x00, 0x80,  //     Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0C,        //     Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //     Usage (AC Pan)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
#else
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        //   Usage (Mouse)
    0xA1, 0x02,        //   Collection (Logical)
    0x85, 0x01,        //     Report ID (1)
    0x09, 0x01,        //     Usage (Pointer)
    0xA1, 0x00,        //     Collection (Physical)
    0x05, 0x09,        //       Usage Page (Button)
    0x19, 0x01,        //       Usage Minimum (0x01)
    0x29, 0x05,        //       Usage Maximum (0x05)
    0x15, 0x00,        //       Logical Minimum (0)
    0x25, 0x01,        //       Logical Maximum (1)
    0x95, 0x05,        //       Report Count (5)
    0x75, 0x01,        //       Report Size (1)
    0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //       Report Count (1)
    0x75, 0x03,        //       Report Size (3)
    0x81, 0x01,        //       Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //       Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //       Usage (X)
    0x09, 0x31,        //       Usage (Y)
    0x95, 0x02,        //       Report Count (2)
    0x75, 0x10,        //       Report Size (16)
    0x16, 0x01, 0x80,  //       Logical Minimum (-32767)
    0x26, 0xFF, 0x7F,  //       Logical Maximum (32767)
    0x81, 0x06,        //       Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xA1, 0x02,        //       Collection (Logical)
    0x85, 0x01,        //         Report ID (1)
    0x09, 0x38,        //         Usage (Wheel)
    0x35, 0x00,        //         Physical Minimum (0)
    0x45, 0x00,        //         Physical Maximum (0)
    0x95, 0x01,        //         Report Count (1)
    0x75, 0x10,        //         Report Size (16)
    0x16, 0x01, 0x80,  //         Logical Minimum (-32767)
    0x26, 0xFF, 0x7F,  //         Logical Maximum (32767)
    0x81, 0x06,        //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x01,        //         Report ID (1)
    0x05, 0x0C,        //         Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //         Usage (AC Pan)
    0x35, 0x00,        //         Physical Minimum (0)
    0x45, 0x00,        //         Physical Maximum (0)
    0x95, 0x01,        //         Report Count (1)
    0x75, 0x10,        //         Report Size (16)
    0x16, 0x01, 0x80,  //         Logical Minimum (-32767)
    0x26, 0xFF, 0x7F,  //         Logical Maximum (32767)
    0x81, 0x06,        //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //       End Collection
    0xC0,              //     End Collection
    0xC0,              //   End Collection
    0xC0,              // End Collection
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x1A, 0xE0, 0x00,  //   Usage Minimum (0xE0)
    0x2A, 0xE7, 0x00,  //   Usage Maximum (0xE7)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0x91, 0x00,  //   Usage Maximum (0x91)
    0x16, 0x00, 0x00,  //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x0A,        //   Report Count (10)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x06, 0x07, 0xFF,  // Usage Page (Vendor Defined 0xFF07)
    0x0A, 0x12, 0x02,  // Usage (0x0212)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0xF0,        //   Report ID (-16)
    0x09, 0x01,        //   Usage (0x01)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x13,        //   Report Count (19)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF1,        //   Report ID (-15)
    0x95, 0x13,        //   Report Count (19)
    0x09, 0x02,        //   Usage (0x02)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
#endif

};
static const u8 keyboard_sHIDReportDesc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x1A, 0xE0, 0x00,  //   Usage Minimum (0xE0)
    0x2A, 0xE7, 0x00,  //   Usage Maximum (0xE7)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0x91, 0x00,  //   Usage Maximum (0x91)
    0x16, 0x00, 0x00,  //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x06,        //   Report Count (6)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x03,        //   Report Count (3)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x03,        //   Usage Maximum (Scroll Lock)
    0x25, 0x01,        //   Logical Maximum (1)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x19, 0x00,        //   Usage Minimum (Unassigned)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (0x03FF)
    0x75, 0x0C,        //   Report Size (12)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    USAGE_PAGE(1, CONSUMER_PAGE),
    USAGE(1, CONSUMER_CONTROL),
    COLLECTION(1, APPLICATION),
    0x85, 0x03,

    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(1, 0x01),

    USAGE(1, VOLUME_INC),
    USAGE(1, VOLUME_DEC),
    USAGE(1, MUTE),
    USAGE(1, PLAY_PAUSE),
    USAGE(1, SCAN_NEXT_TRACK),
    USAGE(1, SCAN_PREV_TRACK),
    USAGE(1, FAST_FORWARD),
    USAGE(1, STOP),

    USAGE(1, TRACKING_INC),
    USAGE(1, TRACKING_DEC),
    USAGE(1, STOP_EJECT),
    USAGE(1, VOLUME),
    USAGE(2, BALANCE_LEFT),
    USAGE(2, BALANCE_RIGHT),
    USAGE(1, PLAY),
    USAGE(1, PAUSE),

    REPORT_SIZE(1, 0x01),
    REPORT_COUNT(1, 0x10),
    INPUT(1, 0x42),
    0xC0,              // End Collection
};

static const u8 vendor_sHIDReportDesc[] = {
    USAGE_PAGE(2, 0x00, 0xff),      // vendor-defined 1
    USAGE(1, 0x01),                 // vendor-value 1
    COLLECTION(1, APPLICATION),     // application
    /* REPORT_ID(1, 1), */
    USAGE(1, 0x02),
    LOGICAL_MIN(2, 0, 0),
    LOGICAL_MAX(2, 0xff, 0),
    REPORT_SIZE(1, 0x08),          // 1 bit
    REPORT_COUNT(1, 64),         // eight 1bit data
    INPUT(1, 0x02),             // data, variabie, absolute
    USAGE(1, 0x03),
    LOGICAL_MIN(2, 0, 0),
    LOGICAL_MAX(2, 0xff, 0),
    REPORT_SIZE(1, 0x08),
    REPORT_COUNT(1, 64),
    OUTPUT(1, 0x02),
    END_COLLECTION,
};


//接口序号映射
static int itf_num_tab[3] = {
    0
};

//获取report描述符
static u32 get_hid_report_desc_len(int itf_num, u32 index)
{
    u32 len = 0;
    if (itf_num == itf_num_tab[0]) {
        //鼠标
        len = sizeof(mouse_sHIDReportDesc);
    } else if (itf_num == itf_num_tab[1]) {
        //键盘
        len = sizeof(keyboard_sHIDReportDesc);
    } else if (itf_num == itf_num_tab[2]) {
        //自定义
        len = sizeof(vendor_sHIDReportDesc);
    }
    //printf("get_hid_report_desc_len, itf_num = %d, len = %d\n", itf_num, len);
    return len;
}

///获取report描述符
static void *get_hid_report_desc(int itf_num, u32 index)
{
    u8 *ptr  = NULL;
    //printf("++++++++++++++++++++++++get_hid_report_desc = %d\n", itf_num);
    if (itf_num == itf_num_tab[0]) {
        //鼠标
        ptr = mouse_sHIDReportDesc;
    } else if (itf_num == itf_num_tab[1]) {
        //键盘
        ptr = keyboard_sHIDReportDesc;
    } else if (itf_num == itf_num_tab[2]) {
        //自定义
        ptr = vendor_sHIDReportDesc;
    }
    return ptr;
}

///获取接口描述符
static void *get_hid_itf_descriptor(int itf_num)
{
    u8 *ptr  = NULL;
    if (itf_num == itf_num_tab[0]) {
        //鼠标
        ptr = mouse_sHIDDescriptor;
    } else if (itf_num == itf_num_tab[1]) {
        //键盘
        ptr = keyboard_sHIDDescriptor;
    } else if (itf_num == itf_num_tab[2]) {
        //自定义
        ptr = vendor_sHIDDescriptor;
    }
    return ptr;
}


int mouse_hid_send_data(const u8 *buf, u32 len)
{
    const usb_dev usb_id = usb_device2id(NULL);
    return usb_g_intr_write(0, MOUSE_HID_EP_IN, buf, len);
}

int keyboard_hid_send_data(const u8 *buf, u32 len)
{
    KEYBOARD_MUTEX_PEND();
    u32 ret = 0;
    const usb_dev usb_id = usb_device2id(NULL);
    ret = usb_g_intr_write(0, KEYBOARD_HID_EP_IN, buf, len);
    KEYBOARD_MUTEX_POST();
    return ret;
}


int vendor_hid_send_data(const u8 *buf, u32 len)
{
    const usb_dev usb_id = usb_device2id(NULL);
    return usb_g_intr_write(0, VENDOR_HID_EP_IN, buf, len);
}
static void vender_hid_rx_data(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 rx_len = usb_g_intr_read(usb_id, ep, NULL, 64, 0);
    ///接收到的数据回调
}


static u8 *mouse_ep_in_dma = NULL;
static u8 *keyboard_ep_in_dma = NULL;
static u8 *vendor_ep_in_dma = NULL;
static u8 *vendor_ep_out_dma = NULL;
static void hid_endpoint_init(struct usb_device_t *usb_device, int itf_num)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    if (itf_num == itf_num_tab[0]) {
        usb_g_ep_config(usb_id, MOUSE_HID_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 0, mouse_ep_in_dma, MOUSE_MAXP_SIZE_HIDIN);
    } else if (itf_num == itf_num_tab[1]) {
        usb_g_ep_config(usb_id, KEYBOARD_HID_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 0, keyboard_ep_in_dma, KEYBOARD_MAXP_SIZE_HIDIN);
    } else if (itf_num == itf_num_tab[2]) {
        usb_g_ep_config(usb_id, VENDOR_HID_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 0, vendor_ep_in_dma, VENDOR_MAXP_SIZE_HIDIN);

        usb_g_set_intr_hander(usb_id, VENDOR_HID_EP_OUT, vender_hid_rx_data);
        usb_g_ep_config(usb_id, VENDOR_HID_EP_OUT, USB_ENDPOINT_XFER_INT, 1, vendor_ep_out_dma, VENDOR_MAXP_SIZE_HIDOUT);
        usb_enable_ep(usb_id, VENDOR_HID_EP_OUT);
    }
}



static void hid_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_debug("%s", __func__);
#if USB_ROOT2
    usb_disable_ep(usb_id, HID_EP_OUT);
#else
    for (int i = 0; i < 3; i++) {
        hid_endpoint_init(usb_device, itf_num_tab[i]);
    }
#endif
}




static u32 hid_recv_output_report(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 read_ep[8];
    u8 mute;
    u16 volume = 0;
    usb_read_ep0(usb_id, read_ep, MIN(sizeof(read_ep), setup->wLength));
    ret = USB_EP0_STAGE_SETUP;
    put_buf(read_ep, 8);


    return ret;
}

static u32 hid_itf_hander(int itf_num, struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    if (req == -1) {
        return 0;
    }
    printf("hid_itf_hander &&&&&&&&&&&&&&&&&&&&&&&&&&&&&& %x %d\n", itf_num, req);
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *itf_desc;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
            switch (HIBYTE(req->wValue)) {
            case USB_HID_DT_HID:
                printf("[hid] %s, %d\n", __FUNCTION__, __LINE__);
                itf_desc = get_hid_itf_descriptor(itf_num);
                tx_payload = (u8 *)itf_desc + USB_DT_INTERFACE_SIZE;
                tx_len = 9;
                tx_payload = usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                tx_payload[7] = LOBYTE(get_hid_report_desc_len(itf_num, req->wIndex));
                tx_payload[8] = HIBYTE(get_hid_report_desc_len(itf_num, req->wIndex));
                break;
            case USB_HID_DT_REPORT:
                printf("[hid] %s, %d\n", __FUNCTION__, __LINE__);
                hid_endpoint_init(usb_device, itf_num);
                tx_len = get_hid_report_desc_len(itf_num, req->wIndex);
                tx_payload = get_hid_report_desc(itf_num, req->wIndex);
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                break;
            }// USB_REQ_GET_DESCRIPTOR
            break;
        case USB_REQ_SET_DESCRIPTOR:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_SET_INTERFACE:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                //只有一个interface 没有Alternate
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        case USB_REQ_GET_INTERFACE:
            if (req->wLength) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            }
            break;
        case USB_REQ_GET_STATUS:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        }//bRequest @ USB_TYPE_STANDARD
        break;

    case USB_TYPE_CLASS: {
        switch (req->bRequest) {
        case USB_REQ_SET_IDLE:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_GET_IDLE:
            tx_len = 1;
            tx_payload[0] = 0;
            usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            break;
        case USB_REQ_SET_REPORT:
            usb_set_setup_recv(usb_device, hid_recv_output_report);
            break;
        }//bRequest @ USB_TYPE_CLASS
    }
    break;
    }
    return 0;
}

static u32 hid_itf0_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    printf("mouse_itf hander");
    return hid_itf_hander(itf_num_tab[0], usb_device, req);	//鼠标
}
static u32 hid_itf1_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    printf("keyboard_itf hander");
    return hid_itf_hander(itf_num_tab[1], usb_device, req);	//键盘
}
static u32 hid_itf2_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    printf("vendor_itf hander");
    return hid_itf_hander(itf_num_tab[2], usb_device, req);	//自定义
}
#if TCFG_USB_SLAVE_USER_HID
void adapter_avctp_key_handler(u32 hid_key)
{
    u8 key_buf[3];
    key_buf[0] = 3;
    key_buf[1] = hid_key & 0xff;
    key_buf[2] = hid_key >> 8;
    keyboard_hid_send_data(key_buf, 3);
    key_buf[0] = 3;
    key_buf[1] = 0;
    key_buf[2] = 0;
    keyboard_hid_send_data(key_buf, 3);
}
u32 hid_register(const usb_dev usb_id)
{
    KEYBOARD_MUTEX_INIT();
    mouse_ep_in_dma = usb_alloc_ep_dmabuffer(usb_id, MOUSE_HID_EP_IN | USB_DIR_IN, MOUSE_MAXP_SIZE_HIDIN);
    keyboard_ep_in_dma = usb_alloc_ep_dmabuffer(usb_id, KEYBOARD_HID_EP_IN | USB_DIR_IN, KEYBOARD_MAXP_SIZE_HIDIN);
    vendor_ep_in_dma = usb_alloc_ep_dmabuffer(usb_id, VENDOR_HID_EP_IN | USB_DIR_IN, VENDOR_MAXP_SIZE_HIDIN);
    vendor_ep_out_dma = usb_alloc_ep_dmabuffer(usb_id, VENDOR_HID_EP_OUT, VENDOR_MAXP_SIZE_HIDOUT);
    ASSERT(mouse_ep_in_dma);
    ASSERT(keyboard_ep_in_dma);
    ASSERT(vendor_ep_in_dma);
    ASSERT(vendor_ep_out_dma);
    return 0;
}

void hid_release(const usb_dev usb_id)
{
    return ;
}
u32 hid_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    log_debug("hid interface num:%d\n", *cur_itf_num);
    u8 *_ptr = ptr;
    //interface 0
    memcpy(_ptr, mouse_sHIDDescriptor, sizeof(mouse_sHIDDescriptor));
    _ptr[2] = *cur_itf_num;
    itf_num_tab[0] = _ptr[2];
    printf("mouse itf_num ++++++++++++++++++++++++++++ = %d\n", *cur_itf_num);
    if (usb_set_interface_hander(usb_id, *cur_itf_num, hid_itf0_hander) != *cur_itf_num) {
        ASSERT(0, "hid set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, hid_reset) != *cur_itf_num) {
        ASSERT(0, "hid set interface_reset_hander fail");
    }

    _ptr[USB_DT_INTERFACE_SIZE + 7] = LOBYTE(get_hid_report_desc_len(itf_num_tab[0], 0));
    _ptr[USB_DT_INTERFACE_SIZE + 8] = HIBYTE(get_hid_report_desc_len(itf_num_tab[0], 0));
    _ptr[USB_DT_INTERFACE_SIZE + 11] = USB_DIR_IN | MOUSE_HID_EP_IN;// bEndpointAddress;
    *cur_itf_num = *cur_itf_num + 1;

    //interface 1
    _ptr = ptr + sizeof(mouse_sHIDDescriptor);
    memcpy(_ptr, keyboard_sHIDDescriptor, sizeof(keyboard_sHIDDescriptor));
    _ptr[2] = *cur_itf_num;
    itf_num_tab[1] = _ptr[2];
    printf("keyboard_& consumer_page itf_num ++++++++++++++++++++++++++++ = %d\n", *cur_itf_num);
    if (usb_set_interface_hander(usb_id, *cur_itf_num, hid_itf1_hander) != *cur_itf_num) {
        ASSERT(0, "hid set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, hid_reset) != *cur_itf_num) {
        ASSERT(0, "hid set interface_reset_hander fail");
    }
    _ptr[USB_DT_INTERFACE_SIZE + 7] = LOBYTE(get_hid_report_desc_len(itf_num_tab[1], 0));
    _ptr[USB_DT_INTERFACE_SIZE + 8] = HIBYTE(get_hid_report_desc_len(itf_num_tab[1], 0));
    _ptr[USB_DT_INTERFACE_SIZE + 11] = USB_DIR_IN | KEYBOARD_HID_EP_IN;// bEndpointAddress;
    *cur_itf_num = *cur_itf_num + 1;

    //interface 2
    _ptr = ptr + sizeof(mouse_sHIDDescriptor) + sizeof(keyboard_sHIDDescriptor) ;
    memcpy(_ptr, vendor_sHIDDescriptor, sizeof(vendor_sHIDDescriptor));
    _ptr[2] = *cur_itf_num;
    itf_num_tab[2] = _ptr[2];
    printf("vendor_interface_num %d\n", *cur_itf_num);
    if (usb_set_interface_hander(usb_id, *cur_itf_num, hid_itf2_hander) != *cur_itf_num) {
        ASSERT(0, "hid set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, hid_reset) != *cur_itf_num) {
        ASSERT(0, "hid set interface_reset_hander fail");
    }

    _ptr[USB_DT_INTERFACE_SIZE + 7] = LOBYTE(get_hid_report_desc_len(itf_num_tab[2], 0));
    _ptr[USB_DT_INTERFACE_SIZE + 8] = HIBYTE(get_hid_report_desc_len(itf_num_tab[2], 0));
    _ptr[USB_DT_INTERFACE_SIZE + 11] = USB_DIR_IN | VENDOR_HID_EP_IN;// bEndpointAddress;
    _ptr[USB_DT_INTERFACE_SIZE + 11 + 7] = USB_DIR_OUT | VENDOR_HID_EP_OUT;// bEndpointAddress;
    *cur_itf_num = *cur_itf_num + 1;

    return (sizeof(mouse_sHIDDescriptor) + sizeof(keyboard_sHIDDescriptor) + sizeof(vendor_sHIDDescriptor)) ;
}
#else

void adapter_avctp_key_handler(u32 hid_key)
{
    u16 key_buf = hid_key;
    usb_g_intr_write(0, KEYBOARD_HID_EP_IN, ((const u8 *)&key_buf), 2);
    key_buf = 0;
    usb_g_intr_write(0, KEYBOARD_HID_EP_IN, ((const u8 *)&key_buf), 2);
}
#endif

#endif

