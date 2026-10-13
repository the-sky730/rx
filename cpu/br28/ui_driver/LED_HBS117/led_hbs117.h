#ifndef _LED_HBS117_H_
#define _LED_HBS117_H_

#if TCFG_UI_LED_HBS117_ENABLE

#define GRID_MAX         7 // HBS117的7个位输出
#define SEG_MAX          11// HBS1117的11个段输出
enum HBS117_SEG {
    SEG1,
    SEG2,
    SEG3,
    SEG4,
    SEG5,
    SEG6,
    SEG7,
    SEG8,
    SEG9,
    SEG10,
    SEG11,
};
#define SEG1    0
#define SEG2    1
#define SEG3    2
#define SEG4    3
#define SEG5    4
#define SEG6    5
#define SEG7    6
#define SEG8    7
#define SEG9    8
#define SEG10   9
#define SEG11   10

// HBS117的四种指令
#define MODE_CMD    (u8)0x00    // 显示模式设置命令
#define DATA_CMD    (u8)0x40    // 数据读写设置命令
#define DISP_CMD    (u8)0x80    // 显示控制命令
#define ADDR_CMD    (u8)0xC0    // 地址设置命令

// HBS117的4种显示模式
enum HBS117_DISPLAY_MODE {
    GRID4_SEG14,    // 4 位 14 段
    GRID5_SEG13,    // 5 位 13 段
    GRID6_SEG12,    // 6 位 12 段
    GRID7_SEG11,    // 7 位 11 段
};
// 写入HBS117的显示模式命令
#define MODE_4GRID_14SEG   MODE_CMD | (u8)GRID4_SEG14
#define MODE_5GRID_13SEG   MODE_CMD | (u8)GRID5_SEG13
#define MODE_6GRID_12SEG   MODE_CMD | (u8)GRID6_SEG12
#define MODE_7GRID_11SEG   MODE_CMD | (u8)GRID7_SEG11

// HBS117的四种数据命令
#define WRITE_DATA      (u8)0x40    // 写数据到显示寄存器
#define READ_DATA       (u8)0x42    // 读键盘扫描数据
#define ADDR_AUTO_UP    (u8)0x40    // 自动地址增加
#define ADDR_FIX        (u8)0x44    // 固定地址
// 写入HBS117的数据命令
#define HBS117_WRITE_DATA       DATA_CMD | WRITE_DATA
#define HBS117_READ_DATA        DATA_CMD | READ_DATA
#define HBS117_ADDR_AUTO_UP     DATA_CMD | ADDR_AUTO_UP
#define HBS117_ADDR_FIX         DATA_CMD | ADDR_FIX

#define AUTO_UP     true
#define FIX         false

// HBS1117的显示亮度级别
enum HBS117_BRIGHT {
    BRIGHT1,        // 亮度等级1
    BRIGHT2,        // 亮度等级2
    BRIGHT3,        // 亮度等级3
    BRIGHT4,        // 亮度等级4
    BRIGHT5,        // 亮度等级5
    BRIGHT6,        // 亮度等级6
    BRIGHT7,        // 亮度等级7
    BRIGHT8,        // 亮度等级8
    BRIGHTON,       // 亮度显示开
    BRIGHTOFF,      // 亮度显示关
};
#define HBS117_BRIGHTOFF    (u8)(BRIGHTOFF-BRIGHTOFF)
#define HBS117_BRIGHT1      (u8)BRIGHT1
#define HBS117_BRIGHT2      (u8)BRIGHT2
#define HBS117_BRIGHT3      (u8)BRIGHT3
#define HBS117_BRIGHT4      (u8)BRIGHT4
#define HBS117_BRIGHT5      (u8)BRIGHT5
#define HBS117_BRIGHT6      (u8)BRIGHT6
#define HBS117_BRIGHT7      (u8)BRIGHT7
#define HBS117_BRIGHT8      (u8)BRIGHT8
#define HBS117_BRIGHTON     (u8)BRIGHTON

// 写入HBS117的显示设置命令
#define LED_ON_BRIGHT1     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT1
#define LED_ON_BRIGHT2     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT2
#define LED_ON_BRIGHT3     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT3
#define LED_ON_BRIGHT4     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT4
#define LED_ON_BRIGHT5     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT5
#define LED_ON_BRIGHT6     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT6
#define LED_ON_BRIGHT7     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT7
#define LED_ON_BRIGHT8     DISP_CMD | HBS117_BRIGHTON | HBS117_BRIGHT8
#define LED_ON             DISP_CMD | HBS117_BRIGHTON
#define LED_OFF            DISP_CMD | HBS117_BRIGHTOFF

// HBS117的14个显示地址
#define ADDR_MAX    14
#define C0H         (u8)0xC0
#define C1H         (u8)0xC1
#define C2H         (u8)0xC2
#define C3H         (u8)0xC3
#define C4H         (u8)0xC4
#define C5H         (u8)0xC5
#define C6H         (u8)0xC6
#define C7H         (u8)0xC7
#define C8H         (u8)0xC8
#define C9H         (u8)0xC9
#define CAH         (u8)0xCA
#define CBH         (u8)0xCB
#define CCH         (u8)0xCC
#define CDH         (u8)0xCD

//// 地址分布&SEG_GRID_TABLE
//                   SEG14   SEG13   SEG12   SEG11   SEG10   SEG9       SEG8    SEG7    SEG6    SEG5    SEG4    SEG3    SEG2    SEG1
//C1H|  0      0      X       X       X       X       X       X  | C0H|  X       X       X       X       X       X       X       X   |GRID1
//C3H|  0      0      X       X       X       X       X       X  | C2H|  X       X       X       X       X       X       X       X   |GRID2
//C5H|  0      0      X       X       X       X       X       X  | C4H|  X       X       X       X       X       X       X       X   |GRID3
//C7H|  0      0      X       X       X       X       X       X  | C6H|  X       X       X       X       X       X       X       X   |GRID4
//C9H|  0      0      X       X       X       X       X       X  | C8H|  X       X       X       X       X       X       X       X   |GRID5
//CBH|  0      0      X       X       X       X       X       X  | CAH|  X       X       X       X       X       X       X       X   |GRID6
//CDH|  0      0      X       X       X       X       X       X  | CCH|  X       X       X       X       X       X       X       X   |GRID7
//      B7     B6     B5      B4      B3      B2      B1      B0         B7      B6      B5      B4      B3      B2      B1      B0
// 刷新一次全部显示需要完成14个地址的数据写入，刷新某个SEG的显示需要完成7个地址的数据写入

// GRID地址
#define SEG_L8_GRID1_ADDR     ADDR_CMD | C0H        // 对应数据为SEG_GRID_TABLE[0]
#define SEG_L8_GRID2_ADDR     ADDR_CMD | C2H        // 对应数据为SEG_GRID_TABLE[1]
#define SEG_L8_GRID3_ADDR     ADDR_CMD | C4H        // 对应数据为SEG_GRID_TABLE[2]
#define SEG_L8_GRID4_ADDR     ADDR_CMD | C6H        // 对应数据为SEG_GRID_TABLE[3]
#define SEG_L8_GRID5_ADDR     ADDR_CMD | C8H        // 对应数据为SEG_GRID_TABLE[4]
#define SEG_L8_GRID6_ADDR     ADDR_CMD | CAH        // 对应数据为SEG_GRID_TABLE[5]
#define SEG_L8_GRID7_ADDR     ADDR_CMD | CCH        // 对应数据为SEG_GRID_TABLE[6]
#define SEG_H6_GRID1_ADDR     ADDR_CMD | C1H        // 对应数据为SEG_GRID_TABLE[7]
#define SEG_H6_GRID2_ADDR     ADDR_CMD | C3H        // 对应数据为SEG_GRID_TABLE[8]
#define SEG_H6_GRID3_ADDR     ADDR_CMD | C5H        // 对应数据为SEG_GRID_TABLE[9]
#define SEG_H6_GRID4_ADDR     ADDR_CMD | C7H        // 对应数据为SEG_GRID_TABLE[10]
#define SEG_H6_GRID5_ADDR     ADDR_CMD | C9H        // 对应数据为SEG_GRID_TABLE[11]
#define SEG_H6_GRID6_ADDR     ADDR_CMD | CBH        // 对应数据为SEG_GRID_TABLE[12]
#define SEG_H6_GRID7_ADDR     ADDR_CMD | CDH        // 对应数据为SEG_GRID_TABLE[13]

// HBS117控制端口
#define HBS117_DIO  IO_PORTB_04
#define HBS117_STB  IO_PORTB_05
#define HBS117_CLK  IO_PORTB_06


#define CLEAR   (u8)0x00
#define ALLON   (u8)0xff
#define LSB     (u8)0x01



/* ***************************************************************************/
/**
 * \Brief :        hbs117初始化
 */
/* ***************************************************************************/
void hbs117_init(void);


/* ***************************************************************************/
/**
 * \Brief :        往hbs117写指令
 *
 * \Param :        cmd:指令
 */
/* ***************************************************************************/
void hbs117_write_cmd(u8 cmd);


/* ***************************************************************************/
/**
 * \Brief :        往hbs117写地址
 *
 * \Param :        addr:地址
 */
/* ***************************************************************************/
void hbs117_write_addr(u8 addr);


/* ***************************************************************************/
/**
 * \Brief :        往hbs117指定地址写数据
 *
 * \Param :        addr:指定的地址
 * \Param :        data:写入的数据
 */
/* ***************************************************************************/
void hbs117_write_data_to_address(u8 addr, u8 data);


/* ***************************************************************************/
/**
 * \Brief :        设置hbs117的显示模式
 *
 * \Param :        mode
 */
/* ***************************************************************************/
void hbs117_set_displaymode(enum HBS117_DISPLAY_MODE mode);


/* ***************************************************************************/
/**
 * \Brief :        设置hbs117的地址模式
 *
 * \Param :        addr_mode:true自动地址增加/false固定地址
 */
/* ***************************************************************************/
void hbs117_set_addrmode(bool addr_mode);


/* ***************************************************************************/
/**
 * \Brief :        设置LED亮度
 *
 * \Param :        brightness：亮度
 */
/* ***************************************************************************/
void hbs117_set_brightness(enum HBS117_BRIGHT brightness);


/* ***************************************************************************/
/**
 * \Brief :        关闭所有LED，写0x00数据到HBS117，不影响seg_grid_table数据buf
 */
/* ***************************************************************************/
void hbs117_all_led_off(void);


/* ***************************************************************************/
/**
 * \Brief :        以最大亮度点亮所有LED，写0xff数据到HBS117，不影响seg_grid_table数据buf
 */
/* ***************************************************************************/
void hbs117_all_led_on(void);


/* ***************************************************************************/
/**
 * \Brief :        将共阳极7段数码管段码数据转换为HBS117存储格式的seg_grid_table数据buf
 *
 * \Param :        seg：seg_grid_table中的seg
 * \Param :        code：段码数据
 */
/* ***************************************************************************/
void hbs117_code2seg_grid_table(enum HBS117_SEG seg, u8 code);


/* ***************************************************************************/
/**
 * \Brief :        清除某一SEG在SEG_GRID_TABLE数据buf中的数据
 *
 * \Param :        seg：要清除的SEG
 */
/* ***************************************************************************/
void hbs117_clear_one_seg_in_seg_grid_table(enum HBS117_SEG seg);


/* ***************************************************************************/
/**
 * \Brief :        刷新某一SEG，往7个地址写入数据，不修改其他SEG的seg_grid_table数据buf
 *
 * \Param :        seg；需要刷新的SEG
 * \Param :        show_num；显示的数
 * \Param :        brightness：亮度
 */
/* ***************************************************************************/
void hbs117_refresh_one_led_seg(enum HBS117_SEG seg, u8 show_num, enum HBS117_BRIGHT brightness);


/* ***************************************************************************/
/**
 * \Brief :        刷新所有SEG，往全部14个地址写入数据
 *
 * \Param :        buf：写入的数据
 * \Param :        brightness：亮度
 */
/* ***************************************************************************/
void hbs117_refresh_all_led_seg(u8 *buf, enum HBS117_BRIGHT brightness);


/* ***************************************************************************/
/**
 * \Brief :        清除某一SEG的seg_grid_table数据buf，并关闭该SEG的LED
 *
 * \Param :        seg：清除的SEG
 * \Param :        brightness：亮度
 */
/* ***************************************************************************/
void hbs117_clear_one_led_seg(enum HBS117_SEG seg, enum HBS117_BRIGHT brightness);


/* ***************************************************************************/
/**
 * \Brief :        清除seg_grid_table数据buf，并关闭所有LED
 */
/* ***************************************************************************/
void hbs117_clear_all_led_seg(void);


/* ***************************************************************************/
/**
 * \Brief :        清除seg_grid_table数据buf
 */
/* ***************************************************************************/
void hbs117_clear_seg_grid_table(void);
void hbs117_test_demo(void);
#endif
#endif


