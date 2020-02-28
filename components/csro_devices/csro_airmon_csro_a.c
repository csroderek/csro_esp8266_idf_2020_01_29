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
#define FAN_PIN_NUM GPIO_NUM_15
#define BUTTON_PIN_NUM GPIO_NUM_0
#define GPIO_INPUT_PIN_SEL ((1ULL << BUTTON_PIN_NUM))

#define BLUE 0, 0, 255
#define GREEN 0, 255, 0
#define GREENYELLOW 173, 255, 47
#define YELLOW 255, 255, 0
#define ORANGE 255, 102, 0
#define RED 255, 0, 0

#define TOTAL 60

uint8_t flash_led = false;
uint8_t aqi_index = 0;

uint32_t pm1_ave, pm1[TOTAL];
uint32_t pm2_ave, pm2[TOTAL];
uint32_t pm10_ave, pm10[TOTAL];
float temp_ave, temp[TOTAL];
float humi_ave, humi[TOTAL];
float hcho_ave, hcho[TOTAL];

static uint8_t pm_grade[5] = {35, 75, 115, 150, 250};
static float hcho_grade[5] = {0.05, 0.1, 0.2, 0.5, 0.8};
static uint8_t color[6][3] = {{BLUE}, {GREEN}, {GREENYELLOW}, {YELLOW}, {ORANGE}, {RED}};
static char *itemlist[6] = {"temp", "humi", "hcho", "pm1", "pm2d5", "pm10"};
static char *unitlist[6] = {"°C", "\%", "mg/m^3", "ug/m^3", "ug/m^3", "ug/m^3"};
static char *iconlist[6] = {"mdi:thermometer-lines", "mdi:water-percent", "mdi:alien", "mdi:blur", "mdi:blur", "mdi:blur"};
static uint8_t roundlist[6] = {1, 1, 3, 0, 0, 0};

static QueueHandle_t uart0_queue;

static void csro_airmon_csro_a_mqtt_update(void)
{
    if (mqttclient != NULL)
    {
        cJSON *sensor_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(sensor_json, "pm1", pm1_ave);
        cJSON_AddNumberToObject(sensor_json, "pm2d5", pm2_ave);
        cJSON_AddNumberToObject(sensor_json, "pm10", pm10_ave);
        cJSON_AddNumberToObject(sensor_json, "temp", temp_ave);
        cJSON_AddNumberToObject(sensor_json, "humi", humi_ave);
        cJSON_AddNumberToObject(sensor_json, "hcho", hcho_ave);
        cJSON_AddNumberToObject(sensor_json, "run", sysinfo.time_run);
        char *out = cJSON_PrintUnformatted(sensor_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(sensor_json);
        sprintf(mqttinfo.pub_topic, "csro/%s/%s/state", sysinfo.mac_str, sysinfo.dev_type);
        esp_mqtt_client_publish(mqttclient, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
}

static void uart_event_task(void *args)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 512 * 2, 512 * 2, 100, &uart0_queue, 0);

    uart_event_t event;
    static uint8_t msg_count = 0;
    uint8_t *dtmp = (uint8_t *)malloc(128);

    while (true)
    {
        if (xQueueReceive(uart0_queue, (void *)&event, portMAX_DELAY))
        {
            bzero(dtmp, 128);
            if (event.type == UART_DATA)
            {
                uart_read_bytes(UART_NUM_0, dtmp, event.size, portMAX_DELAY);
                uint16_t check_sum = 0;
                for (uint8_t i = 0; i < 38; i++)
                {
                    check_sum = check_sum + dtmp[i];
                }
                if (dtmp[38] == (check_sum >> 8) && dtmp[39] == (check_sum & 0xFF))
                {
                    pm1[msg_count] = dtmp[10] * 256 + dtmp[11];
                    pm2[msg_count] = dtmp[12] * 256 + dtmp[13];
                    pm10[msg_count] = dtmp[14] * 256 + dtmp[15];
                    hcho[msg_count] = (float)(dtmp[28] * 256 + dtmp[29]) / 1000.0;
                    temp[msg_count] = (float)(dtmp[30] * 256 + dtmp[31]) / 10.0;
                    humi[msg_count] = (float)(dtmp[32] * 256 + dtmp[33]) / 10.0;
                    msg_count = (msg_count + 1) % TOTAL;
                    if (msg_count == 0)
                    {
                        for (uint8_t i = 0; i < TOTAL; i++)
                        {
                            pm1_ave = ((i == 0) ? 0 : pm1_ave) + pm1[i];
                            pm2_ave = ((i == 0) ? 0 : pm2_ave) + pm2[i];
                            pm10_ave = ((i == 0) ? 0 : pm10_ave) + pm10[i];
                            temp_ave = ((i == 0) ? 0 : temp_ave) + temp[i];
                            humi_ave = ((i == 0) ? 0 : humi_ave) + humi[i];
                            hcho_ave = ((i == 0) ? 0 : hcho_ave) + hcho[i];
                        }
                        pm1_ave = pm1_ave / TOTAL;
                        pm2_ave = pm2_ave / TOTAL;
                        pm10_ave = pm10_ave / TOTAL;
                        temp_ave = temp_ave / TOTAL;
                        humi_ave = humi_ave / TOTAL;
                        hcho_ave = hcho_ave / TOTAL;

                        uint8_t aqi_index_temp = 0;
                        for (uint8_t i = 0; i < 5; i++)
                        {
                            if ((pm2_ave > pm_grade[i]) || (hcho_ave > hcho_grade[i]))
                            {
                                aqi_index_temp = i + 1;
                            }
                        }
                        aqi_index = aqi_index_temp;
                        csro_airmon_csro_a_mqtt_update();
                    }
                }
            }
            else if (event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL)
            {
                uart_flush_input(UART_NUM_0);
                xQueueReset(uart0_queue);
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

static void led_task(void *arg)
{
    const uint32_t pin_num[7] = {WIFI_LED_R, WIFI_LED_G, WIFI_LED_B, AQI_LED_R, AQI_LED_G, AQI_LED_B, FAN_PIN_NUM};
    uint32_t duties[7] = {0, 0, 0, 0, 0, 0, (int)(0.6 * 2600)};
    int16_t phase[7] = {0, 0, 0, 0, 0, 0, 0};
    pwm_init(2600, duties, 7, pin_num);
    pwm_set_channel_invert(0x3F);
    pwm_set_phases(phase);
    pwm_start();
    bool led_on = false;

    while (true)
    {
        led_on = !led_on;
        pwm_set_duty(0, (led_on == false) ? 0 : (mqttclient != NULL) ? color[0][0] * 10 : color[5][0] * 10);
        pwm_set_duty(1, (led_on == false) ? 0 : (mqttclient != NULL) ? color[0][1] * 10 : color[5][1] * 10);
        pwm_set_duty(2, (led_on == false) ? 0 : (mqttclient != NULL) ? color[0][2] * 10 : color[5][2] * 10);
        pwm_set_duty(3, color[aqi_index][0] * 10);
        pwm_set_duty(4, color[aqi_index][1] * 10);
        pwm_set_duty(5, color[aqi_index][2] * 10);
        pwm_start();
        if (flash_led == true)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        else
        {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }
    vTaskDelete(NULL);
}

static void button_task(void *arg)
{
    static uint32_t hold_time;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    while (true)
    {
        int button_status = gpio_get_level(BUTTON_PIN_NUM);
        if (button_status == 0)
        {
            hold_time++;
        }
        else
        {
            if (hold_time >= 150)
            {
                csro_reset_router();
            }
            hold_time = 0;
        }
        flash_led = (hold_time >= 150) ? true : false;
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void csro_airmon_csro_a_init(void)
{
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, configMAX_PRIORITIES - 8, NULL);
    xTaskCreate(led_task, "led_task", 2048, NULL, configMAX_PRIORITIES - 9, NULL);
    xTaskCreate(button_task, "button_task", 2048, NULL, configMAX_PRIORITIES - 10, NULL);
}
void csro_airmon_csro_a_on_connect(esp_mqtt_event_handle_t event)
{
    for (size_t i = 0; i < 6; i++)
    {
        char prefix[50], name[50], value_template[50], deviceid[50];
        sprintf(mqttinfo.pub_topic, "csro/sensor/%s_%s_%s/config", sysinfo.mac_str, sysinfo.dev_type, itemlist[i]);
        sprintf(prefix, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(name, "%s_%s_%s", sysinfo.mac_str, sysinfo.dev_type, itemlist[i]);
        sprintf(deviceid, "%s_%s", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(value_template, "{{value_json.%s | round(%d)}}", itemlist[i], roundlist[i]);
        cJSON *config_json = cJSON_CreateObject();
        cJSON *device = cJSON_CreateObject();
        cJSON_AddStringToObject(config_json, "~", prefix);
        cJSON_AddStringToObject(config_json, "unique_id", name);
        cJSON_AddStringToObject(config_json, "name", name);
        cJSON_AddStringToObject(config_json, "unit_of_meas", unitlist[i]);
        cJSON_AddStringToObject(config_json, "avty_t", "~/available");
        cJSON_AddStringToObject(config_json, "stat_t", "~/state");
        cJSON_AddStringToObject(config_json, "icon", iconlist[i]);
        cJSON_AddStringToObject(config_json, "val_tpl", value_template);
        cJSON_AddItemToObject(config_json, "dev", device);
        cJSON_AddStringToObject(device, "ids", deviceid);
        cJSON_AddStringToObject(device, "name", deviceid);
        cJSON_AddStringToObject(device, "mf", MANUFACTURER);
        cJSON_AddStringToObject(device, "mdl", "AIRMON_CSRO_A");
        cJSON_AddStringToObject(device, "sw", SOFT_VERSION);
        char *out = cJSON_PrintUnformatted(config_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(config_json);
        esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, mqttinfo.content, 0, 1, 1);
    }
    sprintf(mqttinfo.pub_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, "online", 0, 0, 1);
}
void csro_airmon_csro_a_on_message(esp_mqtt_event_handle_t event)
{
}

#endif