#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "factors.h"
#include "serial.h"
#include "utils.h"

void factor(void *pvParameter) {
    int num = (*((int*)pvParameter));

    char res[MSG_BUFFER_LENGTH];
    snprintf(res, 10, "%d", num);
    strcat(res, ":");

    if (num < 0) {
        // Is negative, use absolute
        strcat(res, " -1");
        num = abs(num);
    }

    // Repeat until odd number
    while (num%2 == 0) {
        // Divisible by 2
        strcat(res, " 2");
        num = num/2;
    }

    for (int i = 3; i <= sqrt(num); i += 2) {
        while (num % i == 0) {
            // Divisible by i
            strcat(res, " ");
            strcat(res, long_to_string(i));
            num = num/i;
        }
    }

    if (num > 2) {
        // Remaining prime
        strcat(res, " ");
        strcat(res, long_to_string(num));
    }

    serial_out(res);

    vTaskDelete(NULL);
}

int prepare_factor(int value, char* id) {
    BaseType_t success;
    static int val = 0;
    val = value;

    success = xTaskCreatePinnedToCore(
		&factor,
		id,
		8192,
		(void*)&val,
		LOW_PRIORITY,
		NULL,
		tskNO_AFFINITY
	);

    if (success == pdPASS) {
        return 1;
    }

    return 0;
}