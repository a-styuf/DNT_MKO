#include <string.h>
#include <math.h>

#include "adc_header.h"

#define A 0.17  //калибровочные константы для перевода значение АЦП в мВ
#define B 221.87

#define ALPHA -0.786  //калибровочные константы для перевода напряжения на входе АЦП в напряжение генератора 50-ти Вольт в еденицах В/256 c обратным знаком
#define BETTA 18414




void ADC_Init()
{
  ADC_EN = 0x05 | 0x80;
  ADC_SET = 0x38 & 0x20;
}

void ADC_DNT(uint16 *DNT_result, int16 *adc_array, int16 *max, int16 *min, uint16 aver_num)
{
  uint16 i;
  float mean = 0;
  ADC_CON = 0x03 | (12 << 3); //канал для измерения тока DNT
  //ожидаем устаканивания сигнала
  for(i = 0; i < aver_num; i++) //~20ms
  {
    adc_array[i] = ADC_RESULT&0x3FFF;
    adc_array[i] = (adc_array[i] - 0x2000) & 0x3FFF;
  }
  //запоминаем сигнал
  for(i = 0; i < aver_num; i++) //~20ms
  {
    adc_array[i] = ADC_RESULT&0x3FFF;
    adc_array[i] = (adc_array[i] - 0x2000) & 0x3FFF;
  }
  *max = 0x7FFF;
  *min = 0x8000;
  for(i = 0; i < aver_num; i++)
  {
    //adc_array[i] = (adc_array[i] - 0x2000) & 0x3FFF;
    mean += adc_array[i];
    if (*max <  adc_array[i]) *max = adc_array[i];
    if (*min >  adc_array[i]) *min = adc_array[i];
  }
  ADC_CON = 0x03 | (12 << 3); //канал для измерения тока DNT
  mean /= aver_num*1.;
  *DNT_result = (uint16)(1*mean+0); //перевод в мВ
}

uint16 ADC_DNT_summ()
{
  while((ADC_RESULT) & 0x8000);
  ADC_CON = 0x01 | (12 << 3); //канал для измерения тока DNT
  return ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
}

void ADC_50V(uint16 *Result_50V, uint16 N)
{
  uint16 i;
  uint32 mean = 0;
  for(i = 0; i < N; i++)
  {
    ADC_CON = 0x01 | (4 << 3); //канал для измерения тока DNT
    while( (*Result_50V = ADC_RESULT) & 0x8000 );
    *Result_50V = (*Result_50V - 0x2000) & 0x3FFF;
    mean += *Result_50V;
  }
  mean /= N;
  *Result_50V = (uint16)(mean*ALPHA+BETTA); //перевод в В/256
}

void ADC_Pt1000(int16 *DNT_Temp, uint16 N)
{
  uint16 i;
  float mean = 0;
    float a = 8.19E-6, b = -9.445E-2, c= 2.039E+2;
  for(i = 0; i < N; i++)
  {
    ADC_CON = 0x01 | (2 << 3); //канал для измерения тока DNT
    while( (*DNT_Temp = ADC_RESULT) & 0x8000 );
    *DNT_Temp = (*DNT_Temp - 0x2000) & 0x3FFF;
    mean += *DNT_Temp;
  }
  mean /= N;
  mean = (a*mean*mean+b*mean+c)*256.;
  *DNT_Temp = (uint16)(floor(mean));
}

void Oscilloscope(uint16 ku, uint16 mode, uint16 delay_1ms)
{
  uint16 i;
  uint8 tmp ;
  uint16 *MKO_tr_data = (uint16*)0x1000;
    SBUF_TX1 = 0x11;
  if (mode == 0)
  {
    if (ku == 0) IOPORT1 = 0x38&0x08;
    else if (ku == 1) IOPORT1 = 0x38&0x28;
    else if (ku == 2) IOPORT1 = 0x38&0x18;
    else if (ku == 3) IOPORT1 = 0x38&0x38;
  }
  else
  {
    if (ku == 0) IOPORT1 = 0x38&0x08;
    else if (ku == 1) IOPORT1 = 0x38&0x20;
    else if (ku == 2) IOPORT1 = 0x38&0x10;
    else if (ku == 3) IOPORT1 = 0x38&0x30;
  }
  _di_();
    SBUF_TX1 = 0x12;
  IOPORT1 |= 0x40;
  for (i=0; i< delay_1ms; i++)
  {
    WSR = 0x00;
    TIMER2 = 0xFB1D;
    WSR = 0x00;
    tmp = IOS1;
    while((tmp&0x10) == 0)
    {
      tmp = IOS1;
    }
  }
    SBUF_TX1 = 0x13;
  for (i = 1; i<=512; i++)
  {
    ADC_CON = 0x01 | (12 << 3); //канал для измерения тока DNT
    while((ADC_RESULT) & 0x8000);
    if (i%32 == 0)
    {
      MKO_tr_data[i] = ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
    }
    else
    {
      MKO_tr_data[i+32] = ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
    }
  }
  _ei_();
  IOPORT1 &= 0x3F;
}

