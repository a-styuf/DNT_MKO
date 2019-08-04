#include "timers_header.h"
#include "dnt_fun_header.h"

uint16 TIMER_10MS = 0;
uint8_t timer_flag_1 = 0, timer_flag_2 = 0;
uint16_t time_10ms = 0;
extern typeDNTOperationData dnt;

#pragma interrupt 0 IRQ_Timer1 // обработкf Timer1 и Timer 2 по прерыванию для частоты кварцевого генератора 8 МГц
void IRQ_Timer1()
{
    uint8 tmp;
    _di_();
    tmp = IOS1;
    //SBUF_TX1 = tmp;
    if (tmp&0x20)//обработка таймера 1
    {
        WSR = 0x0F;
        TIMER1 = 0xCF2C; //53036 10 мс для частоты МК 20МГц
        WSR = 0x00;
		//формирование глобального времени
		if (time_10ms < 100){
			time_10ms += 1;
		}
		else{
			dnt.time += 1;
			time_10ms = 0;
		}
		//
        if (TIMER_10MS != 0){
            TIMER_10MS -= 1;
            timer_flag_1 = 0;
        }
        if (TIMER_10MS == 0){
            timer_flag_1 = 1;
        }
    }
    if(tmp&0x10){
        WSR = 0x00;
        //TIMER2 = 0xCF2C; //53036 10 мс для частоты МК 20МГц
        WSR = 0x00;
    }
    _ei_();
}

void Timer_Init()
{
  uint8 tmp;
  WSR = 0x0F;
  tmp = IOC1;
  WSR = 0x00;
  IOC1 = tmp | 0x0C; //разрешаем прерывания таймера 1 и 2
  WSR = 0x01;
  IOC3 |= 0x01;
  WSR = 0x00;
  INT_MASK |= 0x01;
  //INT_MASK1 |= 0x10;
  WSR = 0x00;
  INT_PEND1 &= ~0x10;
  INT_PEND &= ~0x01;
}

uint8 Timer10ms_Flag()
{
  if (timer_flag_1 != 0)
  {
    timer_flag_1 = 0;
    return 1;
  }
  return 0;
}
