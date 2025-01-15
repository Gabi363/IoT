#ifndef BMP280_H
#define BMP280_H

typedef struct {
    float temperature;
    float pressure;
} bmp280_data_t;

void bmp280_init();
void bmp280_read_calibration_data();
bmp280_data_t bmp280_read_data();
#endif