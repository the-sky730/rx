#include "asm/includes.h"
//#include "asm/ldo.h"
//#include "asm/cache.h"
#include "asm/wdt.h"
#include "asm/debug.h"
#include "asm/efuse.h"
#include "system/task.h"
#include "timer.h"
#include "asm/power_interface.h"
#include "app_config.h"
#include "gpio.h"
//#include "power_manage.h"
//
#define LOG_TAG_CONST       SETUP
#define LOG_TAG             "[SETUP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

//extern void dv15_dac_early_init(u8 ldo_sel, u8 pwr_sel, u32 dly_msecs);
//
extern void sys_timer_init(void);

extern void tick_timer_init(void);

extern void exception_irq_handler(void);
int __crc16_mutex_init();

extern int __crc16_mutex_init();

#define DEBUG_SINGAL_IDLE(x)        //if (x) IO_DEBUG_1(A, 7) else IO_DEBUG_0(A, 7)
#define DEBUG_SINGAL_1S(x)          //if (x) IO_DEBUG_1(A, 6) else IO_DEBUG_0(A, 6)

#if (defined CONFIG_DEBUG_ENABLE) || (defined CONFIG_DEBUG_LITE_ENABLE)
void debug_uart_init(const struct uart_platform_data *data);
#endif

#if 0
___interrupt
void exception_irq_handler(void)
{
    ___trig;

    exception_analyze();

    log_flush();
    while (1);
}
#endif



/*
 * 此函数在cpu0上电后首先被调用,负责初始化cpu内部模块
 *
 * 此函数返回后，操作系统才开始初始化并运行
 *
 */

#if 0
static void early_putchar(char a)
{
    if (a == '\n') {
        UT2_BUF = '\r';
        __asm_csync();
        while ((UT2_CON & BIT(15)) == 0);
    }
    UT2_BUF = a;
    __asm_csync();
    while ((UT2_CON & BIT(15)) == 0);
}

void early_puts(char *s)
{
    do {
        early_putchar(*s);
    } while (*(++s));
}
#endif

void cpu_assert_debug()
{
#ifdef CONFIG_DEBUG_ENABLE
    log_flush();
    __local_irq_disable();
    while (1);
#else
    P3_PCNT_SET0 = 0xac;
    cpu_reset();
#endif
}

/* AT(.psram_data_code) */
void timer(void *p)
{
    /* DEBUG_SINGAL_1S(1); */
    /* sys_timer_dump_time(); */
    /* printf("cpu %d run", current_cpu_id()); */
    /* DEBUG_SINGAL_1S(0);*/

    /* os_system_info_output(); */

    /* mem_stats(); */
}

extern void sputchar(char c);
extern void sput_buf(const u8 *buf, int len);
void sput_u32hex(u32 dat);

void test_fun()
{
    wdt_close();
    while (1);

}


__attribute__((weak))
void maskrom_init(void)
{
    return;
}


#if (CPU_CORE_NUM > 1)
void cpu1_setup_arch()
{
    q32DSP(core_num())->PMU_CON1 &= ~BIT(8); //open bpu

    request_irq(IRQ_EXCEPTION_IDX, 7, exception_irq_handler, 1);

    //用于控制其他核进入停止状态。
    extern void cpu_suspend_handle(void);
    request_irq(IRQ_SOFT0_IDX, 7, cpu_suspend_handle, 0);
    request_irq(IRQ_SOFT1_IDX, 7, cpu_suspend_handle, 1);
    /* irq_unmask_set(IRQ_SOFT0_IDX, 0); //设置CPU0软中断0为不可屏蔽中断 */
    /* irq_unmask_set(IRQ_SOFT1_IDX, 1); //设置CPU1软中断1为不可屏蔽中断 */

    debug_init();
}
#else
void cpu1_setup_arch()
{
    return;
}
#endif /* #if (CPU_CORE_NUM > 1) */

//==================================================//

extern void gpio_longpress_pin0_reset_config(u32 pin, u32 level, u32 time);
void memory_init(void);
void setup_arch()
{
    q32DSP(core_num())->PMU_CON1 &= ~BIT(8); //open bpu
    gpio_longpress_pin0_reset_config(IO_PORTB_01, 0, 8);

    memory_init();

    //P11 系统必须提前打开
    p11_init();

    wdt_init(WDT_16S);
    /* wdt_close(); */
    efuse_init();

    clk_voltage_init(TCFG_CLOCK_MODE, SYSVDD_VOL_SEL_126V);
    xosc_hcs_trim();
    clk_early_init(TCFG_CLOCK_SYS_SRC, TCFG_CLOCK_OSC_HZ, TCFG_CLOCK_SYS_HZ);
    tick_timer_init();
    /*interrupt_init();*/

    //上电初始所有io
    port_init();

#if (defined CONFIG_DEBUG_ENABLE) || (defined CONFIG_DEBUG_LITE_ENABLE)
    debug_uart_init(NULL);

#ifdef CONFIG_DEBUG_ENABLE
    log_early_init(1024);
#endif

#endif

    log_i("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    log_i("         setup_arch %s %s", __DATE__, __TIME__);
    log_i("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");


    clock_dump();

    /* log_info("resour est: %d", get_boot_flag()); */
    //set_boot_flag(99);
    /* log_info("resour est: %d", get_boot_flag()); */

    reset_source_dump();

    power_wakeup_reason_dump();

    /* power_reset_source_dump(); */

    //Register debugger interrupt
    request_irq(0, 2, exception_irq_handler, 0);
    request_irq(1, 2, exception_irq_handler, 0);

    sys_timer_init();

    debug_init();

    /* sys_timer_add(NULL, timer, 3 * 1000); */
    /* usr_timer_add(NULL, timer, 5 * 1000, 0); */
#if CONFIG_BT_RF_USING_EXTERNAL_PA_EN
    bt_set_rxtx_status_enable(1);
    /* setting PA contrl io,if this API not be called,using default IO(PC2/PC3) at 697N series;  */
    /* extern void bt_rf_PA_control_io_remap(u16 tx_io,u16 rx_io); */
    /* bt_rf_PA_control_io_remap(IO_PORTC_01,IO_PORTC_02); */
#endif

    __crc16_mutex_init();
}

/*-----------------------------------------------------------*/
