#ifndef MACRO_H
#define MACRO_H

#include "asm/clock_define.h"
#include "btcontroller_mode.h"
#include "user_cfg_id.h"

//*********************************************************************************//
//                                  ENABLE/DISABLE                              //
//*********************************************************************************//
#define ENABLE_THIS_MOUDLE					1
#define DISABLE_THIS_MOUDLE					0

#define ENABLE								1
#define DISABLE								0

#define NO_CONFIG_PORT						(-1)


//*********************************************************************************//
//                                  APP MAIN CHOSE                              //
//*********************************************************************************//
// #define APP_DONGLE                  0
// #define APP_DONGLE_1T2 	            1
// #define APP_WIRELESS_MIC_1T1        2
// #define APP_WIRELESS_MIC_2T1        3
// #define APP_WIRELESS_MIC_1TN        4
// #define APP_WIRELESS_DUPLEX         5

//*********************************************************************************//
//                                  LINEIN_INPUT_WAY                              //
//*********************************************************************************//
#define LINEIN_INPUT_WAY_ANALOG      0
#define LINEIN_INPUT_WAY_ADC         1
#define LINEIN_INPUT_WAY_DAC         2

//*********************************************************************************//
//                                  SD DECT
//*********************************************************************************//
#define     SD_CMD_DECT 	0
#define     SD_CLK_DECT  	1
#define     SD_IO_DECT 		2

//*********************************************************************************//
//                                 dac output define                               //
//*********************************************************************************//
#define DAC_OUTPUT_MONO_L                  0    //左声道
#define DAC_OUTPUT_MONO_R                  1    //右声道
#define DAC_OUTPUT_LR                      2    //立体声
#define DAC_OUTPUT_MONO_LR_DIFF            3    //单声道差分输出
#define DAC_OUTPUT_DUAL_LR_DIFF            6    //双声道差分
#define DAC_OUTPUT_FRONT_LR_REAR_L         7    //三声道单端输出 前L+前R+后L (不可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_R         8    //三声道单端输出 前L+前R+后R (可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_LR        9    //四声道单端输出(不可设置vcmo公共端)

//*********************************************************************************//
//                                 style select                                    //
//*********************************************************************************//
#define STYLE_JL_WTACH                      (1) //彩屏demo
#define STYLE_JL_SOUNDBOX                   (2) //点阵屏demo
#define STYLE_JL_CHARGE                     (3) //点阵屏充电仓
#define STYLE_JL_LED7                       (4) //led7
#define STYLE_UI_SIMPLE                     (5) //没有ui框架
#define STYLE_IC_RECORDER                   (6) //录音笔

//*********************************************************************************//
//                                 对耳配置方式配置                                    //
//*********************************************************************************//
/* 配对方式选择 */
#define CONFIG_TWS_PAIR_BY_CLICK                0      /* 按键发起配对 */
#define CONFIG_TWS_PAIR_BY_AUTO                 1      /* 开机自动配对 */
#define CONFIG_TWS_PAIR_BY_FAST_CONN            2      /* 开机快速连接,连接速度比自动配对快,不支持取消配对操作 */

/* 声道确定方式选择 */
#define CONFIG_TWS_MASTER_AS_LEFT               0 //主机作为左耳
#define CONFIG_TWS_AS_LEFT_CHANNEL              1 //固定左耳
#define CONFIG_TWS_AS_RIGHT_CHANNEL             2 //固定右耳
#define CONFIG_TWS_LEFT_START_PAIR              3 //双击发起配对的耳机做左耳
#define CONFIG_TWS_RIGHT_START_PAIR             4 //双击发起配对的耳机做右耳
#define CONFIG_TWS_EXTERN_UP_AS_LEFT            5 //外部有上拉电阻作为左耳
#define CONFIG_TWS_EXTERN_DOWN_AS_LEFT          6 //外部有下拉电阻作为左耳
#define CONFIG_TWS_SECECT_BY_CHARGESTORE        7 //充电仓决定左右耳

/* 对耳手机端电量显示 */
#define CONFIG_DISPLAY_TWS_BAT_LOWER            1 //对耳手机端电量显示，显示低电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_HIGHER           2 //对耳手机端电量显示，显示高电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_LEFT             3 //对耳手机端电量显示，显示左耳的电量
#define CONFIG_DISPLAY_TWS_BAT_RIGHT            4 //对耳手机端电量显示，显示右耳的电量


//*********************************************************************************//
//                                串口升级配置                                     //
//*********************************************************************************//
#define UART_UPDATE_SLAVE	0
#define UART_UPDATE_MASTER	1

//*********************************************************************************//
//                                USB配置                                     //
//*********************************************************************************//
#define     MASSSTORAGE_CLASS   0x00000001
#define     SPEAKER_CLASS       0x00000002
#define     MIC_CLASS           0x00000004
#define     HID_CLASS           0x00000008
#define     CDC_CLASS           0x00000010


/*
 *系统音量类型选择
 *软件数字音量是指纯软件对声音进行运算后得到的
 *硬件数字音量是指dac内部数字模块对声音进行运算后输出
 */
#define VOL_TYPE_DIGITAL		0	//软件数字音量
#define VOL_TYPE_ANALOG			1	//硬件模拟音量
#define VOL_TYPE_AD				2	//联合音量(模拟数字混合调节)
#define VOL_TYPE_DIGITAL_HW		3  	//硬件数字音量
#define VOL_TYPE_DIGGROUP       4   //独立通道数字音量


//*********************************************************************************//
//                                混响配置                                     //
//*********************************************************************************//
#define MIC_EFFECT_REVERB               0
#define MIC_EFFECT_ECHO                 1


//*********************************************************************************//
//                            AUDIO_OUTPUT_WAY配置                                 //
//*********************************************************************************//
#define AUDIO_OUTPUT_WAY_DAC        0
#define AUDIO_OUTPUT_WAY_IIS        1
#define AUDIO_OUTPUT_WAY_FM         2
#define AUDIO_OUTPUT_WAY_HDMI       3
#define AUDIO_OUTPUT_WAY_SPDIF      4
#define AUDIO_OUTPUT_WAY_BT      	5	// bt emitter
#define AUDIO_OUTPUT_WAY_DONGLE		7

//*********************************************************************************//
/*
 *系统音量类型选择
 *软件数字音量是指纯软件对声音进行运算后得到的
 *硬件数字音量是指dac内部数字模块对声音进行运算后输出
 */
//*********************************************************************************//
#define VOL_TYPE_DIGITAL		0	//软件数字音量
#define VOL_TYPE_ANALOG			1	//硬件模拟音量
#define VOL_TYPE_AD				2	//联合音量(模拟数字混合调节)
#define VOL_TYPE_DIGITAL_HW		3  	//硬件数字音量
#define VOL_TYPE_DIGGROUP       4   //独立通道数字音量


//*********************************************************************************//
//                                无线麦配置                                     //
//*********************************************************************************//
#define APP_WIRELESS_MASTER					0
#define APP_WIRELESS_SLAVE					1

#define WIRELESS_MIC_RX_OUTPUT_DAC 			0
#define WIRELESS_MIC_RX_OUTPUT_USB_MIC 		1

//*********************************************************************************//
//                            MIC模式配置                                 //
//*********************************************************************************//
#define MIC_MODE_SINGLE_END                 0
#define MIC_MODE_DIFF_END                   1

#endif //MACRO_H
