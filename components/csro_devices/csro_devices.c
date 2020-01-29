#include "csro_devices.h"

void csro_device_init(void)
{
#ifdef NLIGHT_NB_4K4R
    csro_nlight_nb_4k4r_init();
#elif defined NLIGHT_NB_6K4R
    csro_nlight_nb_6k4r_init();
#elif defined MOTOR_NB_4K4R
    csro_motor_nb_4k4r_init();
#elif defined AIRMON_CSRO_A
    csro_airmon_csro_a_init() ；
#endif
}

void csro_device_on_connect(esp_mqtt_event_handle_t event)
{
#ifdef NLIGHT_NB_4K4R
    csro_nlight_nb_4k4r_on_connect(event);
#elif defined NLIGHT_NB_6K4R
    csro_nlight_nb_6k4r_on_connect(event);
#elif defined MOTOR_NB_4K4R
    csro_motor_nb_4k4r_on_connect(event);
#elif defined AIRMON_CSRO_A
    csro_airmon_csro_a_on_connect(event);
#endif
}

void csro_device_on_message(esp_mqtt_event_handle_t event)
{
#ifdef NLIGHT_NB_4K4R
    csro_nlight_nb_4k4r_on_message(event);
#elif defined NLIGHT_NB_6K4R
    csro_nlight_nb_6k4r_on_message(event);
#elif defined MOTOR_NB_4K4R
    csro_motor_nb_4k4r_on_message(event);
#elif defined AIRMON_CSRO_A
    csro_airmon_csro_a_on_message(event);
#endif
}