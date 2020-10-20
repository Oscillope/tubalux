#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "hal/gpio_types.h"
#include "ui.h"
#include "ui_buttons.h"

#define TAG "UI_btn"

static void (*ui_buttons_callback)(uint32_t);
static TaskHandle_t ui_buttons_task;

void ui_isr_disable()
{
	gpio_intr_disable(GPIO_NUM_36);
	gpio_intr_disable(GPIO_NUM_39);
}

void ui_isr_enable()
{
	gpio_intr_enable(GPIO_NUM_36);
	gpio_intr_enable(GPIO_NUM_39);
}

void ui_buttons_reg_callback(void (*cb)(uint32_t data))
{
	ui_buttons_callback = cb;
}

void ui_buttons_debounce_task(void* parameters)
{
	while (true) {
		uint32_t button_bits = 0;
		uint32_t buttons_debounced = 0;
		xTaskNotifyWait(0, UINT32_MAX, &button_bits, portMAX_DELAY);
		vTaskDelay(pdMS_TO_TICKS(50));
		if (!gpio_get_level(GPIO_NUM_32)) {
			buttons_debounced |= button_bits & UI_BTN_R;
		}
		if (!gpio_get_level(GPIO_NUM_33)) {
			buttons_debounced |= button_bits & UI_BTN_L;
		}
		if (!gpio_get_level(GPIO_NUM_34)) {
			buttons_debounced |= button_bits & UI_BTN_UP;
		}
		if (!gpio_get_level(GPIO_NUM_36)) {
			buttons_debounced |= button_bits & UI_BTN_DN;
		}
		if (!gpio_get_level(GPIO_NUM_39)) {
			buttons_debounced |= button_bits & UI_BTN_PRS;
		}
		if (buttons_debounced && ui_buttons_callback) {
			ui_buttons_callback(buttons_debounced);
			ESP_LOGD(TAG, "buttons debounced %08x", buttons_debounced);
		}
	}
}

void IRAM_ATTR ui_button_r(void* arg)
{
	TaskHandle_t* buttons_task = (TaskHandle_t*)arg;
	xTaskNotifyFromISR(*buttons_task, UI_BTN_R, eSetBits, NULL);
}
void IRAM_ATTR ui_button_l(void* arg)
{
	TaskHandle_t* buttons_task = (TaskHandle_t*)arg;
	xTaskNotifyFromISR(*buttons_task, UI_BTN_L, eSetBits, NULL);
}
void IRAM_ATTR ui_button_up(void* arg)
{
	TaskHandle_t* buttons_task = (TaskHandle_t*)arg;
	xTaskNotifyFromISR(*buttons_task, UI_BTN_UP, eSetBits, NULL);
}
void IRAM_ATTR ui_button_dn(void* arg)
{
	TaskHandle_t* buttons_task = (TaskHandle_t*)arg;
	xTaskNotifyFromISR(*buttons_task, UI_BTN_DN, eSetBits, NULL);
}
void IRAM_ATTR ui_button_prs(void* arg)
{
	TaskHandle_t* buttons_task = (TaskHandle_t*)arg;
	xTaskNotifyFromISR(*buttons_task, UI_BTN_PRS, eSetBits, NULL);
}

void ui_buttons_init()
{
	/* Button configuration */
	gpio_config_t gpio_conf = {
		/* R: 32    L: 33       UP: 34      DN: 36      PRS: 39 */
		BIT64(32) | BIT64(33) | BIT64(34) | BIT64(36) | BIT64(39),
		GPIO_MODE_INPUT,
		GPIO_PULLUP_ENABLE,
		GPIO_PULLDOWN_DISABLE,
		GPIO_INTR_NEGEDGE };
	gpio_config(&gpio_conf);

	xTaskCreatePinnedToCore(ui_buttons_debounce_task, "UI debounce", 1024, NULL, 2, &ui_buttons_task, 0);

	/* Interrupt configuration */
	gpio_install_isr_service(0);
	gpio_isr_handler_add(GPIO_NUM_32, ui_button_r, &ui_buttons_task);
	gpio_isr_handler_add(GPIO_NUM_33, ui_button_l, &ui_buttons_task);
	gpio_isr_handler_add(GPIO_NUM_34, ui_button_up, &ui_buttons_task);
	gpio_isr_handler_add(GPIO_NUM_36, ui_button_dn, &ui_buttons_task);
	gpio_isr_handler_add(GPIO_NUM_39, ui_button_prs, &ui_buttons_task);
}
