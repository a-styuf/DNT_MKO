#include "types.h"
#ifndef _UART_HEADER_H_
#define _UART_HEADER_H_

#define MCU_FREQ_HZ 20000000  //задаем частоту кварцевого генератора
#define UART0_BR (38400)
#define UART1_BR (38400)
#define UART0_BR_VAL (uint8_t)((MCU_FREQ_HZ/(2*8*UART0_BR)) - 1) & 0x7F
#define UART1_BR_VAL (uint8_t)((MCU_FREQ_HZ/(2*8*UART1_BR)) - 1) & 0x7F

void UART_Init();
uint8 UART1_RX(char *buff);
uint8 UART1_TX(char *buff, uint8 len);
uint8 UART0_RX(char *buff);
uint8 UART0_TX(char *buff, uint8 len);
//void UART0_SendPacket(uint8 *buff, uint8 leng);
//void UART1_SendPacket(uint8 *buff, uint8 leng);

#endif

