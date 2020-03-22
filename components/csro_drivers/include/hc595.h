#ifndef HC595_H_
#define HC595_H_

#include "csro_common.h"

void csro_hc595_init(void);
void csro_hc595_set_bit(uint8_t index, bool value);
void csro_hc595_send_data(void);

#endif