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
#include "esp_wifi.h"


static TaskHandle_t xHeartIndicateTask = NULL;


static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static void wifi_cred_chr_access_pass(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static void wifi_cred_chr_access_restart(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

static int registration_chr_access_link(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static void registration_chr_access_pass(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);


static char registration_chr_val_link[100] = REGISTRATION_URI;
bool mac_got = false;
bool authorized = false;
static const ble_uuid16_t registration_svc_uuid = 
    BLE_UUID128_INIT(0x24, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x01);;
static uint16_t registration_chr_val_handle_link;
static uint16_t registration_chr_val_handle_pass;
static const ble_uuid128_t registration_chr_uuid_link =
    BLE_UUID128_INIT(0x24, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x02);
static const ble_uuid128_t registration_chr_uuid_pass =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x03);


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
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &registration_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {
              .uuid = &registration_chr_uuid_link.u,
              .access_cb = registration_chr_access_link,
              .flags = BLE_GATT_CHR_F_READ,
              .val_handle = &registration_chr_val_handle_link},
            {
            .uuid = &registration_chr_uuid_pass.u,
            .access_cb = registration_chr_access_pass,
            .flags = BLE_GATT_CHR_F_WRITE,
            .val_handle = &registration_chr_val_handle_pass},
             {
                 0,
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
        0,
    },
};


static void wifi_cred_chr_access_ssid(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI("GATT_SVR", "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
        } else {
            ESP_LOGI("GATT_SVR", "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
        }

        if (attr_handle == wifi_cred_chr_val_handle_ssid) {
            ESP_LOGI("GATT_SVR", "Received data length: %d", ctxt->om->om_len);
            char buffer[ctxt->om->om_len + 1];
            memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
            buffer[ctxt->om->om_len] = '\0';
            ESP_LOGI("GATT_SVR", "Received data: %s", buffer);
            save_wifi_ssid_to_nvs(buffer);  
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
            char buffer[ctxt->om->om_len + 1];
            memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
            buffer[ctxt->om->om_len] = '\0';
            ESP_LOGI("GATT_SVR", "Received data: %s", buffer);
            save_wifi_pass_to_nvs(buffer);  
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

static int registration_chr_access_link(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;
    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI("GATT_SVR", "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI("GATT_SVR", "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        if(!mac_got) {
            uint8_t mac[6];
            char mac_addr[18] = "mac";
            if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
                ESP_LOGI("MQTT", "Adres MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                snprintf(mac_addr, sizeof(mac_addr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                ESP_LOGI("tagtagtag", "%s", mac_addr);
            }

            if (strlen(registration_chr_val_link) + strlen(mac_addr) < sizeof(registration_chr_val_link)) {
                strcat(registration_chr_val_link, mac_addr);
                printf("Result: %s\n", registration_chr_val_link);
            }
            mac_got = true;
        }

        ESP_LOGI("tagtagtag", "%d", authorized);
        if (attr_handle == registration_chr_val_handle_link) {
            if(authorized) {
                rc = os_mbuf_append(ctxt->om, &registration_chr_val_link,
                                    sizeof(registration_chr_val_link));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            } else {
                char buffer[20] = "Wrong pin!";
                rc = os_mbuf_append(ctxt->om, &buffer,
                                    sizeof(buffer));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
        }
        goto error;

    default:
        goto error;
    }

error:
    ESP_LOGE(
        "GATT_SVR",
        "unexpected access operation to characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}
static void registration_chr_access_pass(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI("GATT_SVR", "Wi-Fi restart; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
        } else {
            ESP_LOGI("GATT_SVR", "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
        }

        if (attr_handle == registration_chr_val_handle_pass) {
            char pass[ctxt->om->om_len + 1];
            memcpy(pass, ctxt->om->om_data, ctxt->om->om_len);
            pass[ctxt->om->om_len] = '\0';
            if(strcmp(pass, REGISTRATION_PASSWORD) == 0) {
                authorized = true;
            }
            ESP_LOGI("GATT_SVR", "Received characteristic value: %s", pass);
        }
    }
}

/* Public functions */
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
    // if (event->subscribe.attr_handle == heart_rate_chr_val_handle) {
    //     /* Update heart rate subscription status */
    //     heart_rate_chr_conn_handle = event->subscribe.conn_handle;
    //     heart_rate_chr_conn_handle_inited = true;
    //     heart_rate_ind_status = event->subscribe.cur_indicate;

    //     /* Create a task to send heart rate indication */
    //     if (heart_rate_ind_status && xHeartIndicateTask == NULL) {
    //         ESP_LOGI("GATT_SVR", "Start heart rate indication task");
    //         xTaskCreate(heart_rate_ind_task, "HeartRateIndTask", 4096, NULL, 5, &xHeartIndicateTask);
    //     } else {
    //         ESP_LOGI("GATT_SVR", "Stop heart rate indication task");
    //         vTaskDelete(xHeartIndicateTask);
    //         xHeartIndicateTask = NULL;
    //     }
    // }
}


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