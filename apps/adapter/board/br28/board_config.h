#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

//不导出不需要头文件
// FOLLOW-INC: OFF

/*
 *  板级配置选择
 */

// #ifndef APP_MAIN
// #error "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
// #endif


#if (APP_MAIN == APP_DONGLE)
#define CONFIG_BOARD_AC897N_BTEMITTER
#include "board_ac897n_btemitter/board_ac897n_btemitter_cfg.h"
#elif (APP_MAIN == APP_DONGLE_1T2)
#define CONFIG_BOARD_AC897N_DONGLE_1T2
#include "board_ac897n_dongle_1t2/board_ac897n_dongle_1t2_cfg.h"
#elif (APP_MAIN == APP_WIRELESS_MIC_1T1)
#if (CONFIG_1T1_ROLE_SEL == 0)
#define CONFIG_BOARD_WIRELESS_MIC_1T1_TX
#include "board_wireless_mic_1t1_tx/board_wireless_mic_1t1_tx_cfg.h"
#else
#define CONFIG_BOARD_WIRELESS_MIC_1T1_RX
#include "board_wireless_mic_1t1_rx/board_wireless_mic_1t1_rx_cfg.h"
#endif
#elif (APP_MAIN == APP_WIRELESS_MIC_2T1)
#if (CONFIG_2T1_ROLE_SEL == 0)
#define CONFIG_BOARD_WIRELESS_MIC_2T1_TX
#include "board_wireless_mic_2t1_tx/board_wireless_mic_2t1_tx_cfg.h"
#else
#define CONFIG_BOARD_WIRELESS_MIC_2T1_RX
#include "board_wireless_mic_2t1_rx/board_wireless_mic_2t1_rx_cfg.h"
#endif
#elif (APP_MAIN == APP_WIRELESS_MIC_1TN)
#if (CONFIG_1TN_ROLE_SEL == 0)
#define CONFIG_BOARD_WIRELESS_MIC_1TN_TX
#include "board_wireless_mic_1tN_tx/board_wireless_mic_1tN_tx_cfg.h"
#else
#define CONFIG_BOARD_WIRELESS_MIC_1TN_RX
#include "board_wireless_mic_1tN_rx/board_wireless_mic_1tN_rx_cfg.h"
#endif
#elif (APP_MAIN == APP_WIRELESS_DUPLEX)
#if (CONFIG_DUPLEX_ROLE_SEL == 0)
#define CONFIG_BOARD_WIRELESS_DUPLEX_DONGLE
#include "board_wireless_duplex_dongle/board_wireless_duplex_dongle_cfg.h"
#else
#define CONFIG_BOARD_WIRELESS_DUPLEX_EARPHONE
#include "board_wireless_duplex_earphone/board_wireless_duplex_earphone_cfg.h"
#endif
#else
//给一个默认的之配置
#define CONFIG_BOARD_AC897N_BTEMITTER
#endif

#ifdef CONFIG_NEW_CFG_TOOL_ENABLE
#define CONFIG_ENTRY_ADDRESS                    0x6000100
#endif

#endif

