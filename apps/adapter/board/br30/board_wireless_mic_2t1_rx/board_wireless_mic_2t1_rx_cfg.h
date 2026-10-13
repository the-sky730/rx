#ifndef CONFIG_BOARD_WIRELESS_MIC_2T1_RX_CFG_H
#define CONFIG_BOARD_WIRELESS_MIC_2T1_RX_CFG_H

#ifdef CONFIG_BOARD_WIRELESS_MIC_2T1_RX

#include "board_wireless_mic_2t1_rx_global_build_cfg.h"

//*********************************************************************************//
//                                 无线麦配置                                      //
//*********************************************************************************//
//两发一收无线mic方案，发射端配置为从机，接收端配置为主机
#define WIRELESS_ROLE_SEL				APP_WIRELESS_MASTER//角色选择

//适用于话筒音箱这种与经典蓝牙靠近并且同时使用的方案，彼此干扰小,距离远。不是和经典蓝牙靠近并且同时使用的方案，不建议打开
#define WIRELESS_2T1_MICROPHONE_MODE	ENABLE

//配置输出到usb会默认同时输出到dac，配置输出到dac不会输出到usb
#define WIRELESS_MIC_RX_OUTPUT_SEL		WIRELESS_MIC_RX_OUTPUT_DAC// 从机输出类型选择

#define WIRELESS_24G_ENABLE				ENABLE //使能此功能可以屏蔽手机搜索到此无线设备名
#define WIRELESS_NOISEGATE_EN		    DISABLE	//噪声门限使能
//低延时模式,比非低延时模式少3ms，距离会变短，用户可根据产品应用场景选择,txrx需要保持一致
#define WIRELESS_LOW_LATENCY			DISABLE
//编解码采样率如果有修改，请对应修改另一方的编解码采样率
#define WIRELESS_CODING_SAMPLERATE		(48000)

#if WIRELESS_2T1_MICROPHONE_MODE
#define WIRELESS_CODING_FRAME_LEN		100
#define TCFG_RF_OOB		1
#else
#if WIRELESS_LOW_LATENCY
#define WIRELESS_CODING_FRAME_LEN		25
#else
#define WIRELESS_CODING_FRAME_LEN		50
#endif
#endif

#define WIRELESS_DECODE_SAMPLERATE		WIRELESS_CODING_SAMPLERATE


//两发一收支持两个tx分别作为左右声道，需要RXTX都开这个功能
#define WIRELESS_TX_MIC_STEREO_OUTPUT		DISABLE
#if (WIRELESS_TX_MIC_STEREO_OUTPUT)
//两发一收立体声自适应，TX不需要电阻区分左右声道，默认第一个连接上为左声道，第二个为右声道
#define WIRELESS_TX_MIC_STEREO_OUTPUT_AUTO		DISABLE
#define WIRELESS_STEREO_OUTPUT_DIFF             ENABLE
#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
#define MIC_CHANNEL							2
#endif
#endif


#define	WIRELESS_TOOL_BLE_NAME_EN		ENABLE
//rf测试，由工具触发样机进入dut模式
#define TCFG_RF_TEST_EN					DISABLE
//产线近距离快速配对测试功能
//如果配对名以特殊字符串开头，程序会判断信号强度来进行连接,即靠得近的才可以连上
#define SPECIFIC_STRING					"#@#@#@"//用户可自定义修改
#define TCFG_WIRELESS_RSSI				50//demo板测试50大概对应1米的距离,越大越远

//配对绑定
#define WIRELESS_PAIR_BONDING			DISABLE

//使用PA延长距离,需要硬件添加PA电路,默认使用PC2/PC3
#define CONFIG_BT_RF_USING_EXTERNAL_PA_EN	DISABLE
//2t1只连接1个tx，即1t1,不需要使用JL697M/JL701M系列芯片
#define WIRELESS_2t1_RX_ONLY_CONN_1TX		DISABLE

//无线话筒和音箱单线串口通信使能,同时支持音箱芯片唤醒和关机无线话筒接收芯片功能
#define ADAPTER_UART_DEMO		0
#define ADAPTER_UART_TX_PIN   							IO_PORTB_02
#define ADAPTER_UART_RX_PIN  							IO_PORTB_02


#define WIRELESS_MIC_DAC_ONLINE_CONFIG		0
//*******************************************************
//*********************************************************************************//
//                                  app 配置                                       //
//*********************************************************************************//
#define TCFG_APP_BT_EN			            1
#define TCFG_APP_MUSIC_EN			        0
#define TCFG_APP_LINEIN_EN					0
#define TCFG_APP_FM_EN					    0
#define TCFG_APP_PC_EN					    0
#define TCFG_APP_RTC_EN					    0
#define TCFG_APP_RECORD_EN				    0
#define TCFG_APP_SPDIF_EN                   0
//*********************************************************************************//
//                                 UART配置                                        //
//*********************************************************************************//
#define TCFG_UART0_ENABLE					ENABLE_THIS_MOUDLE                     //串口打印模块使能
#define TCFG_UART0_RX_PORT					NO_CONFIG_PORT                         //串口接收脚配置（用于打印可以选择NO_CONFIG_PORT）
#define TCFG_UART0_TX_PORT  				IO_PORT_DP//IO_PORTC_05                            //串口发送脚配置
#define TCFG_UART0_BAUDRATE  				1000000                                //串口波特率配置

//*********************************************************************************//
//                                 USB 配置                                        //
//*********************************************************************************//
#if WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC
#define TCFG_PC_ENABLE                      1
#endif
#define USB_MALLOC_ENABLE                   1
#define USB_PC_NO_APP_MODE                  1
#define USB_MEM_NO_USE_OVERLAY_EN		    1
//部分手机APP录音需要同时开SPEAKER_CLASS,才可以使用USB_MIC录音,能提高兼容性,修改为MIC_CLASS|SPEAKER_CLASS
//不过开了SPEAKER_CLASS,手机播放音频，就无法外放，需要用户自己根据产品使用场景进行修改
#define USB_DEVICE_CLASS_CONFIG             MIC_CLASS//|SPEAKER_CLASS
#if (USB_DEVICE_CLASS_CONFIG & SPEAKER_CLASS)
#define TCFG_TYPEC_EARPHONE_TEST_FILTER				1
#endif


//*********************************************************************************//
//                                 key 配置                                        //
//*********************************************************************************//
//#define KEY_NUM_MAX                        	10
//#define KEY_NUM                            	3
#define KEY_IO_NUM_MAX						6
#define KEY_AD_NUM_MAX						10
#define KEY_IR_NUM_MAX						21
#define KEY_TOUCH_NUM_MAX					6
#define KEY_RDEC_NUM_MAX                    6
#define KEY_CTMU_TOUCH_NUM_MAX				6

#define MULT_KEY_ENABLE						DISABLE 		//是否使能组合按键消息, 使能后需要配置组合按键映射表

#define TCFG_KEY_TONE_EN					DISABLE		// 按键提示音。建议音频输出使用固定采样率

//*********************************************************************************//
//                                 iokey 配置                                      //
//*********************************************************************************//
#define TCFG_IOKEY_ENABLE					ENABLE_THIS_MOUDLE //是否使能IO按键

#define TCFG_IOKEY_POWER_CONNECT_WAY		ONE_PORT_TO_LOW    //按键一端接低电平一端接IO

#define TCFG_IOKEY_POWER_ONE_PORT			IO_PORTB_01        //IO按键端口

// #define TCFG_IOKEY_PREV_CONNECT_WAY			ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
// #define TCFG_IOKEY_PREV_ONE_PORT			IO_PORTB_00

// #define TCFG_IOKEY_NEXT_CONNECT_WAY 		ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
// #define TCFG_IOKEY_NEXT_ONE_PORT			IO_PORTB_02
//*********************************************************************************//
//                                  NTC配置                                       //
//*********************************************************************************//
#define NTC_DET_EN  					    DISABLE_THIS_MOUDLE
#define NTC_POWER_IO   						IO_PORTC_04
#define NTC_DETECT_IO   					IO_PORTC_05
#define NTC_DET_AD_CH   					(AD_CH_PC5)   //根据adc_api.h修改通道号

#define NTC_DET_UPPER        				799  //正常范围AD值上限，0度时
#define NTC_DET_LOWER        				297  //正常范围AD值下限，45度时

//*********************************************************************************//
//                                  充电参数配置                                   //
//*********************************************************************************//
#define TCFG_CHARGE_ENABLE					ENABLE_THIS_MOUDLE
#define TCFG_TEST_BOX_ENABLE				ENABLE_THIS_MOUDLE
#define TCFG_CHARGESTORE_PORT				IO_PORTP_00
//是否支持开机充电
#define TCFG_CHARGE_POWERON_ENABLE			DISABLE
//是否支持拔出充电自动开机功能
#define TCFG_CHARGE_OFF_POWERON_NE			DISABLE
#define TCFG_CHARGE_FULL_V					CHARGE_FULL_V_4222
#define TCFG_CHARGE_FULL_MA					CHARGE_FULL_mA_15
#define TCFG_CHARGE_MA						CHARGE_mA_60
//涓流电流
#define TCFG_CHARGE_TRICKLE_MA				CHARGE_mA_20

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15//PWR_DCDC15                   //电源模式设置，可选DCDC和LDO

//*********************************************************************************//
//                                 Audio配置                                       //
//*********************************************************************************//
#define TCFG_AUDIO_ADC_ENABLE				ENABLE_THIS_MOUDLE
/*
 *LADC_CH_MIC_L: MIC0(PA1)
 *LADC_CH_MIC_R: MIC1(PB8)
 */
#define TCFG_AUDIO_ADC_MIC_CHA				LADC_CH_MIC_L
/*MIC LDO电流档位设置：
    0:0.625ua    1:1.25ua    2:1.875ua    3:2.5ua*/
#define TCFG_AUDIO_ADC_LDO_SEL				3

// LADC通道
#define TCFG_AUDIO_ADC_LINE_CHA0			LADC_LINE1_MASK
#define TCFG_AUDIO_ADC_LINE_CHA1			LADC_CH_LINE0_L

#define TCFG_AUDIO_DAC_ENABLE				ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_DAC_LDO_SEL				1
/*
DACVDD电压设置(要根据具体的硬件接法来确定):
    DACVDD_LDO_1_20V        DACVDD_LDO_1_30V        DACVDD_LDO_2_35V        DACVDD_LDO_2_50V
    DACVDD_LDO_2_65V        DACVDD_LDO_2_80V        DACVDD_LDO_2_95V        DACVDD_LDO_3_10V*/
#define TCFG_AUDIO_DAC_LDO_VOLT				DACVDD_LDO_1_25V
/*预留接口，未使用*/
#define TCFG_AUDIO_DAC_PA_PORT				NO_CONFIG_PORT
/*
DAC硬件上的连接方式,可选的配置：
    DAC_OUTPUT_MONO_L               左声道
    DAC_OUTPUT_MONO_R               右声道
    DAC_OUTPUT_LR                   立体声
    DAC_OUTPUT_MONO_LR_DIFF         单声道差分输出
*/
#if WIRELESS_TX_MIC_STEREO_OUTPUT
#define TCFG_AUDIO_DAC_CONNECT_MODE    DAC_OUTPUT_LR
#else
#define TCFG_AUDIO_DAC_CONNECT_MODE    DAC_OUTPUT_MONO_LR_DIFF
#endif
/*
解码后音频的输出方式:
    AUDIO_OUTPUT_ORIG_CH            按原始声道输出
    AUDIO_OUTPUT_STEREO             按立体声
    AUDIO_OUTPUT_L_CH               只输出原始声道的左声道
    AUDIO_OUTPUT_R_CH               只输出原始声道的右声道
    AUDIO_OUTPUT_MONO_LR_CH         输出左右合成的单声道
 */

#define AUDIO_OUTPUT_MODE          AUDIO_OUTPUT_MONO_LR_CH

#define AUDIO_OUTPUT_WAY            AUDIO_OUTPUT_WAY_BT
#define LINEIN_INPUT_WAY            LINEIN_INPUT_WAY_ANALOG

#define AUDIO_OUTPUT_AUTOMUTE       0//ENABLE

#define  DUT_AUDIO_DAC_LDO_VOLT   				DACVDD_LDO_1_25V

//每个解码通道都开启数字音量管理,音量类型为VOL_TYPE_DIGGROUP时要使能
#define SYS_DIGVOL_GROUP_EN     DISABLE

#define SYS_VOL_TYPE            VOL_TYPE_ANALOG


/*
 *通话的时候使用数字音量
 *0：通话使用和SYS_VOL_TYPE一样的音量调节类型
 *1：通话使用数字音量调节，更加平滑
 */
#define TCFG_CALL_USE_DIGITAL_VOLUME		0

// 使能改宏，提示音音量使用music音量
#define APP_AUDIO_STATE_WTONE_BY_MUSIC      (1)
// 0:提示音不使用默认音量； 1:默认提示音音量值
#define TONE_MODE_DEFAULE_VOLUME            (0)

/*
 *支持省电容MIC模块
 *(1)要使能省电容mic,首先要支持该模块:TCFG_SUPPORT_MIC_CAPLESS
 *(2)只有支持该模块，才能使能该模块:TCFG_MIC_CAPLESS_ENABLE
 */
#define TCFG_SUPPORT_MIC_CAPLESS			ENABLE_THIS_MOUDLE
//省电容MIC使能
#define TCFG_MIC_CAPLESS_ENABLE				DISABLE_THIS_MOUDLE


#define WIRELESS_MIC_ADC_RESUME_CODING_EN   ENABLE
#define WIRELESS_MIC_ADC_GAIN				(13)//mic增益设置
#if (WIRELESS_CODING_SAMPLERATE == 44100)
#define WIRELESS_MIC_ADC_POINT_UNIT			(48000*WIRELESS_CODING_FRAME_LEN/10000)
#else
#define WIRELESS_MIC_ADC_POINT_UNIT			(WIRELESS_CODING_SAMPLERATE*WIRELESS_CODING_FRAME_LEN/10000)
#endif


//*********************************************************************************//
//                                  PWM_LED 配置                                       //
//******************************************************************************
#define TCFG_PWMLED_ENABLE					0//ENABLE_THIS_MOUDLE			//是否支持PMW LED推灯模块
#define TCFG_PWMLED_IOMODE					LED_ONE_IO_MODE				//LED模式，单IO还是两个IO推灯
#define TCFG_PWMLED_PIN						IO_PORTB_06					//LED使用的IO口 注意和led7是否有io冲突

//*********************************************************************************//
//                                  系统配置                                         //
//*********************************************************************************//
#define TCFG_AUTO_SHUT_DOWN_TIME		    0   //没有蓝牙连接自动关机时间
#define TCFG_SYS_LVD_EN						1   //电量检测使能

#if ADAPTER_UART_DEMO
#define TCFG_POWER_ON_NEED_KEY				1	  //是否需要按按键开机配置
#else
#define TCFG_POWER_ON_NEED_KEY				0	  //是否需要按按键开机配置
#endif

#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_32V             //VDDIO 设置的值要和vbat的压差要大于300mv左右，否则会出现DAC杂音


//*********************************************************************************//
//                                  共晶振配置                                       //
//*********************************************************************************//
#define TCFG_SHARE_OSC_EN				    DISABLE_THIS_MOUDLE
#define TCFG_SHARE_OSC_UART_PORT			IO_PORTP_00	//不建议使用其他的IO口,只能用PP0,即LDOIN这个脚
#if TCFG_SHARE_OSC_EN
//共用晶振需要关掉充电，需要使用PP0来通信
#undef TCFG_CHARGE_ENABLE
#define TCFG_CHARGE_ENABLE					0

#undef TCFG_CHARGESTORE_PORT
#define TCFG_CHARGESTORE_PORT				NO_CONFIG_PORT

#undef CONFIG_RESET_PIN
#define CONFIG_RESET_PIN					PP_00//IO_PORTP_00

#undef CONFIG_RESET_TIME
#define CONFIG_RESET_TIME					01

#undef CONFIG_RESET_LEVEL
#define CONFIG_RESET_LEVEL				    0

//不需要电量检测，由soundbox那边检测
#undef TCFG_SYS_LVD_EN
#define TCFG_SYS_LVD_EN						0
//不需要按键开机
#undef TCFG_POWER_ON_NEED_KEY
#define TCFG_POWER_ON_NEED_KEY				0

#endif


//*********************************************************************************//
//                                  EQ配置                                         //
//*********************************************************************************//
#define TCFG_EQ_ENABLE						DISABLE_THIS_MOUDLE
#define TCFG_EQ_ONLINE_ENABLE               0     //支持在线EQ调试, 如果使用蓝牙串口调试，需要打开宏 APP_ONLINE_DEBUG，否则，默认使用uart调试(二选一)
#define TCFG_PHONE_EQ_ENABLE				ENABLE_THIS_MOUDLE
#define EQ_SECTION_MAX                      10    //eq段数
#define TCFG_USE_EQ_FILE                    1    //离线eq使用配置文件还是默认系数表 1：使用文件  0 使用默认系数表
#define TCFG_AEC_DCCS_EQ_ENABLE				TCFG_MIC_CAPLESS_ENABLE//省电容mic需要加dccs直流滤波
// ONLINE CCONFIG
#define TCFG_ONLINE_TX_PORT					IO_PORT_DP                 //EQ调试TX口选择
#define TCFG_ONLINE_RX_PORT  				IO_PORT_DM                 //EQ调试RX口选择

//*********************************************************************************//
//                          新音箱配置工具 && 调音工具                             //
//*********************************************************************************//
#define TCFG_EFFECT_TOOL_ENABLE				DISABLE		  	//是否支持在线音效调试,使能该项还需使能EQ总使能TCFG_EQ_ENABL,
#define TCFG_NULL_COMM						0				//不支持通信
#define TCFG_UART_COMM						1				//串口通信
#define TCFG_USB_COMM						2				//USB通信
#if (TCFG_CFG_TOOL_ENABLE || TCFG_EFFECT_TOOL_ENABLE)
#define TCFG_COMM_TYPE						TCFG_UART_COMM	//通信方式选择
#else
#define TCFG_COMM_TYPE						TCFG_NULL_COMM
#endif
#define TCFG_TOOL_TX_PORT					IO_PORT_DP      //UART模式调试TX口选择
#define TCFG_TOOL_RX_PORT					IO_PORT_DM      //UART模式调试RX口选择
#define TCFG_ONLINE_ENABLE                  (TCFG_EFFECT_TOOL_ENABLE)    //是否支持音效在线调试功能

#if (TCFG_EQ_ONLINE_ENABLE && !TCFG_EFFECT_TOOL_ENABLE) || (!TCFG_EQ_ONLINE_ENABLE && TCFG_EFFECT_TOOL_ENABLE)
#error "在线调音，两个宏定义都需要使能"
#endif

#if TCFG_SHARE_OSC_EN
#undef TCFG_TOOL_TX_PORT
#undef TCFG_TOOL_RX_PORT
#define TCFG_TOOL_TX_PORT					TCFG_SHARE_OSC_UART_PORT        //UART模式调试TX口选择
#define TCFG_TOOL_RX_PORT					TCFG_SHARE_OSC_UART_PORT        //UART模式调试RX口选择
#endif

#if (((TCFG_COMM_TYPE == TCFG_UART_COMM) && TCFG_ONLINE_ENABLE) && TCFG_SHARE_OSC_EN)
#error "online uart enable, plaease close TCFG_SHARE_OSC_EN"
#endif


//*********************************************************************************//
//                                  蓝牙配置                                       //
//*********************************************************************************//
#if TCFG_RF_TEST_EN
#define TCFG_USER_BT_CLASSIC_ENABLE         1   //经典蓝牙功能使能
#endif

#define TCFG_USER_BLE_ENABLE                1   //BLE功能使能
#define BT_FOR_APP_EN                       0

#if TCFG_USER_BLE_ENABLE
//BLE多连接,多开注意RAM的使用


/*蓝牙BLE多连接*/
#if TCFG_USER_BLE_ENABLE
#define TRANS_MULTI_BLE_EN                1 /*蓝牙BLE多连 ENABLE*/
#if TRANS_MULTI_BLE_EN
#define TRANS_MULTI_BLE_SLAVE_NUMS        0 /*配置从机个数*/
#if WIRELESS_2t1_RX_ONLY_CONN_1TX
#define TRANS_MULTI_BLE_MASTER_NUMS       1 /*配置主机个数*/
#else
#define TRANS_MULTI_BLE_MASTER_NUMS       2 /*配置主机个数*/
#endif
#define TRANS_MULTI_BLE_SM_ENABLE         1 /*配置加密 en*/
#define TRANS_MULTI_BLE_CONNECT_NUMS      (TRANS_MULTI_BLE_SLAVE_NUMS + TRANS_MULTI_BLE_MASTER_NUMS) /*连接个数*/
#endif
#endif

#endif
//wifi抗干扰
#define TCFG_WIFI_DETECT_ENABLE			ENABLE
#define TCFG_WIFI_DETCET_PRIOR			ENABLE//2t1 client收数多 主机优先
#define WIRELESS_24G_CODE_ID            13

#if	(WIRELESS_MIC_STEREO_EN)
#define WIRELESS_DECODE_CHANNEL_NUM		2
#endif

#define WIRELESS_HIGH_BW_EN             ENABLE
#if !WIRELESS_HIGH_BW_EN && WIRELESS_LOW_LATENCY
#error "!!!! no support"
#endif


#if WIRELESS_HIGH_BW_EN
#define WIRELESS_BLE_CONNECT_INTERVAL	2
#else
#define WIRELESS_BLE_CONNECT_INTERVAL	8//6
#endif /* WIRELESS_HIGH_BW_EN */

#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
#if (WIRELESS_TX_MIC_STEREO_OUTPUT)
#if WIRELESS_LOW_LATENCY
#if (WIRELESS_CODING_SAMPLERATE <= 32000&&(!(USB_DEVICE_CLASS_CONFIG&(!MIC_CLASS))))
#define WIRELESS_CLK					160	//usb_mic+low_latency+stereo&&samplerate<=32k
#else
#error "!!!! no support low_latency+USB_MIC+USB_SPK+STEREO&&samplerate>32k"
#error "please DISABLE SPEAKER_CLASS and set samplerate<=32"
#endif
#else
#if USB_DEVICE_CLASS_CONFIG == (MIC_CLASS|SPEAKER_CLASS)
#define WIRELESS_CLK					160	//usb_mic+usb_spk+stereo
#else
#define WIRELESS_CLK					144	//usb_mic+stereo
#endif
#endif
#else
#if WIRELESS_LOW_LATENCY
#define WIRELESS_CLK					144//usb&&low_latency
#endif
#endif
#else
//#if (WIRELESS_TX_MIC_STEREO_OUTPUT)
//#if WIRELESS_LOW_LATENCY
#define WIRELESS_CLK					144//stereo&&latency
//#endif
//#endif
#endif

//<!以下门限参数可调， 降噪延时适当调大一些， 非降噪门限可以调低一些， 对延时要求高的话， 可以适当调低， 但不建议低于10
//<!信号好， 低延时门限

#if WIRELESS_2T1_MICROPHONE_MODE
#define WL_LOW_DELAY_LIMIT              22
#define WL_HIGH_DELAY_LIMIT             WL_LOW_DELAY_LIMIT	//做无线话筒方案的，一般tx和rx的距离比较远，很快就会触发HIGH_DELAY,有需要的也可以加大
#else
#if WIRELESS_LOW_LATENCY
#define WL_LOW_DELAY_LIMIT              9
#else
#define WL_LOW_DELAY_LIMIT              12
#endif
#define WL_HIGH_DELAY_LIMIT             WL_LOW_DELAY_LIMIT+6
#endif


//产测rx端支持
#define TCFG_2T1_RX_PRODUCT_TEST_EN				1
//*********************************************************************************//
//                                  encoder 配置                                   //
//*********************************************************************************//
#define TCFG_ENC_JLA_ENABLE				    DISABLE

//*********************************************************************************//
//                                  decoder 配置                                   //
//*********************************************************************************//
#define TCFG_DEC_JLA_ENABLE				    ENABLE

#define TCFG_MEDIA_LIB_USE_MALLOC										1

#define TCFG_DEV_MANAGER_ENABLE											0

#define WIRELESS_CODING_BIT_RATE            (64000)

#if(WIRELESS_CODING_BIT_RATE <= 96000)
#define TCFG_SERVER_RX_PAYLOAD_LEN		    (140)
#else
#define TCFG_SERVER_RX_PAYLOAD_LEN		     \
    ((WIRELESS_CODING_BIT_RATE * WIRELESS_CODING_FRAME_LEN / 1000 / 10 / 8) + 6 + 7)
#endif

//*********************************************************************************//
//                                 配置结束                                         //
//*********************************************************************************//



#endif //CONFIG_BOARD_WIRELESS_MIC_2T1_RX
#endif //CONFIG_BOARD_WIRELESS_MIC_2T1_RX_CFG_H

