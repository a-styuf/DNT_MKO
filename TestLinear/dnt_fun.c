
#include <string.h>
#include <stdlib.h>
#include "dnt_fun_header.h"
#include "crc16_header.h"
#include "mko_ud_header.h"
#include "adc_header.h"
#include "uart_header.h"
#include "pwm_header.h"

#define log10_2 0.30103
#define max(A, B) ((A) > (B) ? (A) : (B))
#define min(A, B) ((A) < (B) ? (A) : (B))

typeDNTFrame dnt_data_frame;
typeDNTOperationData dnt;

//Кадр с данными ДНТ
void DNT_Frame_Init(typeDNTFrame *dnt_data_frame_ptr, typeDNTOperationData* dnt_ptr)
{
  //форматируем весь кадр значением 0xFE
    memset((uint8_t*)dnt_data_frame_ptr, 0xFE, sizeof(typeDNTFrame));
  //обязательная часть кадра
  dnt_data_frame_ptr->label = 0x0FF1;
  dnt_data_frame_ptr->definer = _get_frame_definer(dnt_ptr, 0);
  dnt_data_frame_ptr->num = 0x0000;
  dnt_data_frame_ptr->time_hi = 0x0000;
  dnt_data_frame_ptr->time_lo = 0x0000;
  //данные
  dnt_data_frame_ptr->current = 0x0000;
  dnt_data_frame_ptr->temperature = 0x0000;
  dnt_data_frame_ptr->shut_off_grid_voltage = 0x0000;
  dnt_data_frame_ptr->signal = 0x0;
  dnt_data_frame_ptr->zero = 0x0;
  dnt_data_frame_ptr->dnt_state = 0x0;
  //обязательная часть кадра - контрольная сумма
  dnt_data_frame_ptr->crc16 = crc16_ccitt((uint8_t*)dnt_data_frame_ptr, 62);
}

void DNT_Frame_Create(typeDNTFrame *dnt_data_frame_ptr, typeDNTOperationData* dnt_ptr)
{

    dnt_data_frame_ptr->definer = _get_frame_definer(dnt_ptr, 0);
    dnt_ptr->frame_num ++;
    //заполняем данные для кадра
    dnt_data_frame_ptr->num = dnt_ptr->frame_num;
    dnt_data_frame_ptr->time_hi = (dnt_ptr->time >> 16);
    dnt_data_frame_ptr->time_lo = dnt_ptr->time & 0xFFFF;
    //данные
    dnt_data_frame_ptr->current = dnt_ptr->current_result;
    dnt_data_frame_ptr->temperature = dnt_ptr->temperature.value;
    dnt_data_frame_ptr->shut_off_grid_voltage = dnt_ptr->shut_off_grid.value;
    dnt_data_frame_ptr->signal = dnt_ptr->signal.value;
    dnt_data_frame_ptr->zero = dnt_ptr->zero.value;
    dnt_data_frame_ptr->dnt_state = dnt_ptr->control.ku & 0x03;
    //обязательная часть кадра - контрольная сумма
    dnt_data_frame_ptr->crc16 = crc16_ccitt((uint8_t*)dnt_data_frame_ptr, 62);
    //
    MKO_data_to_transmit((uint16_t*)dnt_data_frame_ptr, 30); //складываем данные на 30й подадрес
}

void Update_DNT_Prameters_from_MKO(typeDNTOperationData* dnt_ptr)
{
  uint16_t dnt_data[32] = {0};
  //
  MKO_receive_data(dnt_data, 29);

  //
  if ((dnt_data[0] == 0x0FF1) & (dnt_data[1] == _get_frame_definer(dnt_ptr, 1))) // проверка на запись в субадрес
  {
    dnt_ptr->control.measure_leng_s = (uint16_t)_check_bounds(dnt_data[2], 1, 20);

    dnt_ptr->control.dead_time_ms = (uint16_t)_check_bounds(dnt_data[3], 20, (dnt_ptr->control.measure_leng_s*1000/4));
    dnt_ptr->control.osc_mode = dnt_data[5] & 0x01;
    dnt_ptr->control.osc_ku = dnt_data[6] & 0x3;
    //
    dnt_ptr->control.mode = dnt_data[6] & 0xFF;
    if (dnt_data[7]&0x01 != 0)
    {
      Oscilloscope(dnt_ptr->control.osc_ku, dnt_ptr->control.osc_mode, dnt_ptr->control.dead_time_ms);
      dnt_data[7] = 0;
      Update_MKO_from_DNT_Parameters(dnt_ptr);
    }
  }
}

void Update_MKO_from_DNT_Parameters(typeDNTOperationData* dnt_ptr)
{
  uint16_t dnt_data[32] = {0};
  dnt_data[0] = 0x0FF1;
  dnt_data[1] = _get_frame_definer(dnt_ptr, 1);
  dnt_data[2] = dnt_ptr->control.measure_leng_s;
  dnt_data[4] = dnt_ptr->control.dead_time_ms;
  dnt_data[5] = dnt_ptr->control.osc_mode;
  dnt_data[6] = dnt_ptr->control.osc_ku;
  dnt_data[7] = dnt_ptr->control.mode;
  MKO_data_to_transmit(dnt_data, 29); //складываем в регистр чтения для удобства проверки записи данных
}

void DNT_Parameters_Init(typeDNTOperationData* dnt_ptr, uint16_t meas_leng_s, uint16_t dead_time_ms, uint16_t mode)
{
    dnt_ptr->control.measure_leng_s = meas_leng_s;
    dnt_ptr->control.dead_time_ms = dead_time_ms;
    dnt_ptr->control.osc_mode = 0;
    dnt_ptr->control.osc_ku = 0;
    dnt_ptr->control.mode = mode;
}

void DNT_Frame_Definer_Init(typeDNTOperationData* dnt_ptr, uint8_t dev_type, uint8_t zav_num)
{
    dnt_ptr->frame_modificator = 0x01;
    dnt_ptr->device_number = dev_type;
    dnt_ptr->zavod_number = zav_num;
}

int32_t _check_bounds(int32_t var, int32_t bound_min, int32_t bound_max)
{
    int32_t uint32_tmp_var;
    uint32_tmp_var = min(var, bound_max);
    uint32_tmp_var = max(uint32_tmp_var, bound_min);
    return uint32_tmp_var;
}

//Подсчет тока ДНТ
uint8_t Current_Calc_Step_10ms(typeDNTOperationData* dnt_ptr) //некая  функция, которая должна запускать один раз в 1 мс, для осуществления итеративного процесса подсчета тока
{
    uint16_t i, type;
    typeDNTCurrent *signal_ptr;

    if (dnt_ptr->control.mode & 0x04){ //запустили циклический режим
        if (dnt_ptr->control.measure_cycle_time_ms == 0){ //инициализация переменных для подсчета тока
            _ku_change_checker(dnt_ptr);
            _cycle_measure_struct_init(dnt_ptr);
        }
        else{
            dnt_ptr->control.measure_cycle_time_ms += 10; //не гарантировано точные часы подсчета времени цикла
            //
            if ((dnt_ptr->control.measure_cycle_time_ms > 0) && (dnt_ptr->control.measure_cycle_time_ms < dnt_ptr->control.zero_cycle_ref_point)){ //определяем мертвое время для измерения нуля
                IOPORT1 = 0x38&(0x00|ku_gpio_set(dnt_ptr->control.ku)); //определяем состояние gpio и КУ для измерения  нуля
                if ((dnt_ptr->control.measure_cycle_time_ms >=  dnt_ptr->control.zero_dead_time_ref_point)){ //измерение нуля
                     _adc_data_get(&dnt_ptr->zero);
                }
            }
            else if ((dnt_ptr->control.measure_cycle_time_ms >= dnt_ptr->control.zero_cycle_ref_point) && (dnt_ptr->control.measure_cycle_time_ms < dnt_ptr->control.signal_cycle_ref_point)){ //мертвое время измерения сигнала
                IOPORT1 = 0x38&(0x08|ku_gpio_set(dnt_ptr->control.ku)); //определяем состояние gpio и КУ для измерения сигнала
                if ((dnt_ptr->control.measure_cycle_time_ms >= dnt_ptr->control.signal_dead_time_ref_point)){ //измерение сигнала
                    _adc_data_get(&dnt_ptr->signal);
                }
            }
            else if (dnt_ptr->control.measure_cycle_time_ms >= dnt_ptr->control.signal_cycle_ref_point){
                _current_result_calc(dnt_ptr);
                dnt_ptr->control.measure_cycle_time_ms = 0;
				IOPORT1 = 0x38&(0x00|ku_gpio_set(dnt_ptr->control.ku)); //сбрасываем ток с обмотки реле
                //Update_MKO_from_DNT_Parameters(dnt_ptr);
                return 1; //передаем сигнал о готовности измерения
            }
        }
    }
    else if (dnt_ptr->control.mode & 0x02){ //запустили одиночное измерение
        if (dnt_ptr->control.measure_cycle_time_ms == 0){ //инициализация переменных для подсчета тока
            _single_measure_struct_init(dnt_ptr);
			SBUF_TX1 = 0x02;
        }
        else{
            dnt_ptr->control.measure_cycle_time_ms += 10; //не гарантировано точные часы подсчета времени цикла
            //
            for(i=0; i<8; i++){
                if (i<4) {
                    type=0x00;
                    signal_ptr = &dnt_ptr->zero;
                }
                else {
                    type=0x08;
                    signal_ptr = &dnt_ptr->signal;
                }
                if (dnt_ptr->control.measure_cycle_time_ms < dnt_ptr->control.single_ref_points[2*i]) {
                    IOPORT1 = 0x38&(type|ku_gpio_set(i&0x03)); //определяем состояние gpio и КУ для измерения  нуля
					break;
                }
                else if(dnt_ptr->control.measure_cycle_time_ms < dnt_ptr->control.single_ref_points[2*i+1]) {
                    _single_meas_adc_data_get(signal_ptr, i&0x3);
					break;
                }
            }
            if (dnt_ptr->control.measure_cycle_time_ms >= dnt_ptr->control.single_ref_points[15]) {
                _single_meas_current_result_calc(dnt_ptr);
                dnt_ptr->control.measure_cycle_time_ms = 0;
                dnt_ptr->control.mode &= ~(0x02); //сбрасываем флаг единичного запуска
                Update_MKO_from_DNT_Parameters(dnt_ptr); //прописываем флаг на ПА
				IOPORT1 = 0x38&(0x00|ku_gpio_set(dnt_ptr->control.ku)); //сбрасываем ток с обмотки реле
				SBUF_TX1 = 0x03;
                return 1; //передаем сигнал о готовности измерения
            }
        }
    }
    return 0;
}

uint8_t Temp_Calc_Step_10ms(typeDNTOperationData* dnt_ptr)
{
    if (dnt_ptr->control.temp_cycle_time_ms < 128){
        dnt_ptr->temperature.value_summ_buffer += temperature_adc_value_get();
        dnt_ptr->control.temp_cycle_time_ms ++;
    }
    else if (dnt_ptr->control.temp_cycle_time_ms >= 128){
        dnt_ptr->temperature.value = temperature_adc_to_degree((uint16_t)(dnt_ptr->temperature.value_summ_buffer>>7));
        dnt_ptr->temperature.value_summ_buffer = 0;
        dnt_ptr->control.temp_cycle_time_ms = 0;
    }
    return 0;
}

uint8_t Grid_Voltage_Calc_Step_10ms(typeDNTOperationData* dnt_ptr)
{
    if (dnt_ptr->control.grid_voltage_cycle_time_ms < 128){
        dnt_ptr->shut_off_grid.value_summ_buffer += grid_voltage_adc_value_get();
        if (dnt_ptr->control.grid_voltage_cycle_time_ms == 0){
            dnt_ptr->shut_off_grid.duty = _grid_voltage_duty_feedback (dnt_ptr->shut_off_grid.value, dnt_ptr->shut_off_grid.duty);
            PWM_Duty(dnt_ptr->shut_off_grid.duty);
        }
        dnt_ptr->control.grid_voltage_cycle_time_ms ++;
    }
    else if (dnt_ptr->control.grid_voltage_cycle_time_ms >= 128){
        dnt_ptr->shut_off_grid.value = grid_voltage_adc_to_voltage((uint16_t)(dnt_ptr->shut_off_grid.value_summ_buffer>>7));
        dnt_ptr->shut_off_grid.value_summ_buffer = 0;
        dnt_ptr->control.grid_voltage_cycle_time_ms = 0;
    }
    return 0;
}

uint8_t ku_gpio_set(uint16_t ku) //функция для преобразования КУ в маску для GPIO
{
  if (ku == 0) return 0x00;
  else if (ku == 1) return 0x20;
  else if (ku == 2) return 0x10;
  else if (ku == 3) return 0x30;
  else return 0x00;
}

void _cycle_measure_struct_init(typeDNTOperationData* dnt_ptr)
{
  dnt_ptr->control.measure_cycle_time_ms = 1;
  dnt_ptr->control.zero_dead_time_ref_point = dnt_ptr->control.dead_time_ms;
  dnt_ptr->control.zero_cycle_ref_point = (dnt_ptr->control.measure_leng_s*1000/2) - 5;
  dnt_ptr->control.signal_dead_time_ref_point = dnt_ptr->control.dead_time_ms + (dnt_ptr->control.measure_leng_s*1000/2) - 10;
  dnt_ptr->control.signal_cycle_ref_point = dnt_ptr->control.measure_leng_s*1000 - 10; //оставляем запас по вермени в 20мс для того, что бы успеть обработать данные
  //
  dnt_ptr->signal.value_summ_buffer = 0;
  dnt_ptr->signal.value_summ_number = 0;
  dnt_ptr->signal.value_max = 0;
  dnt_ptr->signal.value_min = 17000;
  //
  dnt_ptr->zero.value_summ_buffer = 0;
  dnt_ptr->zero.value_summ_number = 0;
  dnt_ptr->zero.value_max = 0;
  dnt_ptr->zero.value_min = 17000;
}

void _single_measure_struct_init(typeDNTOperationData* dnt_ptr)
{
    uint8_t i;
    uint16_t short_dead_time, long_short_time, single_cycle_time_s;
    single_cycle_time_s = max(2, dnt_ptr->control.measure_cycle_time_ms); //выбираем время цикла, не меньше 2 сек
    short_dead_time = max(single_cycle_time_s*1000/32, dnt_ptr->control.dead_time_ms); //выбираем мертвое время для КУ1, 10, но не меньше половини от измирительного цикла
    long_short_time = max(single_cycle_time_s*1000/16, dnt_ptr->control.dead_time_ms); //выбираем мертвое время для КУ1, 10, но не меньше половини от измирительного цикла
    //
    dnt_ptr->control.measure_cycle_time_ms = 1;
    //
    for(i=0; i<2; i++){
        dnt_ptr->control.single_ref_points[0+i*8] = i*8*single_cycle_time_s*1000/16 + short_dead_time;
        dnt_ptr->control.single_ref_points[1+i*8] = i*8*single_cycle_time_s*1000/16 + single_cycle_time_s*1000/16;
        dnt_ptr->control.single_ref_points[2+i*8] = i*8*single_cycle_time_s*1000/16 + single_cycle_time_s*1000/16 + short_dead_time;
        dnt_ptr->control.single_ref_points[3+i*8] = i*8*single_cycle_time_s*1000/16 + 2*single_cycle_time_s*1000/16;
        dnt_ptr->control.single_ref_points[4+i*8] = i*8*single_cycle_time_s*1000/16 + 2*single_cycle_time_s*1000/16 + short_dead_time;
        dnt_ptr->control.single_ref_points[5+i*8] = i*8*single_cycle_time_s*1000/16 + 4*single_cycle_time_s*1000/16;
        dnt_ptr->control.single_ref_points[6+i*8] = i*8*single_cycle_time_s*1000/16 + 4*single_cycle_time_s*1000/16 + long_short_time;
        dnt_ptr->control.single_ref_points[7+i*8] = i*8*single_cycle_time_s*1000/16 + 8*single_cycle_time_s*1000/16;
    }
    //
    for(i=0; i<4; i++){
        dnt_ptr->signal.value_summ_buffer_arr[i] = 0;
        dnt_ptr->signal.value_summ_number_arr[i] = 0;
        dnt_ptr->signal.value_max_arr[i] = 0;
        dnt_ptr->signal.value_min_arr[i] = 17000;
        //
        dnt_ptr->zero.value_summ_buffer_arr[i] = 0;
        dnt_ptr->zero.value_summ_number_arr[i] = 0;
        dnt_ptr->zero.value_max_arr[i] = 0;
        dnt_ptr->zero.value_min_arr[i] = 17000;
    }
}

void _adc_data_get(typeDNTCurrent *signal_ptr)
{
    uint16_t adc_var;
    _di_();
    adc_var =  adc_value_get();
    _ei_();
    signal_ptr->value_summ_buffer += adc_var;
    signal_ptr->value_summ_number += 1;
    signal_ptr->value_max = max(signal_ptr->value_max, adc_var);
    signal_ptr->value_min = min(signal_ptr->value_min, adc_var);
}

void _current_result_calc(typeDNTOperationData* dnt_ptr)
{
    if ((dnt_ptr->signal.value_summ_number == 0) || (dnt_ptr->zero.value_summ_number == 0)) {
        dnt_ptr->current_result = 0x0000;
        dnt_ptr->signal.value = 0x0000;
        dnt_ptr->zero.value = 0x0000;
    }
    else{
        dnt_ptr->signal.value = (int16_t)(dnt_ptr->signal.value_summ_buffer/dnt_ptr->signal.value_summ_number);
        dnt_ptr->zero.value = (int16_t)(dnt_ptr->zero.value_summ_buffer/dnt_ptr->zero.value_summ_number);
        dnt_ptr->current_result = dnt_ptr->signal.value - dnt_ptr->zero.value;
    }
}

void _single_meas_adc_data_get(typeDNTCurrent *signal_ptr, uint8_t ku)
{
    uint16_t adc_var;
    _di_();
    adc_var =  adc_value_get();
    _ei_();
    signal_ptr->value_summ_buffer_arr[ku] += adc_var;
    signal_ptr->value_summ_number_arr[ku] += 1;
    signal_ptr->value_max_arr[ku] = max(signal_ptr->value_max_arr[ku], adc_var);
    signal_ptr->value_min_arr[ku] = min(signal_ptr->value_max_arr[ku], adc_var);
}

void _single_meas_current_result_calc(typeDNTOperationData* dnt_ptr)
{
    uint8_t i;
    for (i=3; i<=0; i--){
        if ((dnt_ptr->signal.value_max_arr[i]  <=  16000) || (dnt_ptr->zero.value_max_arr[i]  <= 16000)) //проверяем, подходит ли нам данный КУ
        {
            if ((dnt_ptr->signal.value_summ_number == 0) || (dnt_ptr->zero.value_summ_number == 0)) {
                dnt_ptr->current_result = 0x0000;
                dnt_ptr->signal.value = 0x0000;
                dnt_ptr->zero.value = 0x0000;
            }
            else{
                dnt_ptr->signal.value = (int16_t)(dnt_ptr->signal.value_summ_buffer_arr[i]/dnt_ptr->signal.value_summ_number_arr[i]);
                dnt_ptr->zero.value = (int16_t)(dnt_ptr->zero.value_summ_buffer_arr[i]/dnt_ptr->zero.value_summ_number_arr[i]);
                dnt_ptr->current_result = dnt_ptr->signal.value - dnt_ptr->zero.value;
            }
            dnt.control.ku = i;
            break;
        }
    }
}


void _ku_change_checker(typeDNTOperationData* dnt_ptr)
{
    /* определение корректного ku */
    switch (dnt_ptr->control.ku)
    {
        case 3:
            if ((abs(dnt_ptr->signal.value_max) > 16000) || (abs(dnt_ptr->zero.value_max) > 16000) || (abs(dnt_ptr->current_result) > 8000)) dnt_ptr->control.ku = 2;
            break;
        case 2:
            if ((abs(dnt_ptr->signal.value_max) > 16000) || (abs(dnt_ptr->zero.value_max) > 16000) || (abs(dnt_ptr->current_result) > 8000)) dnt_ptr->control.ku = 1;
            else if ((abs(dnt_ptr->signal.value_min)  < 380) || (abs(dnt_ptr->zero.value_min) < 380) || (abs(dnt_ptr->current_result) < 320)) dnt_ptr->control.ku = 3;
            break;
        case 1:
            if ((abs(dnt_ptr->signal.value_max) > 16000) || (abs(dnt_ptr->zero.value_max) > 16000) || (abs(dnt_ptr->current_result) > 8000)) dnt_ptr->control.ku = 0;
            else if ((abs(dnt_ptr->signal.value_min)  < 380) || (abs(dnt_ptr->zero.value_min) < 380) || (abs(dnt_ptr->current_result) < 320)) dnt_ptr->control.ku = 2;
            break;
        case 0:
            if ((abs(dnt_ptr->signal.value_min) < 380) || (abs(dnt_ptr->zero.value_min) < 380) || (abs(dnt_ptr->current_result) < 320)) dnt_ptr->control.ku = 1;
            break;
        default:
            dnt_ptr->control.ku = 0;
            break;
    }
}

uint16_t _get_frame_definer(typeDNTOperationData* dnt_ptr, uint8_t frame_type)
{
    uint16_t tmp_var;
    tmp_var =   (uint16_t)((dnt_ptr->frame_modificator & 0x03) << 14) +   // модификатор кадра
                        (uint16_t)((dnt_ptr->device_number & 0x0F) << 10) +   // номер аппаратуры
                        (uint16_t)((dnt_ptr->zavod_number & 0x7F) << 3) +    // заводской номер
                        (uint16_t)(frame_type & 0x07);   // тип кадра
    return  tmp_var;
}

uint8 _grid_voltage_duty_feedback (uint16 MON_50V, uint8 Duty)
{
  if (MON_50V/256 > 54) return Duty-1;
  else if (MON_50V/256 < 50) return Duty+1;
}

void uint16_buffer_rev_to_uint8_buffer(uint16_t *u16_buff, uint8_t * u8_buff, uint8_t u16_len)
{
    uint16_t i;
    for(i=0; i<u16_len; i++){
        u8_buff[2*i + 0] = u16_buff[i] >> 8;
        u8_buff[2*i + 1] = u16_buff[i] >> 0;
    }
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

