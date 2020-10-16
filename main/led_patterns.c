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
	int step = 360 / CONFIG_NUM_LEDS;
	int pos = 0;
	while (!led_should_stop()) {
		for (int i = 0; i < CONFIG_NUM_LEDS; i++) {
			uint32_t hue = ((i + pos) * step) % 360;
			uint8_t r, g, b;
			led_strip_hsv2rgb(hue, 100, led_get_intensity(), &r, &g, &b);
			ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		}
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		pos = (pos + 1) % CONFIG_NUM_LEDS;
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
			i = CONFIG_NUM_LEDS - pos - 1;
		} else {
			i = pos;
		}
		led_strip_hsv2rgb(led_get_primary_hue(), 100, led_get_intensity(), &r, &g, &b);
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, 0, 0, 0));
		if (pos == (CONFIG_NUM_LEDS - 1)) {
			reverse = !reverse;
		}
		pos = (pos + 1) % CONFIG_NUM_LEDS;
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_marquee(led_strip_t* strip)
{
	int pos = 0;
	while (!led_should_stop()) {
		for (int i = 0; i < CONFIG_NUM_LEDS; i++) {
			uint8_t r, g, b;
			if ((i + pos) % 4 == 0) {
				led_strip_hsv2rgb(led_get_primary_hue(), 100, led_get_intensity(), &r, &g, &b);
			} else {
				r = g = b = 0;
			}
			ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		}
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		pos = (pos + 1) % CONFIG_NUM_LEDS;
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_rainbowcyl(led_strip_t* strip)
{
	int pos = 0, i = 0;
	bool reverse = false;
	int step = 360 / CONFIG_NUM_LEDS;
	while (!led_should_stop()) {
		uint8_t r, g, b;
		int hue = (pos * step) % 360;
		if (reverse) {
			i = CONFIG_NUM_LEDS - pos - 1;
		} else {
			i = pos;
		}
		led_strip_hsv2rgb(hue, 100, led_get_intensity(), &r, &g, &b);
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
		ESP_ERROR_CHECK(strip->refresh(strip, 0));
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, 0, 0, 0));
		if (pos == (CONFIG_NUM_LEDS - 1)) {
			reverse = !reverse;
		}
		pos = (pos + 1) % CONFIG_NUM_LEDS;
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

void pat_solid(led_strip_t* strip)
{
	for (int i = 0; i < CONFIG_NUM_LEDS; i++) {
		uint8_t r, g, b;
		led_strip_hsv2rgb(led_get_primary_hue(), 100, led_get_intensity(), &r, &g, &b);
		ESP_ERROR_CHECK(strip->set_pixel(strip, i, r, g, b));
	}
	ESP_ERROR_CHECK(strip->refresh(strip, 0));
	while (!led_should_stop()) {
		vTaskDelay(pdMS_TO_TICKS(led_get_period()));
	}
}

/*
void pat_pulse(led_strip_t* strip)
{
	int pos = 0;
	bool pulsing = 0;
	while (!led_should_stop()) {
		if (pos == 0) {
			pulsing = !pulsing;
		}
		self.leds.fill(self.hsv2rgb(self.hue, 1, self.intens))
		if (pulsing) {
			int i = 0;
			for (i = 0; i < (pos - 8); i += pos) {
				if (i >= 0):
					self.leds[i] = self.hsv2rgb(self.hue, 1, self.intens / (2 ** (9 - (pos - i))))
			}
			self.leds[pos] = (0, 0, 0)
		elif (pos < 8):
			for i in range(1, 9 - pos):
				self.leds[num - i] = self.hsv2rgb(self.hue, 1, self.intens / (2 ** (9 - (i + pos))))
		}
		self.leds.write()
		pos = (pos + 1) % num
		sleep(self.period / 4)
	}
}

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

# adapted from an arduino pattern at:
# http://www.funkboxing.com/wordpress/wp-content/_postfiles/fluxbox_octo.ino
void _pat_flame_internal(led_strip_t* strip, uint32_t hmin, uint32_t hmax)
{
	uint8_t r, g, b;
	int hdif = hmax-hmin;
	uint32_t ahue = hmin;
	ESP_ERROR_CHECK(strip->clear(strip, 0));
	while (!led_should_stop()){
		uint32_t idelay = (20 / esp_random()) + 2;
		uint32_t randtemp = (6 / esp_random()) + 3;
		uint32_t hinc = (hdif/CONFIG_NUM_LEDS) + randtemp;
		spread = random.randint(5, 40)
		start = random.randint(0, num-spread)
		for i in range(start, start + spread):
			if ((ahue + hinc) > hmax):
				ahue = hmin
			else:
				ahue = ahue + hinc
			acolor = self.hsv2rgb(ahue, 1, self.intens)
			self.leds[i] = acolor
			self.leds[num - i - 1] = acolor
			self.leds.write()
			sleep(idelay/100.0);
	}
}

def pat_flame(self, num):
	self._pat_flame_internal(0.1, 40.0, num)

def pat_flame_g(self, num):
	self._pat_flame_internal(80.0, 160.0, num)

def pat_flame_b(self, num):
	self._pat_flame_internal(170.0, 290.0, num)

def pat_flame_rbow(self, num):
	self._pat_flame_internal(0.1, 360.0, num)
*/

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
		.name = "Marquee",
		.start = pat_marquee,
	},
	(led_pattern_t) {
		.name = "RCylon",
		.start = pat_rainbowcyl,
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
