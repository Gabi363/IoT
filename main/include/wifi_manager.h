#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"


typedef struct wifi_credentials_t
{
    char ssid[100];
    char pass[100];
} wifi_credentials_t;


void save_wifi_ssid_to_nvs(char* ssid);
void save_wifi_pass_to_nvs(char* pass);
bool load_wifi_credentials_from_nvs(wifi_credentials_t *wifi_credentials);
void save_wifi_credentials_to_nvs(wifi_credentials_t wifi_credentials);
void led_task(void *pvParameters);
void wifi_init_sta(void);

#endif // WIFI_MANAGER_H