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

uint16_t adc_value_get()
{
  ADC_CON = 0x01 | (12 << 3); //канал для измерения тока DNT
  while((ADC_RESULT) & 0x8000);
  return ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
}

uint16_t grid_voltage_adc_value_get()
{
    ADC_CON = 0x01 | (4 << 3); //канал для измерения тока DNT
    while((ADC_RESULT) & 0x8000);
    return ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
}

uint16_t grid_voltage_adc_to_voltage(int16_t adc_data)
{
    return (uint16_t)(adc_data*ALPHA+BETTA);
}

uint16_t temperature_adc_value_get()
{
    ADC_CON = 0x01 | (2 << 3); //канал для измерения тока DNT
    while((ADC_RESULT) & 0x8000);
    return ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
}

uint16_t temperature_adc_to_degree(int16_t adc_data)
{
    float mean;
    float a = 8.19E-6, b = -9.445E-2, c= 2.039E+2;
    mean = (a*adc_data*adc_data+b*adc_data+c)*256.;
    return (uint16)(floor(mean));
}

void Oscilloscope(uint16 ku, uint16 mode, uint16 delay_10ms) //блокирующая функция для вычитки АЦП
{
    uint16 i;
    uint8 tmp ;
    uint16 *MKO_tr_data = (uint16*)0x1000;
    SBUF_TX1 = 0x11;
    if (mode == 0)  {
        if (ku == 0) IOPORT1 = 0x38&0x08;
        else if (ku == 1) IOPORT1 = 0x38&0x28;
        else if (ku == 2) IOPORT1 = 0x38&0x18;
        else if (ku == 3) IOPORT1 = 0x38&0x38;
    }
    else    {
        if (ku == 0) IOPORT1 = 0x38&0x08;
        else if (ku == 1) IOPORT1 = 0x38&0x20;
        else if (ku == 2) IOPORT1 = 0x38&0x10;
        else if (ku == 3) IOPORT1 = 0x38&0x30;
    }
    _di_();
    SBUF_TX1 = 0x12;
    IOPORT1 |= 0x40;
    for (i=0; i< delay_10ms; i++)    {
        WSR = 0x00;
        TIMER2 = 0xFB1D;
        WSR = 0x00;
        tmp = IOS1;
        while((tmp&0x10) == 0)  {
            tmp = IOS1;
        }
    }
    SBUF_TX1 = 0x13;
    for (i = 1; i<=512; i++)
    {
        ADC_CON = 0x01 | (12 << 3); //канал для измерения тока DNT
        while((ADC_RESULT) & 0x8000);
        if (i%32 == 0){
            MKO_tr_data[i] = ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
        }
        else {
            MKO_tr_data[i+32] = ((ADC_RESULT&0x3FFF)-0x2000) & 0x3FFF;
        }
    }
    _ei_();
    IOPORT1 &= 0x3F;
    SBUF_TX1 = 0x12;
}

