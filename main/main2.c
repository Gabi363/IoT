// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "nvs.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"
// #include "esp_mac.h"
// #include "driver/gpio.h"
// #include "lwip/sockets.h"
// #include <stdio.h>
// #include "ble_manager.h"
// #include <stdbool.h>
// #include "sdkconfig.h"
// #include "host/ble_hs.h"
// #include "host/ble_uuid.h"
// #include "host/util/util.h"
// #include "nimble/ble.h"
// #include "nimble/nimble_port.h"
// #include "nimble/nimble_port_freertos.h"
// #include "gap.h"
// #include "gatt_svc.h"
// #include "cJSON.h"
// #include "esp_bt.h"
// #include "esp_random.h"

// #include "wifi_manager.h"
// #include "config.h"

// // #define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

// // #if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
// // #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
// // #define EXAMPLE_H2E_IDENTIFIER ""
// // #elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
// // #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
// // #define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
// // #elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
// // #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
// // #define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
// // #endif
// // #if CONFIG_ESP_WIFI_AUTH_OPEN
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
// // #elif CONFIG_ESP_WIFI_AUTH_WEP
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
// // #elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
// // #elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
// // #elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
// // #elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
// // #elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
// // #elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
// // #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
// // #endif

// // static EventGroupHandle_t s_wifi_event_group;

// // #define WIFI_CONNECTED_BIT BIT0
// // #define WIFI_FAIL_BIT      BIT1

// // #define BLINK_GPIO GPIO_NUM_2



// // #define DEVICE_NAME "My ESP32 Beacon"
// // #define WIFI_SSID_MAX_LEN 64
// // #define WIFI_PASS_MAX_LEN 64
// // #define BLINK_GPIO GPIO_NUM_2


// // static TaskHandle_t xHeartIndicateTask = NULL;
// // static int s_retry_num = 0;
// // bool volatile WIFI_CONNECTED = false;
// // typedef struct
// // {
// //     char ssid[100];
// //     char pass[100];
// // } wifi_credentials_t;
// // wifi_credentials_t wifi_credentials;


// // void save_wifi_ssid_to_nvs(char* ssid)
// // {
// //     esp_err_t err;
// //     nvs_handle my_handle;
// //     err = nvs_open("storage", NVS_READWRITE, &my_handle);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
// //         return;
// //     }
// //     err = nvs_set_str(my_handle, "ssid", ssid);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) saving ssid to NVS!\n", esp_err_to_name(err));
// //     }
// //     nvs_commit(my_handle);
// //     nvs_close(my_handle);
// //     ESP_LOGI("NVS", "New ssid: %s saved successfully to NVS", ssid);
// // }
// // void save_wifi_pass_to_nvs(char* pass)
// // {
// //     esp_err_t err;
// //     nvs_handle my_handle;
// //     err = nvs_open("storage", NVS_READWRITE, &my_handle);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
// //         return;
// //     }
// //     err = nvs_set_str(my_handle, "pass", pass);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) saving pass to NVS!\n", esp_err_to_name(err));
// //     }
// //     nvs_commit(my_handle);
// //     nvs_close(my_handle);
// //     ESP_LOGI("NVS", "New password: %s saved successfully to NVS", pass);
// // }
// // bool load_wifi_credentials_from_nvs(wifi_credentials_t *wifi_credentials)
// // {
// //     esp_err_t err;
// //     nvs_handle my_handle;
// //     err = nvs_open("storage", NVS_READWRITE, &my_handle);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
// //         return false;
// //     }
// //     size_t required_size;
// //     err = nvs_get_str(my_handle, "ssid", NULL, &required_size);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) getting ssid from NVS!\n", esp_err_to_name(err));
// //         nvs_close(my_handle);
// //         save_wifi_ssid_to_nvs("ssid");
// //         save_wifi_pass_to_nvs("pass");
// //         return false;
// //     }
// //     err = nvs_get_str(my_handle, "ssid", wifi_credentials->ssid, &required_size);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) getting ssid from NVS!\n", esp_err_to_name(err));
// //         return false;
// //     }

// //     err = nvs_get_str(my_handle, "pass", NULL, &required_size);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) getting pass from NVS!\n", esp_err_to_name(err));
// //         nvs_close(my_handle);
// //         save_wifi_pass_to_nvs("pass");
// //         return false;
// //     }
// //     err = nvs_get_str(my_handle, "pass", wifi_credentials->pass, &required_size);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) getting pass from NVS!\n", esp_err_to_name(err));
// //         return false;
// //     }
// //     nvs_close(my_handle);
// //     ESP_LOGI("NVS", "Wifi credentials loaded successfully from NVS");
// //     return true;
// // }

// // void save_wifi_credentials_to_nvs(wifi_credentials_t wifi_credentials)
// // {
// //     esp_err_t err;
// //     nvs_handle my_handle;
// //     err = nvs_open("storage", NVS_READWRITE, &my_handle);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
// //         return;
// //     }
// //     err = nvs_set_str(my_handle, "ssid", wifi_credentials.ssid);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) saving ssid to NVS!\n", esp_err_to_name(err));
// //     }
// //     err = nvs_set_str(my_handle, "pass", wifi_credentials.pass);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) saving pass to NVS!\n", esp_err_to_name(err));
// //     }
// //     nvs_commit(my_handle);
// //     nvs_close(my_handle);
// //     ESP_LOGI("NVS", "Wifi credentials saved successfully to NVS");
// // }



// // void led_task(void *pvParameters){
// //     while(1){
// //         if(WIFI_CONNECTED){
// //             gpio_set_level(BLINK_GPIO, 1);
// //             vTaskDelay(100 / portTICK_PERIOD_MS);
// //         }
// //         else{
// //             gpio_set_level(BLINK_GPIO, 1);
// //             vTaskDelay(1000 / portTICK_PERIOD_MS);
// //             gpio_set_level(BLINK_GPIO, 0);
// //             vTaskDelay(1000 / portTICK_PERIOD_MS);
// //         }
// //     }
// // }

// // // static void http_task(void *pvParameters);
// // static void event_handler(void* arg, esp_event_base_t event_base,
// //                                 int32_t event_id, void* event_data)
// // {
// //     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
// //         esp_wifi_connect();
// //     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
// //         WIFI_CONNECTED = true;
// //     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {

// //         WIFI_CONNECTED = false;
// //         if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
// //             esp_wifi_connect();
// //             s_retry_num++;
// //             ESP_LOGI(TAG, "retry to connect to the AP");
// //         } else {
// //             xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
// //         }
// //         ESP_LOGI(TAG,"connect to the AP fail");
// //     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
// //         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
// //         ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
// //         s_retry_num = 0;
// //         xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
// //         WIFI_CONNECTED = true;
// //         // xTaskCreate(&http_task, "http_task", 8192, NULL, 5, NULL);  // Tworzenie zadania HTTP GET
// //     }
// // }

// // void wifi_init_sta(void)
// // {
// //     s_wifi_event_group = xEventGroupCreate();
// //     ESP_ERROR_CHECK(esp_netif_init());
// //     ESP_ERROR_CHECK(esp_event_loop_create_default());
// //     esp_netif_create_default_wifi_sta();

// //     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
// //     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

// //     esp_event_handler_instance_t instance_any_id;
// //     esp_event_handler_instance_t instance_got_ip;
// //     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
// //                                                         ESP_EVENT_ANY_ID,
// //                                                         &event_handler,
// //                                                         NULL,
// //                                                         &instance_any_id));
// //     ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
// //                                                         IP_EVENT_STA_GOT_IP,
// //                                                         &event_handler,
// //                                                         NULL,
// //                                                         &instance_got_ip));

// //     wifi_config_t wifi_config = {
// //         .sta = {
// //             .ssid = {wifi_credentials.ssid},
// //             .password = {wifi_credentials.pass},
// //             .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
// //             .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
// //             .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
// //         },
// //     };
// //     strncpy((char *)wifi_config.sta.ssid, (char *)wifi_credentials.ssid, 100);
// //     strncpy((char *)wifi_config.sta.password, (char *)wifi_credentials.pass, 100);

// //     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
// //     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
// //     ESP_ERROR_CHECK(esp_wifi_start() );

// //     ESP_LOGI(TAG, "wifi_init_sta finished.");

// //     EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
// //             WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
// //             pdFALSE,
// //             pdFALSE,
// //             portMAX_DELAY);

// //     if (bits & WIFI_CONNECTED_BIT) {
// //         ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_credentials.ssid, wifi_credentials.pass);
// //     } else if (bits & WIFI_FAIL_BIT) {
// //         ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_credentials.ssid, wifi_credentials.pass);
// //     } else {
// //         ESP_LOGE(TAG, "UNEXPECTED EVENT");
// //     }
// // }

// // void save_wifi_credentials_to_nvs(wifi_credentials_t wifi_credentials)
// // {
// //     esp_err_t err;
// //     nvs_handle my_handle;
// //     err = nvs_open("storage", NVS_READWRITE, &my_handle);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
// //         return;
// //     }
// //     err = nvs_set_str(my_handle, "ssid", wifi_credentials.ssid);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) saving ssid to NVS!\n", esp_err_to_name(err));
// //     }
// //     err = nvs_set_str(my_handle, "pass", wifi_credentials.pass);
// //     if (err != ESP_OK)
// //     {
// //         ESP_LOGE("NVS", "Error (%s) saving pass to NVS!\n", esp_err_to_name(err));
// //     }
// //     nvs_commit(my_handle);
// //     nvs_close(my_handle);
// //     ESP_LOGI("NVS", "Wifi credentials saved successfully to NVS");
// //     ESP_LOGI("NVS", "New ssid: %s, new password: %s", wifi_credentials.ssid, wifi_credentials.pass);
// // }


// // // BLE_MANAGER

// // // Function prototypes for static functions
// // static void on_stack_reset(int reason);
// // static void on_stack_sync(void);

// // bool ble_init() {
// //     esp_err_t ret;
// //     int rc = 0;

// //     ESP_LOGI(TAG, "Initializing NimBLE host stack");
// //     ret = nimble_port_init();
// //     if (ret != ESP_OK) {
// //         ESP_LOGE(TAG, "NimBLE stack initialization failed");
// //         return false;
// //     }

// //     /* Initialize the NimBLE GAP configuration */
// //     rc = gap_init();
// //     if (rc != 0) {
// //         ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
// //         return false;
// //     }

// //     /* GATT server initialization */
// //     rc = gatt_svc_init();
// //     if (rc != 0) {
// //         ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
// //         return false;
// //     }

// //     /* Set host callbacks */
// //     ESP_LOGI(TAG, "Setting host callbacks");
// //     ble_hs_cfg.reset_cb = on_stack_reset;
// //     ble_hs_cfg.sync_cb = on_stack_sync;  // when controller and host are synchronized -> APP_START
// //     ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

// //     return true;
// // }

// // void nimble_host_task(void *param) {
// //     ESP_LOGI(TAG, "nimble host task has been started!");
// //     /* This function won't return until nimble_port_stop() is executed */
// //     nimble_port_run();
// //     vTaskDelete(NULL);
// // }

// // void nimble_host_stop_task() {
// //     ESP_LOGI(TAG, "Cleaning up NimBLE host stack");
// //     nimble_port_stop();
// //     nimble_port_deinit();
    
// //     ESP_LOGI(TAG, "NimBLE port deinit");
// //     vTaskDelay(5000 / portTICK_PERIOD_MS);
// //     ESP_LOGI(TAG, "NimBLE host stack has been cleaned up");
// // }


// // static void on_stack_reset(int reason) {
// //     /* On reset, print reset reason to console */
// //     ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
// // }

// // static void on_stack_sync(void) {
// //     /* On stack sync, do advertising initialization */
// //     adv_init();
// // }

// //// GATT_SVC
// //// static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
// ////                           struct ble_gatt_access_ctxt *ctxt, void *arg);
// // static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
// //                           struct ble_gatt_access_ctxt *ctxt, void *arg);
// // static void wifi_cred_chr_access_pass(uint16_t conn_handle, uint16_t attr_handle,
// //                           struct ble_gatt_access_ctxt *ctxt, void *arg);
// // static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
// //                           struct ble_gatt_access_ctxt *ctxt, void *arg);
// // static void wifi_cred_chr_access_restart(uint16_t conn_handle, uint16_t attr_handle,
// //                           struct ble_gatt_access_ctxt *ctxt, void *arg);

// // static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
// //                           struct ble_gatt_access_ctxt *ctxt, void *arg);

// /* Private variables */
// /* Heart Rate service */
// static const ble_uuid16_t heart_rate_svc_uuid = BLE_UUID16_INIT(0x180D);

// static uint8_t heart_rate_chr_val[2] = {0};
// static uint16_t heart_rate_chr_val_handle;
// static const ble_uuid16_t heart_rate_chr_uuid = BLE_UUID16_INIT(0x2A37);

// static uint16_t heart_rate_chr_conn_handle = 0;
// static bool heart_rate_chr_conn_handle_inited = false;
// static bool heart_rate_ind_status = false;

// /* Automation IO service */
// static const ble_uuid16_t wifi_cred_svc_uuid = 
//     BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
//                      0x12, 0x12, 0x25, 0x15, 0x00, 0x01);;
// static uint16_t wifi_cred_chr_val_handle_ssid;
// static uint16_t wifi_cred_chr_val_handle_pass;
// static uint16_t wifi_cred_chr_val_handle_restart;

// static const ble_uuid128_t wifi_cred_chr_uuid_ssid =
//     BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
//                      0x12, 0x12, 0x25, 0x15, 0x00, 0x02);
// static const ble_uuid128_t wifi_cred_chr_uuid_pass =
//     BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
//                      0x12, 0x12, 0x25, 0x15, 0x00, 0x03);
// static const ble_uuid128_t wifi_cred_chr_uuid_restart =
//     BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
//                      0x12, 0x12, 0x25, 0x15, 0x00, 0x04);

// /* GATT services table */
// static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
//     /* Heart rate service */
//     {.type = BLE_GATT_SVC_TYPE_PRIMARY,
//      .uuid = &heart_rate_svc_uuid.u,
//      .characteristics =
//          (struct ble_gatt_chr_def[]){
//              {/* Heart rate characteristic */
//               .uuid = &heart_rate_chr_uuid.u,
//               .access_cb = heart_rate_chr_access,
//               .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
//               .val_handle = &heart_rate_chr_val_handle},
//              {
//                  0, /* No more characteristics in this service. */
//              }}},
//     /* Wifi cred service */
//     {
//         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//         .uuid = &wifi_cred_svc_uuid.u,
//         .characteristics =
//             (struct ble_gatt_chr_def[]){/* Wifi cred characteristic */
//                                         {.uuid = &wifi_cred_chr_uuid_ssid.u,
//                                          .access_cb = wifi_cred_chr_access_ssid,
//                                          .flags = BLE_GATT_CHR_F_WRITE,
//                                          .val_handle = &wifi_cred_chr_val_handle_ssid},

//                                         {.uuid = &wifi_cred_chr_uuid_pass.u,
//                                          .access_cb = wifi_cred_chr_access_pass,
//                                          .flags = BLE_GATT_CHR_F_WRITE,
//                                          .val_handle = &wifi_cred_chr_val_handle_pass},

//                                         {.uuid = &wifi_cred_chr_uuid_restart.u,
//                                          .access_cb = wifi_cred_chr_access_restart,
//                                          .flags = BLE_GATT_CHR_F_WRITE,
//                                          .val_handle = &wifi_cred_chr_val_handle_restart},
//                                         {0}},
//     },
//     {
//         0, /* No more services. */
//     },
// };


// // static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
// //                             struct ble_gatt_access_ctxt *ctxt, void *arg) {

// //     /* Handle access events */
// //     /* Note: Wifi cred characteristic is write only */
// //     switch (ctxt->op) {

// //     /* Write characteristic event */
// //     case BLE_GATT_ACCESS_OP_WRITE_CHR:
// //         /* Verify connection handle */
// //         if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
// //             ESP_LOGI(TAG, "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
// //         } else {
// //             ESP_LOGI(TAG, "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
// //         }

// //         /* Verify attribute handle */
// //         if (attr_handle == wifi_cred_chr_val_handle) {
// //             // Allocate a buffer to hold the incoming data
// //             ESP_LOGI(TAG, "Received data length: %d", ctxt->om->om_len);

// //             char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
// //             memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
// //             buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer
// //             ESP_LOGI(TAG, "Received data: %s", buffer);

// //             if(strcmp(buffer, "on") == 0){
// //                 ESP_LOGI(TAG, "turn on led");
// //                 gpio_set_level(BLINK_GPIO, 1);
// //             }
// //             else if(strcmp(buffer, "off") == 0){
// //                 ESP_LOGI(TAG, "turn off led");
// //                 gpio_set_level(BLINK_GPIO, 0);
// //             }

// //             // // Initialize NVS
// //             // esp_err_t err = nvs_flash_init();
// //             // if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
// //             //     // NVS partition was truncated and needs to be erased
// //             //     // Retry nvs_flash_init
// //             //     ESP_ERROR_CHECK(nvs_flash_erase());
// //             //     err = nvs_flash_init();
// //             // }
// //             // ESP_ERROR_CHECK( err );


// //             char *token = strtok(buffer, " ");
// //             if(token != NULL) {
// //                 printf("%s\n", token);
// //                 strcpy(wifi_credentials.ssid, token);    
// //                 token = strtok(NULL, "-");
// //             } if(token != NULL) {
// //                 printf("%s\n", token);
// //                 strcpy(wifi_credentials.pass, token);    
// //             }
// //             save_wifi_credentials_to_nvs(wifi_credentials);
// //             wifi_init_sta();

// //         }
// //     }
// // }
// static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
//                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
//     /* Handle access events */
//     /* Note: Wifi cred characteristic is write only */
//     switch (ctxt->op) {
//     /* Write characteristic event */
//     case BLE_GATT_ACCESS_OP_WRITE_CHR:
//         /* Verify connection handle */
//         if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//             ESP_LOGI(TAG, "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
//         } else {
//             ESP_LOGI(TAG, "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
//         }

//         /* Verify attribute handle */
//         if (attr_handle == wifi_cred_chr_val_handle_ssid) {
//             ESP_LOGI(TAG, "Received data length: %d", ctxt->om->om_len);
//             char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
//             memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
//             buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer
//             ESP_LOGI(TAG, "Received data: %s", buffer);
//             save_wifi_ssid_to_nvs(buffer);  
//             strcpy(wifi_credentials.ssid, buffer);    
//         }
//     }
// }
// static void wifi_cred_chr_access_pass(uint16_t conn_handle, uint16_t attr_handle,
//                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
//     switch (ctxt->op) {
//     case BLE_GATT_ACCESS_OP_WRITE_CHR:
//         if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//             ESP_LOGI(TAG, "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
//         } else {
//             ESP_LOGI(TAG, "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
//         }

//         if (attr_handle == wifi_cred_chr_val_handle_pass) {
//             ESP_LOGI(TAG, "Received data length: %d", ctxt->om->om_len);
//             char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
//             memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
//             buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer
//             ESP_LOGI(TAG, "Received data: %s", buffer);
//             save_wifi_pass_to_nvs(buffer);  
//             strcpy(wifi_credentials.pass, buffer);    
//         }
//     }
// }
// static void wifi_cred_chr_access_restart(uint16_t conn_handle, uint16_t attr_handle,
//                             struct ble_gatt_access_ctxt *ctxt, void *arg) {
//     switch (ctxt->op) {
//     case BLE_GATT_ACCESS_OP_WRITE_CHR:
//         if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//             ESP_LOGI(TAG, "Wi-Fi restart; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
//         } else {
//             ESP_LOGI(TAG, "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
//         }

//         if (attr_handle == wifi_cred_chr_val_handle_restart) {
//             esp_restart();
//         }
//     }
// }

// static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
//     /* Local variables */
//     int rc;

//     /* Handle access events */
//     /* Note: Heart rate characteristic is read only */
//     switch (ctxt->op) {

//     /* Read characteristic event */
//     case BLE_GATT_ACCESS_OP_READ_CHR:
//         /* Verify connection handle */
//         if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//             ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
//                      conn_handle, attr_handle);
//         } else {
//             ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
//                      attr_handle);
//         }

//         /* Verify attribute handle */
//         if (attr_handle == heart_rate_chr_val_handle) {
//             /* Update access buffer value */
//             heart_rate_chr_val[1] = 60 + (uint8_t)(esp_random() % 31);
//             rc = os_mbuf_append(ctxt->om, &heart_rate_chr_val,
//                                 sizeof(heart_rate_chr_val));
//             return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
//         }
//         goto error;

//     /* Unknown event */
//     default:
//         goto error;
//     }

// error:
//     ESP_LOGE(
//         TAG,
//         "unexpected access operation to heart rate characteristic, opcode: %d",
//         ctxt->op);
//     return BLE_ATT_ERR_UNLIKELY;
// }

// /* Public functions */
// void send_heart_rate_indication(void) {
//     if (heart_rate_ind_status && heart_rate_chr_conn_handle_inited) {
//         ble_gatts_indicate(heart_rate_chr_conn_handle,
//                            heart_rate_chr_val_handle);
//         ESP_LOGI(TAG, "heart rate indication sent!");
//     }
// }

// void heart_rate_ind_task(void *pvParameter) {
//     while (1) {
//         send_heart_rate_indication();
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

// /*
//  *  Action when someone subscribes to characteristics
//  */ 
// void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
//     /* Check connection handle */
//     if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//         ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
//                  event->subscribe.conn_handle, event->subscribe.attr_handle);
//     } else {
//         ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
//                  event->subscribe.attr_handle);
//     }

//     /* Check attribute handle */
//     if (event->subscribe.attr_handle == heart_rate_chr_val_handle) {
//         /* Update heart rate subscription status */
//         heart_rate_chr_conn_handle = event->subscribe.conn_handle;
//         heart_rate_chr_conn_handle_inited = true;
//         heart_rate_ind_status = event->subscribe.cur_indicate;

//         /* Create a task to send heart rate indication */
//         if (heart_rate_ind_status && xHeartIndicateTask == NULL) {
//             ESP_LOGI(TAG, "Start heart rate indication task");
//             xTaskCreate(heart_rate_ind_task, "HeartRateIndTask", 4096, NULL, 5, &xHeartIndicateTask);
//         } else {
//             ESP_LOGI(TAG, "Stop heart rate indication task");
//             vTaskDelete(xHeartIndicateTask);
//             xHeartIndicateTask = NULL;
//         }
//     }
// }

// /*
//  *  GATT server initialization
//  *      1. Initialize GATT service
//  *      2. Update NimBLE host GATT services counter
//  *      3. Add GATT services to
//  * 
//  *  server
//  *      4. Create a task to send heart rate indication
//  */
// int gatt_svc_init(void) {
//     int rc;

//     ble_svc_gatt_init();

//     rc = ble_gatts_count_cfg(gatt_svr_svcs);
//     if (rc != 0) {
//         return rc;
//     }

//     rc = ble_gatts_add_svcs(gatt_svr_svcs);
//     if (rc != 0) {
//         return rc;
//     }

//     return 0;
// }











// // GAP
// inline static void format_addr(char *addr_str, uint8_t addr[]);
// static void start_advertising(void);
// static void print_conn_desc(struct ble_gap_conn_desc *desc);
// static int gap_event_handler(struct ble_gap_event *event, void *arg);

// /* Private variables */
// static uint8_t own_addr_type;
// static uint8_t addr_val[6] = {0};
// static uint8_t esp_uri[] = {BLE_GAP_URI_PREFIX_HTTPS, '/', '/', 'e', 's', 'p', 'r', 'e', 's', 's', 'i', 'f', '.', 'c', 'o', 'm'};

// /* Private functions */
// inline static void format_addr(char *addr_str, uint8_t addr[]) {
//     sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
//             addr[2], addr[3], addr[4], addr[5]);
// }

// static void print_conn_desc(struct ble_gap_conn_desc *desc) {
//     /* Local variables */
//     char addr_str[18] = {0};

//     /* Connection handle */
//     ESP_LOGI(TAG, "connection handle: %d", desc->conn_handle);

//     /* Local ID address */
//     format_addr(addr_str, desc->our_id_addr.val);
//     ESP_LOGI(TAG, "device id address: type=%d, value=%s",
//              desc->our_id_addr.type, addr_str);

//     /* Peer ID address */
//     format_addr(addr_str, desc->peer_id_addr.val);
//     ESP_LOGI(TAG, "peer id address: type=%d, value=%s", desc->peer_id_addr.type,
//              addr_str);

//     /* Connection info */
//     ESP_LOGI(TAG,
//              "conn_itvl=%d, conn_latency=%d, supervision_timeout=%d, "
//              "encrypted=%d, authenticated=%d, bonded=%d\n",
//              desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
//              desc->sec_state.encrypted, desc->sec_state.authenticated,
//              desc->sec_state.bonded);
// }

// static void start_advertising(void) {
//     /* Local variables */
//     int rc = 0;
//     const char *name;
//     struct ble_hs_adv_fields adv_fields = {0};
//     struct ble_hs_adv_fields rsp_fields = {0};
//     struct ble_gap_adv_params adv_params = {0};

//     /* Set advertising flags */
//     adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

//     /* Set device name */
//     name = ble_svc_gap_device_name();
//     adv_fields.name = (uint8_t *)name;
//     adv_fields.name_len = strlen(name);
//     adv_fields.name_is_complete = 1;

//     /* Set device tx power */
//     adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
//     adv_fields.tx_pwr_lvl_is_present = 1;

//     /* Set device appearance */
//     adv_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
//     adv_fields.appearance_is_present = 1;

//     /* Set device LE role */
//     adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
//     adv_fields.le_role_is_present = 1;

//     /* Set advertiement fields */
//     rc = ble_gap_adv_set_fields(&adv_fields);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
//         return;
//     }

//     /* Set device address */
//     rsp_fields.device_addr = addr_val;
//     rsp_fields.device_addr_type = own_addr_type;
//     rsp_fields.device_addr_is_present = 1;

//     /* Set URI */
//     rsp_fields.uri = esp_uri;
//     rsp_fields.uri_len = sizeof(esp_uri);

//     /* Set advertising interval */
//     rsp_fields.adv_itvl = BLE_GAP_ADV_ITVL_MS(500);
//     rsp_fields.adv_itvl_is_present = 1; 

//     /* Set scan response fields */
//     rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to set scan response data, error code: %d", rc);
//         return;
//     }

//     /* Set non-connetable and general discoverable mode to be a beacon */
//     adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
//     adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

//     /* Set advertising interval */
//     adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(500);
//     adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(510); 

//     /* Start advertising */
//     rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
//                            gap_event_handler, NULL);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
//         return;
//     }
//     ESP_LOGI(TAG, "advertising started!");
// }

// static int gap_event_handler(struct ble_gap_event *event, void *arg) {
//     int rc = 0;
//     struct ble_gap_conn_desc desc;

//     /* Handle different GAP event */
//     switch (event->type) {

//     /* Connect event */
//     case BLE_GAP_EVENT_CONNECT:
//         /* A new connection was established or a connection attempt failed. */
//         ESP_LOGI(TAG, "connection %s; status=%d",
//                  event->connect.status == 0 ? "established" : "failed",
//                  event->connect.status);

//         /* Connection succeeded */
//         if (event->connect.status == 0) {
//             /* Check connection handle */
//             rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
//             if (rc != 0) {
//                 ESP_LOGE(TAG,
//                          "failed to find connection by handle, error code: %d",
//                          rc);
//                 return rc;
//             }

//             /* Print connection descriptor */
//             print_conn_desc(&desc);

//             /* Try to update connection parameters */
//             struct ble_gap_upd_params params = {.itvl_min = desc.conn_itvl,
//                                                 .itvl_max = desc.conn_itvl,
//                                                 .latency = 3,
//                                                 .supervision_timeout =
//                                                     desc.supervision_timeout};
//             rc = ble_gap_update_params(event->connect.conn_handle, &params);
//             if (rc != 0) {
//                 ESP_LOGE(
//                     TAG,
//                     "failed to update connection parameters, error code: %d",
//                     rc);
//                 return rc;
//             }
//         }
//         /* Connection failed, restart advertising */
//         else {
//             start_advertising();
//         }
//         return rc;

//     /* Disconnect event */
//     case BLE_GAP_EVENT_DISCONNECT:
//         /* A connection was terminated, print connection descriptor */
//         ESP_LOGI(TAG, "disconnected from peer; reason=%d",
//                  event->disconnect.reason);

//         /* Restart advertising */
//         start_advertising();
//         return rc;

//     /* Connection parameters update event */
//     case BLE_GAP_EVENT_CONN_UPDATE:
//         /* The central has updated the connection parameters. */
//         ESP_LOGI(TAG, "connection updated; status=%d",
//                  event->conn_update.status);

//         /* Print connection descriptor */
//         rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
//         if (rc != 0) {
//             ESP_LOGE(TAG, "failed to find connection by handle, error code: %d",
//                      rc);
//             return rc;
//         }
//         print_conn_desc(&desc);
//         return rc;

//     /* Advertising complete event */
//     case BLE_GAP_EVENT_ADV_COMPLETE:
//         /* Advertising completed, restart advertising */
//         ESP_LOGI(TAG, "advertise complete; reason=%d",
//                  event->adv_complete.reason);
//         // start_advertising();
//         return rc;
        
//     /* Notification sent event */
//     case BLE_GAP_EVENT_NOTIFY_TX:
//         if ((event->notify_tx.status != 0) &&
//             (event->notify_tx.status != BLE_HS_EDONE)) {
//             /* Print notification info on error */
//             ESP_LOGI(TAG,
//                      "notify event; conn_handle=%d attr_handle=%d "
//                      "status=%d is_indication=%d",
//                      event->notify_tx.conn_handle, event->notify_tx.attr_handle,
//                      event->notify_tx.status, event->notify_tx.indication);
//         }
//         return rc;

//     /* Subscribe event */
//     case BLE_GAP_EVENT_SUBSCRIBE:
//         /* Print subscription info to log */
//         ESP_LOGI(TAG,
//                  "subscribe event; conn_handle=%d attr_handle=%d "
//                  "reason=%d prevn=%d curn=%d previ=%d curi=%d",
//                  event->subscribe.conn_handle, event->subscribe.attr_handle,
//                  event->subscribe.reason, event->subscribe.prev_notify,
//                  event->subscribe.cur_notify, event->subscribe.prev_indicate,
//                  event->subscribe.cur_indicate);

//         /* GATT subscribe event callback */
//         gatt_svr_subscribe_cb(event);
//         return rc;

//     /* MTU update event */
//     case BLE_GAP_EVENT_MTU:
//         /* Print MTU update info to log */
//         ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
//                  event->mtu.conn_handle, event->mtu.channel_id,
//                  event->mtu.value);
//         return rc;
//     }

//     return rc;
// }

// /* Public functions */
// void adv_init(void) {
//     int rc = 0;
//     char addr_str[18] = {0};

//     /* Make sure we have proper BT identity address set */
//     rc = ble_hs_util_ensure_addr(0);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "device does not have any available bt address!");
//         return;
//     }

//     /* Figure out BT address to use while advertising */
//     rc = ble_hs_id_infer_auto(0, &own_addr_type);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
//         return;
//     }

//     /* Copy device address to addr_val */
//     rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
//         return;
//     }
//     format_addr(addr_str, addr_val);
//     ESP_LOGI(TAG, "device address: %s", addr_str);

//     /* Start advertising. */
//     start_advertising();
// }

// int gap_init(void) {
//     /* Local variables */
//     int rc = 0;

//     /* Initialize GAP service */
//     ble_svc_gap_init();

//     /* Set GAP device name */
//     rc = ble_svc_gap_device_name_set(DEVICE_NAME);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to set device name to %s, error code: %d", DEVICE_NAME, rc);
//         return rc;
//     }

//     /* Set GAP device appearance */
//     rc = ble_svc_gap_device_appearance_set(BLE_GAP_APPEARANCE_GENERIC_TAG);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
//         return rc;
//     }

//     return rc;
// }




// void app_main(void)
// {
//     // led blinking
//     esp_rom_gpio_pad_select_gpio(BLINK_GPIO);
//     gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
//     xTaskCreate(&led_task, "led_task", 8192, NULL, 5, NULL); 


//     // Initialize NVS
//     esp_err_t err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( err );

//     // BLE PERIPHERIAL
//     ble_init();
//     xTaskCreate(nimble_host_task,"NimBLE", 4096, NULL, 5, NULL);
//     // while(true) {
//         vTaskDelay(500 / portTICK_PERIOD_MS);
//     // }

//     // WIFI
//     load_wifi_credentials_from_nvs(&wifi_credentials);
//     ESP_LOGI("MAIN", "ssid: %s password: %s", wifi_credentials.ssid, wifi_credentials.pass);

//     ESP_LOGI("MAIN", "ESP_WIFI_MODE_STA");
//     wifi_init_sta();
        
//     // MQTT
//     // mqtt_app_start();
// }
