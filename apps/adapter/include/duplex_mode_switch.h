#ifndef _DUPLEX_MODE_SWITCH_H_
#define _DUPLEX_MODE_SWITCH_H_

#include "system/includes.h"
#include "app_config.h"

#define MEMORY_MODE				0//记忆模式
#define	DUPLEX_MODE 			1//游戏耳机模式
#define	EARPHONE_MODE	 		2//蓝牙耳机模式


#define POWERON_MODE	MEMORY_MODE
#define MODE_VM_ID	    40
#define CHECK_KEY	 	0x55
#define MODE_CODE	 	0x12345678

#define OFFSET_LEN		400
#if CONFIG_CPU_BR28		//701
#define STORE_ADDR		((u32 *)0x19FE00)+IRQ_SOFT1_IDX
#define CODE_ADDR		0x6000100
#elif CONFIG_CPU_BR30	//697
#define STORE_ADDR		((u32 *)0x2BF00)+IRQ_SOFT1_IDX
#define CODE_ADDR		0x1E00100
#endif

void duplex_earphone_mode_switch(u8 mode);

#endif
