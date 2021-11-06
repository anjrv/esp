
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_timer.h>	// esp_timer_get_time(..)

#include <driver/gpio.h> // IO via pins

// Some definitions for clarity.
#define PIN_HIGH 1
#define PIN_LOW 0
#define RISING PIN_HIGH
#define FALLING PIN_LOW

#define DHT_DATA GPIO_NUM_18
#define DHT_MASK (1ull << DHT_DATA )
// #define DHT_LOG

#define TIMEOUT_MICROS (20000ll)
#define THRESHOLD_MICROS (40ll)

// Structure to store data from DHT22 sensor.
typedef struct DhtRead_s {
	int16_t temperature;
	int16_t humidity;
} DhtRead_t;

// Forward declarations for convenience.
void dht_setup();
int dht_scan(DhtRead_t* out);

void dht_setup() {
	gpio_config_t io_mix = {};

	io_mix.intr_type = GPIO_INTR_DISABLE;
	io_mix.mode = GPIO_MODE_INPUT_OUTPUT_OD;
	io_mix.pull_up_en = 0;
	io_mix.pull_down_en = 0;
	io_mix.pin_bit_mask = DHT_MASK;
	gpio_config(&io_mix);

	printf("Setup complete.\n");
	fflush(stdout);
}

// Method samples the DHT22 sensor device.  Returns 0 on success,
//	non-zero otherwise.
int dht_scan(DhtRead_t* out) {
	assert(out != NULL);

	int16_t temp = 0;
	int16_t humid = 0;
	int16_t check = 0;
	int16_t balance = 0;

	int last = PIN_HIGH;
	int edge = RISING;

	int64_t t0 = 0;
	int64_t t_last = 0;
	int64_t t_now = 0;

	// Signal the request for sensor data.
	gpio_set_level(DHT_DATA, PIN_LOW);
	vTaskDelay(10 / portTICK_RATE_MS);
	gpio_set_level(DHT_DATA, PIN_HIGH);

	// Initialize time-keeping, which we will need in order to discern
	//	0-bits from 1-bits.
	t0 = esp_timer_get_time();
	t_last = t0;
	t_now = t0;
	// NOTE: esp_timer_get_time() has microsecond resolution.

	// Ignore the first 2 falling edges, these correspond to the sensor's
	//  response & prepare signal.
	int ignore = 1;

	// Keep track of the data bits we've received; this will be used to fill
	//	the correct components of the read.
	int bits = 0;
	int16_t* target = NULL;

	// Within the TIMEOUT window, track the signal edges on the data wire
	//	which indicate transmission from the DHT22.
	while (t_now - t0 < TIMEOUT_MICROS && ignore > -41) {
		t_now = esp_timer_get_time();
		edge = gpio_get_level(DHT_DATA);

		if (last != edge) {
			last = edge;
			if (edge == FALLING) {
				if (ignore < 0) {

					if (bits < 16)			target = &humid;
					else if	(bits < 32)		target = &temp;
					else					target = &check;

					*target = *target << 1;
					if (t_now - t_last > THRESHOLD_MICROS) {
						*target |= 1;
					}
					++bits;
				}
				--ignore;
			}
			t_last = t_now;
		}
	}

	// Failure condition -- timeout window passed without receiving the
	//	expected number of bits from DHT22.
	if (t_now - t0 >= TIMEOUT_MICROS) {
#		ifdef DHT_LOG
		printf("[DHT Error]: Sensor read timed out.\n");
		fflush(stdout);
#		endif

		return 1;
	}

	balance = ((temp & 0x00FF) + (temp >> 8) + (humid & 0x00FF) + (humid >> 8)) & 0x00FF;

	// Failure condition -- checksum doesn't match received data.
	if (check != balance) {
#		ifdef DHT_LOG
		printf("[DHT Error]: Sensor read corrupted.\n");
		fflush(stdout);
#		endif

		return 2;
	}

	out->humidity = humid;
	out->temperature = temp;
	return 0;
}

int app_main()
{
	dht_setup();

	DhtRead_t data = {};
	while (1) {
		vTaskDelay(2500 / portTICK_RATE_MS);
		if (!dht_scan(&data)) {
			printf("T: %.1f, RH: %.1f\n", ((float)data.temperature) * 0.1f, ((float)data.humidity) * 0.1f);
		}
	}
}