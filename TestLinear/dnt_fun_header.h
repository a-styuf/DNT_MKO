#include "types.h"
#ifndef _DNT_FUN_HEADER_H_
#define _DNT_FUN_HEADER_H_

// #pragma pack(1)  не работает с компилятором от производителя для МК 1874ВЕ7Т

typedef struct //кадр с данными ДНТ ПА30 !!! нельзя делать 32-битные поля, иначе потом есть проблемы с выкладыванием на подадрес !!!
{
    uint16_t label;  //+0
    uint16_t definer; //+2 
    uint16_t num; //+4
    uint16_t time_hi; //+6
    uint16_t time_lo; //+8
    //
    uint16_t current; //+10
    uint16_t temperature; //+12
    uint16_t shut_off_grid_voltage; //+14 
	uint16_t signal; //+16
	uint16_t zero; //+18
    uint16_t dnt_state; //+20
    //резерв
	uint16_t reserved[20]; //+22
	//обязательная часть - контрольная сумма
    uint16_t crc16; //+62
}typeDNTFrame;

typedef struct //данные для работы с током
{
	float value_amper;
	int16_t value;
	int32_t value_summ_buffer;
	uint16_t value_summ_number;
	int16_t value_max;
	int16_t value_min;
	int16_t value_arr[4];
	int32_t value_summ_buffer_arr[4];
	uint16_t value_summ_number_arr[4];
	int16_t value_max_arr[4];
	int16_t value_min_arr[4];
}typeDNTCurrent;

typedef struct //данные для работы с напряжением запорной сетки
{
	uint16_t value;
	int32_t value_summ_buffer;
	uint16_t desired_value;
	uint8_t duty;
}typeDNTShutOffGridVoltage;

typedef struct //данные для работы с температурой
{
	uint16_t value; //актуальное значение температуры
	int32_t value_summ_buffer;
	uint16_t last_calibration_value; //значение температуры последней калибровки
	uint16_t calibration_step; //порог изменения температуры при котором необходима калибровка
}typeDNTTemperature;

typedef struct //флаги и параметры для управления работой ДНТ
{
	uint16_t mode; //флаги для управления режимами: 0x01 - осциллограмма, 0х02 - единичный запуск измерения, 0х04 - цикличные измерения, 0х10 - режим констант, 0х20 - режим отладки
	uint16_t measure_leng_s; //длительность одиночного измерения
	uint16_t dead_time_ms; //время после переключения КУ или калибровки, которое исключается из подсчета тока
	uint16_t ku; //коэффициент усиления: 0-1, 1-10, 2-100, 3-1000
	uint32_t measure_cycle_time_ms;
	//переменные для циклического режима
	uint32_t zero_dead_time_ref_point, zero_cycle_ref_point, signal_dead_time_ref_point, signal_cycle_ref_point; //границы различных измерительных режимов
	//переменные для единичного запуска
	uint32_t single_ref_points[16]; // границы для единиченого режима
	//
	uint16_t osc_ku; //коэффициент усиления: 0-1, 1-10, 2-100, 3-1000
	uint16_t osc_mode; //тип измерения: 1 - сигнал, 0 - калибровочный ноль
	//
	uint8_t pwm_duty;
	//
	uint32_t temp_cycle_time_ms;
	uint32_t grid_voltage_cycle_time_ms;

}typeDNTControl;

typedef struct //оперативные рабочие данные ДНТ
{
	uint32_t time;
	uint16_t frame_num;
	//
	int16_t current_result; //разность значений current_signal и zero_signal, но выделяю отдельно, т.к. по ней проводится куча проверок
	//
	uint8_t frame_modificator, device_number, zavod_number;
	//
	typeDNTCurrent signal;
	typeDNTCurrent zero;
	typeDNTShutOffGridVoltage shut_off_grid;
	typeDNTTemperature temperature;
	//
	typeDNTControl control;
	//
}typeDNTOperationData;

void DNT_Frame_Init(typeDNTFrame *dnt_data_frame_ptr, typeDNTOperationData* dnt_ptr);
void DNT_Frame_Create(typeDNTFrame *dnt_data_frame_ptr, typeDNTOperationData* dnt_ptr);
void DNT_Parameters_Init(typeDNTOperationData* dnt_ptr, uint16_t meas_leng_s, uint16_t dead_time_ms, uint16_t mode);
void DNT_Frame_Definer_Init(typeDNTOperationData* dnt_ptr, uint8_t dev_type, uint8_t zav_num);
void Update_MKO_from_DNT_Parameters(typeDNTOperationData* dnt_ptr);
uint16_t _get_frame_definer(typeDNTOperationData* dnt_ptr, uint8_t frame_type);
//current measurement
uint8_t Current_Calc_Step_10ms(typeDNTOperationData* dnt_ptr); //некая  функция, которая должна запускать один раз в 1 мс, для осуществления итеративного процесса подсчета тока
uint8_t ku_gpio_set(uint16_t ku); //функция для преобразования КУ в маску для GPIO
//cyclic measurement
void _cycle_measure_struct_init(typeDNTOperationData* dnt_ptr);
void _adc_data_get(typeDNTCurrent *signal_ptr);
void _current_result_calc(typeDNTOperationData* dnt_ptr);
void _ku_change_checker(typeDNTOperationData* dnt_ptr);
//single measurement
void _single_measure_struct_init(typeDNTOperationData* dnt_ptr);
void _single_meas_adc_data_get(typeDNTCurrent *signal_ptr, uint8_t ku);
void _single_meas_current_result_calc(typeDNTOperationData* dnt_ptr);
//temperature measurment
uint8_t Temp_Calc_Step_10ms(typeDNTOperationData* dnt_ptr);
//grid measurement
uint8_t Grid_Voltage_Calc_Step_10ms(typeDNTOperationData* dnt_ptr);
uint8 _grid_voltage_duty_feedback (uint16 MON_50V, uint8 Duty);
//general purpose
int32_t _check_bounds(int32_t var, int32_t bound_min, int32_t bound_max);
void uint16_buffer_rev_to_uint8_buffer(uint16_t *u16_buff, uint8_t * u8_buff, uint8_t u16_len);

uint16 COMAnsForm (uint8 req_id, uint8 self_id, uint8 sub_adress, uint8 *seq_num, uint8 com, uint8 leng, uint8* com_data, uint8* ans_com);

#endif
