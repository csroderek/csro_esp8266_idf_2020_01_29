#include "csro_devices.h"

static void device_run_time_task(void *args)
{
    while (true)
    {
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        sysinfo.rssi = ap_info.rssi;
        sysinfo.time_run++;
        vTaskDelay(60000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void csro_device_init(void)
{
    xTaskCreate(device_run_time_task, "device_run_time_task", 2048, NULL, configMAX_PRIORITIES - 10, NULL);

#ifdef NLIGHT_NB_4K4R
    sprintf(sysinfo.dev_type, "nlight-nb-4k4r");
    csro_nlight_nb_4k4r_init();

#elif defined NLIGHT_NB_6K4R
    sprintf(sysinfo.dev_type, "nlight-nb-6k4r");
    csro_nlight_nb_6k4r_init();

#elif defined MOTOR_NB_4K4R
    sprintf(sysinfo.dev_type, "motor-nb-4k4r");
    csro_motor_nb_4k4r_init();

#elif defined AIRMON_CSRO_A
    sprintf(sysinfo.dev_type, "airmon-csro-a");
    csro_airmon_csro_a_init();

#elif defined NLIGHT_SZ_2K2R
    sprintf(sysinfo.dev_type, "nlight-sz-2k2r");
    csro_nlight_sz_2k2r_init();

#elif defined MOTOR_CSRO_3T2R
    sprintf(sysinfo.dev_type, "motor-csro-3t2r");
    csro_motor_csro_3t2r_init();

#elif defined DLIGHT_CSRO_3T3SCR
    sprintf(sysinfo.dev_type, "dlight-csro-3t3scr");
    csro_dlight_csro_3t3scr_init();

#elif defined NLIGHT_CSRO_2T2SCR
    sprintf(sysinfo.dev_type, "nlight-csro-2t2scr");
    csro_nlight_csro_2t2scr_init();

#elif defined NLIGHT_CSRO_3T3SCR
    sprintf(sysinfo.dev_type, "nlight-csro-3t3scr");
    csro_nlight_csro_3t3scr_init();
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

#elif defined MOTOR_CSRO_3T2R
    csro_motor_csro_3t2r_on_connect(event);

#elif defined DLIGHT_CSRO_3T3SCR
    csro_dlight_csro_3t3scr_on_connect(event);

#elif defined NLIGHT_CSRO_2T2SCR
    csro_nlight_csro_2t2scr_on_connect(event);

#elif defined NLIGHT_CSRO_3T3SCR
    csro_nlight_csro_3t3scr_on_connect(event);
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

#elif defined MOTOR_CSRO_3T2R
    csro_motor_csro_3t2r_on_message(event);

#elif defined DLIGHT_CSRO_3T3SCR
    csro_dlight_csro_3t3scr_on_message(event);

#elif defined NLIGHT_CSRO_2T2SCR
    csro_nlight_csro_2t2scr_on_message(event);

#elif defined NLIGHT_CSRO_3T3SCR
    csro_nlight_csro_3t3scr_on_message(event);
#endif
}