#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"

#include "led_patterns.h"
#include "leds.h"

#define TAG "LEDs"
#define RMT_TX_CHANNEL RMT_CHANNEL_0

static bool do_stop = false;
static led_pattern_t* cur_pattern;
static uint32_t hue = 0;		/* 0-360 */
static uint8_t intensity = 20;		/* 0-100 */
static uint32_t period = 200;		/* milliseconds */

bool led_should_stop()
{
	if (do_stop) {
		do_stop = false;
		return true;
	}
	return false;
}

void led_set_primary_hue(uint32_t new)
{
	hue = new;
}

uint32_t led_get_primary_hue()
{
	return hue;
}

void led_set_intensity(uint8_t new)
{
	intensity = new;
}

uint8_t led_get_intensity()
{
	return intensity;
}

void led_set_pattern(led_pattern_t* pattern)
{
	do_stop = true;
	cur_pattern = pattern;
}

led_pattern_t* led_get_pattern()
{
	return cur_pattern;
}

void led_set_period(uint32_t new)
{
	period = new;
}

uint32_t led_get_period()
{
	return period;
}

uint32_t led_get_num()
{
	// Someday this may come from EEPROM
	return CONFIG_NUM_LEDS;
}

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint8_t s, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b)
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
	led_strip_t* strip = (led_strip_t*)parameters;

	// Show simple rainbow chasing pattern
	ESP_LOGI(TAG, "LED Thread Start");
	while (true) {
		ESP_LOGI(TAG, "Starting pattern %s", cur_pattern->name);
		cur_pattern->start(strip);
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

	cur_pattern = &(get_patterns()[0]);

	xTaskCreatePinnedToCore(led_loop, "LED loop", 4096, strip, 2, NULL, 0);

	return 0;
}
