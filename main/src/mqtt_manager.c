#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_manager.h"
#include "config.h"
#include "bmp280.h"


char user[30];
char sending_frequency[18] = "10";
char limits[30] = "limits";
char mac_addr[18] = "mac_address";

esp_mqtt_client_handle_t client;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE("MQTT", "Last error %s: 0x%x", message, error_code);
    }
}

void save_to_nvs(char* data, char* id)
{
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }
    err = nvs_set_str(my_handle, id, data);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) saving ssid to NVS!\n", esp_err_to_name(err));
    }
    nvs_commit(my_handle);
    nvs_close(my_handle);
    ESP_LOGI("NVS", "New %s: %s saved successfully to NVS", id, data);
}
bool load_from_nvs(char* data, char* id)
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
    err = nvs_get_str(my_handle, id, NULL, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) getting %s from NVS!\n", esp_err_to_name(err), id);
        nvs_close(my_handle);
        save_to_nvs(RECEIVE_USER_TOPIC, id);
        return false;
    }
    err = nvs_get_str(my_handle, id, data, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) getting ssid from NVS!\n", esp_err_to_name(err));
        return false;
    }
    nvs_close(my_handle);
    ESP_LOGI("NVS", "%s loaded successfully from NVS: %s", id, data);
    return true;
}



// MQTT PUBLIKACJA
void mqtt_publish_task(void *pvParameters)
{
    while (true)
    {
            bmp280_data_t sensor_data = bmp280_read_data();
            char temperature[50] = "";
            snprintf(temperature, sizeof(temperature), "%.2f", sensor_data.temperature);
            char pressure[50] = "";
            snprintf(pressure, sizeof(pressure), "%.2f ", sensor_data.pressure);

            char topic[100];
            char payload[150];

            snprintf(topic, sizeof(topic), "%s/%s/readings", user, mac_addr);
            snprintf(payload, sizeof(payload), "{\"temperature\": \"%s\", \"pressure\": \"%s\"}", temperature, pressure);

            esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
            ESP_LOGI("MQTT", "Published: %s -> %s", topic, payload);

            int freq = atoi(sending_frequency);
            vTaskDelay(pdMS_TO_TICKS(freq * 10000));
    }
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD("MQTT", "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI("MQTT", "MQTT_EVENT_CONNECTED");

        int msg_id = esp_mqtt_client_subscribe(client, RECEIVE_USER_TOPIC, 1);
        ESP_LOGI("MQTT", "Subscribed to topic, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, RECEIVE_SENDING_TIME, 1);
        ESP_LOGI("MQTT", "Subscribed to topic, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, RECEIVE_TEMPERATURE_LIMITS, 1);
        ESP_LOGI("MQTT", "Subscribed to topic, msg_id=%d", msg_id);
        
        xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
        // xTaskCreate(mqtt_receive_task, "mqtt_receive_task", 4096, NULL, 5, NULL);
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI("MQTT", "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI("MQTT", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI("MQTT", "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI("MQTT", "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI("MQTT", "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI("MQTT", "MQTT_EVENT_DATA");

        char topic[128];
        char data[128];

        snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
        snprintf(data, sizeof(data), "%.*s", event->data_len, event->data);
        ESP_LOGI("MQTT", "Topic: %s, data: %s", topic, data);

        if(strcmp(topic, RECEIVE_USER_TOPIC) == 0) {
            save_to_nvs(data, RECEIVE_USER_TOPIC);
            load_from_nvs(&user, RECEIVE_USER_TOPIC);
        }
        else if(strcmp(topic, RECEIVE_SENDING_TIME) == 0) {
            save_to_nvs(data, RECEIVE_SENDING_TIME);
            load_from_nvs(&sending_frequency, RECEIVE_SENDING_TIME);
            ESP_LOGI("MQTT", "Sending frequency: %s", sending_frequency);
        }
        else if(strcmp(topic, RECEIVE_TEMPERATURE_LIMITS) == 0) {
            save_to_nvs(data, RECEIVE_TEMPERATURE_LIMITS);
            load_from_nvs(&limits, RECEIVE_TEMPERATURE_LIMITS);
            ESP_LOGI("MQTT", "Temperature limits: %s", limits);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI("MQTT", "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI("MQTT", "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI("MQTT", "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        ESP_LOGI("MQTT", "Adres MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        snprintf(mac_addr, sizeof(mac_addr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    load_from_nvs(&user, RECEIVE_USER_TOPIC);
    load_from_nvs(&sending_frequency, RECEIVE_SENDING_TIME);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
        .network.timeout_ms = 50000,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}