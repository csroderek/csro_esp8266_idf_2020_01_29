#include "csro_devices.h"
#include "aw9523b.h"

#define RELAY_01_NUM GPIO_NUM_14
#define RELAY_02_NUM GPIO_NUM_12
#define KEY_01_INDEX 2
#define KEY_02_INDEX 3
#define GPIO_OUTPUT_PIN_SEL ((1ULL << RELAY_01_NUM) | (1ULL << RELAY_02_NUM))

int light_state[2] = {0, 0};
uint8_t flash_led = false;

static void nlight_sz_2k2r_mqtt_update(void)
{
    if (mqttclient != NULL)
    {
        cJSON *state_json = cJSON_CreateObject();
        cJSON_AddItemToObject(state_json, "state", cJSON_CreateIntArray(light_state, 2));
        cJSON_AddNumberToObject(state_json, "run", sysinfo.time_run);
        char *out = cJSON_PrintUnformatted(state_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(state_json);
        sprintf(mqttinfo.pub_topic, "csro/%s/%s/state", sysinfo.mac_str, sysinfo.dev_type);
        esp_mqtt_client_publish(mqttclient, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
}

static void nlight_sz_2k2r_key_task(void *args)
{
    static uint32_t hold_time[4];
    while (true)
    {
        uint8_t key_states[4];
        csro_get_key_sz(key_states);

        if (key_states[KEY_01_INDEX] == 0)
        {
            hold_time[KEY_01_INDEX]++;
            if (hold_time[KEY_01_INDEX] == 2)
            {
                light_state[0] = !light_state[0];
            }
        }
        else
        {
            if (hold_time[KEY_01_INDEX] >= 750)
            {
                csro_reset_router();
            }
            hold_time[KEY_01_INDEX] = 0;
        }

        if (key_states[KEY_02_INDEX] == 0)
        {
            hold_time[KEY_02_INDEX]++;
            if (hold_time[KEY_02_INDEX] == 2)
            {
                light_state[1] = !light_state[1];
            }
        }
        else
        {
            if (hold_time[KEY_02_INDEX] >= 750)
            {
                csro_reset_router();
            }
            hold_time[KEY_02_INDEX] = 0;
        }
        flash_led = (hold_time[KEY_01_INDEX] >= 750 || hold_time[KEY_02_INDEX] >= 750) ? true : false;
        vTaskDelay(20 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void nlight_sz_2k2r_relay_led_task(void *args)
{
    static uint8_t status[2];
    static uint8_t flash_flag = 0;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    while (true)
    {
        bool update = false;
        for (uint8_t i = 0; i < 2; i++)
        {
            if (status[i] != light_state[i])
            {
                status[i] = light_state[i];
                update = true;
            }
        }
        gpio_set_level(RELAY_01_NUM, light_state[0]);
        gpio_set_level(RELAY_02_NUM, light_state[1]);
        if (flash_led == true)
        {
            flash_flag = (flash_flag + 1) % 20;
            csro_set_led_sz(KEY_01_INDEX, flash_flag > 9 ? 0 : 200, flash_flag > 9 ? 0 : 200);
            csro_set_led_sz(KEY_02_INDEX, flash_flag > 9 ? 0 : 200, flash_flag > 9 ? 0 : 200);
        }
        else
        {
            csro_set_led_sz(KEY_01_INDEX, light_state[0] == 0 ? 0 : 200, light_state[0] == 0 ? 200 : 0);
            csro_set_led_sz(KEY_02_INDEX, light_state[1] == 0 ? 0 : 200, light_state[1] == 0 ? 200 : 0);
        }
        if (update == true)
        {
            nlight_sz_2k2r_mqtt_update();
        }
        vTaskDelay(20 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void csro_nlight_sz_2k2r_init(void)
{
    csro_aw9523b_init_sz();
    xTaskCreate(nlight_sz_2k2r_key_task, "nlight_sz_2k2r_key_task", 2048, NULL, configMAX_PRIORITIES - 7, NULL);
    xTaskCreate(nlight_sz_2k2r_relay_led_task, "nlight_sz_2k2r_relay_led_task", 2048, NULL, configMAX_PRIORITIES - 8, NULL);
}
void csro_nlight_sz_2k2r_on_connect(esp_mqtt_event_handle_t event)
{
    sprintf(mqttinfo.sub_topic, "csro/%s/%s/set/#", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_subscribe(event->client, mqttinfo.sub_topic, 0);
    for (uint8_t i = 0; i < 2; i++)
    {
        char prefix[50], name[50], state[50], command[50], deviceid[50];
        sprintf(mqttinfo.pub_topic, "csro/light/%s_%s_%d/config", sysinfo.mac_str, sysinfo.dev_type, i);
        sprintf(prefix, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(name, "%s_%s_%d", sysinfo.mac_str, sysinfo.dev_type, i);
        sprintf(deviceid, "%s_%s", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(state, "{{value_json.state[%d]}}", i);
        sprintf(command, "~/set/%d", i);

        cJSON *config_json = cJSON_CreateObject();
        cJSON *device = cJSON_CreateObject();
        cJSON_AddStringToObject(config_json, "~", prefix);
        cJSON_AddStringToObject(config_json, "name", name);
        cJSON_AddStringToObject(config_json, "unique_id", name);
        cJSON_AddStringToObject(config_json, "cmd_t", command);
        cJSON_AddNumberToObject(config_json, "pl_on", 1);
        cJSON_AddNumberToObject(config_json, "pl_off", 0);
        cJSON_AddStringToObject(config_json, "stat_t", "~/state");
        cJSON_AddStringToObject(config_json, "stat_val_tpl", state);
        cJSON_AddStringToObject(config_json, "avty_t", "~/available");
        cJSON_AddItemToObject(config_json, "dev", device);
        cJSON_AddStringToObject(device, "ids", deviceid);
        cJSON_AddStringToObject(device, "name", deviceid);
        cJSON_AddStringToObject(device, "mf", MANUFACTURER);
        cJSON_AddStringToObject(device, "mdl", "NLIGHT_SZ_2K2R");
        cJSON_AddStringToObject(device, "sw", SOFT_VERSION);

        char *out = cJSON_PrintUnformatted(config_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(config_json);
        esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
    sprintf(mqttinfo.pub_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, "online", 0, 0, 1);
    nlight_sz_2k2r_mqtt_update();
}
void csro_nlight_sz_2k2r_on_message(esp_mqtt_event_handle_t event)
{
    char topic[50];
    for (uint8_t i = 0; i < 2; i++)
    {
        sprintf(topic, "csro/%s/%s/set/%d", sysinfo.mac_str, sysinfo.dev_type, i);
        if (strncmp(topic, event->topic, event->topic_len) == 0)
        {
            if (strncmp("0", event->data, event->data_len) == 0)
            {
                light_state[i] = 0;
            }
            else if (strncmp("1", event->data, event->data_len) == 0)
            {
                light_state[i] = 1;
            }
        }
    }
}