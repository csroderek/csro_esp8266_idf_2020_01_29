#include "csro_devices.h"
#include "driver/pwm.h"
#include "driver/uart.h"

#ifdef AIRMON_CSRO_A

#define WIFI_LED_R GPIO_NUM_5
#define WIFI_LED_G GPIO_NUM_4
#define WIFI_LED_B GPIO_NUM_10
#define AQI_LED_R GPIO_NUM_12
#define AQI_LED_G GPIO_NUM_14
#define AQI_LED_B GPIO_NUM_13
#define FAN_PIN GPIO_NUM_15
#define BTN_PIN GPIO_NUM_0

#define AVE_COUNT 60

#define BLUE 0, 0, 255
#define GREEN 0, 255, 0
#define GREENYELLOW 173, 255, 47
#define YELLOW 255, 255, 0
#define ORANGE 255, 102, 0
#define RED 255, 0, 0

typedef struct
{
    uint32_t pm1_atm[AVE_COUNT + 1];
    uint32_t pm2_atm[AVE_COUNT + 1];
    uint32_t pm10_atm[AVE_COUNT + 1];
    float hcho[AVE_COUNT + 1];
    float temp[AVE_COUNT + 1];
    float humi[AVE_COUNT + 1];
} pms_data;

static char *itemlist[6] = {"temp", "humi", "hcho", "pm1", "pm2d5", "pm10"};
static char *unitlist[6] = {"°C", "\%", "mg/m^3", "ug/m^3", "ug/m^3", "ug/m^3"};
static char *iconlist[6] = {"mdi:thermometer-lines", "mdi:water-percent", "mdi:alien", "mdi:blur", "mdi:blur", "mdi:blur"};

static uint8_t color[6][3] = {{BLUE}, {GREEN}, {GREENYELLOW}, {YELLOW}, {ORANGE}, {RED}};
static uint8_t roundlist[6] = {1, 1, 3, 0, 0, 0};
static uint8_t pm_array[5] = {35, 75, 115, 150, 250};
static float hchi_array[5] = {0.05, 0.1, 0.2, 0.5, 0.8};

static QueueHandle_t uart0_queue;
static xSemaphoreHandle pub_sem;
static pms_data pms;
static uint8_t aqi_index = 0;

void csro_airmon_csro_a_init(void)
{
}

void csro_airmon_csro_a_on_connect(esp_mqtt_event_handle_t event)
{
}

void csro_airmon_csro_a_on_message(esp_mqtt_event_handle_t event)
{
}

#endif
