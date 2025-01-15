#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
// #include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_manager.h"
#include "config.h"

char user[30] = "user";
char mac_addr[18] = "mac_address";

char *receive_user_topic = "username";

esp_mqtt_client_handle_t client;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE("MQTT", "Last error %s: 0x%x", message, error_code);
    }
}

// MQTT PUBLIKACJA
void mqtt_publish_task(void *pvParameters)
{
    // uint8_t mac[6];
    // char mac_addr[18];
    // if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
    //     ESP_LOGI("MQTT", "Adres MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    //     snprintf(mac_addr, sizeof(mac_addr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // }

    float temperature = 0.0;

    while (true)
    {
        temperature += 0.1;

        char topic[100];
        char payload[100];

        snprintf(topic, sizeof(topic), "%s/%s/temperature", user, mac_addr);
        snprintf(payload, sizeof(payload), "{\"value\": %.2f, \"unit\": \"C\"}", temperature);

        esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        ESP_LOGI("MQTT", "Published: %s -> %s", topic, payload);

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
// mqtt_message_receive
// void mqtt_receive_task(void *pvParameters)
// {
//     while (true)
//     {
//         int status = mqtt_message_receive(client, 10000);
//         ESP_LOGI("MQTT", "Received: %d", status);

//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD("MQTT", "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI("MQTT", "MQTT_EVENT_CONNECTED");

        int msg_id = esp_mqtt_client_subscribe(client, "username", 1);
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

        if(strcmp(topic, receive_user_topic) == 0) {
            strncpy(user, data, sizeof(user) - 1);
            user[sizeof(user) - 1] = '\0';
            
            printf("USERNAME=%s", user);
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

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
        .network.timeout_ms = 50000,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}