#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

#include "freertos/FreeRTOS.h"
#include "led_strip.h"
#include "ui.h"

typedef struct {
	/* this must be first! */
	char name[17];
	void (*start)(led_strip_t*);
} led_pattern_t;

#define LED_NUM_PATTERNS	12

led_pattern_t* get_patterns(void);
ui_menu_t* get_pattern_menu(void);
void led_pattern_init(void);

#endif /* LED_PATTERNS_H */
