#ifndef CSRO_DEVICES_H_
#define CSRO_DEVICES_H_

#include "csro_common.h"

//csro_devices.c
void csro_device_init(void);
void csro_device_on_connect(esp_mqtt_event_handle_t event);
void csro_device_on_message(esp_mqtt_event_handle_t event);

//csro_nlight_nb_4k4r.c
void csro_nlight_nb_4k4r_init(void);
void csro_nlight_nb_4k4r_on_connect(esp_mqtt_event_handle_t event);
void csro_nlight_nb_4k4r_on_message(esp_mqtt_event_handle_t event);

//csro_nlight_nb_6k4r.c
void csro_nlight_nb_6k4r_init(void);
void csro_nlight_nb_6k4r_on_connect(esp_mqtt_event_handle_t event);
void csro_nlight_nb_6k4r_on_message(esp_mqtt_event_handle_t event);

//csro_motor_nb_4k4r.c
void csro_motor_nb_4k4r_init(void);
void csro_motor_nb_4k4r_on_connect(esp_mqtt_event_handle_t event);
void csro_motor_nb_4k4r_on_message(esp_mqtt_event_handle_t event);

//csro_airmon_csro_a.c
void csro_airmon_csro_a_init(void);
void csro_airmon_csro_a_on_connect(esp_mqtt_event_handle_t event);
void csro_airmon_csro_a_on_message(esp_mqtt_event_handle_t event);

//csro_nlight_sz_2k2r.c
void csro_nlight_sz_2k2r_init(void);
void csro_nlight_sz_2k2r_on_connect(esp_mqtt_event_handle_t event);
void csro_nlight_sz_2k2r_on_message(esp_mqtt_event_handle_t event);

#endif