
#include "csro_devices.h"

#ifdef NLIGHT_CSRO_2T2SCR

#define TOUCH_01_NUM GPIO_NUM_2
#define TOUCH_02_NUM GPIO_NUM_4
#define TOUCH_03_NUM GPIO_NUM_5

#define LED_01_BIT 1
#define LED_02_BIT 2
#define LED_03_BIT 0
#define SCR_01_BIT 5
#define SCR_02_BIT 4
#define SCR_03_BIT 3
#define TOUCH_CTRL_BIT 6

#define GPIO_INPUT_PIN_SEL ((1ULL << TOUCH_01_NUM) | (1ULL << TOUCH_02_NUM) | (1ULL << TOUCH_03_NUM))

void csro_nlight_csro_2t2scr_init(void)
{
}
void csro_nlight_csro_2t2scr_on_connect(esp_mqtt_event_handle_t event)
{
}
void csro_nlight_csro_2t2scr_on_message(esp_mqtt_event_handle_t event)
{
}
#endif