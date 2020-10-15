#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

typedef struct {
	void (*step)(int);
	unsigned num_steps;
	const char* name;
} led_pattern_t;

#define LED_NUM_PATTERNS	2

led_pattern_t* get_patterns(void);

led_pattern_t* get_led_pattern(void);
void set_led_pattern(led_pattern_t* pattern);

#endif /* LED_PATTERNS_H */
