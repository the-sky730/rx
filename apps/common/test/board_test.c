#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "app_config.h"

#if  BOARD_TEST_EN
#define bprintf printf

#define BOARD_TX_PIN                        IO_PORT_DP
#define BOARD_RX_PIN                        IO_PORT_DM
#define BOARD_RESET_PIN_TEST                IO_PORTB_08

static void board_uart_write(char *buf, u16 len);
extern void vdd_trim();
/* static void my_put_u8hex(u8 dat); */
extern void bt_update_mac_addr(u8 *addr);
extern void swapX(const uint8_t *src, uint8_t *dst, int len);
extern const u8 *bt_get_mac_addr();

static u8 uart_cbuf[512] __attribute__((aligned(4)));
static u8 uart_rxbuf[512] __attribute__((aligned(4)));
static u8 uart_txbuf[512] __attribute__((aligned(4)));
static u8 data_temp[32];
static const u8 data_head[] = {0x01, 0xE0, 0xFC};
static u16 uart_timer = 0;

#define RX_PARAM_LEN   (data[sizeof(data_head)])
#define CMD_OFFSET     (sizeof(data_head) + 1)
#define DATA_OFFSET    (sizeof(data_head) + 2)
#define REAL_LEN       (len - CMD_OFFSET)

struct st_uart_cmd {
    u8 cmd;
    u32(*callback)(u8 const *data, u16 len);
};

static u32 get_mac_address(u8 *data, u16 len);
static u32 set_mac_address(u8 const *data, u16 len);
static u32 enter_dut(u8 const *data, u16 len);
static u32 enter_fcc(u8 const *data, u16 len);
static u32 set_freq_offset(u8 const *data, u16 len);
static u32 reset(u8 const *data, u16 len);
static u32 confirm_freq_offset(u8 const *data, u16 len);
static u32 set_sys_power_off(u8 const *data, u16 len);
static u32 gpio_test(u8 const *data, u16 len);
static u32 reset_pin_test(u8 const *data, u16 len);
static u32 current_test(u8 const *data, u16 len);
static u32 start_trim(u8 *data, u16 len);

#define CMD_GET_MAC                 0x62
#define CMD_SET_MAC                 0x6A
#define GET_MAC_PARAM_LEN       (6 * 2 + 1)//协议长度等于DATA+CMD
#define SET_MAC_PARAM_LEN       (6 + 1)
#define CMD_GET_CRC                 0x7A
#define CMD_SET_DUT                 0xAF
#define SET_DUT_PARAM_LEN       (1 + 1)
#define CMD_ENTER_FCC               0x80
#define ENTER_FCC_PARAM_LEN     (1 + 1)
#define CMD_SET_FRE                 0x81
#define SET_FRE_PARAM_LEN       (4 + 1)
#define CMD_RESET                   0x82
#define RESET_PARAM_LEN         (1 + 1)
#define CMD_FRE_CONFIRM             0x83
#define FRE_CONFIRM_PARAM_LEN   (1 + 1)
#define CMD_SOFT_POWER_OFF          0x84
#define SET_SOFT_POWER_OFF_LEN  (1 + 1)
#define CMD_GPIO_TEST         		0x85
#define SET_CMD_GPIO_TEST_LEN   (5 + 1)
#define CMD_TEST_MODE         		0x86
#define SET_CMD_TEST_MODE_LEN       (1 + 1)
#define CMD_RESET_PIN_TEST         	0x87
#define SET_CMD_RESET_PIN_TEST_LEN       (1 + 1)
#define CMD_CURRENT_TEST         	0x88
#define SET_CMD_CURRENT_TEST_LEN       (1 + 1)
#define CMD_TRIM_START         	0x89
#define SET_CMD_TRIM_START_LEN       (1 + 1)

static struct st_uart_cmd uart_cmd_list[] = {
    {CMD_GET_MAC, get_mac_address},
    {CMD_SET_MAC, set_mac_address},
    {CMD_SET_DUT, enter_dut},
    {CMD_ENTER_FCC, enter_fcc},
    {CMD_SET_FRE, set_freq_offset},
    {CMD_RESET, reset},
    {CMD_FRE_CONFIRM, confirm_freq_offset},
    {CMD_SOFT_POWER_OFF, set_sys_power_off},
    {CMD_GPIO_TEST, gpio_test},
    {CMD_RESET_PIN_TEST, reset_pin_test},
    {CMD_CURRENT_TEST, current_test},
    {CMD_TRIM_START, start_trim}
};

/* static void my_put_u8hex(u8 dat) */
/* { */
/*     u8 tmp; */
/*     tmp = dat / 16; */
/*     if (tmp < 10) { */
/*         putchar(tmp + '0'); */
/*     } else { */
/*         putchar(tmp - 10 + 'A'); */
/*     } */
/*     tmp = dat % 16; */
/*     if (tmp < 10) { */
/*         putchar(tmp + '0'); */
/*     } else { */
/*         putchar(tmp - 10 + 'A'); */
/*     } */
/*     putchar(0x20); */
/* } */


static u16 uart_tx_buff_input(u8 const *data, u16 offset, u16 len)
{
    if ((offset + len) > sizeof(uart_txbuf)) {
        return 0;
    }

    if (offset == 0) {
        memset(uart_txbuf, 0x00, sizeof(uart_txbuf));
    }

    memcpy(uart_txbuf + offset, data, len);

    return len;
}

static u16 uart_tx_data_input(u8 data, u16 offset, u16 len)
{
    if ((offset + len) > sizeof(uart_txbuf)) {
        return 0;
    }

    if (offset == 0) {
        memset(uart_txbuf, 0x00, sizeof(uart_txbuf));
    }

    uart_txbuf[offset] = data;

    return len;
}

//获取补码
static u8 *get_data_complement_code(u8 const *data, u8 len)
{
    int i = 0;
    if (len > sizeof(data_temp)) {
        return NULL;
    }

    for (i = 0; i < len; i++) {
        data_temp[i] = 0xff - data[i];
    }
//printf("len = %d, date_temp: ",len);
//put_buf(data_temp,len);
    return data_temp;
}



u32 board_test_edr_handle = 0;
/* #define OPCODE(ogf, ocf) (ocf | ogf << 10) */
/* const hci_cmd_t hci_write_scan_enable = { */
/*         OPCODE(OGF_CONTROLLER_BASEBAND, 0x1A), "1" */
/* }; */
/*  */
/* const hci_cmd_t hci_disconnect = { */
/*     OPCODE(OGF_LINK_CONTROL, 0x06), "H1" */
/* }; */


/*
conn_en: 0:断开连接;  1:不操作
wait_scan_en: 广播开关
 */

extern int lmp_hci_disconnect(u16 handle, u8 reason);
extern int lmp_hci_write_scan_enable(u8 enable);
void edr_module_enable(u8 conn_en, u8 wait_scan_en)
{
    printf("%s,%d", __func__, __LINE__);
    static u8 scan_status = 0; //广播状态
    if (conn_en == 0) {
        printf("%s,%d", __func__, __LINE__);
        if (board_test_edr_handle != 0) {
            printf("%s,%d", __func__, __LINE__);
            lmp_hci_disconnect(board_test_edr_handle, 0x16);
        }
    }

    if (wait_scan_en) {
        printf("%s,%d", __func__, __LINE__);
        if (scan_status == 0) {
            printf("%s,%d", __func__, __LINE__);
            scan_status = 1;
            lmp_hci_write_scan_enable(3);
            /* hci_send_cmd(&hci_write_scan_enable, 3);  */
        }
    } else {
        printf("%s,%d", __func__, __LINE__);
        if (scan_status == 1) {
            printf("%s,%d", __func__, __LINE__);
            scan_status = 0;
            lmp_hci_write_scan_enable(0);
            /* hci_send_cmd(&hci_write_scan_enable, 0);  */
        }
    }
}

//输出0/1
static void my_gpio_set_out(u32 gpio, u8 value)
{
    gpio_set_pull_up(gpio, 1);
    gpio_set_pull_down(gpio, 0);
    gpio_set_direction(gpio, 0);
    gpio_set_die(gpio, 1);
    gpio_set_dieh(gpio, 0);
    gpio_write(gpio, value);
}

//下拉输入
static void key_io_pull_down_input(u8 key_io)
{
    gpio_direction_input(key_io);
    gpio_set_pull_down(key_io, 1);
    gpio_set_pull_up(key_io, 0);
    gpio_set_die(key_io, 1);
}

//上拉输入
static void key_io_pull_up_input(u8 key_io)
{
    gpio_direction_input(key_io);
    gpio_set_pull_down(key_io, 0);
    gpio_set_pull_up(key_io, 1);
    gpio_set_die(key_io, 1);
}


#define GPIO_GOOD 0
#define GPIO_BAD 1
static u32 gpio_test_array[] = {

    IO_PORTA_01, IO_PORTA_02,

    IO_PORTB_04, IO_PORTB_05,

    IO_PORTB_06, IO_PORTB_07,

    IO_PORTB_02, IO_PORTB_03,

    IO_PORTB_00, IO_PORTB_01,

    IO_PORTC_04, IO_PORTC_05,

    IO_PORTC_02, IO_PORTC_03,

    IO_PORTC_00, IO_PORTC_01,

    IO_PORTA_07, IO_PORTA_08,

    IO_PORTA_05, IO_PORTA_06,

    IO_PORTA_03, IO_PORTA_04
};



//修改蓝牙地址
static u8 set_mac_addr(u8 *addr)
{
    static u8 temp_addr[6];
    temp_addr[0] = addr[5];
    temp_addr[1] = addr[4];
    temp_addr[2] = addr[3];
    temp_addr[3] = addr[2];
    temp_addr[4] = addr[1];
    temp_addr[5] = addr[0];

    int ret = syscfg_write(BOARD_TEST_MAC, temp_addr, 6);
    printf("ret = %d", ret);
    if (ret == 6) {
        bt_update_mac_addr(temp_addr);
    }

    return ret == 6 ? 1 : 0;
    //return 1;
}

static int one_pin_test(u32 portA, u32 portB)
{
    key_io_pull_down_input(portA);
    my_gpio_set_out(portB, 1);
    /* delay(1ms); */

    if (0 == gpio_read(portA)) {
        return GPIO_BAD;
    }

    key_io_pull_up_input(portA);
    my_gpio_set_out(portB, 0);
    /* delay(1ms); */
    if (1 == gpio_read(portA)) {
        return GPIO_BAD;
    }

    return GPIO_GOOD;
}


/*******************添加功能******************/
static u32 start_trim(u8 *data, u16 len)
{
    u8 offset = 0;

    if (2 == RX_PARAM_LEN) {

        bprintf("start_trim");
        vdd_trim();

        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(SET_CMD_TRIM_START_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_TRIM_START, offset, 1);//cmd
        offset += uart_tx_data_input(1, offset, 1);//data
        board_uart_write(uart_txbuf, offset);  //完成trim
    }
    return 0;
}




#define WAIT_DISCONN_TIME_MS     (300)
static void app_set_soft_poweroff(void)
{
    bprintf("set_soft_poweroff\n");
    //必须先主动断开蓝牙链路,否则要等链路超时断开
#if TCFG_USER_BLE_ENABLE
    ble_module_enable(0);
#endif

#if TCFG_USER_EDR_ENABLE
    edr_module_enable(0, 0);
#endif

    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}


//第一次上电需充电激活后关机
static u32 set_sys_power_off(u8 const *data, u16 len)
{
    u32 offset = 0;

    //校验数据长度，和实际收到的数据的长度
    if ((2 == RX_PARAM_LEN) && (2 == REAL_LEN)) {
        bprintf("set_sys_power_offs\n");
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(SET_SOFT_POWER_OFF_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_SOFT_POWER_OFF, offset, 1);//cmd
        offset += uart_tx_data_input(2, offset, 1);//data
        board_uart_write(uart_txbuf, offset);
        app_set_soft_poweroff();  //软关机同时关掉BLE
    }
    return 0;
}


/**
 * 获取蓝牙地址
 */
static u32 get_mac_address(u8 *data, u16 len)
{
    u32 offset = 0;
    u8 addr_temp[6];
    if (1 == RX_PARAM_LEN) { //检验一下收到的数据长度
        //填写回传数据，顺序不能变

        bprintf("get_mac_addr\n");
        swapX(bt_get_mac_addr(), addr_temp, 6);
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(GET_MAC_PARAM_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_GET_MAC, offset, 1);//cmd
        offset += uart_tx_buff_input(addr_temp, offset, 6);//mac
        offset += uart_tx_buff_input(get_data_complement_code(addr_temp, 6), offset, 6);//mac的补码
    }

    return offset;
}


static u32 set_mac_address(u8 const *data, u16 len)
{

    u32 offset = 0;
    u8 ret = 0;
    //校验数据长度，和实际收到的数据的长度
    if (7 == RX_PARAM_LEN) {

        bprintf("set mac addr");
        ret = set_mac_addr(data + DATA_OFFSET);

        //填写回传数据，顺序不能变
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(SET_MAC_PARAM_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_SET_MAC, offset, 1);//cmd
        offset += uart_tx_buff_input(data + DATA_OFFSET, offset, 6);//mac
    }
    return offset;
}

/**
 * 进入dut测试:
 */
static u32 enter_dut(u8 const *data, u16 len)
{
    u32 offset = 0;

    if (2 == RX_PARAM_LEN) {
        bredr_set_dut_enble(1, 0);
        edr_module_enable(1, 1);
        bprintf("enter dut");

        //填写回传数据，顺序不能变
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(SET_DUT_PARAM_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_SET_DUT, offset, 1);//cmd
        offset += uart_tx_data_input(1, offset, 1);//data
    }
    return offset;
}

/**
 * 进入定频，用于测试和校准频偏 TODO
 */
extern void bt_fix_fre_api(u8 fre);
int frq_offset = 0;
static u32 enter_fcc(u8 const *data, u16 len)
{
    u32 offset = 0;

    if (2 == RX_PARAM_LEN) {
        bprintf("enter fcc");
        bt_fix_fre_api(39);
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(ENTER_FCC_PARAM_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_ENTER_FCC, offset, 1);//cmd
        offset += uart_tx_data_input(1, offset, 1);//data
    }
    return offset;
}

/**
 * 修改频偏（只修改，不保存）
 */
static u32 set_freq_offset(u8 const *data, u16 len)
{
    u32 offset = 0;

    if (5 == RX_PARAM_LEN) {
        //调整信号频率(这里的单位是Hz)
        /* frq_offset = 0; */
        frq_offset += (s32)data[DATA_OFFSET + 0] << 24;
        frq_offset += (s32)data[DATA_OFFSET + 1] << 16;
        frq_offset += (s32)data[DATA_OFFSET + 2] << 8;
        frq_offset += (s32)data[DATA_OFFSET + 3];

        bprintf("frq_offset %d", frq_offset);
        bt_osc_offset_ext_updata(frq_offset / 1000); //frq_offset是Hz，但是函数的参数是KHz

        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(2, offset, 1);//len
        offset += uart_tx_data_input(CMD_SET_FRE, offset, 1);//cmd
        offset += uart_tx_data_input(1, offset, 1);//data
    }
    return offset;
}

/**
 * 保存频偏值（修改并保存）
 */
static u32 confirm_freq_offset(u8 const *data, u16 len)
{
    u32 offset = 0;

    if (2 == RX_PARAM_LEN) {
        bt_osc_offset_ext_save(frq_offset / 1000);
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(2, offset, 1);//len
        offset += uart_tx_data_input(CMD_FRE_CONFIRM, offset, 1);//cmd
        offset += uart_tx_data_input(1, offset, 1);//data
        bprintf("confirm_freq_offset\n");
    }
    return offset;
}


/**
 * 复位
 */
static u32 reset(u8 const *data, u16 len)
{
    u32 offset = 0;
    if (2 == RX_PARAM_LEN) {

        if (frq_offset != 0) { //为了适应小牛测试机没有发保存频偏指令
            bt_osc_offset_ext_save(frq_offset / 1000);  //只会存一次
        }

        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(RESET_PARAM_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_RESET, offset, 1);//cmd
        offset += uart_tx_data_input(1, offset, 1);//data
        board_uart_write(uart_txbuf, offset);
        cpu_reset();
    }

    return 0;
}


/**
 *GPIO测试
 */

static u32 gpio_test(u8 const *data, u16 len)
{
    u32 offset = 0;
    int i = 0;
    static u32 ret = 0xffffffff;
    u32 gpio_all_good = (0xffffffff << (sizeof(gpio_test_array) / sizeof(gpio_test_array[0]) / 2));
    u8 result = 1; //0表示通过, 1表示不通过
    //校验收到的数据长度，校验实际收到的数据的长度

    if (2 == RX_PARAM_LEN) {
        bprintf("gpio test");
        ret = 0xffffffff;
        for (i = 0; i < (sizeof(gpio_test_array) / sizeof(gpio_test_array[0])); i += 2) {
            bprintf("i = %d", i);
            ret <<= 1;
            if (one_pin_test(gpio_test_array[i], gpio_test_array[i + 1]) == GPIO_GOOD) {
            } else {
                ret |= GPIO_BAD;
            }
        }
        //执行GPIO测试命令
        printf("gpio_all_good=%x, ret=%x", gpio_all_good, ret);
        if (gpio_all_good == ret) {
            result = 0;
        }
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(SET_CMD_GPIO_TEST_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_GPIO_TEST, offset, 1);//cmd
        offset += uart_tx_buff_input(&ret, offset, sizeof(ret));//
        offset += uart_tx_data_input(result, offset, 1);//result
        board_uart_write(uart_txbuf, offset);
    }
    return 0;
}


/**复位引脚测试***/
static u32 reset_pin_test(u8 const *data, u16 len)
{
    u32 offset = 0;
    if (2 == RX_PARAM_LEN) {
        bprintf("reset_pin_test");
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(SET_CMD_RESET_PIN_TEST_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_RESET_PIN_TEST, offset, 1);//cmd
        offset += uart_tx_data_input(1, offset, 1);//data
        board_uart_write(uart_txbuf, offset);

        gpio_write(IO_PORTB_08, 0);
    }

    return 0;
}



/**电流测试***/
static u32 current_test(u8 const *data, u16 len)
{
    u32 offset = 0;
    if (2 == RX_PARAM_LEN) {

        bprintf("current test");
        switch (data[DATA_OFFSET]) {
        case 0x01:
            break;
        case 0x02:
            break;
        }
        offset += uart_tx_buff_input((u8 *)data_head, offset, sizeof(data_head));//头
        offset += uart_tx_data_input(SET_CMD_CURRENT_TEST_LEN, offset, 1);//len
        offset += uart_tx_data_input(CMD_CURRENT_TEST, offset, 1);//cmd
        offset += uart_tx_data_input(data[DATA_OFFSET], offset, 1);//data
        board_uart_write(uart_txbuf, offset);

    }

    return 0;
}


static uart_bus_t *uart_bus = NULL;
static void uart_timeout(void *prv)
{
    bprintf("board test uart timeout\n");
    if (uart_bus) {
        uart_dev_close(uart_bus);
        uart_bus = NULL;
    }
}
extern void set_at_uart_wakeup(void);
#define CMD_ERROR_MAX_TIMES 10
static void uart_rx_data_analyze(u8 const *data, u16 len)
{
    u32 offset = 0;
    u8 addr_temp[6];
    u8 i = 0;
    static u8 error_times = 0; //允许10次错误

    //连续发错命令则关闭串口
    if (error_times >= CMD_ERROR_MAX_TIMES) {
        uart_timeout(NULL);
        return ;
    }

    //校验头
    bprintf("len=%d,rlen=%d", data[sizeof(data_head)], (len -  sizeof(data_head)));
    if (memcmp(data_head, data, sizeof(data_head)) == 0) {
        bprintf("uart_rx_data_analyze\n");
        for (i = 0; i < sizeof(uart_cmd_list) / sizeof(uart_cmd_list[0]); i++) {
            if (uart_cmd_list[i].cmd == data[CMD_OFFSET]) { //校验命令
                if (data[sizeof(data_head)] != (len -  sizeof(data_head) - 1)) {  //校验长度
                    error_times ++;
                    break; //长度校验失败
                }
                if (uart_timer != 0) {
                    sys_timeout_del(uart_timer);
                    uart_timer = 0;
                    //set_at_uart_wakeup();  //测试模式需要关掉at串口
                }
                offset = uart_cmd_list[i].callback(data, len);
                break;
            }
        }
        if (i == sizeof(uart_cmd_list) / sizeof(uart_cmd_list[0])) {
            error_times ++;
            //命令校验失败
        }
        if (offset) {
            board_uart_write(uart_txbuf, offset);
        }
    }
}


static void board_test_uart_rx_handler(u8 const *uart_rxbuf, u32 uart_rxcnt)
{
    if (uart_rxcnt) {
        bprintf("get_buffer:\n");
        put_buf(uart_rxbuf, uart_rxcnt);
        uart_rx_data_analyze(uart_rxbuf, uart_rxcnt);
    }

}




#define BOARD_CBUF_SIZE          0x100  //串口驱动缓存大小(单次数据超过cbuf, 则驱动会丢失数据)
#define BOARD_FRAM_SIZE          0x100   //单次数据包最大值(每次cbuf缓存达到fram或串口收到一次数据, 就会起一次中断)

#define BOARD_RX_SIZE          BOARD_CBUF_SIZE  //接收串口驱动数据的缓存
#define BOARD_TX_SIZE          0x100  //

#define BOARD_BAUD_RATE        115200

#define BOARD_TEST_UART_RECIEVE_TIMEOUT 10*1000L //10秒内未收到测试命令则关闭串口

static u8 pRxBuffer_static[BOARD_RX_SIZE] __attribute__((aligned(4)));       //rx memory
static u8 pTxBuffer_static[BOARD_TX_SIZE] __attribute__((aligned(4)));       //tx memory
static u8 devBuffer_static[BOARD_CBUF_SIZE] __attribute__((aligned(4)));       //dev DMA memory

static void board_uart_isr_cb(void *ut_bus, u32 status)
{
    struct sys_event e;
    /* printf("{##%d}"); */

    if (status == UT_RX || status == UT_RX_OT) {

        e.type = SYS_DEVICE_EVENT;
        e.arg  = (void *)DEVICE_EVENT_FROM_BOARD_UART;
        e.u.dev.event = 0;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    }
}




u8 ready_data[] = {0x0d, 0x0a, 'I', 'M', '_', 'R', 'E', 'A', 'D', 'Y', 0x0d, 0x0a};

extern u8 board_test_reset_source;
int board_test_uart_init()
{

    /* if (board_test_reset_source != 0x20) { */
    /*     key_io_pull_down_input(IO_PORT_DM); */
    /*     if (0 == gpio_read(IO_PORT_DM)) { //收到高电平打开串口 */
    /*         return -1; */
    /*     } */
    /*     my_gpio_set_out(IO_PORT_DP, 1); */
    /*     os_time_dly(100); */
    /* } */
    /*  */
    my_gpio_set_out(BOARD_RESET_PIN_TEST, 1);

    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = BOARD_TX_PIN;
    u_arg.rx_pin = BOARD_RX_PIN;
    u_arg.rx_cbuf = devBuffer_static;
    u_arg.rx_cbuf_size = BOARD_CBUF_SIZE;  //>=
    u_arg.frame_length = BOARD_FRAM_SIZE;  //协议数据包
    u_arg.rx_timeout = 1;  //ms
    u_arg.isr_cbfun = board_uart_isr_cb;
    u_arg.baud = BOARD_BAUD_RATE;
    u_arg.is_9bit = 0;

    uart_bus = uart_dev_open(&u_arg);

    if (uart_bus != NULL) {
        bprintf("uart_dev_open() success\n");
        board_uart_write(ready_data, sizeof(ready_data));

        uart_timer = sys_timeout_add(NULL, uart_timeout, BOARD_TEST_UART_RECIEVE_TIMEOUT);
        //开一个超时,10s未收到正确的消息,则再次关闭串口
        return 0;
    }
    return -1;
}

static void board_uart_write(char *buf, u16 len)
{
    if (uart_bus) {
        uart_bus->write(buf, len);
    }
}

void board_test_uart_event_handler()
{
    u8  *p_data = pRxBuffer_static;
    uart_bus_t *uart = uart_bus;
    u32 data_length = 0;
    if (uart == NULL) {
        return;
    }

    //    log_info("uart_len[%d]",__this->udev->get_data_len());
    if (uart->get_data_len() > BOARD_CBUF_SIZE) {
        bprintf("\n uart overflow, Data loss!!!");
        return;
    }

    data_length = uart->read(p_data, BOARD_RX_SIZE, 0);
    if (data_length > BOARD_RX_SIZE) {
        bprintf("cmd overflow");
        data_length = 0;
        return;
    }
    /* bprintf("rx:"); */
    /* put_buf(p_data,data_length); */

    board_test_uart_rx_handler(p_data, data_length);
}

#endif
