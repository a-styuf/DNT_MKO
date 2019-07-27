#include "dnt_fun_header.h"
#include "crc16_header.h"
#include <math.h>
//#include <string.h>
//#include <stdio.h>
#include "uart_header.h"
#define log10_2 0.30103


void PWM_Init()
{
  WSR = 0x01;
  PWM1_CONTROL = 0;
  WSR = 0x00;
}

void PWM_Duty (uint8 Duty)
{
  if (Duty > 128) Duty = 128;
  //else if(Duty < 5) Duty = 5;
  WSR = 0x01;
  PWM1_CONTROL = Duty;
  WSR = 0x00;
}

uint8 PWM_FB (uint16 MON_50V, uint8 Duty) // значение передается в 10   мВ
{
  if (MON_50V/256 > 54) return Duty-1;
  else if (MON_50V/256 < 50) return Duty+1;
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

uint16 COMAnsForm (uint8 req_id, uint8 self_id, uint8 sub_adress, uint8 *seq_num, uint8 com, uint8 leng, uint8* com_data, uint8* ans_com) //функция для формирования ответов на команды
{
  uint16 crc;
  uint8 j;
  *seq_num += 1;
  ans_com[0] = req_id & 0xFF;
  ans_com[1] = self_id & 0xFF;
  ans_com[2] = sub_adress & 0xFF;
  ans_com[3] = (*seq_num) & 0xFF; //не забываем проинкрементировать последовательный номер ответа
  ans_com[4] = com & 0xFF;
  ans_com[5] = leng & 0xFF;
  for(j=0; j < leng; j++)
  {
    ans_com[j+6] = com_data[j];
  }
  crc = crc16_ccitt(ans_com, leng+6);
  ans_com[leng+6] = (uint8)((crc>>8) & 0xFF);
  ans_com[leng+7] = (uint8)((crc>>0) & 0xFF);
  return leng+8;
}

int16 CurrentCalculation(int16* dnt_result, int16* dnt_max, int16* dnt_min, uint16 *status)
{
    int16 tmp_uint16;
    /* вычисление тока */
    tmp_uint16 = dnt_result[0 + (*status)] - dnt_result[4 + (*status)];

    /* определение корректного ku */
    switch (*status)
    {
        case 3:
            if ((abs(dnt_max[0])  > 16200) | (abs(dnt_max[1]) > 16200) | (abs(tmp_uint16)>6000)) *status = 2;
            break;
        case 2:
            if ((abs(dnt_max[0])  > 16200) | (abs(dnt_max[1]) > 16200) | (abs(tmp_uint16)>6000)) *status = 1;
            else if ((abs(dnt_min[0])  < 160) | (abs(dnt_min[1]) < 160) | (abs(tmp_uint16)<400)) *status = 3;
            break;
        case 1:
            if ((abs(dnt_max[0])  > 16200) | (abs(dnt_max[1]) > 16200) | (abs(tmp_uint16)>6000)) *status = 0;
            else if ((abs(dnt_min[0])  < 160) | (abs(dnt_min[1]) < 160) | (abs(tmp_uint16)<400)) *status = 2;
            break;
        case 0:
            if ((abs(dnt_min[0])  < 160) | (abs(dnt_min[1]) < 160) | (abs(tmp_uint16)<400)) *status = 1;
            break;
        default:
            *status = 0;
            break;
    }

    return tmp_uint16;
}

