
#include <string.h>
#include "uart_header.h"

//--------------------------Function Interfaces---------------------------
#pragma interrupt 8 IRQ_uart_rx_tx //разрешение обработки UART по прерыванию

#pragma data_alignment=2
volatile struct structPTS {
  uint8 count;
  uint8 con;
  uint8 *src;
  uint8 *dst;
  uint8 res[2];
} UartPTS;

uint8 status_uart0 = 0, status_uart1 = 0;
char UART1_RX_array[64];
char UART1_TX_array[64];
uint8 len_rx1=0;
uint8 flag_rx1 = 0;

uint8 flag_rx = 0;

//инициализация UART1(Отладочного - NU) и UART0(MUART)//////////////// 0x40 - 19200; 0x1F - 38400; 0x0A - 115200
void UART_Init()
{
  volatile uint8 tmp;
  //UART1
  BAUD_RATE1 = 0x1F;  //38400
  BAUD_RATE1 = 0x80;
  SP_CON1     = 0x19;
  //UART0
  BAUD_RATE0 = 0x1F;  //38400
  BAUD_RATE0 = 0x80;
  SP_CON0     = 0x19;
  //GENERAL
  WSR = 0x0F;      //swap to HWindow15
  INT_MASK1 |= 0x01;      //allow transmit interrupt
  WSR = 0x00;        //swap to HWindow0
  WSR = 0x0F;
  tmp = IOC1;
  WSR = 0x00;
  IOC1 = tmp | 0x21;
  CLKC = 0xFF3F;
  tmp = SP_STAT1;// очистка состояний статусных байтов
  tmp = SP_STAT0;// очистка состояний статусных байтов
   /*пакетная передача с пом. PTS*/
  UartPTS.dst = (uint8*)&SBUF_TX0;
  UartPTS.con = 0x9A;
  UartPTS.res[0] = UartPTS.res[1] = 0;
  _ei_();
  __EPTS();
  
}

void IRQ_uart_rx_tx()
{
	_di_(); // запрет обработки прерываний
	status_uart1 = SP_STAT1;
	if (status_uart1 & 0x40) //oбрабатываем прерывание по приему
	{
		if (INT_PEND1 & 0x10) 
		{
			len_rx1 = 0;
			UART1_RX_array[len_rx1] = SBUF_RX1; 
			// SBUF_TX1 = SBUF_RX1;
		}
		else if (len_rx1 > 63)
		{
			len_rx1 = 0;
		}
		else 
		{
			len_rx1 ++;
			UART1_RX_array[len_rx1] = SBUF_RX1;
			// SBUF_TX1 = SBUF_RX1;
		}
		len_rx1 &= 0x3F;
		INT_PEND1 &= ~0x10;  //сброс флага переполнения таймера 2
		TIMER2 = 0xF63C;  //межпакетный интервал 2ms
		}
	else
	{
		//SP_CON0 = 0x19;  //вновь разрешение приема
		//SP_CON1 = 0x19;  //вновь разрешение приема
	}
	_ei_(); //разрешение обработки прерываний
}

uint8 UART1_RX(char *buff)
{
  uint8 len_tmp = 0;
  if((len_rx1 != 0) && ((INT_PEND1 & 0x10) != 0))
  {
	len_tmp =  len_rx1 + 1; 
	//SBUF_TX1 = len_rx1; 
    memcpy(buff, UART1_RX_array, len_rx1);
	len_rx1 = 0;
  }
  return len_tmp;
}

uint8 UART1_TX(char *buff, uint8 len)
{
  uint8 i;
  SP_CON1 = 0x11;  //запрет приема
  for (i = 0; i < len; i++)
  {
    status_uart1 = SP_STAT1;
    while(!(status_uart1 & 0x08));//дожидаемся освобождения буфера
    SBUF_TX1 = buff[i];
  }
  SP_CON1 = 0x19;  //разрешение приема приема
  return i;
}

void UART1_SendPacket(uint8 *buff, uint8 leng) {
  SP_CON1 = 0x11;  //запрет приема
  UartPTS.dst = (uint8*)&SBUF_TX1;
  WSR = 1;
  while(PTSSEL & 0x100);  //ожидание конца передачи
  memcpy(UART1_TX_array, buff, leng);
  UartPTS.src = (uint8*)&UART1_TX_array[1];
  UartPTS.count = leng-1;
  PTSSEL = 0x100;  //start Tx
  WSR = 0;
  SBUF_TX1 = UART1_TX_array[0];
}