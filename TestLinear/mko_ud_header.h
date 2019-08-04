#include "types.h"

void MKO_UD_Init();
int8 MKO_data_to_transmit(uint16 *data, uint8 subaddr);
int8 MKO_receive_data(uint16 *data, uint8 subaddr);
int8 MKO_receive_data_change(uint16 *data, uint8 subaddr);
void MKO_get_data_from_transmit_subaddr(uint16 *data, uint8 subaddr);
uint16 MKO_State();


