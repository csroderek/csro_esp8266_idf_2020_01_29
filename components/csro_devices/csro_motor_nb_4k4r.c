
#include "csro_devices.h"
#include "aw9523b.h"

#ifdef MOTOR_NB_4K4R

#define KEY_01_NUM GPIO_NUM_0
#define KEY_02_NUM GPIO_NUM_4
#define KEY_03_NUM GPIO_NUM_13
#define KEY_04_NUM GPIO_NUM_5
#define GPIO_INPUT_PIN_SEL ((1ULL << KEY_01_NUM) | (1ULL << KEY_02_NUM) | (1ULL << KEY_03_NUM) | (1ULL << KEY_04_NUM))

typedef enum
{
    STOP = 0,
    OPEN = 1,
    CLOSE = 2,
    STOP_TO_OPEN = 3,
    STOP_TO_CLOSE = 4,
} motor_state;

motor_state motor[2] = {STOP, STOP};

static void motor_nb_4k4r_mqtt_update(void)
{
    if (mqttclient != NULL)
    {
        int state_value[2] = {50, 50};
        cJSON *state_json = cJSON_CreateObject();
        cJSON_AddItemToObject(state_json, "state", cJSON_CreateIntArray(state_value, 2));
        cJSON_AddNumberToObject(state_json, "run", sysinfo.time_run);
        char *out = cJSON_PrintUnformatted(state_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(state_json);
        sprintf(mqttinfo.pub_topic, "csro/%s/%s/state", sysinfo.mac_str, sysinfo.dev_type);
        esp_mqtt_client_publish(mqttclient, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
}

static void motor_nb_4k4r_key_task(void *args)
{
    static uint32_t hold_time[4];
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    while (true)
    {
        int key_status[4] = {gpio_get_level(KEY_01_NUM), gpio_get_level(KEY_02_NUM), gpio_get_level(KEY_03_NUM), gpio_get_level(KEY_04_NUM)};
        for (uint8_t i = 0; i < 4; i++)
        {
            if (key_status[i] == 0)
            {
                hold_time[i]++;
            }
            else
            {
                hold_time[i] = 0;
            }
        }
        if (hold_time[0] == 2) //key1
        {
            motor[0] = (motor[0] == STOP) ? OPEN : STOP;
        }
        else if (hold_time[2] == 2) //key3
        {
            motor[0] = (motor[0] == STOP) ? CLOSE : STOP;
        }
        if (hold_time[1] == 2) //key2
        {
            motor[1] = (motor[1] == STOP) ? OPEN : STOP;
        }
        else if (hold_time[3] == 2) //key4
        {
            motor[1] = (motor[1] == STOP) ? CLOSE : STOP;
        }
        vTaskDelay(20 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void motor_nb_4k4r_relay_led_task(void *args)
{
    static uint8_t motor_last[2] = {STOP, STOP};
    static uint16_t relay_count_100ms[2];
    while (true)
    {
        bool update = false;
        for (uint8_t i = 0; i < 2; i++)
        {
            if (motor_last[i] != motor[i])
            {
                //printf("motor%d, %d\r\n", i, motor[i]);
                motor_last[i] = motor[i];
                update = true;
                if (motor[i] == STOP || motor[i] == STOP_TO_CLOSE || motor[i] == STOP_TO_OPEN)
                {
                    relay_count_100ms[i] = 5;
                }
                else if (motor[i] == OPEN)
                {
                    relay_count_100ms[i] = 600;
                }
                else if (motor[i] == CLOSE)
                {
                    relay_count_100ms[i] = 600;
                }
            }
            if (relay_count_100ms[i] > 0)
            {
                relay_count_100ms[i]--;
                if (relay_count_100ms[i] == 0)
                {
                    motor[i] = (motor[i] == STOP_TO_CLOSE) ? CLOSE : (motor[i] == STOP_TO_OPEN) ? OPEN : STOP;
                }
            }
            csro_set_led_nb(i, (motor[i] == OPEN) ? 128 : 8);
            csro_set_led_nb(i + 2, (motor[i] == CLOSE) ? 128 : 8);
            csro_set_relay_nb(2 * i, (motor[i] == OPEN) ? true : false);
            csro_set_relay_nb(2 * i + 1, (motor[i] == CLOSE) ? true : false);
        }
        if (update)
        {
            csro_set_vibrator_nb();
            motor_nb_4k4r_mqtt_update();
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void csro_motor_nb_4k4r_init(void)
{
    csro_aw9523b_init_nb();
    xTaskCreate(motor_nb_4k4r_key_task, "motor_nb_4k4r_key_task", 2048, NULL, configMAX_PRIORITIES - 7, NULL);
    xTaskCreate(motor_nb_4k4r_relay_led_task, "motor_nb_4k4r_relay_led_task", 2048, NULL, configMAX_PRIORITIES - 8, NULL);
}
void csro_motor_nb_4k4r_on_connect(esp_mqtt_event_handle_t event)
{
    sprintf(mqttinfo.sub_topic, "csro/%s/%s/set/#", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_subscribe(event->client, mqttinfo.sub_topic, 0);
    for (uint8_t i = 0; i < 2; i++)
    {
        char prefix[50], name[50], valuetemp[50], command[50], deviceid[50];
        sprintf(mqttinfo.pub_topic, "csro/cover/%s_%s_%d/config", sysinfo.mac_str, sysinfo.dev_type, i);
        sprintf(prefix, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(name, "%s_%s_%d", sysinfo.mac_str, sysinfo.dev_type, i);
        sprintf(deviceid, "%s_%s", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(valuetemp, "{{value_json.state[%d]}}", i);
        sprintf(command, "~/set/%d", i);

        cJSON *config_json = cJSON_CreateObject();
        cJSON *device = cJSON_CreateObject();
        cJSON_AddStringToObject(config_json, "~", prefix);
        cJSON_AddStringToObject(config_json, "name", name);
        cJSON_AddStringToObject(config_json, "unique_id", name);
        cJSON_AddStringToObject(config_json, "cmd_t", command);
        cJSON_AddStringToObject(config_json, "pos_t", "~/state");
        cJSON_AddStringToObject(config_json, "val_tpl", valuetemp);
        cJSON_AddStringToObject(config_json, "pl_open", "open");
        cJSON_AddStringToObject(config_json, "pl_stop", "stop");
        cJSON_AddStringToObject(config_json, "pl_cls", "close");
        cJSON_AddStringToObject(config_json, "dev_cla", "curtain");
        cJSON_AddStringToObject(config_json, "avty_t", "~/available");
        cJSON_AddItemToObject(config_json, "dev", device);
        cJSON_AddStringToObject(device, "ids", deviceid);
        cJSON_AddStringToObject(device, "name", deviceid);
        cJSON_AddStringToObject(device, "mf", MANUFACTURER);
        cJSON_AddStringToObject(device, "mdl", "MOTOR_NB_4K4R");
        cJSON_AddStringToObject(device, "sw", SOFT_VERSION);

        char *out = cJSON_PrintUnformatted(config_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(config_json);
        esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
    sprintf(mqttinfo.pub_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, "online", 0, 0, 1);
    motor_nb_4k4r_mqtt_update();
}
void csro_motor_nb_4k4r_on_message(esp_mqtt_event_handle_t event)
{
    char topic[50];
    for (uint8_t i = 0; i < 2; i++)
    {
        sprintf(topic, "csro/%s/%s/set/%d", sysinfo.mac_str, sysinfo.dev_type, i);
        if (strncmp(topic, event->topic, event->topic_len) == 0)
        {
            if (strncmp("open", event->data, event->data_len) == 0)
            {
                if (motor[i] == STOP || motor[i] == STOP_TO_CLOSE)
                {
                    motor[i] = OPEN;
                }
                else if (motor[i] == CLOSE)
                {
                    motor[i] = STOP_TO_OPEN;
                }
            }
            else if (strncmp("stop", event->data, event->data_len) == 0)
            {
                motor[i] = STOP;
            }
            else if (strncmp("close", event->data, event->data_len) == 0)
            {
                if (motor[i] == STOP || motor[i] == STOP_TO_OPEN)
                {
                    motor[i] = CLOSE;
                }
                else if (motor[i] == OPEN)
                {
                    motor[i] = STOP_TO_CLOSE;
                }
            }
        }
    }
}

#endif