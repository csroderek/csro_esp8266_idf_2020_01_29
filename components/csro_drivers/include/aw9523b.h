
#ifndef AW9523B_H_
#define AW9523B_H_

#include "csro_common.h"

void csro_aw9523b_init_nb(void);
void csro_set_led_nb(uint8_t led_num, uint8_t bright);
void csro_set_relay_nb(uint8_t relay_num, uint8_t state);
void csro_set_vibrator_nb(void);

void csro_aw9523b_init_sz(void);
void csro_set_led_sz(uint8_t led_num, uint8_t red, uint8_t blue);
void csro_get_key_sz(uint8_t *keys);

#endif