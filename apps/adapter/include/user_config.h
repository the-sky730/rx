#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "macro.h"

//*********************************************************************************//
//						APP应用系统打印总开关                                      //
//*********************************************************************************//
#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif //CONFIG_RELEASE_ENABLE

#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)

#define CONFIG_DEBUG_ENABLE

#ifndef CONFIG_DEBUG_ENABLE
//#define CONFIG_DEBUG_LITE_ENABLE  //轻量级打印开关, 默认关闭
#endif //CONFIG_DEBUG_ENABLE

#define NEW_AUDIO_W_MIC		1

//*********************************************************************************//
//						APP应应用选择                                      //
//*********************************************************************************//
//#define APP_MAIN                    APP_DONGLE
//#define APP_MAIN                    APP_DONGLE_1T2
//#define APP_MAIN                    APP_WIRELESS_MIC_1T1
//#define APP_MAIN                    APP_WIRELESS_MIC_2T1
//#define APP_MAIN                    APP_WIRELESS_MIC_1TN
//#define APP_MAIN                    APP_WIRELESS_DUPLEX


#endif //USER_CONFIG_H

