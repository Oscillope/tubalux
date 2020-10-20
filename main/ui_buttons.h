#ifndef UI_BUTTONS_H
#define UI_BUTTONS_H
#include <stdint.h>
#include "freertos/FreeRTOS.h"

typedef enum {
	UI_BTN_NONE	= 0x00,
	UI_BTN_DN	= BIT(0),
	UI_BTN_UP	= BIT(1),
	UI_BTN_L	= BIT(2),
	UI_BTN_R	= BIT(3),
	UI_BTN_PRS	= BIT(4),
	UI_BTN_MAX	= BIT(31)
} ui_btn_t;

void ui_isr_disable(void);
void ui_isr_enable(void);

void ui_buttons_reg_callback(void (*cb)(uint32_t data));

void ui_buttons_init(void);

#endif /* UI_BUTTONS_H */
