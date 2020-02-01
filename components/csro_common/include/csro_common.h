#ifndef CSRO_COMMON_H_
#define CSRO_COMMON_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "esp_smartconfig.h"
#include "esp_event_loop.h"

#include "nvs_flash.h"
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "cJSON.h"

//#define USE_CLOUD_SERVER

#define MANUFACTURER "NANJING CSRO CO.,LTD."
#define SOFT_VERSION "2020-01-29"

#define AIRMON_CSRO_A
//#define AW9523B
/*
#define NLIGHT_NB_4K4R
#define NLIGHT_NB_6K4R
#define MOTOR_NB_4K4R
#define AIRMON_CSRO_A
*/

typedef struct
{
    uint8_t restore_flag;
    uint8_t router_flag;
    char router_ssid[50];
    char router_pass[50];
    uint8_t mac[6];
    char mac_str[20];
    uint8_t ip[4];
    char ip_str[20];
    char host_name[20];
    char dev_type[20];
    uint32_t power_cnt;
    uint32_t time_run;
} csro_system;

typedef struct
{
    char id[50];
    char name[50];
    char pass[50];
    char sub_topic[100];
    char pub_topic[100];
    char lwt_topic[100];
    char content[1000];
    char broker[50];
    char uri[50];
    char prefix[50];
} csro_mqtt;

extern csro_system sysinfo;
extern csro_mqtt mqttinfo;
extern esp_mqtt_client_handle_t mqttclient;

//csro_common.c
void csro_main(void);
void csro_reset_router(void);

//csro_smart.c
void csro_smart_task(void);

//csro_mqtt.c
void csro_mqtt_task(void);

#endif