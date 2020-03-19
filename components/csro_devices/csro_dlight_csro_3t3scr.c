
#include "csro_devices.h"

#ifdef DLIGHT_CSRO_3T3SCR

#define TOUCH_01_NUM GPIO_NUM_2
#define TOUCH_02_NUM GPIO_NUM_4
#define TOUCH_03_NUM GPIO_NUM_5
#define AC_ZERO_NUM GPIO_NUM_14

#define HC595_SRCLK GPIO_NUM_16
#define HC595_RCLK GPIO_NUM_12
#define HC595_SER GPIO_NUM_13

#define LED_01_BIT 1
#define LED_02_BIT 2
#define LED_03_BIT 0
#define SCR_01_BIT 5
#define SCR_02_BIT 4
#define SCR_03_BIT 3
#define TOUCH_CTRL_BIT 6

#define GPIO_INPUT_PIN_SEL ((1ULL << TOUCH_01_NUM) | (1ULL << TOUCH_02_NUM) | (1ULL << TOUCH_03_NUM))
#define GPIO_OUTPUT_PIN_SEL ((1ULL << HC595_SRCLK) | (1ULL << HC595_RCLK) | (1ULL << HC595_SER))
#define GPIO_INPUT_AC_ZERO (1ULL << AC_ZERO_NUM)

uint16_t bright_index[101] = {
    740, 732, 725, 717, 710, 703, 696, 689, 683, 676, 670,
    664, 658, 653, 647, 641, 636, 631, 625, 620, 615,
    610, 605, 600, 595, 590, 585, 580, 575, 571, 566,
    561, 557, 552, 547, 543, 538, 534, 529, 525, 520,
    515, 511, 507, 502, 497, 493, 488, 484, 479, 475,
    470, 466, 461, 457, 452, 448, 443, 438, 434, 429,
    424, 419, 414, 410, 405, 400, 395, 390, 385, 379,
    374, 369, 363, 358, 352, 347, 341, 335, 329, 323,
    316, 310, 303, 296, 289, 282, 274, 266, 258, 249,
    240, 230, 219, 207, 194, 180, 163, 141, 112, 100};

typedef struct
{
    uint8_t target;
    uint8_t value;
} dim_light;
dim_light dlight;

uint8_t touch_states[3];
uint8_t flash_led = false;
uint8_t hc595_data = 0;
uint8_t timer_flag = 0;

static void csro_delay(uint16_t ticks)
{
    uint32_t count = ticks * 5;
    while (count > 0)
    {
        count--;
    }
}

static void hc595_send_data(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        gpio_set_level(HC595_SER, ((hc595_data << i) & 0x80) == 0 ? 0 : 1);
        os_delay_us(2);
        gpio_set_level(HC595_SRCLK, 1);
        os_delay_us(2);
        gpio_set_level(HC595_SRCLK, 0);
    }
    gpio_set_level(HC595_RCLK, 1);
    os_delay_us(2);
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

void hw_timer_callback_led_scr_action_task(void *arg)
{
    static uint8_t count = 0;
    static uint8_t flash_count = 0;

    if (timer_flag == 0)
    {
        timer_flag = 1;
        hc595_set_bit(LED_01_BIT, touch_states[0] == 0 ? true : false);
        hc595_set_bit(LED_02_BIT, touch_states[1] == 0 ? true : false);
        hc595_set_bit(LED_03_BIT, touch_states[2] == 0 ? true : false);
        // hw_timer_alarm_us(bright_index[dlight.value] * 10 - 950, false);
        hc595_send_data();
    }
    // else if (timer_flag == 1)
    // {
    //     timer_flag = 2;
    //     hc595_set_bit(SCR_01_BIT, dlight.value > 0 ? false : true);
    //     hc595_set_bit(SCR_02_BIT, dlight.value > 0 ? false : true);
    //     hc595_set_bit(SCR_03_BIT, dlight.value > 0 ? false : true);
    //     hw_timer_alarm_us(200, false);
    //     hc595_send_data();
    // }
    // else if (timer_flag == 2)
    // {
    //     timer_flag = 3;
    //     hc595_set_bit(SCR_01_BIT, true);
    //     hc595_set_bit(SCR_02_BIT, true);
    //     hc595_set_bit(SCR_03_BIT, true);
    //     hc595_send_data();
    // }
}

static void dlight_csro_3t3scr_mqtt_update(void)
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

static void dlight_csro_3t3scr_key_task(void *args)
{
    static uint16_t hold_time[3];
    static bool update = false;
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
        if (hold_time[0] >= 2)
        {
            static uint8_t count0 = 0;
            count0 = (count0 + 1) % 2;
            if (count0 == 1 && dlight.target > 0 && dlight.target < 100)
            {
                dlight.target++;
                update = true;
            }
        }
        if (hold_time[1] == 2)
        {
            dlight.target = (dlight.target == 0) ? 50 : 0;
            update = true;
        }
        if (hold_time[2] >= 2)
        {
            static uint8_t count2 = 0;
            count2 = (count2 + 1) % 2;
            if (count2 == 1 && dlight.target > 1)
            {
                dlight.target--;
                update = true;
            }
        }
        if (update == true && touch_states[0] + touch_states[1] + touch_states[2] == 3)
        {
            update = false;
            dlight_csro_3t3scr_mqtt_update();
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void dlight_csro_3t3scr_scr_task(void *args)
{
    while (true)
    {
        if (dlight.value < dlight.target)
        {
            dlight.value++;
        }
        else if (dlight.value > dlight.target)
        {
            dlight.value--;
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void gpio_isr_handler(void *arg)
{
    timer_flag = 0;
    hc595_set_bit(LED_01_BIT, true);
    hc595_set_bit(LED_02_BIT, true);
    hc595_set_bit(LED_03_BIT, true);
    hc595_set_bit(SCR_01_BIT, true);
    hc595_set_bit(SCR_02_BIT, true);
    hc595_set_bit(SCR_03_BIT, true);
    hw_timer_alarm_us(1500, false);
    hc595_send_data();
}

void csro_dlight_csro_3t3scr_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_AC_ZERO;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    hw_timer_init(hw_timer_callback_led_scr_action_task, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(AC_ZERO_NUM, gpio_isr_handler, (void *)AC_ZERO_NUM);

    hc595_set_bit(SCR_01_BIT, false);
    hc595_set_bit(SCR_02_BIT, false);
    hc595_set_bit(SCR_03_BIT, false);
    hc595_set_bit(TOUCH_CTRL_BIT, false);
    hc595_send_data();
    vTaskDelay(200 / portTICK_RATE_MS);
    hc595_set_bit(TOUCH_CTRL_BIT, true);
    hc595_send_data();

    xTaskCreate(dlight_csro_3t3scr_key_task, "dlight_csro_3t3scr_key_task", 2048, NULL, configMAX_PRIORITIES - 7, NULL);
    xTaskCreate(dlight_csro_3t3scr_scr_task, "dlight_csro_3t3scr_scr_task", 2048, NULL, configMAX_PRIORITIES - 8, NULL);
}
void csro_dlight_csro_3t3scr_on_connect(esp_mqtt_event_handle_t event)
{
    sprintf(mqttinfo.sub_topic, "csro/%s/%s/set/#", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_subscribe(event->client, mqttinfo.sub_topic, 0);

    char prefix[50], name[50], deviceid[50];
    sprintf(mqttinfo.pub_topic, "csro/light/%s_%s/config", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(prefix, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(name, "%s_%s_0", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(deviceid, "%s_%s", sysinfo.mac_str, sysinfo.dev_type);

    cJSON *config_json = cJSON_CreateObject();
    cJSON *device = NULL;
    cJSON_AddStringToObject(config_json, "~", prefix);
    cJSON_AddStringToObject(config_json, "name", name);
    cJSON_AddStringToObject(config_json, "unique_id", name);
    cJSON_AddStringToObject(config_json, "cmd_t", "~/set");
    cJSON_AddStringToObject(config_json, "bri_cmd_t", "~/set/bright");
    cJSON_AddNumberToObject(config_json, "bri_scl", 100);
    cJSON_AddStringToObject(config_json, "bri_stat_t", "~/state");
    cJSON_AddStringToObject(config_json, "bri_val_tpl", "{{value_json.state.bright}}");
    cJSON_AddStringToObject(config_json, "on_cmd_type", "brightness");
    cJSON_AddNumberToObject(config_json, "pl_on", 1);
    cJSON_AddNumberToObject(config_json, "pl_off", 0);
    cJSON_AddStringToObject(config_json, "stat_t", "~/state");
    cJSON_AddStringToObject(config_json, "stat_val_tpl", "{{value_json.state.on}}");
    cJSON_AddStringToObject(config_json, "avty_t", "~/available");
    cJSON_AddStringToObject(config_json, "opt", "false");
    cJSON_AddItemToObject(config_json, "dev", device = cJSON_CreateObject());
    cJSON_AddStringToObject(device, "ids", deviceid);
    cJSON_AddStringToObject(device, "name", deviceid);
    cJSON_AddStringToObject(device, "mf", MANUFACTURER);
    cJSON_AddStringToObject(device, "mdl", "DLIGHT_CSRO_3T3SCR");
    cJSON_AddStringToObject(device, "sw", SOFT_VERSION);

    char *out = cJSON_PrintUnformatted(config_json);
    strcpy(mqttinfo.content, out);
    free(out);
    cJSON_Delete(config_json);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, mqttinfo.content, 0, 0, 1);

    sprintf(mqttinfo.pub_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
    esp_mqtt_client_publish(event->client, mqttinfo.pub_topic, "online", 0, 0, 1);
    dlight_csro_3t3scr_mqtt_update();
}
void csro_dlight_csro_3t3scr_on_message(esp_mqtt_event_handle_t event)
{
    char topic[80];
    char command[10];
    sprintf(topic, "csro/%s/%s/set", sysinfo.mac_str, sysinfo.dev_type);
    if (strncmp(topic, event->topic, event->topic_len) == 0)
    {
        if (strncmp("1", event->data, event->data_len) == 0)
        {
            dlight.target = 100;
        }
        else if (strncmp("0", event->data, event->data_len) == 0)
        {
            dlight.target = 0;
        }
    }
    sprintf(topic, "csro/%s/%s/set/bright", sysinfo.mac_str, sysinfo.dev_type);
    if (strncmp(topic, event->topic, event->topic_len) == 0)
    {
        memset(command, 0, 10);
        strncpy(command, event->data, event->data_len);
        int brightness = atoi(command);
        printf("%s, %d\r\n", command, brightness);
        if (brightness >= 0 && brightness <= 100)
        {
            dlight.target = brightness;
        }
    }
    dlight_csro_3t3scr_mqtt_update();
}

#endif