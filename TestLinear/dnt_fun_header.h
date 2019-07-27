#include "types.h"
#ifndef _DNT_FUN_HEADER_H_
#define _DNT_FUN_HEADER_H_

void PWM_Init();
void PWM_Duty (uint8 Duty);
uint8 PWM_FB (uint16 MON_50V, uint8 Duty); // значение передается в 10   мВ
void Timer_Init();
uint16 COMAnsForm (uint8 req_id, uint8 self_id, uint8 sub_adress, uint8 *seq_num, uint8 com, uint8 leng, uint8* com_data, uint8* ans_com);
int16 CurrentCalculation(int16* dnt_result, int16* dnt_max, int16* dnt_min, uint16 *status);

#endif
