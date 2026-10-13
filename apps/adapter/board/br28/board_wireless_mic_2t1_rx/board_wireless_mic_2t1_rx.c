#include "app_config.h"

#ifdef CONFIG_BOARD_WIRELESS_MIC_2T1_RX

#include "system/includes.h"
#include "media/includes.h"
#include "asm/sdmmc.h"
#include "asm/chargestore.h"
#include "asm/charge.h"
#include "asm/pwm_led.h"
#include "asm/rtc.h"
#include "tone_player.h"
#include "audio_config.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "key_event_deal.h"
#include "user_cfg.h"
#include "usb/otg.h"
#include "ui/ui_api.h"
#include "dev_manager.h"
#include "asm/power/p33.h"
#include "norflash.h"
#include "asm/spi.h"
#include "ui_manage.h"
#include "spi/nor_fs.h"
//#include "audio_link.h"
#include "fs/virfat_flash.h"
#include "app_power_manage.h"
#include "asm/power/power_port.h"
#include "nandflash.h"
#include "adapter_uart_demo.h"

#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

void board_power_init(void);

/*各个状态下默认的闪灯方式和提示音设置，如果USER_CFG中设置了USE_CONFIG_STATUS_SETTING为1，则会从配置文件读取对应的配置来填充改结构体*/
STATUS_CONFIG status_config = {
    //提示音设置
    .tone = {
        .charge_start  = IDEX_TONE_NONE,
        .charge_full   = IDEX_TONE_NONE,
        .power_on      = IDEX_TONE_POWER_ON,
        .power_off     = IDEX_TONE_POWER_OFF,
        .lowpower      = IDEX_TONE_LOW_POWER,
        .max_vol       = IDEX_TONE_MAX_VOL,
        .phone_in      = IDEX_TONE_NONE,
        .phone_out     = IDEX_TONE_NONE,
        .phone_activ   = IDEX_TONE_NONE,
        .bt_init_ok    = IDEX_TONE_BT_MODE,
        .bt_connect_ok = IDEX_TONE_BT_CONN,
        .bt_disconnect = IDEX_TONE_BT_DISCONN,
        .tws_connect_ok   = IDEX_TONE_TWS_CONN,
        .tws_disconnect   = IDEX_TONE_TWS_DISCONN,
    }
};

#define __this (&status_config)


// *INDENT-OFF*
/************************** UART config****************************/
#if TCFG_UART0_ENABLE
UART0_PLATFORM_DATA_BEGIN(uart0_data)
    .tx_pin = TCFG_UART0_TX_PORT,
    .rx_pin = TCFG_UART0_RX_PORT,
    .baudrate = TCFG_UART0_BAUDRATE,

    .flags = UART_DEBUG,
UART0_PLATFORM_DATA_END()
#endif //TCFG_UART0_ENABLE


/************************** SD config ****************************/
#if TCFG_SD0_ENABLE
/* int sdmmc_0_io_detect(const struct sdmmc_platform_data *data) */
/* { */
/*     return 1; */
/* } */
/* void sd_set_power(u8 enable) */
/* { */
/*     static u8 old_enable = 0xff; */
/*  */
/*     if(old_enable == enable){ */
/*         return; */
/*     } */
/*     if(enable){ */
/*         sdpg_config(1); */
/*     }else{ */
/*         sdpg_config(0); */
/*     } */
/*     old_enable = enable; */
/* } */
SD0_PLATFORM_DATA_BEGIN(sd0_data)
	.port 					= TCFG_SD0_PORTS,  //sd0 support port 'A' and port 'B'
	.data_width             = TCFG_SD0_DAT_MODE,
	.speed                  = TCFG_SD0_CLK,
	.detect_mode            = TCFG_SD0_DET_MODE,
	.priority				= 3,
#if (TCFG_SD0_DET_MODE == SD_IO_DECT)
	.detect_io              = TCFG_SD0_DET_IO,//当检测模式为io口检测是有效
	.detect_io_level        = TCFG_SD0_DET_IO_LEVEL,//0：低电平检测到卡。 1：高电平检测到卡
	.detect_func            = sdmmc_0_io_detect,//库函数，需要detect_io和detect_io_level两个元素作为参数。可以自行重写一个检测函数，在线返回1，不在线返回0，即可。
    .power                  = sd_set_power,//库函数，使用SDPG引脚。可以自行重写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。如果电源硬件已固定不可控，则该函数无效，可以填NULL
    /* .power                  = NULL, */
#elif (TCFG_SD0_DET_MODE == SD_CLK_DECT)
	.detect_io_level        = TCFG_SD0_DET_IO_LEVEL,//0：低电平检测到卡。 1：高电平检测到卡
	.detect_func            = sdmmc_0_clk_detect,//库函数，需要detect_io_level元素作为参数。可以自行重写一个检测函数，在线返回1，不在线返回0，即可。
    .power                  = sd_set_power,//库函数，使用SDPG引脚。可以自行重写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。如果电源硬件已固定不可控，则该函数无效，可以填NULL
    /* .power                  = NULL, */
#else
	.detect_func            = sdmmc_cmd_detect,
    .power                  = NULL,//cmd检测需要全程供电，建议用硬件固定电源。当然，可以自行写其他的SD卡电源控制函数，传入0：关电源。传入1，开电源。
    /* .power                  = sd_set_power, */
#endif
SD0_PLATFORM_DATA_END()
#endif


/************************** CHARGE config****************************/
#if TCFG_CHARGE_ENABLE
CHARGE_PLATFORM_DATA_BEGIN(charge_data)
    .charge_en              = TCFG_CHARGE_ENABLE,              //内置充电使能
    .charge_poweron_en      = TCFG_CHARGE_POWERON_ENABLE,      //是否支持充电开机
    .charge_full_V          = TCFG_CHARGE_FULL_V,              //充电截止电压
    .charge_full_mA			= TCFG_CHARGE_FULL_MA,             //充电截止电流
    .charge_mA				= TCFG_CHARGE_MA,                  //充电电流
	.charge_trickle_mA		= TCFG_CHARGE_TRICKLE_MA,          //涓流电流
/*ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出
 对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的
 充电仓，需要根据实际情况设置该值大小*/
	.ldo5v_off_filter		= 100,
	.ldo5v_on_filter        = 50,
	.ldo5v_keep_filter      = 220,
	.ldo5v_pulldown_lvl     = CHARGE_PULLDOWN_200K,
	.ldo5v_pulldown_keep    = 0,
/*ldo5v的10k下拉电阻使能,若充电舱需要更大的负载才能检测到插入时，请将该变量置1,默认值设置为1
  对于充电舱需要按键升压,且维持电压是从充电舱经过上拉电阻到充电口的舱，请将该值改为0*/
	.ldo5v_pulldown_en		= 1,
CHARGE_PLATFORM_DATA_END()
#endif//TCFG_CHARGE_ENABLE


#if TCFG_IIS_ENABLE
ALINK_PARM alink_param = {
#if (TCFG_IIS_OUTPUT_PORT == ALINK0_PORTA)
    .port_select = ALINK0_PORTA,            //MCLK:PA2 SCLK:PA3  LRCK:PA4 CH0:PA0 CH1:PA1 CH2:PA5 CH3:PA6
#else
    .port_select = ALINK0_PORTB,            //MCLK:PC2 SCLK:PA5  LRCK:PA6 CH0:PA0 CH1:PA2 CH2:PA3 CH3:PA4
#endif
    .ch_cfg[0].enable = 1,
    .ch_cfg[2].enable = 1,
    .mode = ALINK_MD_IIS,
#if TCFG_IIS_MODE
    .role = ALINK_ROLE_SLAVE,
#else
    .role = ALINK_ROLE_MASTER,
#endif
    .clk_mode = ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE,
    .bitwide = ALINK_LEN_16BIT,
    .sclk_per_frame = ALINK_FRAME_64SCLK,
    .dma_len = ALNK_BUF_POINTS_NUM * 2 * 2 * 2,
    .sample_rate = TCFG_IIS_OUTPUT_SR,
};
#endif


/************************** DAC ****************************/
#if TCFG_AUDIO_DAC_ENABLE
struct dac_platform_data dac_data = {
	//.mode 			= TCFG_AUDIO_DAC_MODE,						 //DAC输出功率
	.ldo_id			= TCFG_AUDIO_DAC_LDO_SEL,					 //保留位
    .ldo_volt       = TCFG_AUDIO_DAC_LDO_VOLT,                   //DACVDD等级.需要根据具体硬件来设置（高低压）可选:1.2V/1.3V/2.35V/2.5V/2.65V/2.8V/2.95V/3.1V
    .vcmo_en        = 0,                                         //是否打开VCOMO
    .output         = TCFG_AUDIO_DAC_CONNECT_MODE,               //DAC输出配置，和具体硬件连接有关，需根据硬件来设置
    //.ldo_fb_isel    = 0,
    .lpf_isel       = 0x2,
    //.dsm_clk        = DAC_DSM_6MHz,
	/*.zero_cross_detect = 1,*/
	.lpf_isel = 0xf,
	.sys_vol_type  = SYS_VOL_TYPE,                              //系统音量选择：模拟音量/数字音量，调节时调节对应的音量
	.max_ana_vol   = MAX_ANA_VOL,                               //模拟音量最大等级
	.max_dig_vol   = MAX_DIG_VOL,                               //数字音量最大等级

};
#endif

/************************** ADC ****************************/
#if TCFG_AUDIO_ADC_ENABLE
#ifndef TCFG_AUDIO_MIC0_BIAS_EN
#define TCFG_AUDIO_MIC0_BIAS_EN				0
#endif/*TCFG_AUDIO_MIC0_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC1_BIAS_EN
#define TCFG_AUDIO_MIC1_BIAS_EN				0
#endif/*TCFG_AUDIO_MIC1_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC2_BIAS_EN
#define TCFG_AUDIO_MIC2_BIAS_EN				0
#endif/*TCFG_AUDIO_MIC2_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC3_BIAS_EN
#define TCFG_AUDIO_MIC3_BIAS_EN				0
#endif/*TCFG_AUDIO_MIC3_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC_LDO_EN
#define TCFG_AUDIO_MIC_LDO_EN				0
#endif/*TCFG_AUDIO_MIC_LDO_EN*/


struct adc_platform_data adc_data = {
	.mic_channel    = TCFG_AUDIO_ADC_MIC_CHA,                   //MIC通道选择，对于693x，MIC只有一个通道，固定选择右声道
/*MIC LDO电流档位设置：
    0:0.625ua    1:1.25ua    2:1.875ua    3:2.5ua*/
	.mic_ldo_isel   = TCFG_AUDIO_ADC_LDO_SEL,

	/*mic_mode 工作模式定义
#define AUDIO_MIC_CAP_MODE          0   //单端隔直电容模式
#define AUDIO_MIC_CAP_DIFF_MODE     1   //差分隔直电容模式
#define AUDIO_MIC_CAPLESS_MODE      2   //单端省电容模式
	 */
	.mic_mode = TCFG_AUDIO_MIC_MODE,
	.mic1_mode = TCFG_AUDIO_MIC1_MODE,
	.mic2_mode = TCFG_AUDIO_MIC2_MODE,
	.mic3_mode = TCFG_AUDIO_MIC3_MODE,

	.mic_bias_inside = TCFG_AUDIO_MIC0_BIAS_EN,
	.mic1_bias_inside = TCFG_AUDIO_MIC1_BIAS_EN,
	.mic2_bias_inside = TCFG_AUDIO_MIC2_BIAS_EN,
	.mic3_bias_inside = TCFG_AUDIO_MIC3_BIAS_EN,

	/*MICLDO供电输出到PAD(PA0)控制使能*/
	.mic_ldo_pwr = TCFG_AUDIO_MIC_LDO_EN,	// MIC LDO 输出到 PA0

	/*MIC免电容方案需要设置，影响MIC的偏置电压
	  0b0001~0b1001 : 0.5k ~ 4.5k step = 0.5k
	  0b1010~0b1111 : 5k ~ 10k step = 1k */
	.mic_bias_res    = 4,
	.mic1_bias_res   = 4,
	.mic2_bias_res   = 4,
	.mic3_bias_res   = 4,
	/*MIC LDO电压档位设置,也会影响MIC的偏置电压
	  0:2.3v  1:2.5v  2:2.7v  3:3.0v */
	.mic_ldo_vsel  = 2,

};
#endif

/* struct audio_pf_data audio_pf_d = { */
/* #if TCFG_AUDIO_DAC_ENABLE */
    /* .adc_pf_data = &adc_data, */
/* #endif */

/* #if TCFG_AUDIO_ADC_ENABLE */
    /* .dac_pf_data = &dac_data, */
/* #endif */
/* }; */

/* struct audio_platform_data audio_data = { */
    /* .private_data = (void *) &audio_pf_d, */
/* }; */


/************************** IO KEY ****************************/
#if TCFG_IOKEY_ENABLE
const struct iokey_port iokey_list[] = {
    {
        .connect_way = TCFG_IOKEY_POWER_CONNECT_WAY,          //IO按键的连接方式
        .key_type.one_io.port = TCFG_IOKEY_POWER_ONE_PORT,    //IO按键对应的引脚
        .key_value = 0,                                       //按键值
    },

    /* { */
        /* .connect_way = TCFG_IOKEY_PREV_CONNECT_WAY, */
        /* .key_type.one_io.port = TCFG_IOKEY_PREV_ONE_PORT, */
        /* .key_value = 1, */
    /* }, */

    /* { */
        /* .connect_way = TCFG_IOKEY_NEXT_CONNECT_WAY, */
        /* .key_type.one_io.port = TCFG_IOKEY_NEXT_ONE_PORT, */
        /* .key_value = 2, */
    /* }, */
};
const struct iokey_platform_data iokey_data = {
    .enable = TCFG_IOKEY_ENABLE,                              //是否使能IO按键
    .num = ARRAY_SIZE(iokey_list),                            //IO按键的个数
    .port = iokey_list,                                       //IO按键参数表
};

#if MULT_KEY_ENABLE
//组合按键消息映射表
//配置注意事项:单个按键按键值需要按照顺序编号,如power:0, prev:1, next:2
//bit_value = BIT(0) | BIT(1) 指按键值为0和按键值为1的两个按键被同时按下,
//remap_value = 3指当这两个按键被同时按下后重新映射的按键值;
const struct key_remap iokey_remap_table[] = {
	{.bit_value = BIT(0) | BIT(1), .remap_value = 3},
	{.bit_value = BIT(0) | BIT(2), .remap_value = 4},
	{.bit_value = BIT(1) | BIT(2), .remap_value = 5},
};

const struct key_remap_data iokey_remap_data = {
	.remap_num = ARRAY_SIZE(iokey_remap_table),
	.table = iokey_remap_table,
};
#endif

#endif

/************************** AD KEY ****************************/
#if TCFG_ADKEY_ENABLE
const struct adkey_platform_data adkey_data = {
    .enable = TCFG_ADKEY_ENABLE,                              //AD按键使能
    .adkey_pin = TCFG_ADKEY_PORT,                             //AD按键对应引脚
    .ad_channel = TCFG_ADKEY_AD_CHANNEL,                      //AD通道值
    .extern_up_en = TCFG_ADKEY_EXTERN_UP_ENABLE,              //是否使用外接上拉电阻
    .ad_value = {                                             //根据电阻算出来的电压值
        TCFG_ADKEY_VOLTAGE0,
        TCFG_ADKEY_VOLTAGE1,
        TCFG_ADKEY_VOLTAGE2,
        TCFG_ADKEY_VOLTAGE3,
        TCFG_ADKEY_VOLTAGE4,
        TCFG_ADKEY_VOLTAGE5,
        TCFG_ADKEY_VOLTAGE6,
        TCFG_ADKEY_VOLTAGE7,
        TCFG_ADKEY_VOLTAGE8,
        TCFG_ADKEY_VOLTAGE9,
    },
    .key_value = {                                             //AD按键各个按键的键值
        TCFG_ADKEY_VALUE0,
        TCFG_ADKEY_VALUE1,
        TCFG_ADKEY_VALUE2,
        TCFG_ADKEY_VALUE3,
        TCFG_ADKEY_VALUE4,
        TCFG_ADKEY_VALUE5,
        TCFG_ADKEY_VALUE6,
        TCFG_ADKEY_VALUE7,
        TCFG_ADKEY_VALUE8,
        TCFG_ADKEY_VALUE9,
    },
};
#endif


/************************** IR KEY ****************************/
#if TCFG_IRKEY_ENABLE
const struct irkey_platform_data irkey_data = {
	    .enable = TCFG_IRKEY_ENABLE,                              //IR按键使能
	    .port = TCFG_IRKEY_PORT,                                       //IR按键口
};
#endif


/************************** TOUCH_KEY ****************************/
#if TCFG_TOUCH_KEY_ENABLE
const struct touch_key_port touch_key_list[] = {
	{
		.port 		= TCFG_TOUCH_KEY0_PORT,
		.key_value 	= TCFG_TOUCH_KEY0_VALUE, 	 	//该值由时钟配置和硬件结构决定, 需要调试
	},

	{
		.port 		= TCFG_TOUCH_KEY1_PORT,
		.key_value 	= TCFG_TOUCH_KEY1_VALUE, 	 	//该值由时钟配置和硬件结构决定, 需要调试
	},
};

const struct touch_key_platform_data touch_key_data = {
	.num = ARRAY_SIZE(touch_key_list),
	.clock = TCFG_TOUCH_KEY_CLK,
	.change_gain 	= TCFG_TOUCH_KEY_CHANGE_GAIN,
	.press_cfg		= TCFG_TOUCH_KEY_PRESS_CFG,
	.release_cfg0 	= TCFG_TOUCH_KEY_RELEASE_CFG0,
	.release_cfg1 	= TCFG_TOUCH_KEY_RELEASE_CFG1,
	.port_list = touch_key_list,
};
#endif  /* #if TCFG_TOUCH_KEY_ENABLE */

/**************************CTMU_TOUCH_KEY ****************************/
#if TCFG_CTMU_TOUCH_KEY_ENABLE
#include "asm/ctmu.h"
static const struct ctmu_key_port touch_ctmu_port[] = {
    {
        .port = TCFG_CTMU_TOUCH_KEY0_PORT,
        .key_value = TCFG_CTMU_TOUCH_KEY0_VALUE,
    },
    {
		.port = TCFG_CTMU_TOUCH_KEY1_PORT,
        .key_value = TCFG_CTMU_TOUCH_KEY1_VALUE,
    },
};

const struct ctmu_touch_key_platform_data ctmu_touch_key_data = {
    .num = ARRAY_SIZE(touch_ctmu_port),
    .port_list = touch_ctmu_port,
};
#endif  /* #if TCFG_CTMU_TOUCH_KEY_ENABLE */

/************************** linein KEY ****************************/
#if TCFG_LINEIN_ENABLE
struct linein_dev_data linein_data = {
	.enable = TCFG_LINEIN_ENABLE,
	.port = TCFG_LINEIN_CHECK_PORT,
	.up = TCFG_LINEIN_PORT_UP_ENABLE,
	.down = TCFG_LINEIN_PORT_DOWN_ENABLE,
	.ad_channel = TCFG_LINEIN_AD_CHANNEL,
	.ad_vol = TCFG_LINEIN_VOLTAGE,
};
#endif

/************************** RDEC_KEY ****************************/
#if TCFG_RDEC_KEY_ENABLE
const struct rdec_device rdeckey_list[] = {
	   {
			   .index = RDEC0 ,
			   .sin_port0 = TCFG_RDEC0_ECODE1_PORT,
			   .sin_port1 = TCFG_RDEC0_ECODE2_PORT,
			   .key_value0 = TCFG_RDEC0_KEY0_VALUE | BIT(7),
			   .key_value1 = TCFG_RDEC0_KEY1_VALUE | BIT(7),
	   },

	   {
			   .index = RDEC1 ,
			   .sin_port0 = TCFG_RDEC1_ECODE1_PORT,
			   .sin_port1 = TCFG_RDEC1_ECODE2_PORT,
			   .key_value0 = TCFG_RDEC1_KEY0_VALUE | BIT(7),
			   .key_value1 = TCFG_RDEC1_KEY1_VALUE | BIT(7),
	   },

	   {
			   .index = RDEC2 ,
			   .sin_port0 = TCFG_RDEC2_ECODE1_PORT,
			   .sin_port1 = TCFG_RDEC2_ECODE2_PORT,
			   .key_value0 = TCFG_RDEC2_KEY0_VALUE | BIT(7),
			   .key_value1 = TCFG_RDEC2_KEY1_VALUE | BIT(7),
	   },


};
const struct rdec_platform_data  rdec_key_data = {
       .enable = 1, //TCFG_RDEC_KEY_ENABLE,                              //是否使能RDEC按键
       .num = ARRAY_SIZE(rdeckey_list),                            //RDEC按键的个数
       .rdec = rdeckey_list,                                       //RDEC按键参数表
};
#endif

/************************** otg data****************************/
#if TCFG_OTG_MODE
struct otg_dev_data otg_data = {
    .usb_dev_en = TCFG_OTG_USB_DEV_EN,
	.slave_online_cnt = TCFG_OTG_SLAVE_ONLINE_CNT,
	.slave_offline_cnt = TCFG_OTG_SLAVE_OFFLINE_CNT,
	.host_online_cnt = TCFG_OTG_HOST_ONLINE_CNT,
	.host_offline_cnt = TCFG_OTG_HOST_OFFLINE_CNT,
	.detect_mode = TCFG_OTG_MODE,
	.detect_time_interval = TCFG_OTG_DET_INTERVAL,
};
#endif


/************************** rtc ****************************/

#if TCFG_RTC_ENABLE
const struct rtc_dev_data rtc_data = {
    .port = (u8)-1,
    .edge = 0,    //0 leading edge, 1 falling edge
    .port_en = false,
    .rtc_ldo = false,//使用内部ldo 供电
    .clk_res = RTC_CLK_RES_SEL,
};
#endif

/************************** PWM_LED ****************************/
#if TCFG_PWMLED_ENABLE
LED_PLATFORM_DATA_BEGIN(pwm_led_data)
	.io_mode = TCFG_PWMLED_IOMODE,              //推灯模式设置:支持单个IO推两个灯和两个IO推两个灯
	.io_cfg.one_io.pin = TCFG_PWMLED_PIN,       //单个IO推两个灯的IO口配置
LED_PLATFORM_DATA_END()
#endif


#if TCFG_UI_LED7_ENABLE
LED7_PLATFORM_DATA_BEGIN(led7_data)
	.pin_type = LED7_PIN7,
    .pin_cfg.pin7.pin[0] = IO_PORTC_01,
    .pin_cfg.pin7.pin[1] = IO_PORTC_02,
    .pin_cfg.pin7.pin[2] = IO_PORTC_03,
    .pin_cfg.pin7.pin[3] = IO_PORTC_04,
    .pin_cfg.pin7.pin[4] = IO_PORTC_05,
    .pin_cfg.pin7.pin[5] = IO_PORTB_06,
    .pin_cfg.pin7.pin[6] = IO_PORTB_07,
LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LED_7,
	.private_data = (void *)&led7_data,
};
#endif /* #if TCFG_UI_LED7_ENABLE */

#if TCFG_UI_LED1888_ENABLE
LED7_PLATFORM_DATA_BEGIN(led7_data)
	.pin_type = LED7_PIN7,
	.pin_cfg.pin7.pin[0] = IO_PORTB_00,
	.pin_cfg.pin7.pin[1] = IO_PORTB_01,
	.pin_cfg.pin7.pin[2] = IO_PORTB_02,
	.pin_cfg.pin7.pin[3] = IO_PORTB_03,
	.pin_cfg.pin7.pin[4] = IO_PORTB_04,
	.pin_cfg.pin7.pin[5] = IO_PORTB_05,
	.pin_cfg.pin7.pin[6] = (u8)-1,//IO_PORTB_06,
LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LED_7,
	.private_data = (void *)&led7_data,
};
#endif /* #if TCFG_UI_LED7_ENABLE */


#if TCFG_UI_LCD_SEG3X9_ENABLE
LCD_SEG3X9_PLATFORM_DATA_BEGIN(lcd_seg3x9_data)
	.vlcd = LCD_SEG3X9_VOLTAGE_3_3V,
	.bias = LCD_SEG3X9_BIAS_1_3,
	.pin_cfg.pin_com[0] = IO_PORTC_05,
	.pin_cfg.pin_com[1] = IO_PORTC_04,
	.pin_cfg.pin_com[2] = IO_PORTC_03,

	.pin_cfg.pin_seg[0] = IO_PORTA_00,
	.pin_cfg.pin_seg[1] = IO_PORTA_01,
	.pin_cfg.pin_seg[2] = IO_PORTA_02,
	.pin_cfg.pin_seg[3] = IO_PORTA_03,
	.pin_cfg.pin_seg[4] = IO_PORTA_04,
	.pin_cfg.pin_seg[5] = IO_PORTA_07,
	.pin_cfg.pin_seg[6] = IO_PORTA_12,
	.pin_cfg.pin_seg[7] = IO_PORTA_10,
	.pin_cfg.pin_seg[8] = IO_PORTA_09,
LCD_SEG3X9_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LCD_SEG3X9,
	.private_data = (void *)&lcd_seg3x9_data,
};

#endif /* #if TCFG_UI_LCD_SEG3X9_ENABLE */

const struct soft_iic_config soft_iic_cfg[] = {
    //iic0 data
    {
        .scl = TCFG_SW_I2C0_CLK_PORT,                   //IIC CLK脚
        .sda = TCFG_SW_I2C0_DAT_PORT,                   //IIC DAT脚
        .delay = TCFG_SW_I2C0_DELAY_CNT,                //软件IIC延时参数，影响通讯时钟频率
        .io_pu = 1,                                     //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
#if 0
    //iic1 data
    {
        .scl = IO_PORTA_05,
        .sda = IO_PORTA_06,
        .delay = 50,
        .io_pu = 1,
    },
#endif
};

#if 0

const struct hw_iic_config hw_iic_cfg[] = {
    //iic0 data
    {
        /*硬件IIC端口下选择
 			    SCL         SDA
		  	{IO_PORT_DP,  IO_PORT_DM},    //group a
        	{IO_PORTC_04, IO_PORTC_05},  //group b
        	{IO_PORTC_02, IO_PORTC_03},  //group c
        	{IO_PORTA_05, IO_PORTA_06},  //group d
         */
        .port = TCFG_HW_I2C0_PORTS,
        .baudrate = TCFG_HW_I2C0_CLK,      //IIC通讯波特率
        .hdrive = 0,                       //是否打开IO口强驱
        .io_filter = 1,                    //是否打开滤波器（去纹波）
        .io_pu = 1,                        //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
};
#endif


#if	TCFG_HW_SPI1_ENABLE
const struct spi_platform_data spi1_p_data = {
    .port = {
        IO_PORTC_03,    //CLK
        IO_PORTC_04,    //DO
        IO_PORTC_02,    //DI
    },
    .mode = SPI_MODE_BIDIR_1BIT,
    .clk = 1000000,
    .role = SPI_ROLE_MASTER,
};
#endif

#if	TCFG_HW_SPI2_ENABLE
const struct spi_platform_data spi2_p_data = {
	.port = TCFG_HW_SPI2_PORT,
	.mode = TCFG_HW_SPI2_MODE,
	.clk  = TCFG_HW_SPI2_BAUD,
	.role = TCFG_HW_SPI2_ROLE,
};
#endif

#if TCFG_NOR_FAT
NORFLASH_DEV_PLATFORM_DATA_BEGIN(norflash_fat_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
    .start_addr     = 0,
    .size           = 1*1024*1024,
NORFLASH_DEV_PLATFORM_DATA_END()
#endif

#if TCFG_NOR_FS
NORFLASH_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
    /* .start_addr     = 1*1024*1024, */
    .start_addr     = 0,
    .size           = 4*1024*1024,
NORFLASH_DEV_PLATFORM_DATA_END()
#endif

#if TCFG_NOR_REC
NORFLASH_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_rec_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
    .start_addr     = 2*1024*1024,
    .size           = 2*1024*1024,
NORFLASH_DEV_PLATFORM_DATA_END()
#endif



#if TCFG_SPI_LCD_ENABLE
LCD_SPI_PLATFORM_DATA_BEGIN(lcd_spi_data)
    .pin_reset	= -1,
    .pin_cs		= IO_PORTA_00,
    .pin_rs		= IO_PORTA_01,
    .pin_bl     = IO_PORTA_03,

#if (TCFG_TFT_LCD_DEV_SPI_HW_NUM == 1)
    .spi_cfg	= SPI1,
    .spi_pdata  = &spi1_p_data,
#elif (TCFG_TFT_LCD_DEV_SPI_HW_NUM == 2)
    .spi_cfg	= SPI2,
    .spi_pdata  = &spi2_p_data,
#endif

LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = TFT_LCD,
	.private_data = (void *)&lcd_spi_data,
};
#endif



#if TCFG_GSENSOR_ENABLE
#if TCFG_DA230_EN

GSENSOR_PLATFORM_DATA_BEGIN(gSensor_data)
    .iic = 0,
    .gSensor_name = "da230",
    .gSensor_int_io = IO_PORTB_08,
GSENSOR_PLATFORM_DATA_END();
#endif      //end if DA230_EN
#endif      //end if CONFIG_GSENSOR_ENABLE

#if TCFG_FM_ENABLE

FM_DEV_PLATFORM_DATA_BEGIN(fm_dev_data)
	.iic_hdl = 0,
	.iic_delay = 50,
FM_DEV_PLATFORM_DATA_END();

#endif

#if TCFG_NANDFLASH_DEV_ENABLE
NANDFLASH_DEV_PLATFORM_DATA_BEGIN(nandflash_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
    .start_addr     = 0,
    .size           = 0,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
NANDFLASH_DEV_PLATFORM_DATA_END()
#endif

extern const struct device_operations mass_storage_ops;
REGISTER_DEVICES(device_table) = {
    /* { "audio", &audio_dev_ops, (void *) &audio_data }, */
#if TCFG_NANDFLASH_DEV_ENABLE
	{"nand_flash",   &nandflash_dev_ops , (void *)&nandflash_dev_data},
#endif
/* #if TCFG_CHARGE_ENABLE */
/*     { "charge", &charge_dev_ops, (void *)&charge_data }, */
/* #endif */

#if TCFG_VIRFAT_FLASH_ENABLE
	{ "virfat_flash", 	&virfat_flash_dev_ops, 	(void *)"res_nor"},
#endif

#if TCFG_SD0_ENABLE
	{ "sd0", 	&sd_dev_ops, 	(void *) &sd0_data},
#endif

#if TCFG_SD1_ENABLE
	{ "sd1", 	&sd_dev_ops, 	(void *) &sd1_data},
#endif

#if TCFG_LINEIN_ENABLE
    { "linein",  &linein_dev_ops, (void *)&linein_data},
#endif
#if TCFG_OTG_MODE
    { "otg",     &usb_dev_ops, (void *) &otg_data},
#endif
#if TCFG_UDISK_ENABLE
    { "udisk0",   &mass_storage_ops , NULL},
#endif
#if TCFG_RTC_ENABLE
#if TCFG_USE_VIRTUAL_RTC
    { "rtc",   &rtc_simulate_ops , (void *)&rtc_data},
#else
    { "rtc",   &rtc_dev_ops , (void *)&rtc_data},
#endif
#endif

#if TCFG_NORFLASH_DEV_ENABLE
#if TCFG_NOR_FAT
    { "fat_nor",   &norflash_dev_ops , (void *)&norflash_fat_dev_data},
#endif

#if TCFG_NOR_FS
    { "res_nor",   &norfs_dev_ops , (void *)&norflash_norfs_dev_data},
#endif

#if TCFG_NOR_REC
    {"rec_nor",   &norfs_dev_ops , (void *)&norflash_norfs_rec_dev_data},
#endif
#endif
};

/************************** LOW POWER config ****************************/
const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //0：sniff时芯片不进入低功耗  1：sniff时芯片进入powerdown
    .btosc_hz         = TCFG_CLOCK_OSC_HZ,                   //外接晶振频率
    .delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,        //提供给低功耗模块的延时(不需要需修改)
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.osc_type       = OSC_TYPE_BT_OSC ,
#if TCFG_USE_VIRTUAL_RTC
	.virtual_rtc  = 1,
    .virtual_rtc_interval = 500,                           //ms,虚拟时钟起中断间隔
#endif
};



/************************** PWR config ****************************/
struct port_wakeup port0 = {
    .pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
	.both_edge  = 0,
	.filter     = PORT_FLT_8ms,
    /* .attribute  = BLUETOOTH_RESUME,                        //保留参数 */
    .iomap      = IO_PORTB_01,                             //唤醒口选择
};
#if ADAPTER_UART_DEMO	//设置串口通信脚为唤醒脚
struct port_wakeup port3 = {
    .pullup_down_enable = DISABLE,                            //配置I/O 内部上下拉是否使能
    .edge       = RISING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
	.both_edge  = 0,
	.filter     = PORT_FLT_8ms,
    /* .attribute  = BLUETOOTH_RESUME,                        //保留参数 */
    .iomap      = ADAPTER_UART_TX_PIN,                             //唤醒口选择
};
#endif
#if TCFG_CHARGE_ENABLE
struct port_wakeup charge_port = {
    .edge               = RISING_EDGE,                       //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 0,
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_CHGFL_DET,                      //唤醒口选择
};

struct port_wakeup vbat_port = {
    .edge               = BOTH_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 1,
    .filter             = PORT_FLT_4ms,
    .iomap              = IO_VBTCH_DET,                      //唤醒口选择
};

struct port_wakeup ldoin_rise_port = {
    .edge               = BOTH_EDGE,                       //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 1,
    .filter             = PORT_FLT_4ms,
    .iomap              = IO_LDOIN_DET,                      //唤醒口选择
};
#endif

const struct wakeup_param wk_param = {
    .port[1] = &port0,
#if ADAPTER_UART_DEMO
	.port[3] = &port3,
#endif
#if TCFG_CHARGE_ENABLE
    .aport[0] = &charge_port,
    .aport[1] = &vbat_port,
    .aport[2] = &ldoin_rise_port,
#endif

};

void gSensor_wkupup_disable(void)
{
    log_info("gSensor wkup disable\n");
    power_wakeup_index_disable(1);
}

void gSensor_wkupup_enable(void)
{
    log_info("gSensor wkup enable\n");
    power_wakeup_index_enable(1);
}
static void key_wakeup_disable()
{
    power_wakeup_index_disable(1);
}
static void key_wakeup_enable()
{
    power_wakeup_index_enable(1);
}

void debug_uart_init(const struct uart_platform_data *data)
{
#if TCFG_UART0_ENABLE
    if (data) {
        uart_init(data);
    } else {
        uart_init(&uart0_data);
    }
#endif
}


STATUS *get_led_config(void)
{
    return &(__this->led);
}

STATUS *get_tone_config(void)
{
    return &(__this->tone);
}

u8 get_sys_default_vol(void)
{
    return 21;
}

u8 get_power_on_status(void)
{
#if ADAPTER_UART_DEMO
	static u8 init_one = 1;
	if (init_one) {
	    init_one = 0;
	    gpio_set_pull_down(ADAPTER_UART_TX_PIN,0);
	    gpio_set_pull_up(ADAPTER_UART_TX_PIN,0);
	    gpio_set_die(ADAPTER_UART_TX_PIN,1);
	    gpio_set_direction(ADAPTER_UART_TX_PIN,1);
	    os_time_dly(5);
	}
	if (gpio_read(ADAPTER_UART_TX_PIN) == 1) {
		return 1;
	}
#endif
#if TCFG_IOKEY_ENABLE
	struct iokey_port *power_io_list = NULL;
	power_io_list = iokey_data.port;

	if (iokey_data.enable) {
		if (gpio_read(power_io_list->key_type.one_io.port) == power_io_list->connect_way){
			return 1;
		}
	}
#endif

#if TCFG_ADKEY_ENABLE
	if (adkey_data.enable) {
		if (adc_get_value(adkey_data.ad_channel) < 10) {
			return 1;
		}
	}
#endif
	/* if(gpio_read(port0.iomap) == 0){ */
		/* return 1; */
	/* } */

    return 0;
}


static void board_devices_init(void)
{
#if TCFG_PWMLED_ENABLE
	pwm_led_init(&pwm_led_data);
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE || TCFG_IRKEY_ENABLE || TCFG_RDEC_KEY_ENABLE ||  TCFG_CTMU_TOUCH_KEY_ENABLE)

	key_driver_init();
#endif

#if TCFG_GSENSOR_ENABLE
    gravity_sensor_init(&gSensor_data);
#endif      //end if CONFIG_GSENSOR_ENABLE

#if TCFG_UI_ENABLE
	UI_INIT(&ui_cfg_data);
#endif /* #if TCFG_UI_ENABLE */

#if  TCFG_APP_FM_EMITTER_EN
 	fm_emitter_manage_init(930);
#endif
#if (!TCFG_CHARGE_ENABLE)
    CHARGE_EN(0);
    CHGGO_EN(0);
#endif

}

void uartSendInit();
extern void alarm_init();
extern void cfg_file_parse(u8 idx);
void board_init()
{
    board_power_init();
    adc_vbg_init();
    adc_init();
    cfg_file_parse(0);

#if TCFG_CHARGE_ENABLE
	extern int charge_init(const struct dev_node *node, void *arg);
	charge_init(NULL, (void *)&charge_data);
#endif

    if (!get_charge_online_flag()) {
        check_power_on_voltage();
    }

/* #if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE) */
	/* sdpg_config(1); */
/* #endif */

#if TCFG_FM_ENABLE
	fm_dev_init(&fm_dev_data);
#endif

#if TCFG_NOR_REC
    nor_fs_ops_init();
#endif

#if TCFG_NOR_FS
    init_norsdfile_hdl();
#endif

#if FLASH_INSIDE_REC_ENABLE
    sdfile_rec_ops_init();
#endif

	dev_manager_init();
	board_devices_init();

	if (get_charge_online_flag()) {
    	power_set_mode(PWR_LDO15);
	} else {
    	power_set_mode(TCFG_LOWPOWER_POWER_SEL);
	}

    //针对硅mic要输出1给mic供电(按实际使用情况配置)
    /* if(!adc_data.mic_capless){ */
        /* gpio_set_pull_up(IO_PORTA_04, 0); */
        /* gpio_set_pull_down(IO_PORTA_04, 0); */
        /* gpio_set_direction(IO_PORTA_04, 0); */
        /* gpio_set_output_value(IO_PORTA_04,1); */
    /* } */

 #if TCFG_UART0_ENABLE
    if (uart0_data.rx_pin < IO_MAX_NUM) {
        gpio_set_die(uart0_data.rx_pin, 1);
    }
#endif

#ifdef AUDIO_PCM_DEBUG
	uartSendInit();
#endif

#if TCFG_RTC_ENABLE
    alarm_init();
#endif
#if TCFG_RF_TEST_EN
	extern void rf_test_process();
	rf_test_process();
#endif

}
void adapter_audio_mic_ldo_en(u8 en)
{
#if (TCFG_AUDIO_MIC_MODE == AUDIO_MIC_CAP_DIFF_MODE)
	audio_adc_mic_ldo_en(MIC_LDO_BIAS2,en, &adc_data);//差分麦默认使用mic_bias2(PG7)供电
#endif
}
//maskrom 使用到的io
static void mask_io_cfg()
{
	struct boot_soft_flag_t boot_soft_flag = {0};
	boot_soft_flag.flag0.boot_ctrl.wdt_dis = 0;
	boot_soft_flag.flag0.boot_ctrl.poweroff = 0;
	boot_soft_flag.flag0.boot_ctrl.is_port_b = JL_IOMAP->CON0 & BIT(16) ? 1 : 0;

	boot_soft_flag.flag1.misc.usbdm = SOFTFLAG_HIGH_RESISTANCE;
	boot_soft_flag.flag1.misc.usbdp = SOFTFLAG_HIGH_RESISTANCE;

	boot_soft_flag.flag1.misc.uart_key_port = 0;
	boot_soft_flag.flag1.misc.ldoin = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag2.pg2_pg3.pg2 = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag2.pg2_pg3.pg3 = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag3.pg4_res.pg4 = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag4.fast_boot_ctrl.fast_boot = 1;
    boot_soft_flag.flag4.fast_boot_ctrl.flash_stable_delay_sel = 0;

    mask_softflag_config(&boot_soft_flag);

}
/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
extern void dac_power_off(void);
void board_set_soft_poweroff(void)
{
#if ADAPTER_UART_DEMO
	adapter_uart_demo_close();
#endif
	u16 port_group[] = {
        [PORTA_GROUP] = 0xffff,
        [PORTB_GROUP] = 0xffff,
        [PORTC_GROUP] = 0xffff,
       	[PORTD_GROUP] = 0xffff,
		[PORTE_GROUP] = 0xffff,
        [PORTG_GROUP] = 0xffff,
		[PORTP_GROUP] = 0x1,//
    };

	//flash电源
	if(get_sfc_port()==0){
		port_protect(port_group, SPI0_PWR_A);
		port_protect(port_group, SPI0_CS_A);
		port_protect(port_group, SPI0_CLK_A);
		port_protect(port_group, SPI0_DO_D0_A);
		port_protect(port_group, SPI0_DI_D1_A);
		if(get_sfc_bit_mode()==4){
			port_protect(port_group, SPI0_WP_D2_A);
			port_protect(port_group, SPI0_HOLD_D3_A);
		}
	}else{
		port_protect(port_group, SPI0_PWR_B);
		port_protect(port_group, SPI0_CS_B);
		port_protect(port_group, SPI0_CLK_B);
		port_protect(port_group, SPI0_DO_D0_B);
		port_protect(port_group, SPI0_DI_D1_B);
		if(get_sfc_bit_mode()==4){
			port_protect(port_group, SPI0_WP_D2_B);
			port_protect(port_group, SPI0_HOLD_D3_B);
		}
	}

	//power按键
#if TCFG_IOKEY_ENABLE
    port_protect(port_group, TCFG_IOKEY_POWER_ONE_PORT);
#endif

#if ADAPTER_UART_DEMO
	port_protect(port_group, ADAPTER_UART_TX_PIN);
#endif
#if (!(TCFG_LP_TOUCH_KEY_ENABLE && TCFG_LP_TOUCH_KEY1_EN))
    //默认唤醒io
    port_protect(port_group, IO_PORTB_01);
#endif


#if TCFG_UMIDIGI_BOX_ENABLE
	power_wakeup_index_disable(PORT_WKUP_F_INDEX);
#elif (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
	power_wakeup_index_disable(2);
#endif

	mask_io_cfg();

	gpio_dir(GPIOA,    0, 16,  port_group[PORTA_GROUP], GPIO_OR);
    gpio_set_pu(GPIOA, 0, 16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_set_pd(GPIOA, 0, 16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_die(GPIOA,    0, 16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_dieh(GPIOA,   0, 16, ~port_group[PORTA_GROUP], GPIO_AND);

	gpio_dir(GPIOB,    0, 16,  port_group[PORTB_GROUP], GPIO_OR);
    gpio_set_pu(GPIOB, 0, 16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_set_pd(GPIOB, 0, 16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_die(GPIOB,    0, 16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_dieh(GPIOB,   0, 16, ~port_group[PORTB_GROUP], GPIO_AND);

	gpio_dir(GPIOC,    0, 16,  port_group[PORTC_GROUP], GPIO_OR);
    gpio_set_pu(GPIOC, 0, 16, ~port_group[PORTC_GROUP], GPIO_AND);
    gpio_set_pd(GPIOC, 0, 16, ~port_group[PORTC_GROUP], GPIO_AND);
    gpio_die(GPIOC,    0, 16, ~port_group[PORTC_GROUP], GPIO_AND);
    gpio_dieh(GPIOC,   0, 16, ~port_group[PORTC_GROUP], GPIO_AND);

	gpio_dir(GPIOD,    0, 16,  port_group[PORTD_GROUP], GPIO_OR);
    gpio_set_pu(GPIOD, 0, 16, ~port_group[PORTD_GROUP], GPIO_AND);
    gpio_set_pd(GPIOD, 0, 16, ~port_group[PORTD_GROUP], GPIO_AND);
    gpio_die(GPIOD,    0, 16, ~port_group[PORTD_GROUP], GPIO_AND);
    gpio_dieh(GPIOD,   0, 16, ~port_group[PORTD_GROUP], GPIO_AND);

	gpio_dir(GPIOE,    0, 16,  port_group[PORTE_GROUP], GPIO_OR);
    gpio_set_pu(GPIOE, 0, 16, ~port_group[PORTE_GROUP], GPIO_AND);
    gpio_set_pd(GPIOE, 0, 16, ~port_group[PORTE_GROUP], GPIO_AND);
    gpio_die(GPIOE,    0, 16, ~port_group[PORTE_GROUP], GPIO_AND);
    gpio_dieh(GPIOE,   0, 16, ~port_group[PORTE_GROUP], GPIO_AND);

	gpio_dir(GPIOG,    0, 16,  port_group[PORTG_GROUP], GPIO_OR);
    gpio_set_pu(GPIOG, 0, 16, ~port_group[PORTG_GROUP], GPIO_AND);
    gpio_set_pd(GPIOG, 0, 16, ~port_group[PORTG_GROUP], GPIO_AND);
    gpio_die(GPIOG,    0, 16, ~port_group[PORTG_GROUP], GPIO_AND);
    gpio_dieh(GPIOG,   0, 16, ~port_group[PORTG_GROUP], GPIO_AND);

	gpio_dir(GPIOP,    0, 1,  port_group[PORTP_GROUP],  GPIO_OR);
    gpio_set_pu(GPIOP, 0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);
    gpio_set_pd(GPIOP, 0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);
    gpio_die(GPIOP,    0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);
    gpio_dieh(GPIOP,   0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);

    gpio_set_pull_up(IO_PORT_DP, 0);
    gpio_set_pull_down(IO_PORT_DP, 0);
    gpio_set_direction(IO_PORT_DP, 1);
    gpio_set_die(IO_PORT_DP, 0);
    gpio_set_dieh(IO_PORT_DP, 0);

    gpio_set_pull_up(IO_PORT_DM, 0);
    gpio_set_pull_down(IO_PORT_DM, 0);
    gpio_set_direction(IO_PORT_DM, 1);
    gpio_set_die(IO_PORT_DM, 0);
    gpio_set_dieh(IO_PORT_DM, 0);

    dac_power_off();
}

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

void sleep_exit_callback(u32 usec)
{
    /* putchar('>'); */
    APP_IO_DEBUG_1(A, 6);
    key_wakeup_disable();
}

void sleep_enter_callback(u8  step)
{
    /* 此函数禁止添加打印 */
    if (step == 1) {
        putchar('<');
        APP_IO_DEBUG_0(A, 6);
        /* dac_power_off(); */
    } else {
        usb_iomode(1);

        gpio_set_pull_up(IO_PORT_DP, 0);
        gpio_set_pull_down(IO_PORT_DP, 0);
        gpio_set_direction(IO_PORT_DP, 1);
        gpio_set_die(IO_PORT_DP, 0);
        gpio_set_dieh(IO_PORT_DP, 0);

        gpio_set_pull_up(IO_PORT_DM, 0);
        gpio_set_pull_down(IO_PORT_DM, 0);
        gpio_set_direction(IO_PORT_DM, 1);
        gpio_set_die(IO_PORT_DM, 0);
        gpio_set_dieh(IO_PORT_DM, 0);
    }
}

static void wl_audio_clk_on(void)
{
    JL_WL_AUD->CON0 = 1;
}

static void aport_wakeup_callback(u8 index, u8 gpio, u8 edge)
{
	/* log_info("%s:%d,%d",__FUNCTION__,index,gpio); */
#if TCFG_CHARGE_ENABLE
	switch (gpio) {
		case IO_CHGFL_DET://charge port
			charge_wakeup_isr();
			break;
		case IO_VBTCH_DET://vbat port
		case IO_LDOIN_DET://ldoin port
			ldoin_wakeup_isr();
			break;
	}
#endif
}

void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);

    power_init(&power_param);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

    power_keep_dacvdd_en(0);

    wl_audio_clk_on();
	power_wakeup_init(&wk_param);

    aport_edge_wkup_set_callback(aport_wakeup_callback);
#if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE)
    //charge_check_and_set_pinr(0);
#endif

#if USER_UART_UPDATE_ENABLE
        {
#include "uart_update.h"
            uart_update_cfg  update_cfg = {
                .rx = IO_PORTA_02,
                .tx = IO_PORTA_03,
                .output_channel = CH1_UT1_TX,
                .input_channel = INPUT_CH0
            };
            uart_update_init(&update_cfg);
        }
#endif
}
#if 0
static void board_power_wakeup_init(void)
{
    power_wakeup_init(&wk_param);
    key_wakeup_disable();
#if TCFG_POWER_ON_NEED_KEY
    extern u8 power_reset_src;
    if ((power_reset_src & BIT(0)) || (power_reset_src & BIT(1))) {
#if TCFG_CHARGE_ENABLE
        log_info("is ldo5v wakeup:%d\n",is_ldo5v_wakeup());
        if (is_ldo5v_wakeup()) {
            return;
        }
        if (get_ldo5v_online_hw()) {
            return;
        }
        /*LDO5V,检测上升沿，用于检测ldoin插入*/
        LDO5V_EN(1);
        LDO5V_EDGE_SEL(0);
        LDO5V_PND_CLR();
        LDO5V_EDGE_WKUP_EN(1);
#endif
        power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);
        power_set_soft_poweroff();
    }
#endif
}
early_initcall(board_power_wakeup_init);
#endif
#endif

