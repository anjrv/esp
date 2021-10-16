#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "tasks.h"

void app_main(void)
{
	serial_prepare();

	TaskHandle_t svc_serial = NULL;
	TaskHandle_t svc_command = NULL;

	xTaskCreatePinnedToCore(
		serial_service,
		"svc_serial",
		2048,
		NULL,
		6,
		&svc_serial,
		1);

	xTaskCreatePinnedToCore(
		command_service,
		"svc_command",
		2048,
		NULL,
		1,
		&svc_command,
		1);

	printf("firmware ready\n");

	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
