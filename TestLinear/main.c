/*
�� ��� ���������������� ���. ��� ��������� � ��������� �������:
  - ������� ��������� ��� �������� �������� ���, ������ � ������� ����������;
  -
  -
*/
#include <string.h>
#include <math.h>
#include "types.h"
#include "uart_header.h"
#include "adc_header.h"
#include "crc16_header.h"
#include "mko_ud_header.h"
#include "dnt_fun_header.h"
#include "timers_header.h"
#include "pwm_header.h"

#define ID 0xF0
/*----��������� �� ��������� ��� ��������� ����---*/
#define MEAS_LENG_S 1
#define DEAD_TIME_MS 100
#define MODE 0x02 //��������� ���������
/*---��������� ������----*/
#define DEVICE_TYPE 2 // 2 - ��� (������ ������ ��������� ����� �������)
#define ZAV_NUMBER 11 // 3 - ������, 4 - ����, 9-12 - �����, �.�.02-05

#define DELTA_T_DEG 0x0100 //������ ��� ��� ���������� �� ����������� � (1/256)�� - 1��
#define ZERO_CALIBRATION_TIMEOUT_S 30 //������ ��� ��� ������������ �������������� � ��������

extern typeDNTOperationData dnt;
extern typeDNTFrame dnt_data_frame;
extern uint8_t mko_activity_timeout, mko_read_flag;
extern uint16 TIMER_10MS;
extern uint16 *MKO_tr_data;

uint16_t i;
//���������� ��� ������������ ������ � UART
uint8_t seq_num = 0;
uint8_t leng = 0;
uint8_t req_id = 0x00; // �����, �� ������� ���������� ��������
//������ ��� ������ � ������� �� �������� ��� � UART
uint8_t answer[128] = {0};
uint8_t request[128] = {0};
uint8_t com_data[64] = {0};
uint16_t u16_buff[32] = {0};

void main()
{
    UART_Init();
    ADC_Init();
    PWM_Init();
    Timer_Init();
    MKO_UD_Init();
    IOPORT1 = 0x00;
    TIMER_10MS = 100;
    SBUF_TX1 = 0x01;
    //
    memset(u16_buff, 0xFA, 64);
    MKO_data_to_transmit(u16_buff, 30);
    MKO_receive_data_change(u16_buff, 29);
    //
    DNT_Parameters_Init(&dnt, MEAS_LENG_S, DEAD_TIME_MS, MODE);
    DNT_Frame_Definer_Init(&dnt, DEVICE_TYPE, ZAV_NUMBER);
    DNT_Frame_Init(&dnt_data_frame, &dnt);

    //////////////////////////
    while(1)
    {
        if (Timer10ms_Flag() == 1)
        {
            TIMER_10MS = 1;
            if (mko_activity_timeout != 0) {
                mko_activity_timeout--; //���������� ������ �����, ��� �� ��� �� ������� ������ � ���������
            }
            else{
                if (Current_Calc_Step_10ms(&dnt)) {// ���������� ����������� ������� ����, �� ��������� �������������� ������ �� �������� ���
                    DNT_Frame_Create(&dnt_data_frame, &dnt);
                }
                Grid_Voltage_Calc_Step_10ms(&dnt);
                Temp_Calc_Step_10ms(&dnt);
            }
            //
            if ((mko_read_flag != 0) && (dnt.control.measure_cycle_time_ms == 0)){ //��������� ���� �� ������� �� ������ �� ��� � ���������� �� ���� ���������, ���� ��� ����, �������������� ��������� ���
                mko_read_flag = 0;
                Update_DNT_Prameters_from_MKO(&dnt);
                //
                MKO_get_data_from_transmit_subaddr(u16_buff, 0x1D);
                uint16_buffer_rev_to_uint8_buffer(u16_buff, com_data, 32);
                leng = COMAnsForm(req_id, ID, 0x00, &seq_num, 0x1D, 64, com_data, answer);
                UART1_TX(answer, leng);
            }
        }
        //// ������ � �������� ������������ ////
        // ���������� ����
        if((leng = UART1_RX(request)) != 0)
        {
            if (request[0] == ID)
            {
                if(request[4] == 0x00) // �������� �������, ������ ������� ����� �� ��� (������ ������������ ��� �������� �����)
                {
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 0, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0x01)// ������� ��� ������ ����
                {
                    u16_buff[0] = dnt.current_result;
                    u16_buff[1] = dnt.signal.value;
                    u16_buff[2] = dnt.zero.value;
                    u16_buff[3] = dnt.control.ku;
                    uint16_buffer_rev_to_uint8_buffer(u16_buff, com_data, 4);
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 8, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0x02)// ������� ��� ������ ��������� �������������� �������
                {
                    u16_buff[0] = dnt.temperature.value;
                    uint16_buffer_rev_to_uint8_buffer(u16_buff, com_data, 1);
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 2, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0x03)// ������� ��� ������ ������� ���������� �� ���������� -50�
                {
                    u16_buff[0] = dnt.shut_off_grid.value;
                    uint16_buffer_rev_to_uint8_buffer(u16_buff, com_data, 1);
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 2, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0x04)// ����������� ���������� dnt �� ���������
                {
                    memcpy((uint8_t*)u16_buff, (uint8_t*)&dnt, 64);
                    uint16_buffer_rev_to_uint8_buffer(u16_buff, com_data, 32);
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 64, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0x05)// ������� ��� ������ ������ � ����������� �� ���
                {
                    MKO_get_data_from_transmit_subaddr(u16_buff, request[6]&0x1F);
                    uint16_buffer_rev_to_uint8_buffer(u16_buff, com_data, 32);
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 64, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0x06)// ������� ��� ������ ���������� ��� �� ���������
                {
                    u16_buff[0] = ZAV_NUMBER;
                    u16_buff[1] = DEVICE_TYPE;
                    u16_buff[2] = MEAS_LENG_S;
                    u16_buff[3] = DEAD_TIME_MS;
                    uint16_buffer_rev_to_uint8_buffer(u16_buff, com_data, 4);
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 64, com_data, answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0xF1)// ���������� �������
                {
                    dnt.control.mode = request[6];
                    dnt.control.measure_leng_s = request[7];
                    dnt.control.dead_time_ms = request[8];
                    Update_MKO_from_DNT_Parameters(&dnt);
                    leng = COMAnsForm(req_id, ID, request[2], &seq_num, request[4], 3, &request[6], answer);
                    UART1_TX(answer, leng);
                }
                else if(request[4] == 0xFF)// ������ � ����������� �������������
                {
                    //���������
                    seq_num++;
                    Oscilloscope(request[6], request[7], dnt.control.dead_time_ms/10);
                    answer[0] = 0x00;
                    answer[1] = 0xF0;
                    answer[2] = 0x00;
                    answer[3] = seq_num&0xFF;
                    answer[4] = 0xF3;
                    answer[5] = 0xFF;
                    UART1_TX(answer, 0x06);
                    TIMER_10MS = 10;
                    for(i=0; i<512; i++)
                    {
                        answer[0] = ((*(MKO_tr_data + 32 + i))>>8)&0xFF;
                        answer[1] = ((*(MKO_tr_data + 32 + i))>>0)&0xFF;
                        while (Timer10ms_Flag() != 1);
                        UART1_TX(answer, 0x02);
                        TIMER_10MS = 1;
                    }
                }
            }
        }
    }
}

