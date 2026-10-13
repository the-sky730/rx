#include "app_config.h"
#include "app_sound_box_tool.h"
#include "os/os_api.h"

#if TCFG_SHARE_OSC_EN

#define CFG_TOOL_SHARE_OSC_SINGLE_IO_SEND_ID 0X32

#define ONLINE_SUB_OP_CHECK_SINGLE_IO_SEND_BUF				0x00000045	//查询单io命令
#define ONLINE_SUB_OP_CHECK_SINGLE_IO_SEND_BUF_NO_DATA		0x00000046	//查询单io但无数据命令
#define ONLINE_SUB_OP_CHECK_SINGLE_IO_USER_DATA				0x00000047	//通过这个命令接收主机的用户自定义数据

typedef struct {
    int cmd;
    int data[64];
} CFG_TOOL_SHARE_OSC_PACKET;

struct list_head _single_io_send_buf_list;		// 发送链表

struct single_io_send_buf {
    struct list_head entry;
    u8 *send_buf;
    u16 send_len;
};

static OS_MUTEX _mutex;
struct list_head _single_io_send_buf_list;		// 发送链表

void cfg_tool_share_osc_check_single_io_send_init()
{
    os_mutex_create(&_mutex);

    INIT_LIST_HEAD(&_single_io_send_buf_list);
}

static int share_osc_single_io_send_buf_list_add(u8 *buf, u16 len)
{
    struct single_io_send_buf *send_buf_hd = malloc(sizeof(struct single_io_send_buf));
    u8 *send_buf = malloc(len);
    if ((!send_buf_hd) || (!send_buf)) {
        printf("%s malloc err!\n", __FUNCTION__);
        return -1;
    }
    send_buf_hd->send_buf = send_buf;
    send_buf_hd->send_len = len;
    memcpy(send_buf, buf, len);
    os_mutex_pend(&_mutex, 0);
    list_add_tail(&send_buf_hd->entry, &_single_io_send_buf_list);
    os_mutex_post(&_mutex);
    return 0;
}

/**
 * @brief 共享晶振消息发送
 */
void share_osc_msg_send(u8 *buf, u16 len)
{
    share_osc_single_io_send_buf_list_add(buf, len);
}

#define SHARE_OSC_SINGLE_IO_SEND_BUF_MAX_SIZE 230
static void cfg_tool_share_osc_check_single_io_send(u8 *buf, u32 buf_len)
{
    u8 send_buf[SHARE_OSC_SINGLE_IO_SEND_BUF_MAX_SIZE];
    send_buf[0] = CFG_TOOL_SHARE_OSC_SINGLE_IO_SEND_ID;
    send_buf[1] = 0x00;
    CFG_TOOL_WRITE_LIT_U32(&send_buf[0] + 2, ONLINE_SUB_OP_CHECK_SINGLE_IO_SEND_BUF);//cmd
    memcpy(send_buf + 6, buf, (buf_len > SHARE_OSC_SINGLE_IO_SEND_BUF_MAX_SIZE) ? SHARE_OSC_SINGLE_IO_SEND_BUF_MAX_SIZE : buf_len);
    all_assemble_package_send_for_share_osc(send_buf, 6 + buf_len);
}

static void cfg_tool_share_osc_check_single_io_send_no_data()
{
    u8 send_buf[6];
    send_buf[0] = CFG_TOOL_SHARE_OSC_SINGLE_IO_SEND_ID;
    send_buf[1] = 0x00;
    CFG_TOOL_WRITE_LIT_U32(&send_buf[0] + 2, ONLINE_SUB_OP_CHECK_SINGLE_IO_SEND_BUF_NO_DATA);//cmd
    all_assemble_package_send_for_share_osc(send_buf, sizeof(send_buf));
}

static void cfg_tool_share_osc_check_single_io_send_user_data_response()
{
    u8 send_buf[6];
    send_buf[0] = CFG_TOOL_SHARE_OSC_SINGLE_IO_SEND_ID;
    send_buf[1] = 0x00;
    CFG_TOOL_WRITE_LIT_U32(&send_buf[0] + 2, ONLINE_SUB_OP_CHECK_SINGLE_IO_USER_DATA);//cmd
    all_assemble_package_send_for_share_osc(send_buf, sizeof(send_buf));
}

static void cfg_tool_share_osc_callback(u8 *_packet, u32 size)
{
    u8 *ptr = _packet;
    u8 id = ptr[0];
    u8 sq = ptr[1];
    CFG_TOOL_SHARE_OSC_PACKET *packet = (CFG_TOOL_SHARE_OSC_PACKET *)&ptr[2];
    switch (packet->cmd) {
    case ONLINE_SUB_OP_CHECK_SINGLE_IO_SEND_BUF:
        /* printf("============ONLINE_SUB_OP_CHECK_SINGLE_IO_SEND_BUF==========\n"); */
        // 获取链表的buf，并回复
        os_mutex_pend(&_mutex, 0);
        if (!list_empty(&_single_io_send_buf_list)) {
            struct single_io_send_buf *send_buf_hd = NULL;
            send_buf_hd = list_first_entry(&_single_io_send_buf_list, struct single_io_send_buf, entry);
            u8 *send_buf = send_buf_hd->send_buf;
            u16 send_len = send_buf_hd->send_len;
            list_del(&send_buf_hd->entry);
            free(send_buf_hd);
            cfg_tool_share_osc_check_single_io_send(send_buf, send_len);
            // 取出数据到发送缓存后，删除
            free(send_buf);
            send_buf = NULL;
        } else {
            cfg_tool_share_osc_check_single_io_send_no_data();
        }
        os_mutex_post(&_mutex);
        break;
    case ONLINE_SUB_OP_CHECK_SINGLE_IO_USER_DATA:
        printf("===========ONLINE_SUB_OP_CHECK_SINGLE_IO_USER_DATA==========\n");
        // 打印对端发过来的buf
        put_buf(ptr + 6, size - 6);
        cfg_tool_share_osc_check_single_io_send_user_data_response();
        break;
    default:
        break;
    }
}

REGISTER_DETECT_TARGET(share_osc_single_io_send_target) = {
    .id = CFG_TOOL_SHARE_OSC_SINGLE_IO_SEND_ID,
    .tool_message_deal = cfg_tool_share_osc_callback,
};

#endif

