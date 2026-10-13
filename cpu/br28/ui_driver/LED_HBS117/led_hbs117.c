#include "includes.h"
#include "app_config.h"

#include "led_hbs117.h"

#if TCFG_UI_LED_HBS117_ENABLE

#define HBS117_CS_H()       gpio_direction_output(HBS117_STB, 1)
#define HBS117_CS_L()       gpio_direction_output(HBS117_STB, 0)
#define HBS117_SDA(val)     gpio_direction_output(HBS117_DIO, val)
#define HBS117_SDA_H()      gpio_direction_output(HBS117_DIO, 1)
#define HBS117_SCL_H()      gpio_direction_output(HBS117_CLK, 1)
#define HBS117_SCL_L()      gpio_direction_output(HBS117_CLK, 0)

// HBS117共阳极七段数码管段码表
const u8 code_table[] = {
    (u8)~0xc0, (u8)~0xf9, (u8)~0xa4, (u8)~0xb0,     // 0~3
    (u8)~0x99, (u8)~0x92, (u8)~0x82, (u8)~0xf8,     // 4~7
    (u8)~0x80, (u8)~0x90, (u8)~0x88, (u8)~0x83,     // 8~b
    (u8)~0xc6, (u8)~0xa1, (u8)~0x86, (u8)~0x8e,     // C~F
};

enum HBS117_DISPLAY_MODE display_mode = GRID7_SEG11;// 目前暂时只支持7位11段显示模式
u8 SEG11_GRID7_TABLE[ADDR_MAX] = {0x00};
bool ADDR_MODE;

// 获取更新一个SEG的显示所需要的7个数据
// 注意：只更新其中一个SEG的数据，不修改SEG_GRID_TABLE数据buf中其他SEG的数据
// 将code段码转换成HBS117的SEG_GRID数据表
void hbs117_code2seg_grid_table(enum HBS117_SEG seg, u8 code)
{
    u8 num = code_table[code];
    if (seg < SEG8) {
        for (u8 grid = 0; grid < GRID_MAX; grid++) {
            if (LSB & num) {
                SEG11_GRID7_TABLE[grid] |= BIT(seg);
            } else {
                SEG11_GRID7_TABLE[grid] &= ~BIT(seg);
            }
            num >>= 1;
        }
    } else if (seg <= SEG11) {
        for (u8 grid = 0; grid < GRID_MAX; grid++) {
            if (LSB & num) {
                SEG11_GRID7_TABLE[grid + GRID_MAX] |= BIT(seg - SEG8);
            } else {
                SEG11_GRID7_TABLE[grid + GRID_MAX] &= ~BIT(seg - SEG8);
            }
            num >>= 1;
        }
    }
}
// 清除seg_grid_table中某一SEG的数据
void hbs117_clear_one_seg_in_seg_grid_table(enum HBS117_SEG seg)
{
    if (seg < SEG8) {
        for (u8 grid = 0; grid < GRID_MAX; grid++) {
            SEG11_GRID7_TABLE[grid] &= ~BIT(seg);
        }
    } else if (seg <= SEG11) {
        for (u8 grid = 0; grid < GRID_MAX; grid++) {
            SEG11_GRID7_TABLE[grid + GRID_MAX] &= ~BIT(seg - SEG8);
        }
    }
}
// 清除seg_grid_table中的数据
void hbs117_clear_seg_grid_table(void)
{
    for (u8 addr = 0; addr < ADDR_MAX; addr++) {
        SEG11_GRID7_TABLE[addr] = CLEAR;
    }
}

// 片选输入端初始化
static void hbs117_stb_init(void)
{
    gpio_set_direction(HBS117_STB, 0);  // 输出方向
    gpio_set_die(HBS117_STB, 1);        // 普通数字IO
    gpio_set_pull_up(HBS117_STB, 0);    // 不上拉
    gpio_set_pull_down(HBS117_STB, 0);  // 不下拉I
    HBS117_CS_H();                      // 片选低电平有效，拉高片选不使能CLK
}

// 数据线和时钟线初始化
static void hbs117_soft_iic_init(void)
{
    gpio_set_direction(HBS117_CLK, 0);  // 输出方向
    gpio_set_die(HBS117_CLK, 1);        // 普通数字IO
    gpio_set_pull_up(HBS117_CLK, 0);    // 不上拉
    gpio_set_pull_down(HBS117_CLK, 0);  // 不下拉I
    HBS117_SCL_H();                     // 空闲SCL高电平
    gpio_set_direction(HBS117_DIO, 0);  // 输出方向
    gpio_set_die(HBS117_DIO, 1);        // 普通数字IO
    gpio_set_pull_up(HBS117_DIO, 0);    // 不上拉
    gpio_set_pull_down(HBS117_DIO, 0);  // 不下拉I
    HBS117_SDA_H();
}

// 写一个字节
void hbs117_write_byte(u8 byte)
{
    for (u8 i = 0; i < 8; i++) {
        HBS117_SCL_L(); // 拉低时钟准备写入数据
        HBS117_SDA(byte & LSB); // 从低位开始发送
        delay(1);
        HBS117_SCL_H(); // 时钟上升沿锁存串行数据，数据送完后时钟线拉高保持空闲
        delay(1);
        byte >>= 1; // 发送下一位
    }
}

static void hbs117_write_start(void)
{
    HBS117_SCL_H(); // 开始前将确保时钟空闲
    HBS117_CS_L();  // 拉低STB完成片选
    delay(1);
}

static void hbs117_write_end(void)
{
    HBS117_CS_H();  // 数据送完后拉高STB取消片选
}

// 写指令
void hbs117_write_cmd(u8 cmd)
{
    hbs117_write_start();
    hbs117_write_byte(cmd);
    hbs117_write_end();
}

// 写地址
void hbs117_write_addr(u8 addr)
{
    hbs117_write_start();
    hbs117_write_byte(addr);    // 写地址后接着连续写数据，不需要拉高STB
}

// 指定地址写数据
void hbs117_write_data_to_address(u8 addr, u8 data)
{
    hbs117_write_addr(addr);
    hbs117_write_byte(data);
}
// 设置显示模式
void hbs117_set_displaymode(enum HBS117_DISPLAY_MODE mode)
{
    switch (mode) {
    case GRID4_SEG14:
        hbs117_write_cmd(MODE_4GRID_14SEG);
        break;
    case GRID5_SEG13:
        hbs117_write_cmd(MODE_5GRID_13SEG);
        break;
    case GRID6_SEG12:
        hbs117_write_cmd(MODE_6GRID_12SEG);
        break;
    case GRID7_SEG11:
        hbs117_write_cmd(MODE_7GRID_11SEG);
        break;
defualt:
        break;
    }
}

// 设置亮度
void hbs117_set_brightness(enum HBS117_BRIGHT brightness)
{
    switch (brightness) {
    case BRIGHT1:
        hbs117_write_cmd(LED_ON_BRIGHT1);
        break;
    case BRIGHT2:
        hbs117_write_cmd(LED_ON_BRIGHT2);
        break;
    case BRIGHT3:
        hbs117_write_cmd(LED_ON_BRIGHT3);
        break;
    case BRIGHT4:
        hbs117_write_cmd(LED_ON_BRIGHT4);
        break;
    case BRIGHT5:
        hbs117_write_cmd(LED_ON_BRIGHT5);
        break;
    case BRIGHT6:
        hbs117_write_cmd(LED_ON_BRIGHT6);
        break;
    case BRIGHT7:
        hbs117_write_cmd(LED_ON_BRIGHT7);
        break;
    case BRIGHT8:
        hbs117_write_cmd(LED_ON_BRIGHT8);
        break;
    case BRIGHTON:
        hbs117_write_cmd(LED_ON);       ;
        break;
    case BRIGHTOFF:
        hbs117_write_cmd(LED_OFF);      ;
        break;
defualt:
        break;
    }
}

// HBS117设置地址模式
void hbs117_set_addrmode(bool addr_mode)
{
    if (addr_mode) {
        hbs117_write_cmd(HBS117_ADDR_AUTO_UP); // 地址自增
    } else {
        hbs117_write_cmd(HBS117_ADDR_FIX);     // 固定地址
    }
    ADDR_MODE = addr_mode;  // 更新当前地址模式标志
}

// 连续地址写入n字节数据
static void  hbs117_write_nbytes(u8 *buf, u8 addr)
{
    if (ADDR_MODE) {
        hbs117_write_addr(addr);    // 地址自增模式的起始地址
        for (u8 i = 0; i < (CDH - addr); i++) {
            hbs117_write_byte(buf[i]);
        }
        hbs117_write_end();
    } else {
        for (u8 i = 0; i < (CDH - addr); i++) {
            hbs117_write_data_to_address(addr + i, buf[i]);
            hbs117_write_end();
        }
    }
}

// 清零显示寄存器数据
void hbs117_all_led_off(void)
{
    hbs117_set_displaymode(display_mode);
    hbs117_set_addrmode(AUTO_UP); // 地址自增
    hbs117_write_addr(C0H);         // 起始地址
    for (u8 addr = 0; addr < ADDR_MAX; addr++) {      // 14个地址全部清零
        hbs117_write_byte(CLEAR);
    }
    hbs117_write_end();
    hbs117_set_brightness(BRIGHTOFF);
}

void hbs117_all_led_on(void)
{
    hbs117_set_displaymode(display_mode);
    hbs117_set_addrmode(AUTO_UP);
    hbs117_write_addr(C0H);
    for (u8 addr = 0; addr < ADDR_MAX; addr++) {
        hbs117_write_byte(ALLON);
    }
    hbs117_write_end();
    hbs117_set_brightness(BRIGHT8);
}

// 刷新某一个SEG的显示
void hbs117_refresh_one_led_seg(enum HBS117_SEG seg, u8 show_num, enum HBS117_BRIGHT brightness)
{
    hbs117_code2seg_grid_table(seg, show_num);    // 获取显示内如对应于SEG_GRID_table的数据
    hbs117_set_displaymode(display_mode);
    hbs117_set_addrmode(FIX);  // 刷新某一SEG时不使用地址自增，因为地址不连续
    for (u8 grid = 0; grid < GRID_MAX; grid++) {
        if (seg < SEG8) {
            hbs117_write_data_to_address(SEG_L8_GRID1_ADDR + 2 * grid, SEG11_GRID7_TABLE[grid]); // 写低8段数码管数据
        } else if (seg <= SEG11) {
            hbs117_write_data_to_address(SEG_H6_GRID1_ADDR + 2 * grid, SEG11_GRID7_TABLE[grid + GRID_MAX]); // 写高6段数码管数据
        }
        hbs117_write_end();
    }
    hbs117_set_brightness(brightness);
}
// 清除某一SEG的显示
void hbs117_clear_one_led_seg(enum HBS117_SEG seg, enum HBS117_BRIGHT brightness)
{
    hbs117_clear_one_seg_in_seg_grid_table(seg);
    hbs117_set_displaymode(display_mode);
    hbs117_set_addrmode(FIX);  // 刷新某一SEG时不使用地址自增，因为地址不连续
    for (u8 grid = 0; grid < GRID_MAX; grid++) {
        if (seg < SEG8) {
            hbs117_write_data_to_address(SEG_L8_GRID1_ADDR + 2 * grid, SEG11_GRID7_TABLE[grid]); // 写低8SEG数码管数据
        } else if (seg <= SEG11) {
            hbs117_write_data_to_address(SEG_H6_GRID1_ADDR + 2 * grid, SEG11_GRID7_TABLE[grid + GRID_MAX]); // 写高6SEG数码管数据
        }
        hbs117_write_end();
    }
    hbs117_set_brightness(brightness);
}
// 刷新所有SEG
void hbs117_refresh_all_led_seg(u8 *buf, enum HBS117_BRIGHT brightness)
{
    hbs117_set_displaymode(display_mode);
    hbs117_set_addrmode(AUTO_UP);
    hbs117_write_nbytes(buf, C0H);
    hbs117_set_brightness(brightness);
}
// 清除所有SEG的显示
void hbs117_clear_all_led_seg(void)
{
    hbs117_clear_seg_grid_table();
    hbs117_refresh_all_led_seg(SEG11_GRID7_TABLE, BRIGHTOFF);
}

void hbs117_init(void)
{
    hbs117_stb_init();
    hbs117_soft_iic_init();
    hbs117_all_led_off();
}


void hbs117_test_demo(void)
{
    void wdt_close();
    wdt_close();

    hbs117_init();
    hbs117_all_led_on();
    delay(10000000);
    hbs117_all_led_off();
    delay(10000000);
    while (1) {
        for (u8 seg = SEG1; seg < SEG_MAX; seg++) {
            hbs117_refresh_one_led_seg(seg, seg, BRIGHT6);
            delay(10000000);
            hbs117_clear_one_led_seg(seg, BRIGHT6);
            delay(10000000);
        }
    }
}

#endif


