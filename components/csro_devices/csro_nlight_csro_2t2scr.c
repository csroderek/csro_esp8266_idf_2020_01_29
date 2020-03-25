
#include "csro_devices.h"
#include "hc595.h"

#ifdef NLIGHT_CSRO_2T2SCR

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

int light_states[3] = {0, 0, 0};
uint8_t flash_led = false;

static void hw_timer_callback_led_scr_action_task(void *arg)
{
    static uint8_t dim_count = 0;
    static uint8_t flash_count = 0;
    dim_count = (dim_count + 1) % 2;
    flash_count = (flash_led == true) ? ((flash_count + 1) % 200) : 0;
    if (dim_count == 1)
    {
        if (flash_led == true)
        {
            csro_hc595_set_bit(LED_01_BIT, flash_count < 100 ? true : false);
            csro_hc595_set_bit(LED_03_BIT, flash_count < 100 ? true : false);
        }
        else
        {
            csro_hc595_set_bit(LED_01_BIT, true);
            csro_hc595_set_bit(LED_03_BIT, true);
        }
        csro_hc595_send_data();
        hw_timer_alarm_us(1000 - 50, false);
    }
    else
    {
        if (flash_led == true)
        {
            csro_hc595_set_bit(LED_01_BIT, flash_count < 100 ? true : false);
            csro_hc595_set_bit(LED_03_BIT, flash_count < 100 ? true : false);
        }
        else
        {
            csro_hc595_set_bit(LED_01_BIT, light_states[0] == 1 ? true : false);
            csro_hc595_set_bit(LED_03_BIT, light_states[1] == 1 ? true : false);
        }
        csro_hc595_send_data();
        hw_timer_alarm_us(4000 - 50, false);
    }
}
static void nlight_csro_2t2scr_touch_task(void *args)
{
    static uint8_t touch_states[3];
    static uint32_t hold_time[3];
    while (true)
    {
        touch_states[0] = gpio_get_level(TOUCH_01_NUM);
        touch_states[1] = 1;
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
        if (hold_time[0] == 2)
        {
            light_states[0] = !light_states[0];
        }
        if (hold_time[2] == 2)
        {
            light_states[1] = !light_states[1];
        }
        flash_led = (hold_time[0] > 750 || hold_time[2] > 750) ? true : false;
        if (hold_time[0] > 1500 || hold_time[1] > 1500 || hold_time[2] > 1500)
        {
            esp_restart();
        }
        vTaskDelay(20 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void nlight_csro_2t2scr_mqtt_update(void)
{
    if (mqttclient != NULL)
    {
        cJSON *state_json = cJSON_CreateObject();
        cJSON_AddItemToObject(state_json, "state", cJSON_CreateIntArray(light_states, 2));
        cJSON_AddNumberToObject(state_json, "run", sysinfo.time_run);
        char *out = cJSON_PrintUnformatted(state_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(state_json);
        sprintf(mqttinfo.pub_topic, "csro/%s/%s/state", sysinfo.mac_str, sysinfo.dev_type);
        esp_mqtt_client_publish(mqttclient, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
}

static void nlight_csro_2t2scr_scr_task(void *args)
{
    static uint8_t status[2];
    while (true)
    {
        bool update = false;
        if (status[0] != light_states[0])
        {
            update = true;
            status[0] = light_states[0];
        }
        if (status[1] != light_states[1])
        {
            update = true;
            status[1] = light_states[1];
        }
        csro_hc595_set_bit(SCR_01_BIT, light_states[0] == 1 ? false : true);
        csro_hc595_set_bit(SCR_02_BIT, light_states[1] == 1 ? false : true);
        if (update)
        {
            nlight_csro_2t2scr_mqtt_update();
        }
        vTaskDelay(50 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void csro_nlight_csro_2t2scr_init(void)
{
    csro_hc595_init();

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    csro_hc595_set_bit(SCR_03_BIT, true);
    csro_hc595_set_bit(LED_02_BIT, false);

    csro_hc595_set_bit(SCR_01_BIT, true);
    csro_hc595_set_bit(SCR_02_BIT, true);
    csro_hc595_set_bit(TOUCH_CTRL_BIT, false);
    csro_hc595_send_data();

    vTaskDelay(200 / portTICK_RATE_MS);
    csro_hc595_set_bit(TOUCH_CTRL_BIT, true);
    csro_hc595_send_data();

    hw_timer_init(hw_timer_callback_led_scr_action_task, NULL);
    hw_timer_alarm_us(100000, false);
    xTaskCreate(nlight_csro_2t2scr_touch_task, "nlight_csro_2t2scr_touch_task", 2048, NULL, configMAX_PRIORITIES - 7, NULL);
    xTaskCreate(nlight_csro_2t2scr_scr_task, "nlight_csro_2t2scr_scr_task", 2048, NULL, configMAX_PRIORITIES - 8, NULL);
}
void csro_nlight_csro_2t2scr_on_connect(esp_mqtt_event_handle_t event)
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
        cJSON_AddStringToObject(device, "mdl", "NLIGHT_CSRO_2T2SCR");
        cJSON_AddStringToObject(device, "sw", SOFT_VERSION);

        char *out = cJSON_PrintUnformatted(config_json);
        strcpy(mqttinfo.content, out);
        free(out);
        cJSON_Delete(config_json);
        esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);
    }
    sprintf(mqttinfo.pub_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, "online", 0, 0, 1);
    nlight_csro_2t2scr_mqtt_update();
}
void csro_nlight_csro_2t2scr_on_message(esp_mqtt_event_handle_t event)
{
    char topic[50];
    for (uint8_t i = 0; i < 2; i++)
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