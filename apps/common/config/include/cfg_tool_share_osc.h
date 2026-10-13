#ifndef _CFG_TOOL_SHARE_OSC_H_
#define _CFG_TOOL_SHARE_OSC_H_

#include "system/includes.h"

#define ONLINE_SUB_OP_POWER_OFF					0x00000044	//从机关机

/**
<<<<<<< Updated upstream
 * @brief 通知对端关机
 */
// void cfg_tool_share_osc_power_off();

/**
=======
>>>>>>> Stashed changes
 * @brief 回复对端本设备已关机
 */
void cfg_tool_share_osc_power_off_response();

#endif
