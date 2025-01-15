#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/semphr.h"

#include "config.h"
#include "oled_library.c"
#include "bmp280.h"
#include "mqtt_manager.h"


gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << BUTTON_GPIO),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};

gpio_config_t blink_conf = {
    .pin_bit_mask = (1ULL << BLINK_GPIO),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};


void button_task(void *pvParameters) {
    gpio_config(&io_conf);

    while (true) {
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            int64_t press_start_time = esp_timer_get_time();

            while (gpio_get_level(BUTTON_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            int64_t press_duration_ms = (esp_timer_get_time() - press_start_time) / 1000;
            if (press_duration_ms >= LONG_PRESS_TIME_MS) {
                esp_restart();
            } else {

                time_t now;
                struct tm timeinfo;
                time(&now);
                localtime_r(&now, &timeinfo);
                char time_str[50];
                strftime(time_str, sizeof(time_str), " %H:%M:%S", &timeinfo);


                bmp280_data_t sensor_data = bmp280_read_data();
                ESP_LOGI("BMP", "Temperature: %.2f °C", sensor_data.temperature);
                ESP_LOGI("BMP", "Pressure: %.2f hPa", sensor_data.pressure);
                ESP_LOGI("BMP", "Time: %s hPa", time_str);

                char temperature[50] = "t";
                snprintf(temperature, sizeof(temperature), " %.2f *C", sensor_data.temperature);
                char pressure[50] = "p";
                snprintf(pressure, sizeof(pressure), " %.2f hPa", sensor_data.pressure);

                ssd1306_display_on();
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                display_on_page(" Time:", 0);
                display_on_page(time_str, 1);
                ssd1306_fill_page(0x00, 2);
                display_on_page(" Temperature:", 3);
                display_on_page(temperature, 4);
                ssd1306_fill_page(0x00, 5);
                display_on_page(" Pressure", 6);
                display_on_page(pressure, 7);

                vTaskDelay(5000 / portTICK_PERIOD_MS);
                ssd1306_fill_screen(0x00);
                ssd1306_display_off();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void led_task(void *pvParameters){
    gpio_config(&blink_conf);
    gpio_set_level(BLINK_GPIO, 0);
    char limits[30];
    load_from_nvs(&limits, RECEIVE_TEMPERATURE_LIMITS);

    while (true) {
        bmp280_data_t sensor_data = bmp280_read_data();
        ESP_LOGI("LED", "Temperature: %.2f °C", sensor_data.temperature);

        int limit_1;
        int limit_2;
        if (sscanf(limits, "%d %d", &limit_1, &limit_2) == 2) {
            ESP_LOGI("LED", "Limit 1: %d", limit_1);
            ESP_LOGI("LED", "Limit 2: %d", limit_2);
            int limit = 20;
            if(sensor_data.temperature > limit || sensor_data.temperature < limit_2) {
                gpio_set_level(BLINK_GPIO, 1);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            } else {
                gpio_set_level(BLINK_GPIO, 0);
            }

        } else {
            ESP_LOGI("LED", "Błąd: Nie udało się sparsować dwóch liczb.");
        }


        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}