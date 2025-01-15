#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

void mqtt_app_start(void);
bool load_from_nvs(char* data, char* id);

#endif