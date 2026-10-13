#include "cfg_tool_share_osc.h"
#include "app_sound_box_tool.h"
#include "app_config.h"
#include "asm/power_interface.h"

#if TCFG_SHARE_OSC_EN

#define CFG_TOOL_SHARE_OSC_ID 0X31
#define TEMP_BUF_SIZE	256

typedef struct {
    int cmd;
    int data[64];
} CFG_TOOL_SHARE_OSC_PACKET;

/**
 * @brief 回复对端本设备已关机
 */
void cfg_tool_share_osc_power_off_response()
{
    u8 send_buf[6];
    send_buf[0] = CFG_TOOL_SHARE_OSC_ID;
    send_buf[1] = 0x00;
    CFG_TOOL_WRITE_LIT_U32(&send_buf[0] + 2, ONLINE_SUB_OP_POWER_OFF);//cmd
    all_assemble_package_send_for_share_osc(send_buf, sizeof(send_buf));
}

static void cfg_tool_share_osc_callback(u8 *_packet, u32 size)
{
    put_buf(_packet, size);
    u8 *ptr = _packet;
    /* u8 id = ptr[0]; */
    u8 sq = ptr[1];
    CFG_TOOL_SHARE_OSC_PACKET *packet = (CFG_TOOL_SHARE_OSC_PACKET *)&ptr[2];
    u8 *buf = NULL;
    buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (buf == NULL) {
        printf("%s buf malloc err!\n", __FUNCTION__);
        return;
    }
    memset(buf, 0, TEMP_BUF_SIZE);
    u32 send_len = 0;

    switch (packet->cmd) {
    case ONLINE_SUB_OP_POWER_OFF:
        //收到主机命令，从机回复并进入关机流程
        printf("=============ONLINE_SUB_OP_POWER_OFF==========\n");
        cfg_tool_share_osc_power_off_response();
        extern void power_set_soft_poweroff(void);
        power_set_soft_poweroff();
        return;
        break;
    default:
        break;
    }

    all_assemble_package_send_to_pc(REPLY_STYLE, sq, buf, send_len);

    free(buf);
}

REGISTER_DETECT_TARGET(share_osc_target) = {
    .id = CFG_TOOL_SHARE_OSC_ID,
    .tool_message_deal = cfg_tool_share_osc_callback,
};

#endif

