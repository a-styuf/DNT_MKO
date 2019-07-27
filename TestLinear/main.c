#include <string.h>
#include "types.h"
#include "uart_header.h"
#include "adc_header.h"
#include "crc16_header.h"
#include "mko_ud_header.h"
#include "dnt_fun_header.h"

#define ID 0xF0
#define mcu_freq 20E6  //задаем частоту кварцевого генератора

uint16 *MKO_tr_data_1 = (uint16*)0x1000;
uint16 Result_50V, j=0, k=0, m=0, n=0, Result_50V_arr[8], Result_50V_mean, average_num = 1, DNT_data[32] = {0}, osc_ku=0, osc_mode = 0, dead_time = 100, dnt_status = 0, dnt_status_old = 0;
int16 DNT_result, DNT_cal_result, DNT_result_arr[8], time_out, main_current = 0x0000;
int16 DNT_max_arr[8]={0}, DNT_min_arr[8] = {0}, temp_var, DNT_max, DNT_min;
int16 Result_Temp = 0, max, min;
int32 adc_var=0, adc_var_2=0, DNT_zero=0;
uint32 dnt_time = 0;
char str[64] = {0};
uint8 len = 0, mko_data_flag  = 0, mko_activity_flag = 0, mko_activity_bound = 10;
uint16 i, num_var=0, start = 0, stop = 1, var = 0;
uint8 duty_val = 0, duty_ena = 1, dnt_ku = 0x00;
uint8 timer_flag_1 = 0, timer_flag_2 = 0, pwm_flag = 0;
uint16 TIMER_10MS = 0, period = 6, mc_time = 1; //period = время для записи в таймер в 10-х мс, mc_time - время в секундах полного цикла измерения, псевдорегистр для задания время для таймера
uint16 TIMER2_1MS = 0, adc_time = 5; //шаг 1 мс, псевдорегистр для задания время для таймера
extern uint8 flag_rx;
//переменные для формирования ответа в UART
uint8 seq_num = 0;
uint8 leng = 0;
uint8 req_id = 0x00; // номер, на который необходимо отвечать
char answer[64] = {0};
char com_data[32] = {0};

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
  TIMER1 = 0xCF9C;
  WSR = 0x00;
  if (TIMER_10MS != 0)
  {
    TIMER_10MS -= 1;
    timer_flag_1 = 0;
  }
  else
  {
    timer_flag_1 = 1;
    //SBUF_TX1 = 0xF1;
  }
  }
  if(tmp&0x10)
  {
  WSR = 0x00;
  TIMER2 = 0xFB1D;
  WSR = 0x00;
  if (TIMER2_1MS != 0)
  {
    TIMER2_1MS -= 1;
    timer_flag_2 = 0;
  }
  else
  {
    timer_flag_2 = 1;
    dnt_time += 1;
    //SBUF_TX1 = num_var&0xFF;
  }
  }
  _ei_();
}

#pragma interrupt 9 IRQ_MKO_UD //обработка прерываний МКО
void  IRQ_MKO_UD()
{
    _di_();
    SBUF_TX1 = 0xFF;
    mko_activity_flag = mko_activity_bound;
    _ei_();
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

uint8 Timer1ms_Flag()
{
  if (timer_flag_2 != 0)
  {
    timer_flag_2 = 0;
    return 1;
  }
  return 0;
}

void main()
{
    UART_Init();
    ADC_Init();
    PWM_Init();
    Timer_Init();
    MKO_UD_Init();
    IOPORT1 = 0x00;
    TIMER_10MS = 100;
    TIMER2_1MS = 100;
    SBUF_TX1 = 0x01;
    memset(DNT_data, 0xFA, 64);
    MKO_data_to_transmit(DNT_data, 30);
    MKO_receive_data_change(DNT_data, 29);
    //////////////////////////
    while(1)
    {
        ///////обработка срабатывания таймера раз в 1 сек//////////////////
        if (Timer10ms_Flag() == 1)
        {
            //SBUF_TX1 = 0xEE;
            /////Опрос 50 вольт//////////////////
            ADC_50V(&Result_50V, 16);//результат в мВ
            Result_50V_arr[i] = Result_50V;
            Result_50V_mean = (Result_50V_arr[0] +Result_50V_arr[1])/2;
            /////Опрос температуры//////////////////
            ADC_Pt1000(&Result_Temp, 32);
            ///Опрос ДНТ//////
            DNT_max_arr[i] = DNT_max;
            DNT_min_arr[i] = DNT_min;
            _di_();
            if (num_var == 0) num_var = 1;
            DNT_result = (adc_var/num_var);
            DNT_max =  0;
            DNT_min = 17000;
            adc_var = 0;
            num_var = 0;
            time_out = 0;
            _ei_();
            if (i==0)
            {
                DNT_result_arr[0+dnt_status&0x0F] = DNT_result;
                //DNT_result_arr[0] = DNT_result;
                i = 1;
            }
            else
            {
                if (dnt_status_old != dnt_status) //данное условие помогает правильно усреднить за две точки калибровочный нуль
                {
                     DNT_result_arr[4+dnt_status&0x0F] = DNT_result;
                }
                else
                {
                    DNT_result_arr[4+dnt_status&0x0F] = (DNT_result+DNT_cal_result)/2;
                }
                DNT_cal_result = DNT_result;
                //DNT_result_arr[4] = DNT_result;
                i = 0;
                mko_data_flag     = 1;
                dnt_status_old = dnt_status;
                main_current = CurrentCalculation(DNT_result_arr, DNT_max_arr, DNT_min_arr, &dnt_status);
            }
            // формирование КУ
            if (dnt_status == 0) dnt_ku = 0x00;
            else if (dnt_status == 1) dnt_ku = 0x20;
            else if (dnt_status == 2) dnt_ku = 0x10;
            else if (dnt_status == 3) dnt_ku = 0x30;
            else dnt_ku = 0x00;

            if  (i == 0)
            {
                IOPORT1 = 0x38&(0x08|dnt_ku);
                period = ((mc_time*100)/2);
            }
            else if (i == 1)
            {
                IOPORT1 = 0x38&(0x00|dnt_ku);
                period = ((mc_time*100)/2);
            }
              //работа с исчточником запорного напряжения
            if (i == 1)
            {
                if (pwm_flag == 0)
                {
                    if (duty_ena == 1) duty_val=PWM_FB(Result_50V_mean, duty_val); //подстраиваем под -51.2 В
                }
                PWM_Duty(duty_val);
            }
            TIMER_10MS = period;//задание интервала опроса
            //SBUF_TX1 = 0xEE;
        }
        if (Timer1ms_Flag() == 1)
        {
            TIMER2_1MS = adc_time;
            _di_();
            if (time_out*adc_time > dead_time)
            {
                temp_var = ADC_DNT_summ();
                if (mko_activity_flag == 0)
                    {
                        adc_var += temp_var;
                        num_var++;
                        if (DNT_max < temp_var) DNT_max = temp_var;
                        if (DNT_min > temp_var) DNT_min = temp_var;
                    }
                 else SBUF_TX1 = mko_activity_flag;
            }
            else
            {
                temp_var = ADC_DNT_summ();
                time_out++;
            }
            if (mko_activity_flag != 0) mko_activity_flag--;
            //SBUF_TX1 = time_out&0xFF;
            _ei_();
        }
        //// Работа с внешними интерфейсами ////
        // Работа с МКО //
        if (mko_data_flag     == 1)
        {

            //SBUF_TX1 = 0x03;
            mko_data_flag     = 0; //обнуляем флаг для записи в МКО
            //общие данные
            DNT_data[0] = 0x0FF1;
            DNT_data[1] = 0x4820;
            DNT_data[2] = main_current;
            DNT_data[3] = Result_Temp;
            DNT_data[4] = Result_50V_mean;
            // данные с токового измерителя АЦП
            DNT_data[5] = DNT_result_arr[0];
            DNT_data[6] = DNT_result_arr[1];
            DNT_data[7] = DNT_result_arr[2];
            DNT_data[8] = DNT_result_arr[3];
            DNT_data[9] = DNT_result_arr[4];
            DNT_data[10] = DNT_result_arr[5];
            DNT_data[11] = DNT_result_arr[6];
            DNT_data[12] = DNT_result_arr[7];
            DNT_data[13] = dnt_status_old;
            DNT_data[14] = 0x0FF1;

            //складываем данные в память МКО
            MKO_data_to_transmit(DNT_data, 30);
            //проверям, необходимо ли менять параметры для работы ДНТ
            MKO_receive_data(DNT_data, 29);
            MKO_data_to_transmit(DNT_data, 29); //складываем в регистр чтения для удобства проверки записи данных
            if ((DNT_data[0] == 0x0FF1) & (DNT_data[1] == 0x4821)) // проверка на запись в субадрес
            {
                //SBUF_TX1 = 0xAA;
                mc_time = DNT_data[2];
                if (mc_time < 1) mc_time = 1;
                else if (mc_time > 20) mc_time = 20;
                adc_time = DNT_data[3];
                if (adc_time < 4) adc_time = 4;
                else if (adc_time > 20) adc_time = 20;
                dead_time = DNT_data[4];
                if (dead_time < 10) dead_time = 10;
                else if (dead_time > 200) dead_time = 200;
                if (dead_time > (mc_time*1000)/4) dead_time = (1000*mc_time)/4;
                osc_mode = DNT_data[5];
                osc_ku = DNT_data[6];
                if (DNT_data[7] != 0)
                {
                    Oscilloscope(osc_ku, osc_mode, dead_time);
                    DNT_data[7] = 0;
                    MKO_receive_data_change(DNT_data, 29);
                }
            }
            //SBUF_TX1 = 0x05;
        }
        // отладочный уарт
        if((len = UART1_RX(str)) != 0)
        {
            if (str[0] == ID)
            {
                if(str[4] == 0x00) // тестовая команда, дающая простой ответ от ДНТ (удобно использовать для проверки связи)
                {
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(str[4] == 0xF0)// команда для чтения показаний АЦП токового вход: 1 - показания с выключенным РЕЛЕ, 2-5 - показания с различными КУ (1-10-100-1000)
                {
                    com_data[0] = (DNT_result_arr[0]&0xFF00) >> 8;
                    com_data[1] = (DNT_result_arr[0]&0x00FF) >> 0;
                    com_data[2] = (DNT_result_arr[1]&0xFF00) >> 8;
                    com_data[3] = (DNT_result_arr[1]&0x00FF) >> 0;
                    com_data[4] = (DNT_result_arr[2]&0xFF00) >> 8;
                    com_data[5] = (DNT_result_arr[2]&0x00FF) >> 0;
                    com_data[6] = (DNT_result_arr[3]&0xFF00) >> 8;
                    com_data[7] = (DNT_result_arr[3]&0x00FF) >> 0;
                    com_data[8] = (DNT_result_arr[4]&0xFF00) >> 8;
                    com_data[9] = (DNT_result_arr[4]&0x00FF) >> 0;
                    com_data[10] = (DNT_result_arr[5]&0xFF00) >> 8;
                    com_data[11] = (DNT_result_arr[5]&0x00FF) >> 0;
                    com_data[12] = (DNT_result_arr[6]&0xFF00) >> 8;
                    com_data[13] = (DNT_result_arr[6]&0x00FF) >> 0;
                    com_data[14] = (DNT_result_arr[7]&0xFF00) >> 8;
                    com_data[15] = (DNT_result_arr[7]&0x00FF) >> 0;
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 16, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(str[4] == 0xF1)// команда для чтения показаний температурного датчика: 4-е байта: 0 - температура в °C, 1 - дробная часть температуры, 2,3 - сигнал с Pt1000 в 10мВ.
                {
                    com_data[0] = Result_Temp>>8; //todo пока нет калибровок, вывожу только напряжение с датчика в 10-х мВ
                    com_data[1] = Result_Temp>>0;
                    com_data[2] = 0x00;
                    com_data[3] = 0x00;
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x04, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(str[4] == 0xF2)// команда для чтения сводной информации по генератору -50В
                {
                    com_data[0] = (duty_ena&0xFF);
                    com_data[1] = (duty_val&0xFF);
                    com_data[2] = (Result_50V_mean&0xFF00) >> 8;
                    com_data[3] = (Result_50V_mean&0x00FF) >> 0;
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x04, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(str[4] == 0xF3)// вычитывание осцилограммы
                {
                    //заголовок
                    seq_num++;
                    Oscilloscope(osc_ku, osc_mode, dead_time);
                    answer[0] = 0x00;
                    answer[1] = 0xF0;
                    answer[2] = 0x00;
                    answer[3] = seq_num&0xFF;
                    answer[4] = 0xF3;
                    answer[5] = 0xFF;
                    UART1_TX(answer, 0x06);
                    TIMER2_1MS = 100;
                    for(k=0; k < 512; k++)
                    {
                        answer[0] = ((*(MKO_tr_data_1 + 32 + k))>>8)&0xFF;
                        answer[1] = ((*(MKO_tr_data_1 + 32 + k))>>0)&0xFF;
                        while (Timer1ms_Flag() != 1);
                        UART1_TX(answer, 0x02);
                        TIMER2_1MS = 1;
                    }
                }
                else if(str[4] == 0xF4)// вычитывание переменноя для хранения суммы измерений и числа измерений
                {
                    //заголовок
                    com_data[0] = ((adc_var)>>24)&0xFF;
                    com_data[1] = ((adc_var)>>16)&0xFF;
                    com_data[2] = ((adc_var)>>8)&0xFF;
                    com_data[3] = ((adc_var)>>0)&0xFF;
                    com_data[4] = (num_var&0xFF00)>>8;
                    com_data[5] = (num_var&0x00FF)>>0;
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x06, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(str[4] == 0xF5)//чятение статуса МКО
                {
                    var = MKO_State();
                    com_data[0] = (var>>8)&0xFF;
                    com_data[1] = (var>>0)&0xFF;
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x02, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(str[4] == 0xF6)//чятение подадреса МКО
                {
                    MKO_receive_data(DNT_data, 29);
                    for (k=0; k<16; k++)
                    {
                        com_data[2*k] = (DNT_data[k]&0xFF00)>>8;
                        com_data[2*k+1] = (DNT_data[k]&0xFF)>>0;
                    }
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x32, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(str[4] == 0xFB)// изменение постоянной времени измерения
                {
                    mc_time = (((str[6]&0xFF)<<8) + ((str[7]&0xFF)<<0))*100/8;
                    if (mc_time <= 0) mc_time = 1;
                    else if (mc_time >= 20) mc_time = 20;
                    adc_time = ((str[8]&0xFF)<<8) + ((str[9]&0xFF)<<0);
                    if (adc_time < 4) adc_time = 4;
                    else if (adc_time > 20) adc_time = 20;
                    dead_time = ((str[10]&0xFF)<<8) + ((str[11]&0xFF)<<0);;
                    if (dead_time < 10) dead_time = 10;
                    else if (dead_time > 200) dead_time = 200;
                    else if (dead_time > (mc_time*1000)/4) dead_time = (1000*mc_time)/4;
                    com_data[0] = (mc_time&0xFF00)>>8;
                    com_data[1] = mc_time&0xFF;
                    com_data[2] = (adc_time&0xFF00)>>8;
                    com_data[3] = adc_time&0xFF;
                    com_data[4] = (dead_time&0xFF00)>>8;
                    com_data[5] = dead_time&0xFF;
                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x06, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if (str[4] == 0xF7)//чтение максимумов и минимумов
                {
                    com_data[0] = ((DNT_max_arr[0])>>8)&0xFF;
                    com_data[1] = ((DNT_max_arr[0])>>0)&0xFF;
                    com_data[2] = ((DNT_max_arr[1])>>8)&0xFF;
                    com_data[3] = ((DNT_max_arr[1])>>0)&0xFF;
                    com_data[4] = ((DNT_min_arr[0])>>8)&0xFF;
                    com_data[5] = ((DNT_min_arr[0])>>0)&0xFF;
                    com_data[6] = ((DNT_min_arr[1])>>8)&0xFF;
                    com_data[7] = ((DNT_min_arr[1])>>0)&0xFF;

                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x08, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if (str[4] == 0xF8)//чтение максимумов и минимумов
                {
                    mko_activity_bound = (((str[6]&0xFF)<<8) + ((str[7]&0xFF)<<0));

                    leng = COMAnsForm(req_id, ID, str[2], &seq_num, str[4], 0x02, com_data, answer);
                    UART1_TX(answer, leng);
                }
            }
        }
    }
}

