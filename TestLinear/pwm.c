#include "pwm_header.h"

void PWM_Init()
{
  WSR = 0x01;
  PWM1_CONTROL = 0;
  WSR = 0x00;
}

void PWM_Duty(uint8 Duty)
{
  if (Duty > 128) Duty = 128;
  //else if(Duty < 5) Duty = 5;
  WSR = 0x01;
  PWM1_CONTROL = Duty;
  WSR = 0x00;
}
