#include "adapter_uart_demo.h"
#include "app_config.h"
#include "adapter_process.h"


#if ADAPTER_UART_DEMO

#define	UART_DUPLEX_TRANSPORT_TASK_NAME		"uart_task"

u8 uart_cbuf[64] __attribute__((aligned(4)));
u8 uart_rxbuf[64] __attribute__((aligned(4)));

uart_bus_t *uart_bus;
volatile u8 flag_uart_write_busy = 0;
static u16 check_tid = 0;
void uart_duplex_get_data(void)
{
    u32 uart_rxcnt = 0;
    uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 10);
    put_buf(uart_rxbuf, sizeof(uart_rxbuf));
}

static void uart_isr_hook(void *arg, u32 status)
{
    if (status == UT_RX) {

    } else if (status == UT_RX_OT) {

    }
}

static int write_callback_handle(int msg1, int msg2)
{
    u8 *send_buf = (u8 *)msg1;
    u16 len = (u16)msg2;
    if (uart_bus) {
        flag_uart_write_busy = 1;
        uart_bus->write(send_buf, len);
        flag_uart_write_busy = 0;
    } else {
        putchar('E');
    }
    free(send_buf);
    return 0;
}

void uart_duplex_send_data(u8 *buf, u32 len)
{
    // 发送到指定任务中发送uart数据
    u8 *send_buf = zalloc(len);
    if (send_buf == NULL) {
        printf("UART_DUPLEX_TRANSPORT_TASK_NAME malloc err!\n");
        return;
    }
    memcpy(send_buf, buf, len);

    int argv[4];
    argv[0] = (int)write_callback_handle;      // Function
    argv[1] = 2;                            // 参数个数
    argv[2] = (int)send_buf;                     // 参数1，可以是任意类型强转成int
    argv[3] = (int)len;                     // 参数2，可以是任意类型强转成int
    int ret = os_taskq_post_type(UART_DUPLEX_TRANSPORT_TASK_NAME, Q_CALLBACK, sizeof(argv) / sizeof(int), argv);
    if (ret != OS_NO_ERR) {
        free(send_buf);
    }
}

static void uart_duplex_transport_task(void *p)
{
    printf("create %s task\n", uart_duplex_transport_task);
    int msg[8];
    while (1) {
        if (os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }
    }
}

#define IO_CHECK_CNT	200
static void check_uart_io_level()
{
    static u16 check_cnt = 0;
    if (!flag_uart_write_busy && gpio_read_direction(ADAPTER_UART_TX_PIN)) {
        if (gpio_read(ADAPTER_UART_TX_PIN) == 0) {
            putchar('-');
            if (check_cnt < IO_CHECK_CNT) {
                check_cnt++;
                if (check_cnt == IO_CHECK_CNT) {
                    printf("always low level,DSP already shut down,enter poweroff");
                    adapter_process_event_notify(ADAPTER_EVENT_POWEROFF, 0);
                }
            }
        } else {
            check_cnt = 0;
            return ;
        }
    }
}
void adapter_uart_demo_close(void)
{
    printf("%s", __func__);
    if (uart_bus) {
        uart_dev_close(uart_bus);
    }
    if (check_tid) {
        sys_hi_timer_del(check_tid);
        check_tid = 0;
    }
    gpio_set_direction(ADAPTER_UART_TX_PIN, 1);
    gpio_set_die(ADAPTER_UART_TX_PIN, 1);
    gpio_set_pull_up(ADAPTER_UART_TX_PIN, 0);
    gpio_set_pull_down(ADAPTER_UART_TX_PIN, 0);
}
void adapter_uart_demo_init(void)
{
    struct uart_platform_data_t ut = {0};
    ut.tx_pin = ADAPTER_UART_TX_PIN;
    ut.rx_pin = ADAPTER_UART_RX_PIN;
    ut.baud = 115200;
    ut.rx_timeout = 10;
    ut.isr_cbfun = uart_isr_hook;
    ut.rx_cbuf = uart_cbuf;
    ut.rx_cbuf_size = 64;
    ut.frame_length = 8;
    uart_bus = (uart_bus_t *)uart_dev_open(&ut);
    if (uart_bus == NULL) {
        printf("UART OPEN ERR!!!");
        return;
    }
    //没有发数据的时候，判断uart io是否低电平，DSP是否已关机，是的话跟着一起关机
    if (!check_tid) {
        check_tid = sys_hi_timer_add(NULL, check_uart_io_level, 5);
    }
    task_create(uart_duplex_transport_task, NULL, UART_DUPLEX_TRANSPORT_TASK_NAME);
}

#endif
