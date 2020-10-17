#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "leds.h"
#include "led_patterns.h"

#define TAG "LED_pat"

void pat_rainbow(led_strip_t* strip)
{
	int step = 360 / led_get_num();
	int pos = 0;
	while (!led_should_stop()) {
		for (int i = 0; i < led_get_num(); i++) {
			uint32_t hue = ((i + pos) * step) % 360;
			uint8_t r, g, b;
			led_strip_hsv2rgb(hue, 100, led_get_intensity(), &r, &g, &b);
			ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		}
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		pos = (pos + 1) % led_get_num();
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_bounce(led_strip_t* strip)
{
	int pos = 0, i = 0;
	bool reverse = 0;
	while (!led_should_stop()) {
		uint8_t r, g, b;
		if (reverse) {
			i = led_get_num() - pos - 1;
		} else {
			i = pos;
		}
		led_strip_hsv2rgb(led_get_primary_hue(), 100, led_get_intensity(), &r, &g, &b);
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, 0, 0, 0));
		if (pos == (led_get_num() - 1)) {
			reverse = !reverse;
		}
		pos = (pos + 1) % led_get_num();
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_marquee(led_strip_t* strip)
{
	int pos = 0;
	while (!led_should_stop()) {
		for (int i = 0; i < led_get_num(); i++) {
			uint8_t r, g, b;
			if ((i + pos) % 4 == 0) {
				led_strip_hsv2rgb(led_get_primary_hue(), 100, led_get_intensity(), &r, &g, &b);
			} else {
				r = g = b = 0;
			}
			ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		}
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		pos = (pos + 1) % led_get_num();
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_rainbowcyl(led_strip_t* strip)
{
	int pos = 0, i = 0;
	bool reverse = false;
	int step = 360 / led_get_num();
	while (!led_should_stop()) {
		uint8_t r, g, b;
		int hue = (pos * step) % 360;
		if (reverse) {
			i = led_get_num() - pos - 1;
		} else {
			i = pos;
		}
		led_strip_hsv2rgb(hue, 100, led_get_intensity(), &r, &g, &b);
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, 0, 0, 0));
		if (pos == (led_get_num() - 1)) {
			reverse = !reverse;
		}
		pos = (pos + 1) % led_get_num();
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_solid(led_strip_t* strip)
{
	for (int i = 0; i < led_get_num(); i++) {
		uint8_t r, g, b;
		led_strip_hsv2rgb(led_get_primary_hue(), 100, led_get_intensity(), &r, &g, &b);
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
	}
	ESP_ERROR_CHECK(strip->refresh(strip, 0));
	while (!led_should_stop()) {
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_pulse(led_strip_t* strip)
{
	int pos = 0;
	bool pulsing = 0;
	while (!led_should_stop()) {
		int i = 0;
		uint8_t r, g, b;
		uint32_t intensity;
		if (pos == 0) {
			pulsing = !pulsing;
		}
		if (pulsing) {
			for (i = (pos - 8); i < pos; i++) {
				if (i >= 0) {
					intensity = led_get_intensity() / (1 << (9 - (pos - i)));
					led_strip_hsv2rgb(led_get_primary_hue(), 100, intensity , &r, &g, &b);
					ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
				}
			}
		} else if (pos < 8) {
			for (i = 1; i < 9 - pos; i++) {
				intensity = led_get_intensity() / (1 << (9 - (pos + i)));
				led_strip_hsv2rgb(led_get_primary_hue(), 100, intensity, &r, &g, &b);
				ESP_ERROR_CHECK(strip->set_pixel(strip, (led_get_num() - i), r, g, b));
			}
		}
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		pos = (pos + 1) % led_get_num();
		vTaskDelay(pdMS_TO_TICKS(led_get_period() / 4));
	}
}

/*
	def pat_radar(self, num):
		self.leds.fill((0, 0, 0))
		pos = 0
		while (not self.pat_chg):
			self.leds.fill((0, 0, 0))
			for j, len in enumerate(self.rings):
				offset = sum(self.rings[:j])
				index = (pos % len) + offset
				self.leds[index] = self.hsv2rgb(self.hue, 1, self.intens)
			self.leds.write()
			pos = (pos + 1) % num
			sleep(self.period / 4)

	def pat_tunnel(self, num):
		ring = 0
		while (not self.pat_chg):
			self.leds.fill((0, 0, 0))
			offset = sum(self.rings[:ring])
			for i in range(offset, offset + self.rings[ring]):
				self.leds[i] = self.hsv2rgb(self.hue, 1, self.intens)
			self.leds.write()
			ring = (ring + 1) % len(self.rings)
			sleep(self.period)

	def pat_bubbles(self, num):
		ring = 0
		self.leds.fill((0, 0, 0))
		while (not self.pat_chg):
			self.leds.fill((0, 0, 0))
			ring = random.randint(0, len(self.rings) - 1)
			offset = sum(self.rings[:ring])
			color = self.hsv2rgb(random.randint(0, 359), 1, self.intens)
			for i in range(offset, offset + self.rings[ring]):
				self.leds[i] = color
			self.leds.write()
			sleep(self.period)

	def pat_rgb_party(self, num):
		pos = 0
		cycle = 0
		while (not self.pat_chg):
			self.leds[pos] = self.hsv2rgb(cycle * 90, 1, self.intens)
			self.leds.write()
			pos = (pos + 1) % num
			if (pos == 0):
				cycle = (cycle + 1) % 3
			sleep(self.period / 16)
*/

uint32_t esp_random_range(uint32_t min, uint32_t max)
{
	return (((esp_random() * (max - min)) / UINT_MAX) + min);
}

// adapted from an arduino pattern at:
// http://www.funkboxing.com/wordpress/wp-content/_postfiles/fluxbox_octo.ino
void _pat_flame_internal(led_strip_t* strip, uint32_t hmin, uint32_t hmax)
{
	uint8_t r, g, b;
	int hdif = hmax - hmin;
	uint32_t ahue = hmin;
	ESP_ERROR_CHECK(strip->clear(strip, 0));
	while (!led_should_stop()){
		uint32_t idelay = esp_random_range(2, 20);
		uint32_t randtemp = esp_random_range(3, 6);
		uint32_t hinc = (hdif / led_get_num()) + randtemp;
		uint32_t spread = esp_random_range(5, 40);
		uint32_t start = esp_random_range(0, led_get_num() - spread);
		for (int i = start; i < (start + spread); i++) {
			if ((ahue + hinc) > hmax) {
				ahue = hmin;
			} else {
				ahue = ahue + hinc;
			}
			led_strip_hsv2rgb(ahue, 100, led_get_intensity(), &r, &g, &b);
			ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
			ESP_ERROR_CHECK(strip->set_pixel(strip, led_get_num() - i - 1, r, g, b));
			ESP_ERROR_CHECK(strip->refresh(strip, 0));
			vTaskDelay(pdMS_TO_TICKS(idelay));
		}
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_flame(led_strip_t* strip)
{
	_pat_flame_internal(strip, 0, 40);
}

void pat_flame_g(led_strip_t* strip)
{
	_pat_flame_internal(strip, 80, 160);
}

void pat_flame_b(led_strip_t* strip)
{
	_pat_flame_internal(strip, 170, 290);
}

void pat_flame_rbow(led_strip_t* strip)
{
	_pat_flame_internal(strip, 0, 360);
}

led_pattern_t patterns[LED_NUM_PATTERNS] = {
	(led_pattern_t) {
		.name = "Rainbow",
		.start = pat_rainbow,
	},
	(led_pattern_t) {
		.name = "Cylon",
		.start = pat_bounce,
	},
	(led_pattern_t) {
		.name = "RCylon",
		.start = pat_rainbowcyl,
	},
	(led_pattern_t) {
		.name = "Marquee",
		.start = pat_marquee,
	},
	(led_pattern_t) {
		.name = "Pulse",
		.start = pat_pulse,
	},
	(led_pattern_t) {
		.name = "R flame",
		.start = pat_flame,
	},
	(led_pattern_t) {
		.name = "G flame",
		.start = pat_flame_g,
	},
	(led_pattern_t) {
		.name = "B flame",
		.start = pat_flame_b,
	},
	(led_pattern_t) {
		.name = "RB flame",
		.start = pat_flame_rbow,
	},
	(led_pattern_t) {
		.name = "Solid",
		.start = pat_solid,
	},
};

ui_menu_t led_pattern_menu[LED_NUM_PATTERNS];

led_pattern_t* get_patterns()
{
	return patterns;
}

ui_menu_t* get_pattern_menu()
{
	return led_pattern_menu;
}

void led_pattern_init()
{
	for (int i = 0; i < LED_NUM_PATTERNS; i++) {
		strcpy(led_pattern_menu[i].name, patterns[i].name);
	}
}
