#include "types.h"
#ifndef _UARTS_HEADER_H_
#define _UART_HEADER_H_


void UART_Init();
uint8 UART1_RX(char *buff);
uint8 UART1_TX(char *buff, uint8 len);
uint8 UART0_RX(char *buff);
uint8 UART0_TX(char *buff, uint8 len);
//void UART0_SendPacket(uint8 *buff, uint8 leng);
//void UART1_SendPacket(uint8 *buff, uint8 leng);

#endif

