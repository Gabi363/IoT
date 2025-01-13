/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "cJSON.h"
#include "esp_bt.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "gatt_svc.h"
#include "driver/gpio.h"
#include "wifi_manager.h"


// #define WIFI_SSID_MAX_LEN 64
// #define WIFI_PASS_MAX_LEN 64
// #define BLINK_GPIO GPIO_NUM_2

static TaskHandle_t xHeartIndicateTask = NULL;








// /* Private function declarations */
// static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                           struct ble_gatt_access_ctxt *ctxt, void *arg);

// static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                           struct ble_gatt_access_ctxt *ctxt, void *arg);

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
// static uint16_t wifi_cred_chr_val_handle;
// static const ble_uuid128_t wifi_cred_chr_uuid =
//     BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
//                      0x12, 0x12, 0x25, 0x15, 0x00, 0x02);

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
//                                         {.uuid = &wifi_cred_chr_uuid.u,
//                                          .access_cb = wifi_cred_chr_access,
//                                          .flags = BLE_GATT_CHR_F_WRITE,
//                                          .val_handle = &wifi_cred_chr_val_handle},
//                                         {0}},
//     },

//     {
//         0, /* No more services. */
//     },
// };

// static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                             struct ble_gatt_access_ctxt *ctxt, void *arg) {

//     /* Handle access events */
//     /* Note: Wifi cred characteristic is write only */
//     switch (ctxt->op) {

//     /* Write characteristic event */
//     case BLE_GATT_ACCESS_OP_WRITE_CHR:
//         /* Verify connection handle */
//         if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//             ESP_LOGI("GATT_SVR", "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
//         } else {
//             ESP_LOGI("GATT_SVR", "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
//         }

//         /* Verify attribute handle */
//         if (attr_handle == wifi_cred_chr_val_handle) {
//             // Allocate a buffer to hold the incoming data
//             ESP_LOGI("GATT_SVR", "Received data length: %d", ctxt->om->om_len);

//             char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
//             memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
//             buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer
//             ESP_LOGI("GATT_SVR", "Received data: %s", buffer);

//             if(strcmp(buffer, "on") == 0){
//                 ESP_LOGI("GATT_SVR", "turn on led");
//                 gpio_set_level(BLINK_GPIO, 1);
//             }
//             else if(strcmp(buffer, "off") == 0){
//                 ESP_LOGI("GATT_SVR", "turn off led");
//                 gpio_set_level(BLINK_GPIO, 0);
//             }

//             // Initialize NVS
//             esp_err_t err = nvs_flash_init();
//             if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//                 // NVS partition was truncated and needs to be erased
//                 // Retry nvs_flash_init
//                 ESP_ERROR_CHECK(nvs_flash_erase());
//                 err = nvs_flash_init();
//             }
//             ESP_ERROR_CHECK( err );

//             strcpy(wifi_credentials.ssid, "truskawkii");
//             strcpy(wifi_credentials.pass, "czekoladaa");
//             save_wifi_credentials_to_nvs(wifi_credentials);

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
//             ESP_LOGI("GATT_SVR", "characteristic read; conn_handle=%d attr_handle=%d",
//                      conn_handle, attr_handle);
//         } else {
//             ESP_LOGI("GATT_SVR", "characteristic read by nimble stack; attr_handle=%d",
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
//         "GATT_SVR",
//         "unexpected access operation to heart rate characteristic, opcode: %d",
//         ctxt->op);
//     return BLE_ATT_ERR_UNLIKELY;
// }

// /* Public functions */
// void send_heart_rate_indication(void) {
//     if (heart_rate_ind_status && heart_rate_chr_conn_handle_inited) {
//         ble_gatts_indicate(heart_rate_chr_conn_handle,
//                            heart_rate_chr_val_handle);
//         ESP_LOGI("GATT_SVR", "heart rate indication sent!");
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
//         ESP_LOGI("GATT_SVR", "subscribe event; conn_handle=%d attr_handle=%d",
//                  event->subscribe.conn_handle, event->subscribe.attr_handle);
//     } else {
//         ESP_LOGI("GATT_SVR", "subscribe by nimble stack; attr_handle=%d",
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
//             ESP_LOGI("GATT_SVR", "Start heart rate indication task");
//             xTaskCreate(heart_rate_ind_task, "HeartRateIndTask", 4096, NULL, 5, &xHeartIndicateTask);
//         } else {
//             ESP_LOGI("GATT_SVR", "Stop heart rate indication task");
//             vTaskDelete(xHeartIndicateTask);
//             xHeartIndicateTask = NULL;
//         }
//     }
// }

// /*
//  *  GATT server initialization
//  *      1. Initialize GATT service
//  *      2. Update NimBLE host GATT services counter
//  *      3. Add GATT services to server
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





















static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static void wifi_cred_chr_access_pass(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static void wifi_cred_chr_access_restart(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* Heart Rate service */
static const ble_uuid16_t heart_rate_svc_uuid = BLE_UUID16_INIT(0x180D);

static uint8_t heart_rate_chr_val[2] = {0};
static uint16_t heart_rate_chr_val_handle;
static const ble_uuid16_t heart_rate_chr_uuid = BLE_UUID16_INIT(0x2A37);

static uint16_t heart_rate_chr_conn_handle = 0;
static bool heart_rate_chr_conn_handle_inited = false;
static bool heart_rate_ind_status = false;

/* Automation IO service */
static const ble_uuid16_t wifi_cred_svc_uuid = 
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x01);;
static uint16_t wifi_cred_chr_val_handle_ssid;
static uint16_t wifi_cred_chr_val_handle_pass;
static uint16_t wifi_cred_chr_val_handle_restart;

static const ble_uuid128_t wifi_cred_chr_uuid_ssid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x02);
static const ble_uuid128_t wifi_cred_chr_uuid_pass =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x03);
static const ble_uuid128_t wifi_cred_chr_uuid_restart =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x04);

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* Heart rate service */
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &heart_rate_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {/* Heart rate characteristic */
              .uuid = &heart_rate_chr_uuid.u,
              .access_cb = heart_rate_chr_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
              .val_handle = &heart_rate_chr_val_handle},
             {
                 0, /* No more characteristics in this service. */
             }}},
    /* Wifi cred service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &wifi_cred_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){/* Wifi cred characteristic */
                                        {.uuid = &wifi_cred_chr_uuid_ssid.u,
                                         .access_cb = wifi_cred_chr_access_ssid,
                                         .flags = BLE_GATT_CHR_F_WRITE,
                                         .val_handle = &wifi_cred_chr_val_handle_ssid},

                                        {.uuid = &wifi_cred_chr_uuid_pass.u,
                                         .access_cb = wifi_cred_chr_access_pass,
                                         .flags = BLE_GATT_CHR_F_WRITE,
                                         .val_handle = &wifi_cred_chr_val_handle_pass},

                                        {.uuid = &wifi_cred_chr_uuid_restart.u,
                                         .access_cb = wifi_cred_chr_access_restart,
                                         .flags = BLE_GATT_CHR_F_WRITE,
                                         .val_handle = &wifi_cred_chr_val_handle_restart},
                                        {0}},
    },
    {
        0, /* No more services. */
    },
};


// static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                             struct ble_gatt_access_ctxt *ctxt, void *arg) {

//     /* Handle access events */
//     /* Note: Wifi cred characteristic is write only */
//     switch (ctxt->op) {

//     /* Write characteristic event */
//     case BLE_GATT_ACCESS_OP_WRITE_CHR:
//         /* Verify connection handle */
//         if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//             ESP_LOGI("GATT_SVR", "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
//         } else {
//             ESP_LOGI("GATT_SVR", "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
//         }

//         /* Verify attribute handle */
//         if (attr_handle == wifi_cred_chr_val_handle) {
//             // Allocate a buffer to hold the incoming data
//             ESP_LOGI("GATT_SVR", "Received data length: %d", ctxt->om->om_len);

//             char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
//             memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
//             buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer
//             ESP_LOGI("GATT_SVR", "Received data: %s", buffer);

//             if(strcmp(buffer, "on") == 0){
//                 ESP_LOGI("GATT_SVR", "turn on led");
//                 gpio_set_level(BLINK_GPIO, 1);
//             }
//             else if(strcmp(buffer, "off") == 0){
//                 ESP_LOGI("GATT_SVR", "turn off led");
//                 gpio_set_level(BLINK_GPIO, 0);
//             }

//             // // Initialize NVS
//             // esp_err_t err = nvs_flash_init();
//             // if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//             //     // NVS partition was truncated and needs to be erased
//             //     // Retry nvs_flash_init
//             //     ESP_ERROR_CHECK(nvs_flash_erase());
//             //     err = nvs_flash_init();
//             // }
//             // ESP_ERROR_CHECK( err );


//             char *token = strtok(buffer, " ");
//             if(token != NULL) {
//                 printf("%s\n", token);
//                 strcpy(wifi_credentials.ssid, token);    
//                 token = strtok(NULL, "-");
//             } if(token != NULL) {
//                 printf("%s\n", token);
//                 strcpy(wifi_credentials.pass, token);    
//             }
//             save_wifi_credentials_to_nvs(wifi_credentials);
//             wifi_init_sta();

//         }
//     }
// }
static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Handle access events */
    /* Note: Wifi cred characteristic is write only */
    switch (ctxt->op) {
    /* Write characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI("GATT_SVR", "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
        } else {
            ESP_LOGI("GATT_SVR", "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == wifi_cred_chr_val_handle_ssid) {
            ESP_LOGI("GATT_SVR", "Received data length: %d", ctxt->om->om_len);
            char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
            memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
            buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer
            ESP_LOGI("GATT_SVR", "Received data: %s", buffer);
            save_wifi_ssid_to_nvs(buffer);  
            // strcpy(wifi_credentials.ssid, buffer);    
        }
    }
}
static void wifi_cred_chr_access_pass(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI("GATT_SVR", "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
        } else {
            ESP_LOGI("GATT_SVR", "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
        }

        if (attr_handle == wifi_cred_chr_val_handle_pass) {
            ESP_LOGI("GATT_SVR", "Received data length: %d", ctxt->om->om_len);
            char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
            memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
            buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer
            ESP_LOGI("GATT_SVR", "Received data: %s", buffer);
            save_wifi_pass_to_nvs(buffer);  
            // strcpy(wifi_credentials.pass, buffer);    
        }
    }
}
static void wifi_cred_chr_access_restart(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI("GATT_SVR", "Wi-Fi restart; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
        } else {
            ESP_LOGI("GATT_SVR", "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
        }

        if (attr_handle == wifi_cred_chr_val_handle_restart) {
            esp_restart();
        }
    }
}

static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    /* Note: Heart rate characteristic is read only */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI("GATT_SVR", "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI("GATT_SVR", "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == heart_rate_chr_val_handle) {
            /* Update access buffer value */
            heart_rate_chr_val[1] = 60 + (uint8_t)(esp_random() % 31);
            rc = os_mbuf_append(ctxt->om, &heart_rate_chr_val,
                                sizeof(heart_rate_chr_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        "GATT_SVR",
        "unexpected access operation to heart rate characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

/* Public functions */
void send_heart_rate_indication(void) {
    if (heart_rate_ind_status && heart_rate_chr_conn_handle_inited) {
        ble_gatts_indicate(heart_rate_chr_conn_handle,
                           heart_rate_chr_val_handle);
        ESP_LOGI("GATT_SVR", "heart rate indication sent!");
    }
}

void heart_rate_ind_task(void *pvParameter) {
    while (1) {
        send_heart_rate_indication();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/*
 *  Action when someone subscribes to characteristics
 */ 
void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI("GATT_SVR", "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI("GATT_SVR", "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == heart_rate_chr_val_handle) {
        /* Update heart rate subscription status */
        heart_rate_chr_conn_handle = event->subscribe.conn_handle;
        heart_rate_chr_conn_handle_inited = true;
        heart_rate_ind_status = event->subscribe.cur_indicate;

        /* Create a task to send heart rate indication */
        if (heart_rate_ind_status && xHeartIndicateTask == NULL) {
            ESP_LOGI("GATT_SVR", "Start heart rate indication task");
            xTaskCreate(heart_rate_ind_task, "HeartRateIndTask", 4096, NULL, 5, &xHeartIndicateTask);
        } else {
            ESP_LOGI("GATT_SVR", "Stop heart rate indication task");
            vTaskDelete(xHeartIndicateTask);
            xHeartIndicateTask = NULL;
        }
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to
 * 
 *  server
 *      4. Create a task to send heart rate indication
 */
int gatt_svc_init(void) {
    int rc;

    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}