#include "types.h"
#ifndef _ADC_HEADER_H_
#define _ADC_HEADER_H_

void ADC_Init();
//void ADC_DNT(uint16 *DNT_result, uint16 N);
void ADC_DNT(uint16 *DNT_result, int16 *adc_array, int16 *max, int16 *min, uint16 aver_num);
uint16 ADC_DNT_summ();
void ADC_50V(uint16 *Result_50V, uint16 N);
void ADC_Pt1000(int16 *DNT_Temp, uint16 N);
void Oscilloscope(uint16 ku, uint16 mode, uint16 delay_1ms);

#endif

