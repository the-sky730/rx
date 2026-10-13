#ifndef __ADAPTER_UART_DEMO_H__
#define __ADAPTER_UART_DEMO_H__

#include "typedef.h"
#include "system/event.h"
#include "system/includes.h"



void adapter_uart_demo_init(void);
void adapter_uart_demo_close(void);
void uart_duplex_send_data(u8 *buf, u32 len);
void uart_duplex_get_data(void);















#endif

