
#include "duplex_mode_switch.h"

#if TCFG_DUPLEX_EARPHONE_MODE_SW

const u8 flag_first_mode = DUPLEX_MODE;


void boot_init_hook(void *_info)
{
    u8 reset_sfr = 0;
    u8 flag_reset = 0;
    u32 *p = STORE_ADDR;

    //get reset src
    reset_sfr = P3_RST_SRC;
    if ((P3_PMU_CON0 & BIT(7))) {
        if (reset_sfr & BIT(5)) {
            flag_reset = 1;
        }
    }
    if (p[0] == MODE_CODE) {
        ((void(*)(void *))CODE_ADDR + (OFFSET_LEN * 1024))(_info);
    } else {
        if (flag_reset) {

        } else {
            if (POWERON_MODE == DUPLEX_MODE) {

            } else if (POWERON_MODE == EARPHONE_MODE) {
                p[0] = MODE_CODE;
                p[1] &= ~(0xff);
                p[1] |= CHECK_KEY;
                //cpu_reset();
                ((void(*)(void *))CODE_ADDR + (OFFSET_LEN * 1024))(_info);
            }

        }
    }
}
//记忆模式需要重新检查vm的模式
void recheck_vm_mode(void)
{
#if POWERON_MODE == MEMORY_MODE
    u32 read_mode = 0;
    u32 *p = STORE_ADDR;
    printf("[mode_swicth] p[0] = 0x%x,p[1] =0x%x", p[0], p[1]);
    int ret = syscfg_read(MODE_VM_ID, &read_mode, sizeof(read_mode));
    if (ret == sizeof(read_mode)) {
        if (read_mode == MODE_CODE) {
            printf("1 err mode jump earphone");
#if CONFIG_CPU_BR28
            cpu_suspend_other_core(CPU_SUSPEND_TYPE_SFC);
#endif
            local_irq_disable();
            p[0] = MODE_CODE;
            p[1] &= ~0xff;
            p[1] |= CHECK_KEY;
            cpu_reset();
        }
    } else {
        if (flag_first_mode == DUPLEX_MODE) {

        } else {
            printf("2 err mode jump earphone");
            read_mode = MODE_CODE;
            syscfg_write(MODE_VM_ID, &read_mode, sizeof(read_mode));
#if CONFIG_CPU_BR28
            cpu_suspend_other_core(CPU_SUSPEND_TYPE_SFC);
#endif
            local_irq_disable();
            p[0] = MODE_CODE;
            p[1] &= ~0xff;
            p[1] |= CHECK_KEY;
            cpu_reset();
        }
    }
    printf("correct mode ");
#endif
}

void duplex_earphone_mode_switch(u8 mode)
{
    u32 *p = STORE_ADDR;
    printf("switch mode = %d", mode);
    printf("p[0] = %d", p[0]);
#if POWERON_MODE == MEMORY_MODE
    u32 write_mode = 0;
    if (mode == DUPLEX_MODE) {
        write_mode = 0;
    } else if (mode == EARPHONE_MODE) {
        write_mode = MODE_CODE;
    }
    syscfg_write(MODE_VM_ID, &write_mode, sizeof(write_mode));
    //os_time_dly(5);
#endif

#if CONFIG_CPU_BR28
    cpu_suspend_other_core(CPU_SUSPEND_TYPE_SFC);
#endif
    local_irq_disable();
    if (mode == DUPLEX_MODE) {
        p[0] = 0;
    } else if (mode == EARPHONE_MODE) {
        p[0] = MODE_CODE;
    }
    p[1] &= ~0xff;
    cpu_reset();
}
#endif
