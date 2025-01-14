#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "config.h"
#include "oled_library.c"
#include "bmp280.c"


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

void button_task(void *pvParameters){
    gpio_config(&io_conf);

    ssd1306_init_with_config(&oled_config);
    bmp280_read_calibration_data();
    bmp280_init();

    while (true) {
        int button_state = gpio_get_level(BUTTON_GPIO);
        if (button_state == 0) {

            bmp280_data_t sensor_data = bmp280_read_data();
            ESP_LOGI(TAG2, "Temperature: %.2f °C", sensor_data.temperature);
            ESP_LOGI(TAG2, "Pressure: %.2f hPa", sensor_data.pressure);

            char temperature[50];
            snprintf(temperature, sizeof(temperature), " %.2f *C", sensor_data.temperature);
            char pressure[50];
            snprintf(pressure, sizeof(pressure), " %.2f hPa", sensor_data.pressure);


            ssd1306_display_on();
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            display_on_page(" Temperature:", 0);
            display_on_page(temperature, 1);
            ssd1306_fill_page(0x00, 2);
            display_on_page(" Pressure", 3);
            display_on_page(pressure, 4);
            ssd1306_fill_page(0x00, 5);

            vTaskDelay(5000 / portTICK_PERIOD_MS);
            ssd1306_fill_screen(0x00);
            ssd1306_display_off();
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void led_task(void *pvParameters){
    gpio_config(&blink_conf);
    while(1){

        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}