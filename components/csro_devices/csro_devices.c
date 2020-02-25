#include "csro_devices.h"

static void device_run_time_task(void *args)
{
    while (true)
    {
        printf("Run %d mins. Free heap is %d\r\n", sysinfo.time_run, esp_get_free_heap_size());
        sysinfo.time_run++;
        vTaskDelay(60000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void csro_device_init(void)
{
    xTaskCreate(device_run_time_task, "device_run_time_task", 2048, NULL, configMAX_PRIORITIES - 10, NULL);

#ifdef NLIGHT_NB_4K4R
    sprintf(sysinfo.dev_type, "nlight_nb_4k4r");
    csro_nlight_nb_4k4r_init();

#elif defined NLIGHT_NB_6K4R
    sprintf(sysinfo.dev_type, "nlight_nb_6k4r");
    csro_nlight_nb_6k4r_init();

#elif defined MOTOR_NB_4K4R
    sprintf(sysinfo.dev_type, "motor_nb_4k4r");
    csro_motor_nb_4k4r_init();

#elif defined AIRMON_CSRO_A
    sprintf(sysinfo.dev_type, "airmon_csro_a");
    csro_airmon_csro_a_init();

#elif defined NLIGHT_SZ_2K2R
    sprintf(sysinfo.dev_type, "nlight_sz_2k2r");
    csro_nlight_sz_2k2r_init();
#endif
}

void csro_device_on_connect(esp_mqtt_event_handle_t event)
{
#ifdef NLIGHT_NB_4K4R
    csro_nlight_nb_4k4r_on_connect(event);

#elif defined NLIGHT_NB_6K4R
    csro_nlight_nb_6k4r_on_connect(event);

#elif defined MOTOR_NB_4K4R
    csro_motor_nb_4k4r_on_connect(event);

#elif defined AIRMON_CSRO_A
    csro_airmon_csro_a_on_connect(event);

#elif defined NLIGHT_SZ_2K2R
    csro_nlight_sz_2k2r_on_connect(event);
#endif
}

void csro_device_on_message(esp_mqtt_event_handle_t event)
{
#ifdef NLIGHT_NB_4K4R
    csro_nlight_nb_4k4r_on_message(event);

#elif defined NLIGHT_NB_6K4R
    csro_nlight_nb_6k4r_on_message(event);

#elif defined MOTOR_NB_4K4R
    csro_motor_nb_4k4r_on_message(event);

#elif defined AIRMON_CSRO_A
    csro_airmon_csro_a_on_message(event);

#elif defined NLIGHT_SZ_2K2R
    csro_nlight_sz_2k2r_on_message(event);
#endif
}