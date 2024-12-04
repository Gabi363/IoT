/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_mac.h"
#include "driver/gpio.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "esp_log.h"

#include <stdio.h>
#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define BLINK_GPIO GPIO_NUM_2

#define TEMP_BUS 23
DeviceAddress tempSensors[2];


static const char *TAG = "wifi station";

static int s_retry_num = 0;

bool volatile WIFI_CONNECTED = false;

void led_task(void *pvParameters){
    while(1){
        if(WIFI_CONNECTED){
            gpio_set_level(BLINK_GPIO, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        else{
            gpio_set_level(BLINK_GPIO, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(BLINK_GPIO, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

static void http_task(void *pvParameters);
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
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
         
        WIFI_CONNECTED = true;

        xTaskCreate(&http_task, "http_task", 8192, NULL, 5, NULL);  // Tworzenie zadania HTTP GET
    }
}

void wifi_init_sta(void)
{
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
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

static void http_get_task(void *pvParameters);
static void http_task(void *pvParameters){
    http_get_task(pvParameters);
    vTaskDelete(NULL);
}

static void http_get_task(void *pvParameters) {
    const char *REQUEST = "GET / HTTP/1.1\r\nHost: www.columbia.edu\r\nConnection: close\r\n\r\n";
    const char *WEB_SERVER = "www.columbia.edu";
    const int WEB_PORT = 80;

    char recv_buf[512];
    int sock;

    // while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr("162.159.138.64");  // Adres IP 
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(WEB_PORT);

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            return;
            // break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", WEB_SERVER, WEB_PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            close(sock);
            return;
            // vTaskDelay(10000 / portTICK_PERIOD_MS);
            // continue;
        }
        ESP_LOGI(TAG, "Successfully connected");

        err = send(sock, REQUEST, strlen(REQUEST), 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            close(sock);
            // vTaskDelay(10000 / portTICK_PERIOD_MS);
            // continue;
            return;
        }
        ESP_LOGI(TAG, "Request sent");

        int len;
        do {
            len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
                return;
            } else if (len > 0) {
                recv_buf[len] = 0;  // Zakończ string
                ESP_LOGI(TAG, "Received data: %s", recv_buf);
            }
        } while (len > 0);

        close(sock);
        ESP_LOGI(TAG, "Socket closed");

        // vTaskDelay(10000 / portTICK_PERIOD_MS);  // Opóźnienie 10 s
    // }
}












// MQTT_MQTT
/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// #include <stdio.h>
// #include <stdint.h>
// #include <string.h>
// #include <stdlib.h>
// #include <inttypes.h>
// #include "esp_system.h"
// #include "nvs_flash.h"
// #include "esp_event.h"
// #include "esp_netif.h"
// #include "esp_log.h"
// #include "mqtt_client.h"
// #include "esp_mac.h"

// static const char *TAG = "mqtt_example";

// static void log_error_if_nonzero(const char *message, int error_code)
// {
//     if (error_code != 0) {
//         ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
//     }
// }

// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
// {
//     ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
//     esp_mqtt_event_handle_t event = event_data;
//     esp_mqtt_client_handle_t client = event->client;
//     int msg_id;
//     switch ((esp_mqtt_event_id_t)event_id) {
//     case MQTT_EVENT_CONNECTED:
//         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
//         msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
//         ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

//         msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
//         ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

//         msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
//         ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

//         msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
//         ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
//         break;
//     case MQTT_EVENT_DISCONNECTED:
//         ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
//         break;

//     case MQTT_EVENT_SUBSCRIBED:
//         ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//         msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
//         ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
//         break;
//     case MQTT_EVENT_UNSUBSCRIBED:
//         ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
//         break;
//     case MQTT_EVENT_PUBLISHED:
//         ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
//         break;
//     case MQTT_EVENT_DATA:
//         ESP_LOGI(TAG, "MQTT_EVENT_DATA");
//         printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//         printf("DATA=%.*s\r\n", event->data_len, event->data);
//         break;
//     case MQTT_EVENT_ERROR:
//         ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
//         if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
//             log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
//             log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
//             log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
//             ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
//         }
//         break;
//     default:
//         ESP_LOGI(TAG, "Other event id:%d", event->event_id);
//         break;
//     }
// }

// static void mqtt_app_start(void)
// {
//     esp_mqtt_client_config_t mqtt_cfg = {
//         .broker.address.uri = CONFIG_BROKER_URL,
//     };
// #if CONFIG_BROKER_URL_FROM_STDIN
//     char line[128];

//     if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
//         int count = 0;
//         printf("Please enter url of mqtt broker\n");
//         while (count < 128) {
//             int c = fgetc(stdin);
//             if (c == '\n') {
//                 line[count] = '\0';
//                 break;
//             } else if (c > 0 && c < 127) {
//                 line[count] = c;
//                 ++count;
//             }
//             vTaskDelay(10 / portTICK_PERIOD_MS);
//         }
//         mqtt_cfg.broker.address.uri = line;
//         printf("Broker url: %s\n", line);
//     } else {
//         ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
//         abort();
//     }
// #endif /* CONFIG_BROKER_URL_FROM_STDIN */

//     esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
//     /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
//     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
//     esp_mqtt_client_start(client);
// }


void app_main(void)
{
    // led blinking
    esp_rom_gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    xTaskCreate(&led_task, "led_task", 8192, NULL, 5, NULL); 

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();










//     ESP_LOGI(TAG, "[APP] Startup..");
//     ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
//     ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

//     esp_log_level_set("*", ESP_LOG_INFO);
//     esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
//     esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
//     esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
//     esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
//     esp_log_level_set("transport", ESP_LOG_VERBOSE);
//     esp_log_level_set("outbox", ESP_LOG_VERBOSE);

//     ESP_ERROR_CHECK(nvs_flash_init());
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//      */
//     ESP_ERROR_CHECK(example_connect());

//     mqtt_app_start();


}










// GATT_GATT_GATT

// /****************************************************************************
// *
// * This demo showcases BLE GATT client. It can scan BLE devices and connect to one device.
// * Run the gatt_server demo, the client demo will automatically connect to the gatt_server demo.
// * Client demo will enable gatt_server's notify after connection. The two devices will then exchange
// * data.
// *
// ****************************************************************************/

// #include <stdint.h>
// #include <string.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include "nvs.h"
// #include "nvs_flash.h"

// #include "esp_bt.h"
// #include "esp_gap_ble_api.h"
// #include "esp_gattc_api.h"
// #include "esp_gatt_defs.h"
// #include "esp_bt_main.h"
// #include "esp_gatt_common_api.h"
// #include "esp_log.h"
// #include "freertos/FreeRTOS.h"

// #define GATTC_TAG "GATTC_DEMO"
// #define REMOTE_SERVICE_UUID        0x00FF
// #define REMOTE_NOTIFY_CHAR_UUID    0xFF01
// #define PROFILE_NUM      1
// #define PROFILE_A_APP_ID 0
// #define INVALID_HANDLE   0

// static char remote_device_name[ESP_BLE_ADV_DATA_LEN_MAX] = "ESP_GATTS_DEMO";
// static bool connect    = false;
// static bool get_server = false;
// static esp_gattc_char_elem_t *char_elem_result   = NULL;
// static esp_gattc_descr_elem_t *descr_elem_result = NULL;

// /* Declare static functions */
// static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
// static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
// static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


// static esp_bt_uuid_t remote_filter_service_uuid = {
//     .len = ESP_UUID_LEN_16,
//     .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
// };

// static esp_bt_uuid_t remote_filter_char_uuid = {
//     .len = ESP_UUID_LEN_16,
//     .uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
// };

// static esp_bt_uuid_t notify_descr_uuid = {
//     .len = ESP_UUID_LEN_16,
//     .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
// };

// static esp_ble_scan_params_t ble_scan_params = {
//     .scan_type              = BLE_SCAN_TYPE_ACTIVE,
//     .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
//     .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
//     .scan_interval          = 0x50,
//     .scan_window            = 0x30,
//     .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
// };

// struct gattc_profile_inst {
//     esp_gattc_cb_t gattc_cb;
//     uint16_t gattc_if;
//     uint16_t app_id;
//     uint16_t conn_id;
//     uint16_t service_start_handle;
//     uint16_t service_end_handle;
//     uint16_t char_handle;
//     esp_bd_addr_t remote_bda;
// };

// /* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
// static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
//     [PROFILE_A_APP_ID] = {
//         .gattc_cb = gattc_profile_event_handler,
//         .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
//     },
// };

// static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
// {
//     esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

//     switch (event) {
//     case ESP_GATTC_REG_EVT:
//         ESP_LOGI(GATTC_TAG, "GATT client register, status %d, app_id %d, gattc_if %d", param->reg.status, param->reg.app_id, gattc_if);
//         esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
//         if (scan_ret){
//             ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
//         }
//         break;
//     case ESP_GATTC_CONNECT_EVT:{
//         ESP_LOGI(GATTC_TAG, "Connected, conn_id %d, remote "ESP_BD_ADDR_STR"", p_data->connect.conn_id,
//                  ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
//         gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
//         memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
//         esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
//         if (mtu_ret){
//             ESP_LOGE(GATTC_TAG, "Config MTU error, error code = %x", mtu_ret);
//         }
//         break;
//     }
//     case ESP_GATTC_OPEN_EVT:
//         if (param->open.status != ESP_GATT_OK){
//             ESP_LOGE(GATTC_TAG, "Open failed, status %d", p_data->open.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "Open successfully, MTU %u", p_data->open.mtu);
//         break;
//     case ESP_GATTC_DIS_SRVC_CMPL_EVT:
//         if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
//             ESP_LOGE(GATTC_TAG, "Service discover failed, status %d", param->dis_srvc_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "Service discover complete, conn_id %d", param->dis_srvc_cmpl.conn_id);
//         esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &remote_filter_service_uuid);
//         break;
//     case ESP_GATTC_CFG_MTU_EVT:
//         ESP_LOGI(GATTC_TAG, "MTU exchange, status %d, MTU %d", param->cfg_mtu.status, param->cfg_mtu.mtu);
//         break;
//     case ESP_GATTC_SEARCH_RES_EVT: {
//         ESP_LOGI(GATTC_TAG, "Service search result, conn_id = %x, is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
//         ESP_LOGI(GATTC_TAG, "start handle %d, end handle %d, current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
//         if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
//             ESP_LOGI(GATTC_TAG, "Service found");
//             get_server = true;
//             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
//             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
//             ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
//         }
//         break;
//     }
//     case ESP_GATTC_SEARCH_CMPL_EVT:
//         if (p_data->search_cmpl.status != ESP_GATT_OK){
//             ESP_LOGE(GATTC_TAG, "Service search failed, status %x", p_data->search_cmpl.status);
//             break;
//         }
//         if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
//             ESP_LOGI(GATTC_TAG, "Get service information from remote device");
//         } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
//             ESP_LOGI(GATTC_TAG, "Get service information from flash");
//         } else {
//             ESP_LOGI(GATTC_TAG, "Unknown service source");
//         }
//         ESP_LOGI(GATTC_TAG, "Service search complete");
//         if (get_server){
//             uint16_t count = 0;
//             esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
//                                                                      p_data->search_cmpl.conn_id,
//                                                                      ESP_GATT_DB_CHARACTERISTIC,
//                                                                      gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
//                                                                      gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
//                                                                      INVALID_HANDLE,
//                                                                      &count);
//             if (status != ESP_GATT_OK){
//                 ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
//                 break;
//             }

//             if (count > 0){
//                 char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
//                 if (!char_elem_result){
//                     ESP_LOGE(GATTC_TAG, "gattc no mem");
//                     break;
//                 }else{
//                     status = esp_ble_gattc_get_char_by_uuid( gattc_if,
//                                                              p_data->search_cmpl.conn_id,
//                                                              gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
//                                                              gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
//                                                              remote_filter_char_uuid,
//                                                              char_elem_result,
//                                                              &count);
//                     if (status != ESP_GATT_OK){
//                         ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
//                         free(char_elem_result);
//                         char_elem_result = NULL;
//                         break;
//                     }

//                     /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
//                     if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
//                         gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
//                         esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
//                     }
//                 }
//                 /* free char_elem_result */
//                 free(char_elem_result);
//             }else{
//                 ESP_LOGE(GATTC_TAG, "no char found");
//             }
//         }
//          break;
//     case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
//         if (p_data->reg_for_notify.status != ESP_GATT_OK){
//             ESP_LOGE(GATTC_TAG, "Notification register failed, status %d", p_data->reg_for_notify.status);
//         }else{
//             ESP_LOGI(GATTC_TAG, "Notification register successfully");
//             uint16_t count = 0;
//             uint16_t notify_en = 1;
//             esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
//                                                                          gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//                                                                          ESP_GATT_DB_DESCRIPTOR,
//                                                                          gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
//                                                                          gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
//                                                                          gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//                                                                          &count);
//             if (ret_status != ESP_GATT_OK){
//                 ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
//                 break;
//             }
//             if (count > 0){
//                 descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
//                 if (!descr_elem_result){
//                     ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
//                     break;
//                 }else{
//                     ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
//                                                                          gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//                                                                          p_data->reg_for_notify.handle,
//                                                                          notify_descr_uuid,
//                                                                          descr_elem_result,
//                                                                          &count);
//                     if (ret_status != ESP_GATT_OK){
//                         ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
//                         free(descr_elem_result);
//                         descr_elem_result = NULL;
//                         break;
//                     }
//                     /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
//                     if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
//                         ret_status = esp_ble_gattc_write_char_descr( gattc_if,
//                                                                      gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//                                                                      descr_elem_result[0].handle,
//                                                                      sizeof(notify_en),
//                                                                      (uint8_t *)&notify_en,
//                                                                      ESP_GATT_WRITE_TYPE_RSP,
//                                                                      ESP_GATT_AUTH_REQ_NONE);
//                     }

//                     if (ret_status != ESP_GATT_OK){
//                         ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
//                     }

//                     /* free descr_elem_result */
//                     free(descr_elem_result);
//                 }
//             }
//             else{
//                 ESP_LOGE(GATTC_TAG, "decsr not found");
//             }

//         }
//         break;
//     }
//     case ESP_GATTC_NOTIFY_EVT:
//         if (p_data->notify.is_notify){
//             ESP_LOGI(GATTC_TAG, "Notification received");
//         }else{
//             ESP_LOGI(GATTC_TAG, "Indication received");
//         }
//         ESP_LOG_BUFFER_HEX(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
//         break;
//     case ESP_GATTC_WRITE_DESCR_EVT:
//         if (p_data->write.status != ESP_GATT_OK){
//             ESP_LOGE(GATTC_TAG, "Descriptor write failed, status %x", p_data->write.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "Descriptor write successfully");
//         uint8_t write_char_data[35];
//         for (int i = 0; i < sizeof(write_char_data); ++i)
//         {
//             write_char_data[i] = i % 256;
//         }
//         esp_ble_gattc_write_char( gattc_if,
//                                   gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//                                   gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//                                   sizeof(write_char_data),
//                                   write_char_data,
//                                   ESP_GATT_WRITE_TYPE_RSP,
//                                   ESP_GATT_AUTH_REQ_NONE);
//         break;
//     case ESP_GATTC_SRVC_CHG_EVT: {
//         esp_bd_addr_t bda;
//         memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
//         ESP_LOGI(GATTC_TAG, "Service change from "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(bda));
//         break;
//     }
//     case ESP_GATTC_WRITE_CHAR_EVT:
//         if (p_data->write.status != ESP_GATT_OK){
//             ESP_LOGE(GATTC_TAG, "Characteristic write failed, status %x)", p_data->write.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "Characteristic write successfully");
//         break;
//     case ESP_GATTC_DISCONNECT_EVT:
//         connect = false;
//         get_server = false;
//         ESP_LOGI(GATTC_TAG, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x",
//                  ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda), p_data->disconnect.reason);
//         break;
//     default:
//         break;
//     }
// }

// static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// {
//     uint8_t *adv_name = NULL;
//     uint8_t adv_name_len = 0;
//     switch (event) {
//     case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
//         //the unit of the duration is second
//         uint32_t duration = 30;
//         esp_ble_gap_start_scanning(duration);
//         break;
//     }
//     case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
//         //scan start complete event to indicate scan start successfully or failed
//         if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
//             ESP_LOGE(GATTC_TAG, "Scanning start failed, status %x", param->scan_start_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "Scanning start successfully");

//         break;
//     case ESP_GAP_BLE_SCAN_RESULT_EVT: {
//         esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
//         switch (scan_result->scan_rst.search_evt) {
//         case ESP_GAP_SEARCH_INQ_RES_EVT:
//             adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
//                                                         scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
//                                                         // ESP_BLE_AD_TYPE_NAME_CMPL,
//                                                         &adv_name_len);
//             ESP_LOGI(GATTC_TAG, "Scan result, device "ESP_BD_ADDR_STR", name len %u", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), adv_name_len);
//             ESP_LOG_BUFFER_CHAR(GATTC_TAG, adv_name, adv_name_len);

// #if CONFIG_EXAMPLE_DUMP_ADV_DATA_AND_SCAN_RESP
//             if (scan_result->scan_rst.adv_data_len > 0) {
//                 ESP_LOGI(GATTC_TAG, "adv data:");
//                 ESP_LOG_BUFFER_HEX(GATTC_TAG, &scan_result->scan_rst.ble_adv[0], scan_result->scan_rst.adv_data_len);
//             }
//             if (scan_result->scan_rst.scan_rsp_len > 0) {
//                 ESP_LOGI(GATTC_TAG, "scan resp:");
//                 ESP_LOG_BUFFER_HEX(GATTC_TAG, &scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len], scan_result->scan_rst.scan_rsp_len);
//             }
// #endif

//             if (adv_name != NULL) {
//                 if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
//                     // Note: If there are multiple devices with the same device name, the device may connect to an unintended one.
//                     // It is recommended to change the default device name to ensure it is unique.
//                     ESP_LOGI(GATTC_TAG, "Device found %s", remote_device_name);
//                     if (connect == false) {
//                         connect = true;
//                         ESP_LOGI(GATTC_TAG, "Connect to the remote device");
//                         esp_ble_gap_stop_scanning();
//                         esp_ble_gap_conn_params_t esp_ble_gatt_create_conn;
//                         memcpy(&esp_ble_gatt_create_conn.remote_bda, scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
//                         esp_ble_gatt_create_conn.remote_addr_type = scan_result->scan_rst.ble_addr_type;
//                         esp_ble_gatt_create_conn.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
//                         esp_ble_gatt_create_conn.is_direct = true;
//                         esp_ble_gatt_create_conn.is_aux = false;
//                         esp_ble_gattc_aux_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
//                                             esp_ble_gatt_create_conn);
//                     }
//                 }
//             }
//             break;
//         case ESP_GAP_SEARCH_INQ_CMPL_EVT:
//             break;
//         default:
//             break;
//         }
//         break;
//     }

//     case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
//         if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
//             ESP_LOGE(GATTC_TAG, "Scanning stop failed, status %x", param->scan_stop_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "Scanning stop successfully");
//         break;

//     case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
//         if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
//             ESP_LOGE(GATTC_TAG, "Advertising stop failed, status %x", param->adv_stop_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "Advertising stop successfully");
//         break;
//     case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
//          ESP_LOGI(GATTC_TAG, "Connection params update, status %d, conn_int %d, latency %d, timeout %d",
//                   param->update_conn_params.status,
//                   param->update_conn_params.conn_int,
//                   param->update_conn_params.latency,
//                   param->update_conn_params.timeout);
//         break;
//     case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
//         ESP_LOGI(GATTC_TAG, "Packet length update, status %d, rx %d, tx %d",
//                   param->pkt_data_length_cmpl.status,
//                   param->pkt_data_length_cmpl.params.rx_len,
//                   param->pkt_data_length_cmpl.params.tx_len);
//         break;
//     default:
//         break;
//     }
// }

// static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
// {
//     /* If event is register event, store the gattc_if for each profile */
//     if (event == ESP_GATTC_REG_EVT) {
//         if (param->reg.status == ESP_GATT_OK) {
//             gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
//         } else {
//             ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
//                     param->reg.app_id,
//                     param->reg.status);
//             return;
//         }
//     }

//     /* If the gattc_if equal to profile A, call profile A cb handler,
//      * so here call each profile's callback */
//     do {
//         int idx;
//         for (idx = 0; idx < PROFILE_NUM; idx++) {
//             if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
//                     gattc_if == gl_profile_tab[idx].gattc_if) {
//                 if (gl_profile_tab[idx].gattc_cb) {
//                     gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
//                 }
//             }
//         }
//     } while (0);
// }

// void app_main(void)
// {
//     // Initialize NVS.
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( ret );

//     #if CONFIG_EXAMPLE_CI_PIPELINE_ID
//     memcpy(remote_device_name, esp_bluedroid_get_example_name(), sizeof(remote_device_name));
//     #endif

//     ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

//     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//     ret = esp_bt_controller_init(&bt_cfg);
//     if (ret) {
//         ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
//     if (ret) {
//         ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_bluedroid_init();
//     if (ret) {
//         ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_bluedroid_enable();
//     if (ret) {
//         ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
//         return;
//     }

//     //register the  callback function to the gap module
//     ret = esp_ble_gap_register_callback(esp_gap_cb);
//     if (ret){
//         ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x", __func__, ret);
//         return;
//     }

//     //register the callback function to the gattc module
//     ret = esp_ble_gattc_register_callback(esp_gattc_cb);
//     if(ret){
//         ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x", __func__, ret);
//         return;
//     }

//     ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
//     if (ret){
//         ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x", __func__, ret);
//     }
//     esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
//     if (local_mtu_ret){
//         ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
//     }

// }




