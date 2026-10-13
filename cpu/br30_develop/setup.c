#include "asm/includes.h"
//#include "asm/ldo.h"
//#include "asm/cache.h"
#include "system/task.h"
#include "timer.h"
#include "system/init.h"

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

extern void vPortSysSleepInit(void);

extern u32 reset_source_dump(void);

extern u8 power_reset_source_dump(void);

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
    local_irq_disable();
    while (1);
#else
    cpu_reset();
#endif
}

void timer(void *p)
{
    /* DEBUG_SINGAL_1S(1); */
    sys_timer_dump_time();
    /* DEBUG_SINGAL_1S(0);*/
}

u8 power_reset_src = 0;

extern void sputchar(char c);
extern void sput_buf(const u8 *buf, int len);
void sput_u32hex(u32 dat);
void *vmem_get_phy_adr(void *vaddr);

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

void load_common_code();
void app_bank_init()
{
#ifdef CONFIG_CODE_BANK_ENABLE
    extern void bank_syscall_entry();
    request_irq(IRQ_SYSCALL_IDX, 0, bank_syscall_entry, 0);
#endif

#ifdef CONFIG_CODE_BANK_ENABLE
    load_common_code();
#endif
}


u8 debug_init_flag = 0;
void memory_init(void);
void ble_bb_pkt_err_rate_cfg_set(u16 total_count_num, u16 err_num_limit);
void setup_arch()
{
    //asm("trigger ");
    JL_UART0->CON0 |= BIT(2); //use lp waiting, must set Tx pending Enable

    memory_init();


#if defined(TCFG_FIX_NOISE) && (TCFG_FIX_NOISE == 1)
    extern void fix_dac_popo(u8 en); //用于改善隔直推开dac的杂声
    fix_dac_popo(1);
#endif
    //P11 系统必须提前打开
    p11_init();
    wdt_init(WDT_8S);
    /* wdt_close(); */
#if TCFG_SHARE_OSC_EN
    clk_init_osc_cap(0x00, 0x00);
    clk_hcs12v_osc(0x00);
#else
    clk_init_osc_cap(0x0a, 0x0a);
    clk_hcs12v_osc(0x0a);
#endif
    clk_voltage_init(TCFG_CLOCK_MODE, SYSVDD_VOL_SEL_126V);
    clk_early_init(TCFG_CLOCK_SYS_SRC, TCFG_CLOCK_OSC_HZ, TCFG_CLOCK_SYS_HZ);

    tick_timer_init();

    /*interrupt_init();*/

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

    extern u32 p33_rd_page_test(u8 page);
    //r_printf("page0:%x page1:%x\n",p33_rd_page_test(0),p33_rd_page_test(1));
    extern void set_vbg_level_from_efuse(void);
    set_vbg_level_from_efuse();

    /* power_reset_source_dump(); */

    //Register debugger interrupt
    request_irq(0, 2, exception_irq_handler, 0);
    request_irq(1, 2, exception_irq_handler, 0);

    debug_init();

    sys_timer_init();
#if CONFIG_BT_RF_USING_EXTERNAL_PA_EN
    bt_set_rxtx_status_enable(1);
    /* setting PA contrl io,if this API not be called,using default IO(PC2/PC3) at 697N series;  */
    /* extern void bt_rf_PA_control_io_remap(u16 tx_io,u16 rx_io); */
    /* bt_rf_PA_control_io_remap(IO_PORTC_01,IO_PORTC_02); */
#endif
    /* sys_timer_add(NULL, timer, 10 * 1000); */


    __crc16_mutex_init();
}

/*-----------------------------------------------------------*/



/* --------------------------------------------------------------------------*/
/**
 * @brief 通过遍历链表获取当前已创建的任务
 */
/* ----------------------------------------------------------------------------*/
extern const char *pcTaskName(void *pxTCB);
struct list_head *tl_head = (struct list_head *)0x31df8;
struct task_list {
    struct list_head entry;
    void *task;
};
void task_name_loop(void)
{
    struct task_list *p;
    list_for_each_entry(p, tl_head, entry) {
        printf("task : %s", pcTaskName(p->task));
    }
}



