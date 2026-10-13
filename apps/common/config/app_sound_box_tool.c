#include "app_sound_box_tool.h"
#include "event.h"
#include "usb/device/cdc.h"
#include "boot.h"
#include "ioctl_cmds.h"
#include "app_config.h"
#include "app_online_cfg.h"
#include "asm/crc16.h"

#if (TCFG_SOUNDBOX_TOOL_ENABLE || TCFG_ONLINE_ENABLE || TCFG_SHARE_OSC_EN)

#define LOG_TAG_CONST       APP_SOUNDBOX_TOOL
#define LOG_TAG             "[APP_SOUNDBOX_TOOL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

struct sound_box_tool_info {
    /*PC往小机发送的DATA*/
    R_QUERY_BASIC_INFO 		r_basic_info;
    R_QUERY_FILE_SIZE		r_file_size;
    R_QUERY_FILE_CONTENT	r_file_content;
    R_PREPARE_WRITE_FILE	r_prepare_write_file;
    R_READ_ADDR_RANGE		r_read_addr_range;
    R_ERASE_ADDR_RANGE      r_erase_addr_range;
    R_WRITE_ADDR_RANGE      r_write_addr_range;
    R_ENTER_UPGRADE_MODE    r_enter_upgrade_mode;

    /*小机返回PC发送的DATA*/
    S_QUERY_BASIC_INFO 		s_basic_info;
    S_QUERY_FILE_SIZE		s_file_size;
    S_PREPARE_WRITE_FILE    s_prepare_write_file;
};

static struct sound_box_tool_info info = {
    .s_basic_info.protocolVer = PROTOCOL_VER,
};

#define TEMP_BUF_SIZE	256

extern const char *sdk_version_info_get(void);
extern u8 *sdfile_get_burn_code(u8 *len);
extern int norflash_erase(u32 cmd, u32 addr);

static u8 local_packet[TEMP_BUF_SIZE];
const char error_return[] = "FA";	//表示失败
const char ok_return[] = "OK";		//表示成功
static u32 size_total_write = 0;

extern void doe(u16 k, void *pBuf, u32 lenIn, u32 addr);
extern int norflash_erase(u32 cmd, u32 addr);

#ifdef ALIGN
#undef ALIGN
#endif

#define ALIGN(a, b) \
	({ \
	 	int m = (u32)(a) & ((b)-1); \
		int ret = (u32)(a) + (m?((b)-m):0);	 \
		ret;\
	})

static u32 sound_box_encode_data_by_user_key(u16 key, u8 *buff, u16 size, u32 dec_addr, u8 dec_len)
{
    u16 key_addr;
    u16 r_len;

    while (size) {
        r_len = (size > dec_len) ? dec_len : size;
        key_addr = (dec_addr >> 2)^key;
        doe(key_addr, buff, r_len, 0);
        buff += r_len;
        dec_addr += r_len;
        size -= r_len;
    }

    return dec_addr;
}

#if TCFG_SHARE_OSC_EN
void all_assemble_package_send_for_share_osc(u8 *buf, u32 len)
{
    u8 send_buf[TEMP_BUF_SIZE];
    u16 crc16_data;

    send_buf[0] = 0x5A;
    send_buf[1] = 0xAA;
    send_buf[2] = 0xA5;
    send_buf[5] = len;
    memcpy(send_buf + 6, buf, len);
    crc16_data = CRC16(&send_buf[5], len + 1);
    send_buf[3] = crc16_data & 0xff;
    send_buf[4] = (crc16_data >> 8) & 0xff;

    ci_uart_write(send_buf, len + 6);
}
#endif

/*全封装与发送*/
void all_assemble_package_send_to_pc(u8 id, u8 sq, u8 *buf, u32 len)
{
    u8 *send_buf = NULL;
    u16 crc16_data;

    send_buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (send_buf == NULL) {
        log_error("send_buf malloc err!");
        return;
    }

    /*添加报头*/
    send_buf[0] = 0x5A;
    send_buf[1] = 0xAA;
    send_buf[2] = 0xA5;

    /*L*/
    send_buf[5] = 2 + len;

    /*T*/
    send_buf[6] = id;

    /*SQ*/
    send_buf[7] = sq;

    /*组包完成*/
    memcpy(send_buf + 8, buf, len);

    /*添加CRC16*/
    crc16_data = CRC16(&send_buf[5], len + 3);
    send_buf[3] = crc16_data & 0xff;
    send_buf[4] = (crc16_data >> 8) & 0xff;

    /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
    /* printf_buf(send_buf, len + 8); */

    /*发送给PC*/
#if (TCFG_SOUNDBOX_TOOL_ENABLE || TCFG_ONLINE_ENABLE || TCFG_SHARE_OSC_EN)
#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
    cdc_write_data(0, send_buf, len + 8);
#else
    ci_uart_write(send_buf, len + 8);
#endif
#endif

    free(send_buf);
}

/*组包与发送*/
void assemble_package_send_to_pc(u8 *buf, u32 len)
{
    u8 *send_buf = NULL;
    u16 crc16_data;

    send_buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (send_buf == NULL) {
        log_error("send_buf malloc err!");
        return;
    }

    /*添加报头*/
    send_buf[0] = 0x5A;
    send_buf[1] = 0xAA;
    send_buf[2] = 0xA5;

    /*添加CRC16*/
    crc16_data = CRC16(buf, len);
    send_buf[3] = crc16_data & 0xff;
    send_buf[4] = (crc16_data >> 8) & 0xff;

    /*组包完成*/
    memcpy(send_buf + 5, buf, len);

    /* printf_buf(send_buf, len + 5); */

    /*发送给PC*/
#if (TCFG_SOUNDBOX_TOOL_ENABLE || TCFG_ONLINE_ENABLE)
#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
    cdc_write_data(0, send_buf, len + 5);
#else
    ci_uart_write(send_buf, len + 5);
#endif
#endif

    free(send_buf);
}

/*6个byte的校验码数组转为字符串*/
void hex2text(u8 *buf, u8 *out)
{
    sprintf(out, "%02x%02x-%02x%02x%02x%02x", buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);
}

extern void nvram_set_boot_state(u32 state);
extern void hw_mmu_disable(void);
extern void ram_protect_close(void);

AT(.volatile_ram_code)
void soundbox_tool_go_mask_usb_updata()
{
    local_irq_disable();
    ram_protect_close();
    hw_mmu_disable();
    nvram_set_boot_state(2);

    JL_CLOCK->PWR_CON |= (1 << 4);
    /* chip_reset(); */
    /* cpu_reset(); */
    while (1);
}

int app_soundbox_tool_event_handler(struct soundbox_tool_event *soundbox_tool_dev)
{
    u8 *buf = NULL;
    u8 *buf_temp = NULL;
    u8 *buf_temp_0 = NULL;
    u32 erase_cmd;
    int write_len;
    u8 crc_temp_len, sdkname_temp_len;
    char proCrc_fw[32] = {0};
    char sdkName_fw[32] = {0};
    const struct tool_interface *p;

    /* printf("cfg_tool rx:\n"); */
    /* printf_buf(soundbox_tool_dev->packet, soundbox_tool_dev->size); */

    buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (buf == NULL) {
        log_error("buf malloc err!");
        return 0;
    }
    buf_temp_0 = (u8 *)malloc(TEMP_BUF_SIZE);
    if (buf_temp_0 == NULL) {
        free(buf);
        log_error("buf_temp_0 malloc err!");
        return 0;
    }
    buf_temp_0 = (u8 *)ALIGN(buf_temp_0, 4);
    memset(buf_temp_0, 0, 256);
    memcpy(buf_temp_0 + 1, soundbox_tool_dev->packet, soundbox_tool_dev->size);

    /*数据进行处理*/
    list_for_each_tool_interface(p) {
        if (p->id == soundbox_tool_dev->packet[1]) {
            p->tool_message_deal(buf_temp_0 + 2, soundbox_tool_dev->size - 1);
            free(buf_temp_0);
            free(buf);
            return 0;
        }
    }

    memset(buf, 0, TEMP_BUF_SIZE);

    switch (soundbox_tool_dev->event) {
    case ONLINE_SUB_OP_QUERY_BASIC_INFO:
        log_info("event_ONLINE_SUB_OP_QUERY_BASIC_INFO\n");
        printf("wireless mic no support");
#if 0
        /*获取校验码*/
        u8 *p = sdfile_get_burn_code(&crc_temp_len);
        memcpy(info.s_basic_info.progCrc, p + 8, 6);
        /* printf_buf(info.s_basic_info.progCrc, 6); */
        hex2text(info.s_basic_info.progCrc, proCrc_fw);
        /* log_info("crc:%s\n", proCrc_fw); */

        /*获取固件版本信息*/
        sdkname_temp_len = strlen(sdk_version_info_get());
        memcpy(info.s_basic_info.sdkName, sdk_version_info_get(), sdkname_temp_len);
        memcpy(sdkName_fw, info.s_basic_info.sdkName, sdkname_temp_len);
        log_info("version:%s\n", sdk_version_info_get());

        struct flash_head flash_head_for_pid_vid;

        for (u8 i = 0; i < 5; i++) {
            norflash_read(NULL, (u8 *)&flash_head_for_pid_vid, 32, 0x1000 * i);
            doe(0xffff, (u8 *)&flash_head_for_pid_vid, 32, 0);
            if (flash_head_for_pid_vid.crc == 0xffff) {
                continue;
            } else {
                log_info("flash head addr = 0x%x\n", 0x1000 * i);
                break;
            }
        }

        struct flash_head _head;
        struct flash_head *temp_p = &_head;
        memcpy(temp_p, &flash_head_for_pid_vid, 32);

        /*获取PID*/
        memset(info.s_basic_info.pid, 0, sizeof(info.s_basic_info.pid));
        memcpy(info.s_basic_info.pid, temp_p->pid, sizeof(info.s_basic_info.pid));
        for (u8 i = 0; i < sizeof(info.s_basic_info.pid); i++) {
            if (~info.s_basic_info.pid[i] == 0x00) {
                info.s_basic_info.pid[i] = 0x00;
            }
        }
        /* printf_buf(info.s_basic_info.pid, 16); */

        /*获取VID*/
        memset(info.s_basic_info.vid, 0, sizeof(info.s_basic_info.vid));
        memcpy(info.s_basic_info.vid, temp_p->vid, 4);
        /* printf_buf(info.s_basic_info.vid, 16); */

        /*第一次组包*/
        buf[0] = 1 + 1 + sizeof(info.s_basic_info.protocolVer) + sizeof(proCrc_fw) + sizeof(sdkName_fw) + 32;
        buf[1] = REPLY_STYLE;
        buf[2] = soundbox_tool_dev->packet[2];
        buf[3] = info.s_basic_info.protocolVer & 0xff;
        buf[4] = (info.s_basic_info.protocolVer >> 8) & 0xff;
        memcpy(buf + 5, proCrc_fw, sizeof(proCrc_fw));
        memcpy(buf + 5 + sizeof(proCrc_fw), sdkName_fw, sizeof(sdkName_fw));
        memcpy(buf + 5 + sizeof(proCrc_fw) + sizeof(sdkName_fw), info.s_basic_info.pid, 16);
        memcpy(buf + 5 + sizeof(proCrc_fw) + sizeof(sdkName_fw) + 16, info.s_basic_info.vid, 16);

        /* printf_buf(buf, buf[0] + 1); */

        /*二次组包添加报头与CRC16并发送*/
        assemble_package_send_to_pc(buf, buf[0] + 1);
#endif
        break;
    case ONLINE_SUB_OP_QUERY_FILE_SIZE:
        log_info("event_ONLINE_SUB_OP_QUERY_FILE_SIZE\n");

        /*读取文件ID*/
        info.r_file_size.file_id = (soundbox_tool_dev->packet[7] | (soundbox_tool_dev->packet[8] << 8) | \
                                    (soundbox_tool_dev->packet[9] << 16) | (soundbox_tool_dev->packet[10] << 24));

        if ((info.r_file_size.file_id == CFG_TOOL_FILEID) || (info.r_file_size.file_id == CFG_EQ_FILEID)) {
            /*根据名字获取文件句柄*/
            FILE *cfg_fp = NULL;

            if ((info.r_file_size.file_id == CFG_TOOL_FILEID)) {
                cfg_fp = fopen(CFG_TOOL_FILE, "r");
                log_info("open cfg_tool.bin\n");
                if (cfg_fp == NULL) {
                    log_error("file open error!\n");
                    break;
                }
            } else {
                cfg_fp = fopen(CFG_EQ_FILE, "r");
                log_info("open eq_cfg_hw.bin\n");
                if (cfg_fp == NULL) {
                    log_error("file open error!\n");
                    break;
                }
            }

            /*根据文件句柄获取文件属性*/
            struct vfs_attr attr;
            fget_attrs(cfg_fp, &attr);

            /* log_info("file addr:%x,file size:%d\n", attr.sclust, attr.fsize); */
            info.s_file_size.file_size = attr.fsize;

            /*第一次组包*/
            buf[0] = 1 + 1 + sizeof(info.s_file_size.file_size);//长度
            buf[1] = REPLY_STYLE;//包的类型
            buf[2] = soundbox_tool_dev->packet[2];//回复序列号

            /*小端格式*/
            buf[6] = (info.s_file_size.file_size >> 24) & 0xff;
            buf[5] = (info.s_file_size.file_size >> 16) & 0xff;
            buf[4] = (info.s_file_size.file_size >> 8) & 0xff;
            buf[3] = info.s_file_size.file_size & 0xff;

            /*二次组包添加报头与CRC16并发送*/
            assemble_package_send_to_pc(buf, buf[0] + 1);

            fclose(cfg_fp);
        }
        break;
    case ONLINE_SUB_OP_QUERY_FILE_CONTENT:
        log_info("event_ONLINE_SUB_OP_QUERY_FILE_CONTENT\n");

        /*读取文件的ID*/
        info.r_file_content.file_id = (soundbox_tool_dev->packet[7] | (soundbox_tool_dev->packet[8] << 8) | \
                                       (soundbox_tool_dev->packet[9] << 16) | (soundbox_tool_dev->packet[10] << 24));
        /*读取文件的偏移*/
        info.r_file_content.offset = (soundbox_tool_dev->packet[11] | (soundbox_tool_dev->packet[12] << 8) | \
                                      (soundbox_tool_dev->packet[13] << 16) | (soundbox_tool_dev->packet[14] << 24));
        /*读取文件的大小*/
        info.r_file_content.size = (soundbox_tool_dev->packet[15] | (soundbox_tool_dev->packet[16] << 8) | \
                                    (soundbox_tool_dev->packet[17] << 16) | (soundbox_tool_dev->packet[18] << 24));

        if ((info.r_file_content.file_id == CFG_TOOL_FILEID) || (info.r_file_content.file_id == CFG_EQ_FILEID)) {
            /*根据名字获取文件句柄*/
            FILE *cfg_fp = NULL;

            if ((info.r_file_content.file_id == CFG_TOOL_FILEID)) {
                cfg_fp = fopen(CFG_TOOL_FILE, "r");
                log_info("open cfg_tool.bin\n");
                if (cfg_fp == NULL) {
                    log_error("file open error!\n");
                    break;
                }
            } else {
                cfg_fp = fopen(CFG_EQ_FILE, "r");
                log_info("open eq_cfg_hw.bin\n");
                if (cfg_fp == NULL) {
                    log_error("file open error!\n");
                    break;
                }
            }

            /*根据文件句柄获取文件属性*/
            struct vfs_attr attr;
            fget_attrs(cfg_fp, &attr);

            /* log_info("file addr:%x,file size:%d\n", attr.sclust, attr.fsize); */
            if (info.r_file_content.size > attr.fsize) {
                fclose(cfg_fp);
                log_error("reading size more than actual size!\n");
                break;
            }

            /*逻辑地址转换成flash物理地址*/
            u32 flash_addr = sdfile_cpu_addr2flash_addr(attr.sclust);
            /* log_info("flash_addr:0x%x", flash_addr); */

            /*读取文件内容*/
            buf_temp = (char *)malloc(info.r_file_content.size);
            norflash_read(NULL, (void *)buf_temp, info.r_file_content.size, flash_addr + info.r_file_content.offset);
            /* printf_buf(buf_temp, info.r_file_content.size); */

            /*第一次组包*/
            buf[0] = 1 + 1 + info.r_file_content.size;
            buf[1] = REPLY_STYLE;
            buf[2] = soundbox_tool_dev->packet[2];
            memcpy(buf + 3, buf_temp, info.r_file_content.size);

            /*二次组包添加报头与CRC16并发送*/
            assemble_package_send_to_pc(buf, buf[0] + 1);

            if (buf_temp) {
                free(buf_temp);
            }

            fclose(cfg_fp);
        }
        break;
    case ONLINE_SUB_OP_PREPARE_WRITE_FILE:
        log_info("event_ONLINE_SUB_OP_PREPARE_WRITE_FILE\n");

        /*读取文件的ID*/
        info.r_prepare_write_file.file_id = (soundbox_tool_dev->packet[7] | (soundbox_tool_dev->packet[8] << 8) | \
                                             (soundbox_tool_dev->packet[9] << 16) | (soundbox_tool_dev->packet[10] << 24));
        /*读取文件的大小*/
        info.r_prepare_write_file.size = (soundbox_tool_dev->packet[11] | (soundbox_tool_dev->packet[12] << 8) | \
                                          (soundbox_tool_dev->packet[13] << 16) | (soundbox_tool_dev->packet[14] << 24));

        FILE *cfg_fp = NULL;
        /*根据名字获取文件句柄*/

        if ((info.r_prepare_write_file.file_id == CFG_TOOL_FILEID)) {
            cfg_fp = fopen(CFG_TOOL_FILE, "r");
            log_info("open cfg_tool.bin\n");
            if (cfg_fp == NULL) {
                log_error("file open error!\n");
                break;
            }
        } else if ((info.r_prepare_write_file.file_id == CFG_EQ_FILEID)) {
            cfg_fp = fopen(CFG_EQ_FILE, "r");
            log_info("open eq_cfg_hw.bin\n");
            if (cfg_fp == NULL) {
                log_error("file open error!\n");
                break;
            }
        } else if ((info.r_prepare_write_file.file_id == CFG_OLD_EQ_FILEID)) {
            cfg_fp = fopen(CFG_OLD_EQ_FILE, "r");
            log_info("open cfg_tool.bin\n");
            if (cfg_fp == NULL) {
                log_error("file open error!\n");
                break;
            }
        }             /*根据文件句柄获取文件属性*/
        struct vfs_attr attr;
        fget_attrs(cfg_fp, &attr);

        /* log_info("file addr:%x,file size:%d\n", attr.sclust, attr.fsize); */
        if (info.r_prepare_write_file.size > attr.fsize) {
            //fclose(cfg_fp);
            //log_error("preparing to write size more than actual size!\n");
            //break;
        }

        /*获取文件实际大小*/
        info.s_prepare_write_file.file_size = attr.fsize;
        log_info("file_size = %d\n", info.s_prepare_write_file.file_size);

        /*逻辑地址转换成flash物理地址*/
        info.s_prepare_write_file.file_addr = sdfile_cpu_addr2flash_addr(attr.sclust);
        log_info("file_flash_addr:0x%x\n", info.s_prepare_write_file.file_addr);

        /*获取flash擦除单元*/
        info.s_prepare_write_file.earse_unit = boot_info.vm.align * 256;
        log_info("earse_unit = %d\n", info.s_prepare_write_file.earse_unit);

        /*第一次组包*/
        buf[0] = 1 + 1 + sizeof(info.s_prepare_write_file.file_size) * 3;
        buf[1] = REPLY_STYLE;
        buf[2] = soundbox_tool_dev->packet[2];

        buf[6] = (info.s_prepare_write_file.file_addr >> 24) & 0xff;
        buf[5] = (info.s_prepare_write_file.file_addr >> 16) & 0xff;
        buf[4] = (info.s_prepare_write_file.file_addr >> 8) & 0xff;
        buf[3] = info.s_prepare_write_file.file_addr & 0xff;

        buf[10] = (info.s_prepare_write_file.file_size >> 24) & 0xff;
        buf[9] 	= (info.s_prepare_write_file.file_size >> 16) & 0xff;
        buf[8] 	= (info.s_prepare_write_file.file_size >> 8) & 0xff;
        buf[7] 	= info.s_prepare_write_file.file_size & 0xff;

        buf[14] = (info.s_prepare_write_file.earse_unit >> 24) & 0xff;
        buf[13] = (info.s_prepare_write_file.earse_unit >> 16) & 0xff;
        buf[12] = (info.s_prepare_write_file.earse_unit >> 8) & 0xff;
        buf[11] = info.s_prepare_write_file.earse_unit & 0xff;

        /*二次组包添加报头与CRC16并发送*/
        assemble_package_send_to_pc(buf, buf[0] + 1);

        fclose(cfg_fp);
        break;
    case ONLINE_SUB_OP_READ_ADDR_RANGE:
        log_info("event_ONLINE_SUB_OP_READ_ADDR_RANGE\n");

        /*要读取的flash物理地址*/
        info.r_read_addr_range.addr = (soundbox_tool_dev->packet[7] | (soundbox_tool_dev->packet[8] << 8) | \
                                       (soundbox_tool_dev->packet[9] << 16) | (soundbox_tool_dev->packet[10] << 24));
        log_info("reading flash addr:0x%x\n", info.r_read_addr_range.addr);

        /*要读取的大小*/
        info.r_read_addr_range.size = (soundbox_tool_dev->packet[11] | (soundbox_tool_dev->packet[12] << 8) | \
                                       (soundbox_tool_dev->packet[13] << 16) | (soundbox_tool_dev->packet[14] << 24));
        log_info("reading size = %d\n", info.r_read_addr_range.size);

        /*读取文件内容*/
        buf_temp = (char *)malloc(info.r_read_addr_range.size);
        norflash_read(NULL, (void *)buf_temp, info.r_read_addr_range.size, info.r_read_addr_range.addr);
        /* printf_buf(buf_temp, info.r_read_addr_range.size); */

        /*第一次组包*/
        buf[0] = 1 + 1 + info.r_read_addr_range.size;
        buf[1] = REPLY_STYLE;
        buf[2] = soundbox_tool_dev->packet[2];
        memcpy(buf + 3, buf_temp, info.r_read_addr_range.size);

        /*二次组包添加报头与CRC16并发送*/
        assemble_package_send_to_pc(buf, buf[0] + 1);

        if (buf_temp) {
            free(buf_temp);
        }
        break;
    case ONLINE_SUB_OP_ERASE_ADDR_RANGE:
        log_info("event_ONLINE_SUB_OP_ERASE_ADDR_RANGE\n");

        /*要擦除的flash物理起始地址*/
        info.r_erase_addr_range.addr = (soundbox_tool_dev->packet[7] | (soundbox_tool_dev->packet[8] << 8) | \
                                        (soundbox_tool_dev->packet[9] << 16) | (soundbox_tool_dev->packet[10] << 24));
        log_info("erasing flash start addr:0x%x\n", info.r_erase_addr_range.addr);

        /*要擦除的大小，会保证按earse_unit对齐，即总是erase_unit的倍数*/
        info.r_erase_addr_range.size = (soundbox_tool_dev->packet[11] | (soundbox_tool_dev->packet[12] << 8) | \
                                        (soundbox_tool_dev->packet[13] << 16) | (soundbox_tool_dev->packet[14] << 24));
        log_info("erasing size = %d\n", info.r_erase_addr_range.size);

        log_info("earse_unit = %d\n", info.s_prepare_write_file.earse_unit);
        switch (info.s_prepare_write_file.earse_unit) {
        case 256:
            erase_cmd = IOCTL_ERASE_PAGE;
            break;
        case (4*1024):
            erase_cmd = IOCTL_ERASE_SECTOR;
            break;
        case (64*1024):
            erase_cmd = IOCTL_ERASE_BLOCK;
            break;
defualt:
            memcpy(buf + 3, error_return, sizeof(error_return));
            log_error("erase error!");
            break;
        }

        for (u8 i = 0; i < (info.r_erase_addr_range.size / info.s_prepare_write_file.earse_unit); i ++) {
            u8 ret = norflash_erase(erase_cmd, info.r_erase_addr_range.addr + (i * info.s_prepare_write_file.earse_unit));
            if (ret) {
                memcpy(buf + 3, error_return, sizeof(error_return));
                log_error("erase error!");
            } else {
                memcpy(buf + 3, ok_return, sizeof(ok_return));
                log_info("erase success");
            }
        }

        /*第一次组包*/
        buf[0] = 1 + 1 + sizeof(error_return);
        buf[1] = REPLY_STYLE;
        buf[2] = soundbox_tool_dev->packet[2];

        /*二次组包添加报头与CRC16并发送*/
        assemble_package_send_to_pc(buf, buf[0] + 1);
        break;
    case ONLINE_SUB_OP_WRITE_ADDR_RANGE:
        log_info("event_ONLINE_SUB_OP_WRITE_ADDR_RANGE\n");

        /*要写入的flash物理起始地址*/
        info.r_write_addr_range.addr = (soundbox_tool_dev->packet[7] | (soundbox_tool_dev->packet[8] << 8) | \
                                        (soundbox_tool_dev->packet[9] << 16) | (soundbox_tool_dev->packet[10] << 24));
        /*要写入的大小*/
        info.r_write_addr_range.size = (soundbox_tool_dev->packet[11] | (soundbox_tool_dev->packet[12] << 8) | \
                                        (soundbox_tool_dev->packet[13] << 16) | (soundbox_tool_dev->packet[14] << 24));

        log_info("writing flash start addr:0x%x\n", info.r_write_addr_range.addr);
        log_info("writing size = %d\n", info.r_write_addr_range.size);

        /*读取要写入文件的内容*/
        buf_temp = (char *)malloc(info.r_write_addr_range.size);
        memcpy(buf_temp, soundbox_tool_dev->packet + 15, info.r_write_addr_range.size);
        /* printf_buf(buf_temp, info.r_write_addr_range.size); */

        sound_box_encode_data_by_user_key(boot_info.chip_id, buf_temp, info.r_write_addr_range.size, info.r_write_addr_range.addr - boot_info.sfc.sfc_base_addr, 0x20);

        /*写入数据*/
        write_len = norflash_write(NULL, buf_temp, info.r_write_addr_range.size, info.r_write_addr_range.addr);

        /* norflash_read(NULL, buf_temp, info.r_write_addr_range.size, info.r_write_addr_range.addr); */
        /* printf_buf(buf_temp, info.r_write_addr_range.size); */

        if (write_len != info.r_write_addr_range.size) {
            memcpy(buf + 3, error_return, sizeof(error_return));
            log_error("write error!");
        } else {
            memcpy(buf + 3, ok_return, sizeof(ok_return));
        }

        /*第一次组包*/
        buf[0] = 1 + 1 + sizeof(error_return);
        buf[1] = REPLY_STYLE;
        buf[2] = soundbox_tool_dev->packet[2];

        /*二次组包添加报头与CRC16并发送*/
        assemble_package_send_to_pc(buf, buf[0] + 1);

        if (buf_temp) {
            free(buf_temp);
        }

        if (info.r_prepare_write_file.file_id == CFG_TOOL_FILEID) {
            size_total_write += info.r_write_addr_range.size;

            log_info("size_total_write = %d\n", size_total_write);
            log_info("erasing size = %d\n", info.r_erase_addr_range.size);

            if (size_total_write >= info.r_erase_addr_range.size) {
                size_total_write = 0;
                log_info("cpu_reset\n");
                extern void cpu_reset();
                cpu_reset();
            }
        }
        break;
    case ONLINE_SUB_OP_ENTER_UPGRADE_MODE:
        log_info("event_ONLINE_SUB_OP_ENTER_UPGRADE_MODE\n");
        soundbox_tool_go_mask_usb_updata();
        break;
    default:
        log_error("invalid data\n");
        break;
    }

    free(buf_temp_0);
    free(buf);

    return 0;
}

void soundbox_tool_event_to_user(u8 *packet, u32 type, u8 event, u8 size)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    if (packet != NULL) {
        if (size > sizeof(local_packet)) {
            return;
        }
        e.u.soundbox_tool.packet = local_packet;
        memcpy(e.u.soundbox_tool.packet, packet, size);
    }
    e.arg  = (void *)type;
    e.u.soundbox_tool.event = event;
    e.u.soundbox_tool.size = size;
    sys_event_notify(&e);
}

/*不在中断处理 post消息出去*/
void app_sound_box_tool_message_deal(u8 *buf, u32 len)
{
    u8 lenIndex = 5;
    u8 cmd = buf[lenIndex + 3];
    switch (cmd) {
    case ONLINE_SUB_OP_QUERY_BASIC_INFO:
        /* log_info("PC is querying basic info\n"); */
        info.r_basic_info.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_QUERY_BASIC_INFO, buf[lenIndex] + 1);
        break;
    case ONLINE_SUB_OP_QUERY_FILE_SIZE:
        /* log_info("PC is querying file size\n"); */
        info.r_file_size.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_QUERY_FILE_SIZE, buf[lenIndex] + 1);
        break;
    case ONLINE_SUB_OP_QUERY_FILE_CONTENT:
        /* log_info("PC is querying file content\n"); */
        info.r_file_content.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_QUERY_FILE_CONTENT, buf[lenIndex] + 1);
        break;
    case ONLINE_SUB_OP_PREPARE_WRITE_FILE:
        /* log_info("PC is preparing to write file\n"); */
        info.r_prepare_write_file.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_PREPARE_WRITE_FILE, buf[lenIndex] + 1);
        break;
    case ONLINE_SUB_OP_READ_ADDR_RANGE:
        /* log_info("PC is reading address range\n"); */
        info.r_read_addr_range.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_READ_ADDR_RANGE, buf[lenIndex] + 1);
        break;
    case ONLINE_SUB_OP_ERASE_ADDR_RANGE:
        /* log_info("PC is erasing address range\n"); */
        info.r_erase_addr_range.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_ERASE_ADDR_RANGE, buf[lenIndex] + 1);
        break;
    case ONLINE_SUB_OP_WRITE_ADDR_RANGE:
        /* log_info("PC is writing address range\n"); */
        info.r_write_addr_range.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_WRITE_ADDR_RANGE, buf[lenIndex] + 1);
        break;
    case ONLINE_SUB_OP_ENTER_UPGRADE_MODE:
        /* log_info("PC is entering upgrade mode\n"); */
        info.r_enter_upgrade_mode.cmd_id = cmd;
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, ONLINE_SUB_OP_ENTER_UPGRADE_MODE, buf[lenIndex] + 1);
        break;
    default:
        /* log_info("other tools data\n"); */
        soundbox_tool_event_to_user(&buf[lenIndex], DEVICE_EVENT_FROM_SOUNDBOX_TOOL, DEFAULT_ACTION, buf[lenIndex] + 1);
        break;
    }
}

/*数据包内容校验与处理*/
void online_cfg_tool_data_deal(void *buf, u32 len)
{
    u8 *data_buf = buf;
    u16 crc16_data;

    /* printf_buf(buf, len); */

    /*DATA前的固定字节包括 (5A AA A5 CRC L T SQ)共8个字节 */
    if (len < 8) {
        log_error("Data length is too short, receive an invalid message!\n");
        return;
    }

    /*检查报头 5A AA A5是否正确*/
    if ((data_buf[0] != 0x5a) || (data_buf[1] != 0xaa) || (data_buf[2] != 0xa5)) {
        log_error("Header check error, receive an invalid message!\n");
        return;
    }

    u8 crcIndex = 3;

    /*CRC16校验，CRC16校验码位于数据包的buf[3]和buf[4]*/
    crc16_data = (data_buf[crcIndex + 1] << 8) | data_buf[crcIndex];
    if (crc16_data != CRC16(data_buf + crcIndex + 2, len - crcIndex - 2)) {
        log_error("CRC16 check error, receive an invalid message!\n");
        return;
    }

    /* log_info("The crc16 is %04x\n", crc16_data); */
    /* printf_buf(buf, len); */

    app_sound_box_tool_message_deal(buf, len);
}

#else

void online_cfg_tool_data_deal(void *buf, u32 len)
{

}

void all_assemble_package_send_to_pc(u8 id, u8 sq, u8 *buf, u32 len)
{

}

#endif
