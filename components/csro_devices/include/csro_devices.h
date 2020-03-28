#ifndef CSRO_DEVICES_H_
#define CSRO_DEVICES_H_

#include "csro_common.h"

#define KEY_SCAN_RATE_MS 20
#define KEY_ACTION_THRESHOLD 2    // 2*20 = 40 ms
#define KEY_RESET_THRESHOLD 750   // 750*20 = 15 seconds
#define KEY_REBOOT_THRESHOLD 1500 // 1500*20 = 30 seconds
#define STATE_CHECK_RATE_MS 50

//csro_channels.c
void csro_nlight_channel_config(uint8_t index, char *model);
void csro_motor_channel_config(uint8_t index, char *model);

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

//csro_motor_csro_3t2r.c
void csro_motor_csro_3t2r_init(void);
void csro_motor_csro_3t2r_on_connect(esp_mqtt_event_handle_t event);
void csro_motor_csro_3t2r_on_message(esp_mqtt_event_handle_t event);

//csro_dlight_csro_3t3scr.c
void csro_dlight_csro_3t3scr_init(void);
void csro_dlight_csro_3t3scr_on_connect(esp_mqtt_event_handle_t event);
void csro_dlight_csro_3t3scr_on_message(esp_mqtt_event_handle_t event);

//csro_nlight_csro_2t2scr.c
void csro_nlight_csro_2t2scr_init(void);
void csro_nlight_csro_2t2scr_on_connect(esp_mqtt_event_handle_t event);
void csro_nlight_csro_2t2scr_on_message(esp_mqtt_event_handle_t event);

//csro_nlight_csro_3t3scr.c
void csro_nlight_csro_3t3scr_init(void);
void csro_nlight_csro_3t3scr_on_connect(esp_mqtt_event_handle_t event);
void csro_nlight_csro_3t3scr_on_message(esp_mqtt_event_handle_t event);

#endif