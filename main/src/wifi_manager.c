#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"
#include <stdio.h>
// #include "ble_manager.h"
#include <stdbool.h>
#include "sdkconfig.h"
// #include "host/ble_hs.h"
// #include "host/ble_uuid.h"
// #include "host/util/util.h"
// #include "nimble/ble.h"
// #include "nimble/nimble_port.h"
// #include "nimble/nimble_port_freertos.h"
// #include "gap.h"
// #include "gatt_svc.h"
// #include "cJSON.h"
#include "esp_bt.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "config.h"
#include "wifi_manager.h"

static int s_retry_num = 0;
bool volatile WIFI_CONNECTED = false;
static EventGroupHandle_t s_wifi_event_group;
wifi_credentials_t wifi_credentials = {0};


void save_wifi_ssid_to_nvs(char* ssid)
{
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }
    err = nvs_set_str(my_handle, "ssid", ssid);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) saving ssid to NVS!\n", esp_err_to_name(err));
    }
    nvs_commit(my_handle);
    nvs_close(my_handle);
    ESP_LOGI("NVS", "New ssid: %s saved successfully to NVS", ssid);
}

void save_wifi_pass_to_nvs(char* pass)
{
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }
    err = nvs_set_str(my_handle, "pass", pass);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) saving pass to NVS!\n", esp_err_to_name(err));
    }
    nvs_commit(my_handle);
    nvs_close(my_handle);
    ESP_LOGI("NVS", "New password: %s saved successfully to NVS", pass);
}

bool load_wifi_credentials_from_nvs(wifi_credentials_t *wifi_credentials)
{
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }
    size_t required_size;
    err = nvs_get_str(my_handle, "ssid", NULL, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) getting ssid from NVS!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        save_wifi_ssid_to_nvs("ssid");
        save_wifi_pass_to_nvs("pass");
        return false;
    }
    err = nvs_get_str(my_handle, "ssid", wifi_credentials->ssid, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) getting ssid from NVS!\n", esp_err_to_name(err));
        return false;
    }

    err = nvs_get_str(my_handle, "pass", NULL, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) getting pass from NVS!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        save_wifi_pass_to_nvs("pass");
        return false;
    }
    err = nvs_get_str(my_handle, "pass", wifi_credentials->pass, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) getting pass from NVS!\n", esp_err_to_name(err));
        return false;
    }
    nvs_close(my_handle);
    ESP_LOGI("NVS", "Wifi credentials loaded successfully from NVS\nssid: %s password: %s", wifi_credentials->ssid, wifi_credentials->pass);
    return true;
}

void save_wifi_credentials_to_nvs(wifi_credentials_t wifi_credentials)
{
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }
    err = nvs_set_str(my_handle, "ssid", wifi_credentials.ssid);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) saving ssid to NVS!\n", esp_err_to_name(err));
    }
    err = nvs_set_str(my_handle, "pass", wifi_credentials.pass);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) saving pass to NVS!\n", esp_err_to_name(err));
    }
    nvs_commit(my_handle);
    nvs_close(my_handle);
    ESP_LOGI("NVS", "Wifi credentials saved successfully to NVS");
}

// static void http_task(void *pvParameters);
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        WIFI_CONNECTED = true;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {

        WIFI_CONNECTED = false;
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI("WIFI", "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI("WIFI","connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        WIFI_CONNECTED = true;
        // xTaskCreate(&http_task, "http_task", 8192, NULL, 5, NULL);  // Tworzenie zadania HTTP GET
    }
}

void wifi_init_sta(void)
{
    load_wifi_credentials_from_nvs(&wifi_credentials);

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {wifi_credentials.ssid},
            .password = {wifi_credentials.pass},
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, (char *)wifi_credentials.ssid, 100);
    strncpy((char *)wifi_config.sta.password, (char *)wifi_credentials.pass, 100);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI("WIFI", "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("WIFI", "connected to ap SSID:%s password:%s", wifi_credentials.ssid, wifi_credentials.pass);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI("WIFI", "Failed to connect to SSID:%s, password:%s", wifi_credentials.ssid, wifi_credentials.pass);
    } else {
        ESP_LOGE("WIFI", "UNEXPECTED EVENT");
    }
}