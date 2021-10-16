#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "client.h"

void app_main(void)
{
	int result_prep = data_client_prepare();
	if (result_prep != 0) {
		printf("Initialization failure: %d\n", result_prep);
		return;
	}
	printf("firmware ready\n");
	fflush(stdout);

	TaskHandle_t svc_worker;

	xTaskCreatePinnedToCore(
		worker,
		"svc_worker",
		2048,
		NULL,
		1,
		&svc_worker,
		1);

	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
