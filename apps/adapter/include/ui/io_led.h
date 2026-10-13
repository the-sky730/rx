#ifndef _IO_LED_H_
#define _IO_LED_H_


#include "generic/typedef.h"
#include "app_config.h"

#if TCFG_IOLED_ENABLE

#define LED_ON_LEVEL		1	//高电平点亮
#define FAST_TIME			2	//200ms
#define SLOW_TIME			10	//1000ms

//BIT(0)~BIT(7)  for LED0
//BIT(8)~BIT(15) for LED1
enum ioled_mode {
    LED0_SLOW_FLASH = BIT(0),
    LED0_FAST_FLASH = BIT(1),
    LED0_ON 		= BIT(2),
    LED0_OFF 		= BIT(3),
    LED1_SLOW_FLASH	= BIT(8),
    LED1_FAST_FLASH = BIT(9),
    LED1_ON 		= BIT(10),
    LED1_OFF 		= BIT(11)
};

void ioled_update_status(u16 status);

#endif
#endif
