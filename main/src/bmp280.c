
#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// I2C configuration
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_IO 2
#define I2C_MASTER_SCL_IO 4
#define I2C_MASTER_FREQ_HZ 100000
#define BMP280_ADDR 0x76
static const char *TAG2 = "BMP280";
// BMP280 Registers
#define BMP280_TEMP_PRESS_CALIB_DATA_START 0x88
#define BMP280_TEMP_PRESS_CALIB_DATA_LENGTH 24
#define BMP280_CTRL_MEAS_REG 0xF4
#define BMP280_CONFIG_REG 0xF5
#define BMP280_TEMP_PRESS_DATA_START 0xF7
// Calibration parameters
static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

// void i2c_master_init() {
//    i2c_config_t conf = {
//        .mode = I2C_MODE_MASTER,
//        .sda_io_num = I2C_MASTER_SDA_IO,
//        .sda_pullup_en = GPIO_PULLUP_ENABLE,
//        .scl_io_num = I2C_MASTER_SCL_IO,
//        .scl_pullup_en = GPIO_PULLUP_ENABLE,
//        .master.clk_speed = I2C_MASTER_FREQ_HZ,
//    };
//    i2c_param_config(I2C_MASTER_NUM, &conf);
//    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
// }
void bmp280_read_calibration_data() {
   uint8_t calib_data[BMP280_TEMP_PRESS_CALIB_DATA_LENGTH];
   i2c_master_write_read_device(I2C_MASTER_NUM, BMP280_ADDR,
                                (uint8_t[]){BMP280_TEMP_PRESS_CALIB_DATA_START}, 1,
                                calib_data, BMP280_TEMP_PRESS_CALIB_DATA_LENGTH, 1000 / portTICK_PERIOD_MS);
   // Parse calibration data
   dig_T1 = (calib_data[1] << 8) | calib_data[0];
   dig_T2 = (calib_data[3] << 8) | calib_data[2];
   dig_T3 = (calib_data[5] << 8) | calib_data[4];
   dig_P1 = (calib_data[7] << 8) | calib_data[6];
   dig_P2 = (calib_data[9] << 8) | calib_data[8];
   dig_P3 = (calib_data[11] << 8) | calib_data[10];
   dig_P4 = (calib_data[13] << 8) | calib_data[12];
   dig_P5 = (calib_data[15] << 8) | calib_data[14];
   dig_P6 = (calib_data[17] << 8) | calib_data[16];
   dig_P7 = (calib_data[19] << 8) | calib_data[18];
   dig_P8 = (calib_data[21] << 8) | calib_data[20];
   dig_P9 = (calib_data[23] << 8) | calib_data[22];
}
void bmp280_init() {
   uint8_t ctrl_meas = 0x27; // Normal mode, temp and press oversampling x1
   uint8_t config = 0xA0;    // Standby 1000ms, filter off
   i2c_master_write_to_device(I2C_MASTER_NUM, BMP280_ADDR,
                               (uint8_t[]){BMP280_CTRL_MEAS_REG, ctrl_meas}, 2, 1000 / portTICK_PERIOD_MS);
   i2c_master_write_to_device(I2C_MASTER_NUM, BMP280_ADDR,
                               (uint8_t[]){BMP280_CONFIG_REG, config}, 2, 1000 / portTICK_PERIOD_MS);
}

typedef struct {
    float temperature;
    float pressure;
} bmp280_data_t;


bmp280_data_t bmp280_read_data() {
   bmp280_data_t data_out;  // Struktura, która przechowa wyniki

   uint8_t data[6];
   i2c_master_write_read_device(I2C_MASTER_NUM, BMP280_ADDR,
                                (uint8_t[]){BMP280_TEMP_PRESS_DATA_START}, 1,
                                data, 6, 1000 / portTICK_PERIOD_MS);
   int32_t adc_pressure = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
   int32_t adc_temperature = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
   
   // Obliczanie temperatury
   int32_t var1, var2, t_fine;
   var1 = ((((adc_temperature >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
   var2 = (((((adc_temperature >> 4) - ((int32_t)dig_T1)) * ((adc_temperature >> 4) - ((int32_t)dig_T1))) >> 12) *
           ((int32_t)dig_T3)) >> 14;
   t_fine = var1 + var2;
   data_out.temperature = (t_fine * 5 + 128) >> 8;
   data_out.temperature /= 100.0;

   // Obliczanie ciśnienia
   int64_t var1_p, var2_p, p;
   var1_p = ((int64_t)t_fine) - 128000;
   var2_p = var1_p * var1_p * (int64_t)dig_P6;
   var2_p = var2_p + ((var1_p * (int64_t)dig_P5) << 17);
   var2_p = var2_p + (((int64_t)dig_P4) << 35);
   var1_p = ((var1_p * var1_p * (int64_t)dig_P3) >> 8) + ((var1_p * (int64_t)dig_P2) << 12);
   var1_p = (((((int64_t)1) << 47) + var1_p)) * ((int64_t)dig_P1) >> 33;
   if (var1_p == 0) {
       ESP_LOGE(TAG2, "Pressure calculation failed: division by zero.");
       data_out.pressure = 0;  // Wartość 0 w przypadku błędu
       return data_out;
   }
   p = 1048576 - adc_pressure;
   p = (((p << 31) - var2_p) * 3125) / var1_p;
   var1_p = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
   var2_p = (((int64_t)dig_P8) * p) >> 19;
   p = ((p + var1_p + var2_p) >> 8) + (((int64_t)dig_P7) << 4);
   data_out.pressure = p / 25600.0;

   return data_out;  // Zwróć strukturę zawierającą dane
}

// void bmp280_read_data() {
//    uint8_t data[6];
//    i2c_master_write_read_device(I2C_MASTER_NUM, BMP280_ADDR,
//                                 (uint8_t[]){BMP280_TEMP_PRESS_DATA_START}, 1,
//                                 data, 6, 1000 / portTICK_PERIOD_MS);
//    int32_t adc_pressure = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
//    int32_t adc_temperature = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
//    // Calculate temperature
//    int32_t var1, var2, t_fine;
//    var1 = ((((adc_temperature >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
//    var2 = (((((adc_temperature >> 4) - ((int32_t)dig_T1)) * ((adc_temperature >> 4) - ((int32_t)dig_T1))) >> 12) *
//            ((int32_t)dig_T3)) >>
//           14;
//    t_fine = var1 + var2;
//    float temperature = (t_fine * 5 + 128) >> 8;
//    temperature /= 100.0;
//    // Calculate pressure
//    int64_t var1_p, var2_p, p;
//    var1_p = ((int64_t)t_fine) - 128000;
//    var2_p = var1_p * var1_p * (int64_t)dig_P6;
//    var2_p = var2_p + ((var1_p * (int64_t)dig_P5) << 17);
//    var2_p = var2_p + (((int64_t)dig_P4) << 35);
//    var1_p = ((var1_p * var1_p * (int64_t)dig_P3) >> 8) + ((var1_p * (int64_t)dig_P2) << 12);
//    var1_p = (((((int64_t)1) << 47) + var1_p)) * ((int64_t)dig_P1) >> 33;
//    if (var1_p == 0) {
//        ESP_LOGE(TAG2, "Pressure calculation failed: division by zero.");
//        return;
//    }
//    p = 1048576 - adc_pressure;
//    p = (((p << 31) - var2_p) * 3125) / var1_p;
//    var1_p = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
//    var2_p = (((int64_t)dig_P8) * p) >> 19;
//    p = ((p + var1_p + var2_p) >> 8) + (((int64_t)dig_P7) << 4);
//    float pressure = p / 25600.0;
//    ESP_LOGI(TAG2, "Temperature: %.2f °C", temperature);
//    ESP_LOGI(TAG2, "Pressure: %.2f hPa", pressure);
// }
// void app_main() {
//    ESP_LOGI(TAG2, "Initializing I2C...");
//    i2c_master_init();
//    bmp280_read_calibration_data();
//    bmp280_init();
//    ESP_LOGI(TAG2, "Reading BMP280 data...");
//    while (1) {
//        bmp280_read_data();
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//    }
// }
