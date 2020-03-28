#include "csro_common.h"
#include "csro_devices.h"

#define CSRO_MAC_STR "%02x%02x%02x%02x%02x%02x"

csro_system sysinfo;
csro_mqtt mqttinfo;
esp_mqtt_client_handle_t mqttclient;

void csro_reset_router(void)
{
    nvs_handle handle;
    nvs_open("system", NVS_READWRITE, &handle);
    nvs_set_u8(handle, "router_flag", 0);
    nvs_commit(handle);
    nvs_close(handle);
    esp_restart();
}

void csro_main(void)
{
    //check whether router information exist, if exist read router informations.
    nvs_handle handle;
    nvs_flash_init();
    nvs_open("system", NVS_READONLY, &handle);
    nvs_get_u8(handle, "router_flag", &sysinfo.router_flag);
    if (sysinfo.router_flag == true)
    {
        size_t len = 0;
        nvs_get_str(handle, "router_ssid", NULL, &len);
        nvs_get_str(handle, "router_ssid", sysinfo.router_ssid, &len);
        nvs_get_str(handle, "router_pass", NULL, &len);
        nvs_get_str(handle, "router_pass", sysinfo.router_pass, &len);
    }
    nvs_close(handle);

    //init related hardware functions.
    csro_device_init();

    //check router and enter mqtt or smart tasks.
    if (sysinfo.router_flag == true)
    {
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_MODE_STA, mac);
        sprintf(sysinfo.mac_str, CSRO_MAC_STR, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        sprintf(sysinfo.host_name, "csro-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        sprintf(mqttinfo.lwt_topic, "csro/%s/%s/available", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(mqttinfo.id, "%s-%s", sysinfo.mac_str, sysinfo.dev_type);
        sprintf(mqttinfo.name, CSRO_MAC_STR, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        sprintf(mqttinfo.pass, CSRO_MAC_STR, mac[1], mac[3], mac[5], mac[0], mac[2], mac[4]);
        csro_mqtt_task();
    }
    else
    {
        csro_smart_task();
    }
}