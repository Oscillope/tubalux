#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc_cal.h"
#include "driver/gpio.h"
#include "ssd1306.h"
#include "font8x8_basic.h"

#include "led_patterns.h"
#include "ui.h"

#define TAG "UI"

#define UI_LOOP_PERIOD		100
#define UI_IDLE_TIMEOUT		5000

typedef enum {
	UI_BTN_NONE = 0,
	UI_BTN_DN,
	UI_BTN_UP,
	UI_BTN_L,
	UI_BTN_R,
	UI_BTN_PRS,
	UI_BTN_MAX
} ui_btn_t;

typedef enum {
	UI_STATE_OFF,
	UI_STATE_IDLE,
	UI_STATE_PATTERN,
	UI_STATE_TEMPO,
	UI_STATE_MAX
} ui_state_t;

static ui_state_t state = UI_STATE_IDLE;

uint8_t ui_change_state(ui_state_t new_state)
{
	if (state != new_state) {
		ESP_LOGI(TAG, "state change %u -> %u", state, new_state);
		state = new_state;
		return 1;
	}
	return 0;
}

void ui_loop(void* parameters)
{
	SSD1306_t* dev = (SSD1306_t*)parameters;

	const char* line = "    tubalux";
	ssd1306_clear_screen(dev, false);
	ssd1306_display_text(dev, 0, line, strlen(line), false);
	ssd1306_hardware_scroll(dev, SCROLL_DOWN);
	vTaskDelay(pdMS_TO_TICKS(1000));
	ssd1306_hardware_scroll(dev, SCROLL_UP);
	vTaskDelay(pdMS_TO_TICKS(1000));
	ssd1306_hardware_scroll(dev, SCROLL_STOP);
	//ssd1306_fadeout(dev);
	ssd1306_clear_screen(dev, false);

	/* Battery ADC setup */
	uint32_t voltage = 0;
	char status[17];
	esp_adc_cal_characteristics_t chars = { 0 };
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &chars);
	/* Button configuration */
	gpio_config_t gpio_conf = {
		/* R: 32    L: 33       UP: 34      DN: 36      PRS: 39 */
		BIT64(32) | BIT64(33) | BIT64(34) | BIT64(36) | BIT64(39),
		GPIO_MODE_INPUT,
		GPIO_PULLUP_ENABLE,
		GPIO_PULLDOWN_DISABLE,
		GPIO_INTR_DISABLE };
	gpio_config(&gpio_conf);

	uint32_t idle_timer = 0;
	TickType_t before, after;

	while (true) {
		//ssd1306_clear_screen(dev, false);
		before = xTaskGetTickCount();
		esp_adc_cal_get_voltage(ADC_CHANNEL_7, &chars, &voltage);
		snprintf(status, sizeof(status), "          %4umV", voltage * 2);
		ssd1306_display_text(dev, 0, status, strlen(status), true);

		ui_btn_t current = UI_BTN_NONE;
		if (!gpio_get_level(GPIO_NUM_32)) {
			current = UI_BTN_R;
		} else if (!gpio_get_level(GPIO_NUM_33)) {
			current = UI_BTN_L;
		} else if (!gpio_get_level(GPIO_NUM_34)) {
			current = UI_BTN_UP;
		} else if (!gpio_get_level(GPIO_NUM_36)) {
			current = UI_BTN_DN;
		} else if (!gpio_get_level(GPIO_NUM_39)) {
			current = UI_BTN_PRS;
		}
		switch (state) {
		case UI_STATE_IDLE:
			if (!current) {
				idle_timer += UI_LOOP_PERIOD;
				if (idle_timer > UI_IDLE_TIMEOUT) {
					ssd1306_fadeout(dev);
					ssd1306_contrast(dev, 0);
					ui_change_state(UI_STATE_OFF);
				}
			}
			ssd1306_display_text(dev, 3, "     config", 11, false);
			ssd1306_display_text(dev, 4, "   pat + tempo", 14, false);
			/* fall through */
		case UI_STATE_OFF:
			if (current) {
				idle_timer = 0;
				/* clean this up when I'm sober */
				if (state == UI_STATE_IDLE) {
					ui_state_t new_state = state;
					switch (current) {
					case UI_BTN_L:
						new_state = UI_STATE_PATTERN;
						break;
					case UI_BTN_R:
						new_state = UI_STATE_TEMPO;
						break;
					default:
						break;
					}
					if (ui_change_state(new_state)) {
						ssd1306_clear_screen(dev, false);
					}
				} else if (state == UI_STATE_OFF) {
					ui_change_state(UI_STATE_IDLE);
					ssd1306_contrast(dev, 0xff);
				}
			}
			break;
		case UI_STATE_PATTERN:
		{
			led_pattern_t* patterns = get_patterns();
			ssd1306_software_scroll(dev, 1, 7);
			for (int i = 0; i < LED_NUM_PATTERNS; i++) {
				ssd1306_scroll_text(dev, patterns[i].name, strlen(patterns[i].name), (i == 0));
			}

			switch (current) {
			case UI_BTN_NONE:
				idle_timer += UI_LOOP_PERIOD;
				if (idle_timer > UI_IDLE_TIMEOUT) {
					ssd1306_fadeout(dev);
					ssd1306_contrast(dev, 0);
					ui_change_state(UI_STATE_OFF);
				}
				break;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		after = xTaskGetTickCount();

		if (after - before > 1) {
			ESP_LOGI(TAG, "this loop took %d", ((after - before) * portTICK_PERIOD_MS));
		}

		vTaskDelay(pdMS_TO_TICKS(UI_LOOP_PERIOD));
	}

	vTaskDelete(NULL);
}

void ui_init(void)
{
	SSD1306_t dev;
#if CONFIG_I2C_INTERFACE
	ESP_LOGI(TAG, "INTERFACE is i2c");
	ESP_LOGI(TAG, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(TAG, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#if CONFIG_SSD1306_128x64
	ESP_LOGI(TAG, "Panel is 128x64");
	i2c_init(&dev, 128, 64, 0x3C);
#endif // CONFIG_SSD1306_128x64
#if CONFIG_SSD1306_128x32
	ESP_LOGI(TAG, "Panel is 128x32");
	i2c_init(&dev, 128, 32, 0x3C);
#endif // CONFIG_SSD1306_128x32
#endif // CONFIG_I2C_INTERFACE

#if CONFIG_SPI_INTERFACE
	ESP_LOGI(TAG, "INTERFACE is SPI");
	ESP_LOGI(TAG, "CONFIG_CS_GPIO=%d",CONFIG_CS_GPIO);
	ESP_LOGI(TAG, "CONFIG_DC_GPIO=%d",CONFIG_DC_GPIO);
	ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	spi_master_init(&dev, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
	spi_init(&dev, 128, 64);
#endif // CONFIG_SPI_INTERFACE

	xTaskCreatePinnedToCore(ui_loop, "UI loop", 4096, &dev, 2, NULL, 0);
}
