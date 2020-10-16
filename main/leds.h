#ifndef LEDS_H
#define LEDS_H
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"

#include "led_patterns.h"

void led_strip_hsv2rgb(uint32_t h, uint8_t s, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b);

bool led_should_stop(void);
void led_set_primary_hue(uint32_t new);
uint32_t led_get_primary_hue(void);
void led_set_intensity(uint8_t new);
uint8_t led_get_intensity(void);
void led_set_pattern(led_pattern_t* pattern);
led_pattern_t* led_get_pattern(void);
void led_set_period(uint32_t new);
uint32_t led_get_period(void);

int led_init(void);
#endif /* LEDS_H */
