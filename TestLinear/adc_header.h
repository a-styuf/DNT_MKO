#include "types.h"
#ifndef _ADC_HEADER_H_
#define _ADC_HEADER_H_

void ADC_Init();
uint16_t adc_value_get();
uint16_t grid_voltage_adc_value_get();
uint16_t grid_voltage_adc_to_voltage(int16_t adc_data);
uint16_t temperature_adc_value_get();
uint16_t temperature_adc_to_degree(int16_t adc_data);
void Oscilloscope(uint16 ku, uint16 mode, uint16 delay_1ms);

#endif

