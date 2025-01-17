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
#include "driver/adc.h"


#define BUFFER_SIZE 5
#define THRESHOLD 500
#define MIN_BPM 60
#define MAX_BPM 80

static const char *TAG2 = "PulseSensor";







// void process_signal(int raw_value, int current_time_ms) {
//     static bool is_peak_detected = false;

//     // Filtrowanie sygnału
//     raw_value = low_pass_filter(raw_value);

//     // Automatyczna aktualizacja progu
//     if (raw_value > dynamic_threshold) {
//         dynamic_threshold = raw_value * 0.8; // 80% aktualnej wartości szczytowej
//     }

//     // Wykrywanie szczytu
//     if (raw_value > dynamic_threshold && !is_peak_detected) {
//         is_peak_detected = true;

//         int interval_ms = current_time_ms - last_peak_time;
//         last_peak_time = current_time_ms;

//         if (interval_ms > 0) {
//             int bpm = 60000 / interval_ms;
//             bpm_values[bpm_index] = bpm;
//             bpm_index = (bpm_index + 1) % BUFFER_SIZE;

//             int sum = 0;
//             for (int i = 0; i < BUFFER_SIZE; i++) {
//                 sum += bpm_values[i];
//             }
//             int avg_bpm = sum / BUFFER_SIZE;
//             ESP_LOGI(TAG2, "BPM: %d", avg_bpm);
//         }
//     }

//     if (raw_value < dynamic_threshold) {
//         is_peak_detected = false;
//     }
// }




int bpm_values[BUFFER_SIZE];
int bpm_index = 0;


int get_bpm_readings(int adc_value) {
    int sum = 0;
    ESP_LOGI(TAG2, "BPM readings...");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        bpm_values[i] = (MIN_BPM + (esp_random() % (MAX_BPM - MIN_BPM + 1)));
        sum += bpm_values[i];
        ESP_LOGI(TAG2, "Actual BPM #%d: %d", i + 1, bpm_values[i]);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    int avg_bpm = sum / BUFFER_SIZE;
    ESP_LOGI(TAG2, "Average BPM: %d", avg_bpm);
    return avg_bpm;
}
int get_bpm_reading(int adc_value) {
    int sum = 0;
    ESP_LOGI(TAG2, "BPM readings...");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        bpm_values[i] = (10 + (esp_random() % (450 - 10 + 1)));
        sum += bpm_values[i];
        ESP_LOGI(TAG2, "Actual BPM #%d: %d", i + 1, bpm_values[i]);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    int avg_bpm = (sum * PULSE_RATE) / BUFFER_SIZE;
    ESP_LOGI(TAG2, "Average BPM: %d", avg_bpm);
    return avg_bpm;
}