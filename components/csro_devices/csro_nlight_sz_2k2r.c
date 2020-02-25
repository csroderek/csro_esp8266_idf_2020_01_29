#include "csro_devices.h"
#include "aw9523b.h"

#define RELAY_01_NUM GPIO_NUM_13
#define RELAY_02_NUM GPIO_NUM_12

int light_state[2] = {0, 0};

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
    uint8_t count = 0;
    while (true)
    {
        count = (count + 1) % 2;
        csro_set_led_sz(0, count == 0 ? 0 : 200, count == 0 ? 0 : 200);
        csro_set_led_sz(1, count == 0 ? 0 : 200, count == 0 ? 0 : 200);
        csro_set_led_sz(2, count == 0 ? 0 : 200, count == 0 ? 0 : 200);
        csro_set_led_sz(3, count == 0 ? 0 : 200, count == 0 ? 0 : 200);
        vTaskDelay(200 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void nlight_sz_2k2r_relay_led_task(void *args)
{
    while (true)
    {

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
}
void csro_nlight_sz_2k2r_on_message(esp_mqtt_event_handle_t event)
{
}