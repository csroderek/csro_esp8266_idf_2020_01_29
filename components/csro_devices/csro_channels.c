#include "csro_devices.h"

void csro_nlight_channel_config(uint8_t index, char *model)
{
    char prefix[50], devid[50], uid[50], state[50], command[50];
    sprintf(mqttinfo.pub_topic, "csro/light/%s-%s-%d/config", sysinfo.mac_str, sysinfo.dev_type, index);
    sprintf(prefix, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(devid, "%s-%s", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(uid, "%s-%s-%d", sysinfo.mac_str, sysinfo.dev_type, index);
    sprintf(state, "{{value_json.state[%d]}}", index);
    sprintf(command, "~/set/%d", index);

    cJSON *config_json = cJSON_CreateObject();
    cJSON *device = cJSON_CreateObject();
    cJSON_AddStringToObject(config_json, "~", prefix);
    cJSON_AddStringToObject(config_json, "uniq_id", uid);
    cJSON_AddStringToObject(config_json, "name", uid);
    cJSON_AddStringToObject(config_json, "cmd_t", command);
    cJSON_AddNumberToObject(config_json, "pl_on", 1);
    cJSON_AddNumberToObject(config_json, "pl_off", 0);
    cJSON_AddStringToObject(config_json, "stat_t", "~/state");
    cJSON_AddStringToObject(config_json, "stat_val_tpl", state);
    cJSON_AddStringToObject(config_json, "avty_t", "~/available");

    cJSON_AddItemToObject(config_json, "dev", device);
    cJSON_AddStringToObject(device, "ids", devid);
    cJSON_AddStringToObject(device, "name", devid);
    cJSON_AddStringToObject(device, "mf", MANUFACTURER);
    cJSON_AddStringToObject(device, "mdl", model);
    cJSON_AddStringToObject(device, "sw", SOFT_VERSION);

    char *out = cJSON_PrintUnformatted(config_json);
    strcpy(mqttinfo.content, out);
    free(out);
    cJSON_Delete(config_json);
}

void csro_motor_channel_config(uint8_t index, char *model)
{
    char prefix[50], devid[50], uid[50], state[50], command[50];
    sprintf(mqttinfo.pub_topic, "csro/cover/%s-%s-%d/config", sysinfo.mac_str, sysinfo.dev_type, index);
    sprintf(prefix, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(devid, "%s-%s", sysinfo.mac_str, sysinfo.dev_type);
    sprintf(uid, "%s-%s-%d", sysinfo.mac_str, sysinfo.dev_type, index);
    sprintf(state, "{{value_json.state[%d]}}", index);
    sprintf(command, "~/set/%d", index);

    cJSON *config_json = cJSON_CreateObject();
    cJSON *device = cJSON_CreateObject();
    cJSON_AddStringToObject(config_json, "~", prefix);
    cJSON_AddStringToObject(config_json, "uniq_id", uid);
    cJSON_AddStringToObject(config_json, "name", uid);
    cJSON_AddStringToObject(config_json, "cmd_t", command);
    cJSON_AddStringToObject(config_json, "pos_t", "~/state");
    cJSON_AddStringToObject(config_json, "val_tpl", state);
    cJSON_AddStringToObject(config_json, "pl_open", "open");
    cJSON_AddStringToObject(config_json, "pl_stop", "stop");
    cJSON_AddStringToObject(config_json, "pl_cls", "close");
    cJSON_AddStringToObject(config_json, "dev_cla", "curtain");
    cJSON_AddStringToObject(config_json, "avty_t", "~/available");

    cJSON_AddItemToObject(config_json, "dev", device);
    cJSON_AddStringToObject(device, "ids", devid);
    cJSON_AddStringToObject(device, "name", devid);
    cJSON_AddStringToObject(device, "mf", MANUFACTURER);
    cJSON_AddStringToObject(device, "mdl", model);
    cJSON_AddStringToObject(device, "sw", SOFT_VERSION);

    char *out = cJSON_PrintUnformatted(config_json);
    strcpy(mqttinfo.content, out);
    free(out);
    cJSON_Delete(config_json);
}