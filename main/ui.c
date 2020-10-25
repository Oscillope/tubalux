#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/adc_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "ssd1306.h"
#include "font8x8_basic.h"

#include "leds.h"
#include "led_patterns.h"
#include "ui_buttons.h"
#include "ui.h"

#define TAG "UI"

#define UI_LOOP_PERIOD		100
#define UI_IDLE_TIMEOUT		10000

typedef enum {
	UI_STATE_OFF,
	UI_STATE_IDLE,
	UI_STATE_PATTERN,
	UI_STATE_TEMPO,
	UI_STATE_COLOR,
	UI_STATE_INTENSITY,
	UI_STATE_MAX
} ui_state_t;

static ui_state_t state = UI_STATE_IDLE;
static ui_menu_t* cur_menu = NULL;
static TaskHandle_t ui_task = NULL;

/* Helper functions */
uint8_t ui_change_state(ui_state_t new_state)
{
	if (state != new_state) {
		ESP_LOGI(TAG, "state change %u -> %u", state, new_state);
		state = new_state;
		return 1;
	}
	return 0;
}

uint8_t ui_show_menu(SSD1306_t* dev, ui_menu_t* menu, uint8_t menu_items, int8_t dir)
{
	if (cur_menu != menu || (dir && (menu->selection + dir >= 0) && (menu->selection + dir < menu_items))) {
		menu->selection += dir;
		uint8_t start = ((menu->selection >= 7) ? menu->selection : 0);
		uint8_t end = (((start + 7) > menu_items) ? menu_items : (start + 7));
		/* backtrack a bit in case the number of items is not a multiple of 7 */
		if (end - start < 7) {
			start = end - 7;
		}
		ESP_LOGD(TAG, "generating menu from %u to %u selection %u items %u", start, end, menu->selection, menu_items);
		for (uint8_t i = start; i < end; i++) {
			uint8_t pos = (i - start) + 1;
			ssd1306_display_text(dev, pos, menu[i].name, sizeof(menu[i].name), (i == menu->selection));
		}
		cur_menu = menu;
	}
	return cur_menu->selection;
}

uint8_t ui_idle_service(uint32_t* timer)
{
	*timer += UI_LOOP_PERIOD;
	if (*timer > UI_IDLE_TIMEOUT) {
		*timer = 0;
		return ui_change_state(UI_STATE_IDLE);
	}
	return 0;
}

/* Button callbacks */
void ui_btn_callback(uint32_t buttons)
{
	xTaskNotify(ui_task, buttons, eSetBits);
}

void ui_loop(void* parameters)
{
	SSD1306_t* dev = (SSD1306_t*)parameters;

	ssd1306_clear_screen(dev, false);
	ssd1306_display_text(dev, 0, "    tubalux", 16, false);
	ssd1306_hardware_scroll(dev, SCROLL_DOWN);
	vTaskDelay(pdMS_TO_TICKS(1000));
	ssd1306_hardware_scroll(dev, SCROLL_UP);
	vTaskDelay(pdMS_TO_TICKS(1000));
	ssd1306_hardware_scroll(dev, SCROLL_STOP);
	ssd1306_clear_screen(dev, false);

	/* Battery ADC setup */
	char status[17];
	uint32_t voltage = 0;
	adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);
	adc1_config_width(ADC_WIDTH_12Bit);

	uint32_t idle_timer = 0;
	uint32_t buttons = 0;
	TickType_t before, after;

	while (true) {
		before = xTaskGetTickCount();
		/* Disable interrupts for GPIO36 and 39 before ADC read. See Errata 3.11 */
		ui_isr_disable();
		voltage = (uint32_t)(adc1_get_raw(ADC1_CHANNEL_7) * 1.76f);
		vTaskDelay(1);
		ui_isr_enable();
		snprintf(status, sizeof(status), "          %4umV", voltage);
		ssd1306_display_text(dev, 0, status, strlen(status), true);

		switch (state) {
		case UI_STATE_IDLE:
			ssd1306_display_text(dev, 3, "     color", 11, false);
			ssd1306_display_text(dev, 4, "   pat + tempo", 14, false);
			ssd1306_display_text(dev, 5, "     intens.", 12, false);
			switch (buttons) {
			case UI_BTN_NONE:
				idle_timer += UI_LOOP_PERIOD;
				if (idle_timer > UI_IDLE_TIMEOUT) {
					ssd1306_fadeout(dev);
					ssd1306_contrast(dev, 0);
					ui_change_state(UI_STATE_OFF);
				}
				break;
			case UI_BTN_L:
				idle_timer = 0;
				ui_change_state(UI_STATE_PATTERN);
				break;
			case UI_BTN_R:
				idle_timer = 0;
				ui_change_state(UI_STATE_TEMPO);
				break;
			case UI_BTN_UP:
				idle_timer = 0;
				ui_change_state(UI_STATE_COLOR);
				break;
			case UI_BTN_DN:
				idle_timer = 0;
				ui_change_state(UI_STATE_INTENSITY);
				break;
			default:
				ESP_LOGW(TAG, "Unknown button %08x", buttons);
				break;
			}
			if (state != UI_STATE_OFF) {
				break;
				/* If we just transitioned to the off state, don't bother going around the loop again.
				 * this also covers the case where the user presses the button while the
				 * fade-out is happening */
			}
			/* fall through */
		case UI_STATE_OFF:
			/* Wait for any button press and go back to IDLE */
			xTaskNotifyWait(0, UINT32_MAX, &buttons, portMAX_DELAY);
			ui_change_state(UI_STATE_IDLE);
			idle_timer = 0;
			ssd1306_contrast(dev, 0xff);
			break;
		case UI_STATE_PATTERN:
		{
			uint8_t selection = ui_show_menu(dev, get_pattern_menu(), LED_NUM_PATTERNS, 0);
			switch (buttons) {
			case UI_BTN_NONE:
				if (ui_idle_service(&idle_timer)) {
					ssd1306_clear_screen(dev, false);
				}
				break;
			case UI_BTN_UP:
				idle_timer = 0;
				ui_show_menu(dev, get_pattern_menu(), LED_NUM_PATTERNS, -1);
				break;
			case UI_BTN_DN:
				idle_timer = 0;
				ui_show_menu(dev, get_pattern_menu(), LED_NUM_PATTERNS, 1);
				break;
			case UI_BTN_PRS:
				idle_timer = 0;
				led_pattern_t* patterns = get_patterns();
				led_set_pattern(&patterns[selection]);
				cur_menu = NULL;
				ui_change_state(UI_STATE_IDLE);
				ssd1306_clear_screen(dev, false);
				break;
			default:
				ESP_LOGW(TAG, "Unknown button %08x", buttons);
				break;
			}
			break;
		}
		case UI_STATE_COLOR:
		{
			ssd1306_display_text(dev, 3, "     intens+", 12, false);
			ssd1306_display_text(dev, 4, "color- + color+", 15, false);
			ssd1306_display_text(dev, 5, "     intens-", 12, false);
			switch (buttons) {
			case UI_BTN_NONE:
				if (ui_idle_service(&idle_timer)) {
					ssd1306_clear_screen(dev, false);
				}
				break;
			case UI_BTN_UP:
				idle_timer = 0;
				led_set_intensity((led_get_intensity() + 10) % 100);
				break;
			case UI_BTN_DN:
				idle_timer = 0;
				led_set_intensity((led_get_intensity() - 10) % 100);
				break;
			case UI_BTN_L:
				idle_timer = 0;
				led_set_primary_hue((led_get_primary_hue() - 10) % 360);
				break;
			case UI_BTN_R:
				idle_timer = 0;
				led_set_primary_hue((led_get_primary_hue() + 10) % 360);
				break;
			case UI_BTN_PRS:
				idle_timer = 0;
				ui_change_state(UI_STATE_IDLE);
				ssd1306_clear_screen(dev, false);
				break;
			default:
				ESP_LOGW(TAG, "Unknown button %08x", buttons);
				break;
			}
			break;
		}
		case UI_STATE_TEMPO:
		{
			ssd1306_display_text(dev, 4, "tempo- -TAP- tempo+", 16, false);
			switch (buttons) {
			case UI_BTN_NONE:
				if (ui_idle_service(&idle_timer)) {
					ssd1306_clear_screen(dev, false);
				}
				break;
			}
			break;
		}
		default:
			ESP_LOGW(TAG, "Unknown state %u", state);
			ui_change_state(UI_STATE_IDLE);
			break;
		}
		after = xTaskGetTickCount();

		if (after - before > 10) {
			ESP_LOGI(TAG, "this loop took %d", ((after - before) * portTICK_PERIOD_MS));
		}

		xTaskNotifyWait(0, UINT32_MAX, &buttons, pdMS_TO_TICKS(UI_LOOP_PERIOD));
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

	xTaskCreatePinnedToCore(ui_loop, "UI loop", 4096, &dev, 2, &ui_task, 0);

	/* Initialize button ISR and debouncer */
	ui_buttons_init();
	ui_buttons_reg_callback(ui_btn_callback);
}
