#include "mko_ud_header.h"

#define MKO_ADR 19
#define MKO_TIMEOUT_MS 10

uint16 *MKO_tr_data = (uint16*)0x1000;
uint16 *MKO_rv_data = (uint16*)0x0800;
uint8_t mko_activity_timeout = MKO_TIMEOUT_MS;
uint8_t mko_read_flag = 0;

void MKO_UD_Init()
{
	BSICONFIG = 0x4005; //конфигурация удаленного терминала
	BSICON_TERM = ((0x1F&MKO_ADR)<<11) | 0x0001;
	INT_MASK1 |= 0x0002; //разрешение прерывания по МКО
	INT_PEND1 &= ~0x0002; // запрещаем ждущие прерывания
}

int8 MKO_data_to_transmit(uint16 *data, uint8 subaddr)
{
  uint8 i;
  if (subaddr == 0 | subaddr > 30) return -1;
  for (i=0; i<31; i++)
  {
    MKO_tr_data[i+1+(subaddr*32)] = data[i];
  }
   MKO_tr_data[0+(subaddr*32)] = data[31];
  return 0;
}

int8 MKO_receive_data(uint16 *data, uint8 subaddr)
{
  uint8 i;
  if (subaddr == 0 | subaddr > 30) return -1;
  for (i=0; i<31; i++)
  {
    data[i] =  MKO_rv_data[i+1+(subaddr*32)];
  }
  data[31] = MKO_rv_data[0+(subaddr*32)] ;
  return 0;
}

int8 MKO_receive_data_change(uint16 *data, uint8 subaddr)
{
  uint8 i;
  if (subaddr == 0 & subaddr > 30) return -1;
  for (i=0; i<31; i++)
  {
    MKO_rv_data[i+1+(subaddr*32)] = data[i];
  }
   MKO_rv_data[0+(subaddr*32)] = data[31];
  return 0;
}

void MKO_get_data_from_transmit_subaddr(uint16 *data, uint8 subaddr)
{
	uint8_t i;
	for (i=0; i<31; i++)	{
		data[i] =  MKO_tr_data[i+1+(subaddr*32)];
	}
	data[31] = MKO_tr_data[0+(subaddr*32)] ;
}


uint16 MKO_State()
{
  return BSISTAT_TERM;
}

#pragma interrupt 9 IRQ_MKO_UD //обработка прерываний МКО
void  IRQ_MKO_UD()
{ uint16_t state;
    _di_();
	state = MKO_State();
	if (state&0x2000) mko_activity_timeout = MKO_TIMEOUT_MS;
    if ((state&0x0400) == 0) mko_read_flag = 1;  //проверка бита направления передачи из командного слова
    _ei_();
}

