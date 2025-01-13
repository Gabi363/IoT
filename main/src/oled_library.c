#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1366.h"
#include "font8x8_basic.h"

#define I2C_MASTER_NUM              I2C_NUM_0
#define TAG "SSD1306"

typedef struct {
    uint8_t i2c_address;       // Adres urządzenia I2C
    int sda_pin;               // Numer pinu SDA
    int scl_pin;               // Numer pinu SCL
    uint16_t width;            // Szerokość wyświetlacza (np. 128)
    uint16_t height;           // Wysokość wyświetlacza (np. 64)
    uint8_t contrast;          // Kontrast (0x00 - 0xFF)
    uint8_t addressing_mode;   // Tryb adresowania (np. 0x00 dla poziomego, 0x02 dla Page Addressing)
    bool invert_colors;        // Czy kolory są odwrócone (true: inwersja, false: normalne kolory)
    long freq_hz;
} ssd1306_config_t;

ssd1306_config_t oled_config = {
    .i2c_address = 0x3C,          // Domyślny adres SSD1306
    .sda_pin = 21,                // Pin SDA
    .scl_pin = 23,                // Pin SCL
    .width = 128,                 // Szerokość wyświetlacza
    .height = 64,                 // Wysokość wyświetlacza
    .contrast = 0x80,             // Średni kontrast
    .addressing_mode = 0x02,      // Page Addressing Mode
    .invert_colors = false,        // Brak inwersji kolorów
    .freq_hz = 100000
};


// Funkcja wysyłająca pojedyncze polecenie do wyświetlacza
void i2c_write_command(uint8_t command) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (oled_config.i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Tryb polecenia
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void i2c_write_data(uint8_t *data, size_t size) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (oled_config.i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x40, true); // Tryb danych
    i2c_master_write(cmd, data, size, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void ssd1306_init_with_config(const ssd1306_config_t *config) {
    // Inicjalizacja I2C z dynamicznie ustawionymi pinami
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = config->freq_hz,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    // Konfiguracja SSD1306
    i2c_write_command(0xAE); // Display OFF
    i2c_write_command(0x20); // Memory Addressing Mode
    i2c_write_command(config->addressing_mode); // Ustawiony tryb adresowania
    i2c_write_command(0xB0); // Start at Page 0
    i2c_write_command(0xC8); // COM Output Scan Direction
    i2c_write_command(0x00); // Set Lower Column Start Address
    i2c_write_command(0x10); // Set Higher Column Start Address
    i2c_write_command(0x40); // Set Display Start Line
    i2c_write_command(0x81); // Set Contrast Control
    i2c_write_command(config->contrast); // Ustawienie kontrastu
    i2c_write_command(0xA1); // Segment Re-map
    i2c_write_command(config->invert_colors ? 0xA7 : 0xA6); // Inwersja kolorów
    i2c_write_command(0xA8); // Multiplex Ratio
    i2c_write_command(config->height - 1); // Wysokość wyświetlacza
    i2c_write_command(0xA4); // Entire Display ON
    i2c_write_command(0xD3); // Set Display Offset
    i2c_write_command(0x00); // No offset
    i2c_write_command(0xD5); // Set Display Clock Divide Ratio
    i2c_write_command(0xF0); // Default value
    i2c_write_command(0xD9); // Set Pre-charge Period
    i2c_write_command(0x22); // Default value
    i2c_write_command(0xDA); // Set COM Pins Hardware Configuration
    i2c_write_command(0x12); // Default value
    i2c_write_command(0xDB); // Set VCOMH Deselect Level
    i2c_write_command(0x20); // Default value
    i2c_write_command(0x8D); // Charge Pump
    i2c_write_command(0x14); // Enable charge pump
    i2c_write_command(0xAF); // Display ON
}


// Inicjalizacja magistrali I2C
// esp_err_t i2c_master_init(void) {
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_MASTER_FREQ_HZ,
//     };
//     ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
//     return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
// }

// void ssd1306_init() {
//     i2c_write_command(0xAE); // Display OFF
//     i2c_write_command(0x20); // Memory Addressing Mode
//     i2c_write_command(0x02); // Page Addressing Mode
//     i2c_write_command(0xB0); // Start at Page 0
//     i2c_write_command(0xC8); // COM Output Scan Direction
//     i2c_write_command(0x00); // Set Lower Column Start Address
//     i2c_write_command(0x10); // Set Higher Column Start Address
//     i2c_write_command(0x40); // Set Display Start Line
//     i2c_write_command(0x81); // Set Contrast Control
//     i2c_write_command(0xFF); // Maximum contrast
//     i2c_write_command(0xA1); // Segment Re-map
//     i2c_write_command(0xA6); // Normal Display
//     i2c_write_command(0xA8); // Multiplex Ratio
//     i2c_write_command(0x3F); // 1/64 Duty
//     i2c_write_command(0xA4); // Entire Display ON
//     i2c_write_command(0xD3); // Set Display Offset
//     i2c_write_command(0x00); // No offset
//     i2c_write_command(0xD5); // Set Display Clock Divide Ratio
//     i2c_write_command(0xF0); // Default value
//     i2c_write_command(0xD9); // Set Pre-charge Period
//     i2c_write_command(0x22); // Default value
//     i2c_write_command(0xDA); // Set COM Pins Hardware Configuration
//     i2c_write_command(0x12); // Default value
//     i2c_write_command(0xDB); // Set VCOMH Deselect Level
//     i2c_write_command(0x20); // Default value
//     i2c_write_command(0x8D); // Charge Pump
//     i2c_write_command(0x14); // Enable charge pump
//     i2c_write_command(0xAF); // Display ON
// }

void ssd1306_display_on() {
    i2c_write_command(0xAF); // Display ON
}

void ssd1306_display_off() {
    i2c_write_command(0xAE); // Display OFF
}

void ssd1306_set_contrast(uint8_t contrast) {
    i2c_write_command(0x81);      // Komenda ustawienia kontrastu
    i2c_write_command(contrast); // Wartość kontrastu (0x00 - 0xFF)
}

void ssd1306_set_color(bool invert_color) {
    if(invert_color) i2c_write_command(0xA7);
    else i2c_write_command(0xA6);
}

void ssd1306_fill_screen(uint8_t pattern) {
    for (int page = 0; page < 8; page++) {
        i2c_write_command(0xB0 + page); // Ustaw stronę
        i2c_write_command(0x00);       // Ustaw dolne 4 bity kolumny
        i2c_write_command(0x10);       // Ustaw górne 4 bity kolumny
        for (int col = 0; col < 128 + 2; col++) {
            uint8_t data = pattern;
            i2c_write_data(&data, 1);
        }
    }
}
void ssd1306_fill_page(uint8_t pattern, int page) {
    i2c_write_command(0xB0 + page); // Ustaw stronę
    i2c_write_command(0x00);       // Ustaw dolne 4 bity kolumny
    i2c_write_command(0x10);       // Ustaw górne 4 bity kolumny
    for (int col = 0; col < 128 + 2; col++) {
        uint8_t data = pattern;
        i2c_write_data(&data, 1);
    }
}

void display_on_page(const char *text, uint8_t page) {
    ssd1306_fill_page(0x00, page);

	uint8_t text_len = strlen(text);
    if(text_len > 16) text_len = (uint8_t) 16;
	i2c_cmd_handle_t cmd;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (oled_config.i2c_address << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, 0x00, true); // reset column
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, 0xB0 | page, true); // reset page

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);


	for (uint8_t i = 0; i < text_len; i++) {
			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (oled_config.i2c_address << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
			i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)text[i]], 8, true);

			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
	}
}

void display_on_all_screen(const char *text) {
    ssd1306_fill_screen(0x00);
	uint8_t text_len = strlen(text);
	i2c_cmd_handle_t cmd;
	uint8_t cur_page = 0;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (oled_config.i2c_address << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, 0x00, true); // reset column
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, 0xB0 | cur_page, true); // reset page

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	for (uint8_t i = 0; i < text_len; i++) {
		if (text[i] == '\n') {
			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (oled_config.i2c_address << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
			i2c_master_write_byte(cmd, 0x00, true); // reset column
			i2c_master_write_byte(cmd, 0x10, true);
			i2c_master_write_byte(cmd, 0xB0 | ++cur_page, true); // increment page

			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
		} else {
			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (oled_config.i2c_address << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
			i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)text[i]], 8, true);

			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
		}
	}
}