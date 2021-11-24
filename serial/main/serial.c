#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "serial.h"
#include "stack.h"
#include "dict.h"
#include "utils.h"
#include "commands.h"
#include "tasks.h"
#include "client.h"
#include "data_tasks.h"
#include "noise.h"
#include "network.h"
#include "collatz.h"
#include "dht.h"
#include "app_sensor.h"

const TickType_t read_delay = 50 / portTICK_PERIOD_MS;
// Data structures and global variables to ease communication
// between the main loop and response function
dict *dictionary;
stack *stack_pointer;
int counter;
char q[MSG_BUFFER_LENGTH];

// serial_out(..) method assumes string is null-terminated but does not
//	have the specification-mandated newline terminator.  This method applies
//	the newline terminator and writes the string over the serial connection.
void serial_out(const char *string)
{
	int end = strlen(string);
	if (end >= MSG_BUFFER_LENGTH)
	{
		// Error: Output too long.
		return;
	}

	// NOTE: Working spec requires max of 256 bytes containing a newline terminator.
	//	Thus the storage buffer needs 256+1 bytes since we also need a null terminator
	//	to form a valid string.
	char msg_buffer[MSG_BUFFER_LENGTH + 1];
	memset(msg_buffer, 0, MSG_BUFFER_LENGTH + 1);
	strcpy(msg_buffer, string);
	if (msg_buffer[end - 1] != '\n')
		msg_buffer[end] = '\n';
	printf(msg_buffer);
	fflush(stdout);
}

/**
 * respond(..) provides routing for the given query string
 * after splitting it into space delimited tokens
 */
void respond()
{
	char **command_split = NULL;
	char *query_duplicate = strdup(q);

	char *p = strtok(query_duplicate, " ");
	int quant = 0;

	while (p)
	{
		command_split = realloc(command_split, sizeof(char *) * ++quant);
		command_split[quant - 1] = p;

		p = strtok(NULL, " ");
	}

	command_split = realloc(command_split, sizeof(char *) * (quant + 1));
	command_split[quant] = '\0';

	// If there are no words present then
	// we skip searching for a command
	if (quant > 0)
	{
		// Cast command to uppercase to remove case sensitivity
		char *command = strupr(command_split[0]);

		if (strcmp(command, "PING") == 0)
		{
			command_ping();
		}
		else if (strcmp(command, "MAC") == 0)
		{
			command_mac();
		}
		else if (strcmp(command, "ID") == 0)
		{
			command_id();
		}
		else if (strcmp(command, "VERSION") == 0)
		{
			command_version();
		}
		else if (strcmp(command, "ERROR") == 0)
		{
			get_error();
		}
		else if (strcmp(command, "STORE") == 0)
		{
			command_store(quant, command_split, dictionary);
		}
		else if (strcmp(command, "QUERY") == 0)
		{
			command_query(quant, command_split, dictionary);
		}
		else if (strcmp(command, "PUSH") == 0)
		{
			command_push(quant, command_split, stack_pointer);
		}
		else if (strcmp(command, "POP") == 0)
		{
			command_pop(stack_pointer);
		}
		else if (strcmp(command, "ADD") == 0)
		{
			command_add(quant, command_split, stack_pointer, dictionary);
		}
		else if (strcmp(command, "PS") == 0)
		{
			command_ps();
		}
		else if (strcmp(command, "RESULT") == 0)
		{
			command_result(quant, command_split);
		}
		else if (strcmp(command, "FACTOR") == 0)
		{
			command_factor(quant, command_split, counter++, stack_pointer, dictionary);
		}
		// else if (strcmp(command, "BT_CONNECT") == 0)
		// {
		// 	command_bt_connect(quant, command_split);
		// }
		// else if (strcmp(command, "BT_STATUS") == 0)
		// {
		// 	command_bt_status();
		// }
		// else if (strcmp(command, "BT_CLOSE") == 0)
		// {
		// 	command_bt_close();
		// } // Suppress bluetooth commands, none of them work if device is using ESPNOW
		else if (strcmp(command, "DATA_CREATE") == 0)
		{
			command_data_create(quant, command_split);
		}
		else if (strcmp(command, "DATA_DESTROY") == 0)
		{
			command_data_destroy(quant, command_split);
		}
		else if (strcmp(command, "DATA_INFO") == 0)
		{
			command_data_info(quant, command_split);
		}
		else if (strcmp(command, "DATA_APPEND") == 0)
		{
			command_data_append(quant, command_split, counter++);
		}
		else if (strcmp(command, "DATA_RAW") == 0)
		{
			command_data_raw(quant, command_split);
		}
		else if (strcmp(command, "DATA_STAT") == 0)
		{
			command_data_stat(quant, command_split, counter++);
		}
		// else if (strcmp(command, "NET_LOCATE") == 0)
		// {
		// 	command_net_locate();
		// }
		// else if (strcmp(command, "NET_STATUS") == 0)
		// {
		// 	command_net_status();
		// }
		// else if (strcmp(command, "NET_RESET") == 0)
		// {
		// 	command_net_reset();
		// } // Suppress old 2-node network functionality
		else if (strcmp(command, "NET_TABLE") == 0)
		{
			command_net_table();
		}
		else
		{
			// Default case, command does not exist
			serial_out("command error");
		}
	}
	else
	{
		// No command input
		serial_out("command error");
	}

	free(command_split);
	free(query_duplicate);
	serial_out("");
}

/**
 * Main loop function of the app
 *
 * Reads user input and creates
 * response function as needed
 *
 * @param pvParameter function call parameter not used
 */
void main_task(void *pvParameter)
{
	serial_out("firmware ready");

	while (true)
	{
		int complete = 0;
		int at = 0;
		int whitespace = 0;
		int consecutive_whitespace = 0;
		int leading_whitespace = 0;
		int trailing_whitespace = 0;

		memset(q, 0, MSG_BUFFER_LENGTH);

		while (!complete)
		{
			if (at >= 256)
			{
				serial_out("input length exceeded");
				serial_out("");
				break;
			}
			int result = fgetc(stdin);

			if (at == 0 && ((char)result == ' '))
			{
				leading_whitespace = 1;
			}

			if (result == EOF)
			{
				vTaskDelay(read_delay);
				continue;
			}
			else if ((char)result == '\n')
			{
				if (whitespace)
				{
					trailing_whitespace = 1;
				}

				complete = true;
			}
			else
			{
				q[at++] = (char)result;
			}

			if ((char)result == ' ')
			{
				if (whitespace)
				{
					consecutive_whitespace = 1;
				}
				whitespace = 1;
			}
			else
			{
				whitespace = 0;
			}
		}

		if (complete)
		{
			if (leading_whitespace)
			{
				serial_out("command error");
				serial_out("");
			}
			else if (consecutive_whitespace || trailing_whitespace)
			{
				serial_out("argument error");
				serial_out("");
			}
			else
			{
				respond();
			}
		}
	}
}

void restart(void *pvParameter)
{
	vTaskDelay((4 * 60 * 1000) / portTICK_RATE_MS);
    esp_restart();
}

/**
 * Entry point function, creates the main loop
 */
void app_main(void)
{
	uint8_t local_mac[6];
	esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
	uint8_t id = 0;
	int root = 0;

	if (local_mac[1] == 0xAE)
	{
		// Black device:
		id = 0x1E;
		root = 0;
	}
	else if (local_mac[1] == 0x0A)
	{
		// Yellow device.
		id = 0x1D;
		root = 1;
	}
	else
	{
		// Should be the correct addresses for my devices
		// Need to be adjusted otherwise
		ESP_LOGE("APP_MAIN", "Could not determine Node-id.");
		while (1)
		{
			vTaskDelay(1000 / portTICK_RATE_MS);
		}
	}

	dictionary = create_dict(DICT_CAPACITY);
	stack_pointer = create_stack(STACK_CAPACITY);
	// data_client_prepare(); BT setup, conflicts with WiFi
	// esp_log_level_set("wifi", ESP_LOG_NONE);
	ESP_ERROR_CHECK(nvs_flash_init());
	initialize_bt_tasks();
	initialize_tasks();
	initialize_noise();
	net_init(id, root);
	counter = 0;

	/**
	 * NOTE: 4 tasks initialized off the bat
	 * 
	 * Main serial communication task
	 * Restart counter
	 * Collatz communication task
	 * Collatz computation task
	 */

	// Collatz app
    collatz_init(root);
	// dht_init(18);
	// app_sensor_init(id);

	// Serial commands
	xTaskCreate(
		&main_task,	   // - function ptr
		"main_task",   // - arbitrary name
		8192,		   // - stack size [byte]
		NULL,		   // - optional data for task
		MAIN_PRIORITY, // - priority of the main task
		NULL		   // - handle to task (for control)
	);

	// Low prio background task that will restart the device ... eventually
	xTaskCreate(
		&restart,	  // - function ptr
		"restart",	  // - arbitrary name
		2048,		  // - stack size [byte]
		NULL,		  // - optional data for task
		LOW_PRIORITY, // - priority of the main task
		NULL		  // - handle to task (for control)
	);
}
