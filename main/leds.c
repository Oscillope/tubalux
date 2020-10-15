#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"

#include "led_patterns.h"
#include "leds.h"

#define TAG "LEDs"
#define RMT_TX_CHANNEL RMT_CHANNEL_0

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
	h %= 360; // h -> [0,360]
	uint32_t rgb_max = v * 2.55f;
	uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

	uint32_t i = h / 60;
	uint32_t diff = h % 60;

	// RGB adjustment amount by hue
	uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

	switch (i) {
	case 0:
		*r = rgb_max;
		*g = rgb_min + rgb_adj;
		*b = rgb_min;
		break;
	case 1:
		*r = rgb_max - rgb_adj;
		*g = rgb_max;
		*b = rgb_min;
		break;
	case 2:
		*r = rgb_min;
		*g = rgb_max;
		*b = rgb_min + rgb_adj;
		break;
	case 3:
		*r = rgb_min;
		*g = rgb_max - rgb_adj;
		*b = rgb_max;
		break;
	case 4:
		*r = rgb_min + rgb_adj;
		*g = rgb_min;
		*b = rgb_max;
		break;
	default:
		*r = rgb_max;
		*g = rgb_min;
		*b = rgb_max - rgb_adj;
		break;
	}
}

void led_loop(void* parameters)
{
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;
	uint16_t hue = 0;
	uint16_t start_rgb = 0;
	led_strip_t* strip = (led_strip_t*)parameters;

	// Show simple rainbow chasing pattern
	ESP_LOGI(TAG, "LED Rainbow Chase Start");
	while (true) {
		for (int i = 0; i < 3; i++) {
			for (int j = i; j < CONFIG_NUM_LEDS; j += 3) {
				// Build RGB values
				hue = j * 360 / CONFIG_NUM_LEDS + start_rgb;
				led_strip_hsv2rgb(hue, 100, 20, &red, &green, &blue);
				// Write RGB values to strip driver
				ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
			}
			// Flush RGB values to LEDs
			ESP_ERROR_CHECK(strip->refresh(strip, 0));
			vTaskDelay(pdMS_TO_TICKS(200));
			//get_led_pattern()->step(i);
		}
		start_rgb += 60;
	}
}

int led_init(void)
{
	rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_RMT_TX_GPIO, RMT_TX_CHANNEL);
	config.clk_div = 2;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

	led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_NUM_LEDS, (led_strip_dev_t)config.channel);
	led_strip_t* strip = led_strip_new_rmt_ws2812(&strip_config);

	ESP_ERROR_CHECK(strip->clear(strip, 100));

	xTaskCreatePinnedToCore(led_loop, "LED loop", 4096, strip, 2, NULL, 0);

	return 0;
}
