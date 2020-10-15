#include "esp_log.h"

#include "led_patterns.h"

#define TAG "LED_pat"

void rainbow_step(int step)
{
	ESP_LOGI(TAG, "step %d", step);
}

void cylon_step(int step)
{
	ESP_LOGI(TAG, "cylon step %d", step);
}

led_pattern_t patterns[] = {
	(led_pattern_t) {
		.step = rainbow_step,
		.num_steps = CONFIG_NUM_LEDS,
		.name = "Rainbow"
	},
	(led_pattern_t) {
		.step = cylon_step,
		.num_steps = CONFIG_NUM_LEDS,
		.name = "Cylon"
	}
};

static led_pattern_t* cur_pattern = &patterns[0];

led_pattern_t* get_patterns()
{
	return patterns;
}

led_pattern_t* get_led_pattern()
{
	return cur_pattern;
}

void set_led_pattern(led_pattern_t* pattern)
{
	cur_pattern = pattern;
}
