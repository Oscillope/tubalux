#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "leds.h"
#include "ui.h"

#define TAG "main"

void app_main(void)
{
	ESP_LOGI(TAG, "Welcome to tubalux!");
	led_init();
	ui_init();
}
