#include "csro_devices.h"
#include "hc595.h"

#ifdef NLIGHT_CSRO_3T3SCR

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

#define KEY_NUM 3
#define NLIGHT_NUM 3

bool flash_led = false;
gpio_num_t key_pins[KEY_NUM] = {TOUCH_01_NUM, TOUCH_02_NUM, TOUCH_03_NUM};
uint8_t led_index[KEY_NUM] = {LED_01_BIT, LED_02_BIT, LED_03_BIT};

int light_states[NLIGHT_NUM];
uint8_t scr_index[NLIGHT_NUM] = {SCR_01_BIT, SCR_02_BIT, SCR_03_BIT};

static void mqtt_update(void)
{
    if (mqttclient != NULL)
    {
        cJSON *state_json = cJSON_CreateObject();
        cJSON_AddItemToObject(state_json, "state", cJSON_CreateIntArray(light_states, NLIGHT_NUM));
        cJSON_AddNumberToObject(state_json, "run", sysinfo.time_run);
        cJSON_AddNumberToObject(state_json, "rssi", sysinfo.rssi);
        char *out = cJSON_PrintUnformatted(state_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(state_json);
        sprintf(mqttinfo.pub_topic, "csro/%s/%s/state", sysinfo.mac_str, sysinfo.dev_type);
        esp_mqtt_client_publish(mqttclient, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
}

static void timer_callback_task(void *args)
{
    static uint8_t dim_count = 0;
    static uint8_t flash_count = 0;
    static uint16_t timer_value_us = 0;
    dim_count = (dim_count + 1) % 2;
    flash_count = (flash_led == true) ? ((flash_count + 1) % 200) : 0;
    if (dim_count == 1)
    {
        for (uint8_t i = 0; i < KEY_NUM; i++)
        {
            csro_hc595_set_bit(led_index[i], (flash_led == false) ? true : ((flash_count < 100) ? true : false));
        }
        timer_value_us = 1000 - 50;
    }
    else
    {
        for (uint8_t i = 0; i < KEY_NUM; i++)
        {
            csro_hc595_set_bit(led_index[i], (flash_led == false) ? (light_states[i] == 1 ? true : false) : ((flash_count < 100) ? true : false));
        }
        timer_value_us = 4000 - 50;
    }
    hw_timer_alarm_us(timer_value_us, false);
    csro_hc595_send_data();
}

static void key_scan_task(void *args)
{
    static uint16_t key_holdtimes[KEY_NUM];
    static uint8_t key_states[KEY_NUM];
    while (true)
    {
        bool temp_flash_led = false;
        for (uint8_t i = 0; i < KEY_NUM; i++)
        {
            key_states[i] = gpio_get_level(key_pins[i]);
            if (key_states[i] == 0)
            {
                key_holdtimes[i]++;
            }
            else
            {
                if (key_holdtimes[i] >= KEY_RESET_THRESHOLD)
                {
                    key_holdtimes[i] = 0;
                    csro_reset_router();
                }
                else
                {
                    key_holdtimes[i] = 0;
                }
            }
        }
        for (uint8_t i = 0; i < KEY_NUM; i++)
        {
            if (key_holdtimes[i] == KEY_ACTION_THRESHOLD)
            {
                light_states[i] = !light_states[i];
            }
            if (key_holdtimes[i] > KEY_RESET_THRESHOLD)
            {
                temp_flash_led = true;
            }
            if (key_holdtimes[i] > KEY_REBOOT_THRESHOLD)
            {
                esp_restart();
            }
        }
        flash_led = temp_flash_led;
        vTaskDelay(KEY_SCAN_RATE_MS / portTICK_RATE_MS);
    }
}

static void check_state_change_task(void *args)
{
    static int local_light_states[NLIGHT_NUM];
    while (true)
    {
        bool state_change = false;
        for (uint8_t i = 0; i < NLIGHT_NUM; i++)
        {
            if (local_light_states[i] != light_states[i])
            {
                state_change = true;
                local_light_states[i] = light_states[i];
            }
            csro_hc595_set_bit(scr_index[i], light_states[i] == 1 ? false : true);
        }
        if (state_change == true)
        {
            mqtt_update();
        }
        vTaskDelay(STATE_CHECK_RATE_MS / portTICK_RATE_MS);
    }
}

void csro_nlight_csro_3t3scr_init(void)
{
    csro_hc595_init();
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    csro_hc595_set_bit(TOUCH_CTRL_BIT, false);
    csro_hc595_set_bit(scr_index[0], true);
    csro_hc595_set_bit(scr_index[1], true);
    csro_hc595_set_bit(scr_index[2], true);
    csro_hc595_send_data();
    vTaskDelay(200 / portTICK_RATE_MS);
    csro_hc595_set_bit(TOUCH_CTRL_BIT, true);
    csro_hc595_send_data();

    hw_timer_init(timer_callback_task, NULL);
    hw_timer_alarm_us(1000, false);
    xTaskCreate(key_scan_task, "key_scan_task", 2048, NULL, configMAX_PRIORITIES - 7, NULL);
    xTaskCreate(check_state_change_task, "check_state_change_task", 2048, NULL, configMAX_PRIORITIES - 8, NULL);
}

void csro_nlight_csro_3t3scr_on_connect(esp_mqtt_event_handle_t event)
{
    sprintf(mqttinfo.sub_topic, "csro/%s/%s/set/#", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_subscribe(event->client, mqttinfo.sub_topic, 0);
    for (uint8_t i = 0; i < NLIGHT_NUM; i++)
    {
        csro_nlight_channel_config(i, "NLIGHT_CSRO_3T3SCR");
        esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
    sprintf(mqttinfo.pub_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, "online", 0, 0, 1);
    mqtt_update();
}

void csro_nlight_csro_3t3scr_on_message(esp_mqtt_event_handle_t event)
{
    char topic[50];
    for (uint8_t i = 0; i < NLIGHT_NUM; i++)
    {
        sprintf(topic, "csro/%s/%s/set/%d", sysinfo.mac_str, sysinfo.dev_type, i);
        if (strncmp(topic, event->topic, event->topic_len) == 0)
        {
            if (strncmp("0", event->data, event->data_len) == 0)
            {
                light_states[i] = 0;
            }
            else if (strncmp("1", event->data, event->data_len) == 0)
            {
                light_states[i] = 1;
            }
        }
    }
}

#endif