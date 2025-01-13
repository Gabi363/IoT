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

#include "oled_library.c"
#include "bmp280.c"


void app_main(void)
{
    // // led blinking
    // esp_rom_gpio_pad_select_gpio(BLINK_GPIO);
    // gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    // xTaskCreate(&led_task, "led_task", 8192, NULL, 5, NULL); 


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
        
    // // MQTT
    // mqtt_app_start();
    
    ESP_LOGI(TAG, "Initializing I2C...");
   i2c_master_init();
   bmp280_read_calibration_data();
   bmp280_init();
   ESP_LOGI(TAG, "Reading BMP280 data...");
//    while (1) {
//        bmp280_read_data();
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//    }
bmp280_data_t sensor_data = bmp280_read_data();

ESP_LOGI(TAG2, "Temperature: %.2f °C", sensor_data.temperature);
ESP_LOGI(TAG2, "Pressure: %.2f hPa", sensor_data.pressure);

    bmp280_read_data();
     ESP_LOGI("test", "qaaa");
    // Inicjalizacja wyświetlacza
    ssd1306_init_with_config(&oled_config);
    ssd1306_display_off();

    // i2c_master_init();
    // ssd1306_init();
    
    while (1) {




        // WYPISANIE NA CAŁYM EKRANIE
        display_on_all_screen(" Heart rate:\n 78 bmp");
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // WYŁĄCZENIE
        ssd1306_display_off();
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // WŁĄCZENIE
        ssd1306_display_on();
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // WYPISANIE W DANEJ LINII
        ssd1306_fill_page(0x00, 2);
        display_on_page(" Temperature:", 3);
        display_on_page(" 36.6 *C", 4);
        ssd1306_fill_page(0x00, 5);
        display_on_page(" SpO2:", 6);
        display_on_page(" 100", 7);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

    }

}
