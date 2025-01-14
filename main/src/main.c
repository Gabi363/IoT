#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <stdbool.h>

#include "config.h"
#include "ble_manager.h"
#include "sdkconfig.h"
#include "gap.h"
#include "gatt_svc.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "components_manager.c"


void app_main(void)
{
    xTaskCreate(&led_task, "led_task", 8192, NULL, 5, NULL); 
    xTaskCreate(&button_task, "button_task", 8192, NULL, 5, NULL); 

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // BLE PERIPHERIAL
    ble_init();
    xTaskCreate(nimble_host_task,"NimBLE", 4096, NULL, 5, NULL);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_LOGI("MAIN", "ESP_WIFI_MODE_STA");
    wifi_init_sta();
        
    // MQTT
    mqtt_app_start();

}
