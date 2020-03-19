
#include "csro_devices.h"

#ifdef MOTOR_CSRO_3T2R

#define TOUCH_01_NUM GPIO_NUM_2
#define TOUCH_02_NUM GPIO_NUM_4
#define TOUCH_03_NUM GPIO_NUM_5

#define HC595_SRCLK GPIO_NUM_16
#define HC595_RCLK GPIO_NUM_12
#define HC595_SER GPIO_NUM_13

#define LED_01_BIT 1
#define LED_02_BIT 2
#define LED_03_BIT 0
#define SWITCH_OPEN_BIT 5
#define SWITCH_CLOSE_BIT 4
#define TOUCH_CTRL_BIT 6

#define GPIO_INPUT_PIN_SEL ((1ULL << TOUCH_01_NUM) | (1ULL << TOUCH_02_NUM) | (1ULL << TOUCH_03_NUM))
#define GPIO_OUTPUT_PIN_SEL ((1ULL << HC595_SRCLK) | (1ULL << HC595_RCLK) | (1ULL << HC595_SER))

typedef enum
{
    STOP = 0,
    OPEN = 1,
    CLOSE = 2,
    STOP_TO_OPEN = 3,
    STOP_TO_CLOSE = 4,
} motor_state;

motor_state motor = STOP;
uint8_t touch_states[3];
uint8_t flash_led = false;
uint8_t hc595_data = 0;

static void hc595_send_data(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        gpio_set_level(HC595_SER, ((hc595_data << i) & 0x80) == 0 ? 0 : 1);
        os_delay_us(1);
        gpio_set_level(HC595_SRCLK, 1);
        os_delay_us(1);
        gpio_set_level(HC595_SRCLK, 0);
    }
    gpio_set_level(HC595_RCLK, 1);
    os_delay_us(1);
    gpio_set_level(HC595_RCLK, 0);
}

static void hc595_set_bit(uint8_t index, bool value)
{
    if (value == true)
    {
        hc595_data = hc595_data | (1 << index);
    }
    else
    {
        hc595_data = hc595_data & (~(1 << index));
    }
}

void hw_timer_callback_led_relay_action_task(void *arg)
{
    static uint8_t count = 0;
    static uint8_t flash_count = 0;
    count = (count + 1) % 2;
    flash_count = (flash_led == true) ? ((flash_count + 1) % 200) : 0;
    if (count == 1)
    {
        if (flash_led == true)
        {
            hc595_set_bit(LED_01_BIT, flash_count < 100 ? true : false);
            hc595_set_bit(LED_02_BIT, flash_count < 100 ? true : false);
            hc595_set_bit(LED_03_BIT, flash_count < 100 ? true : false);
        }
        else
        {
            hc595_set_bit(LED_01_BIT, true);
            hc595_set_bit(LED_02_BIT, true);
            hc595_set_bit(LED_03_BIT, true);
        }
        hc595_send_data();
        hw_timer_alarm_us(1000 - 50, false);
    }
    else
    {
        if (flash_led == true)
        {
            hc595_set_bit(LED_01_BIT, flash_count < 100 ? true : false);
            hc595_set_bit(LED_02_BIT, flash_count < 100 ? true : false);
            hc595_set_bit(LED_03_BIT, flash_count < 100 ? true : false);
        }
        else
        {
            hc595_set_bit(LED_01_BIT, touch_states[0] == 0 ? true : false);
            hc595_set_bit(LED_02_BIT, touch_states[1] == 0 ? true : false);
            hc595_set_bit(LED_03_BIT, touch_states[2] == 0 ? true : false);
        }
        hc595_send_data();
        hw_timer_alarm_us(4000 - 50, false);
    }
}

static void motor_csro_3t2r_mqtt_update(void)
{
    if (mqttclient != NULL)
    {
        cJSON *state_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(state_json, "state", 50);
        cJSON_AddNumberToObject(state_json, "run", sysinfo.time_run);
        char *out = cJSON_PrintUnformatted(state_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(state_json);
        sprintf(mqttinfo.pub_topic, "csro/%s/%s/state", sysinfo.mac_str, sysinfo.dev_type);
        esp_mqtt_client_publish(mqttclient, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
}

static void motor_csro_3t2r_key_task(void *args)
{
    static uint32_t hold_time[4];
    while (true)
    {
        touch_states[0] = gpio_get_level(TOUCH_01_NUM);
        touch_states[1] = gpio_get_level(TOUCH_02_NUM);
        touch_states[2] = gpio_get_level(TOUCH_03_NUM);
        for (uint8_t i = 0; i < 3; i++)
        {
            if (touch_states[i] == 0)
            {
                hold_time[i]++;
            }
            else
            {
                if (hold_time[i] >= 750)
                {
                    csro_reset_router();
                }
                hold_time[i] = 0;
            }
        }
        if (hold_time[0] == 2) //key1
        {
            if (motor == STOP || motor == STOP_TO_CLOSE)
            {
                motor = OPEN;
            }
            else if (motor == CLOSE)
            {
                motor = STOP_TO_OPEN;
            }
        }
        else if (hold_time[1] == 2)
        {
            motor = STOP;
        }
        else if (hold_time[2] == 2)
        {
            if (motor == STOP || motor == STOP_TO_OPEN)
            {
                motor = CLOSE;
            }
            else if (motor == OPEN)
            {
                motor = STOP_TO_CLOSE;
            }
        }
        flash_led = (hold_time[0] > 750 || hold_time[1] > 750 || hold_time[2] > 750) ? true : false;
        if (hold_time[0] > 1500 && hold_time[1] > 1500 && hold_time[2] > 1500)
        {
            esp_restart();
        }
        vTaskDelay(20 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void motor_csro_3t2r_relay_task(void *args)
{
    static motor_state motor_last = STOP;
    static uint16_t relay_count_100ms = 0;
    while (true)
    {
        bool update = false;
        if (motor_last != motor)
        {
            motor_last = motor;
            update = true;
            if (motor == STOP || motor == STOP_TO_CLOSE || motor == STOP_TO_OPEN)
            {
                relay_count_100ms = 5;
            }
            else if (motor == OPEN)
            {
                relay_count_100ms = 600;
            }
            else if (motor == CLOSE)
            {
                relay_count_100ms = 600;
            }
        }
        if (relay_count_100ms > 0)
        {
            relay_count_100ms--;
            if (relay_count_100ms == 0)
            {
                motor = (motor == STOP_TO_CLOSE) ? CLOSE : (motor == STOP_TO_OPEN) ? OPEN : STOP;
            }
        }
        hc595_set_bit(SWITCH_OPEN_BIT, (motor == OPEN) ? true : false);
        hc595_set_bit(SWITCH_CLOSE_BIT, (motor == CLOSE) ? true : false);
        if (update)
        {
            motor_csro_3t2r_mqtt_update();
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void csro_motor_csro_3t2r_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    hc595_set_bit(SWITCH_OPEN_BIT, false);
    hc595_set_bit(SWITCH_CLOSE_BIT, false);
    hc595_set_bit(TOUCH_CTRL_BIT, false);
    hc595_send_data();
    vTaskDelay(200 / portTICK_RATE_MS);
    hc595_set_bit(TOUCH_CTRL_BIT, true);
    hc595_send_data();

    hw_timer_init(hw_timer_callback_led_relay_action_task, NULL);
    hw_timer_alarm_us(100000, false);

    xTaskCreate(motor_csro_3t2r_key_task, "motor_csro_3t2r_key_task", 2048, NULL, configMAX_PRIORITIES - 7, NULL);
    xTaskCreate(motor_csro_3t2r_relay_task, "motor_csro_3t2r_relay_task", 2048, NULL, configMAX_PRIORITIES - 8, NULL);
}
void csro_motor_csro_3t2r_on_connect(esp_mqtt_event_handle_t event)
{
    sprintf(mqttinfo.sub_topic, "csro/%s/%s/set/#", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_subscribe(event->client, mqttinfo.sub_topic, 0);

    char prefix[50], name[50], deviceid[50];
    sprintf(mqttinfo.pub_topic, "csro/cover/%s_%s_0/config", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(prefix, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(name, "%s_%s_0", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(deviceid, "%s_%s", sysinfo.mac_str, sysinfo.dev_type);

    cJSON *config_json = cJSON_CreateObject();
    cJSON *device = cJSON_CreateObject();
    cJSON_AddStringToObject(config_json, "~", prefix);
    cJSON_AddStringToObject(config_json, "name", name);
    cJSON_AddStringToObject(config_json, "unique_id", name);
    cJSON_AddStringToObject(config_json, "cmd_t", "~/set");
    cJSON_AddStringToObject(config_json, "pos_t", "~/state");
    cJSON_AddStringToObject(config_json, "val_tpl", "{{value_json.state}}");
    cJSON_AddStringToObject(config_json, "pl_open", "open");
    cJSON_AddStringToObject(config_json, "pl_stop", "stop");
    cJSON_AddStringToObject(config_json, "pl_cls", "close");
    cJSON_AddStringToObject(config_json, "dev_cla", "curtain");
    cJSON_AddStringToObject(config_json, "avty_t", "~/available");
    cJSON_AddItemToObject(config_json, "dev", device);
    cJSON_AddStringToObject(device, "ids", deviceid);
    cJSON_AddStringToObject(device, "name", deviceid);
    cJSON_AddStringToObject(device, "mf", MANUFACTURER);
    cJSON_AddStringToObject(device, "mdl", "MOTOR_CSRO_3T2R");
    cJSON_AddStringToObject(device, "sw", SOFT_VERSION);

    char *out = cJSON_PrintUnformatted(config_json);
    strcpy(mqttinfo.content, out);
    free(out);
    cJSON_Delete(config_json);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);

    sprintf(mqttinfo.pub_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, "online", 0, 0, 1);
    motor_csro_3t2r_mqtt_update();
}
void csro_motor_csro_3t2r_on_message(esp_mqtt_event_handle_t event)
{
    char topic[50];

    sprintf(topic, "csro/%s/%s/set", sysinfo.mac_str, sysinfo.dev_type);
    if (strncmp(topic, event->topic, event->topic_len) == 0)
    {
        if (strncmp("open", event->data, event->data_len) == 0)
        {
            if (motor == STOP || motor == STOP_TO_CLOSE)
            {
                motor = OPEN;
            }
            else if (motor == CLOSE)
            {
                motor = STOP_TO_OPEN;
            }
        }
        else if (strncmp("stop", event->data, event->data_len) == 0)
        {
            motor = STOP;
        }
        else if (strncmp("close", event->data, event->data_len) == 0)
        {
            if (motor == STOP || motor == STOP_TO_OPEN)
            {
                motor = CLOSE;
            }
            else if (motor == OPEN)
            {
                motor = STOP_TO_CLOSE;
            }
        }
    }
}

#endif