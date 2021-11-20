#include <stdio.h>
#include <stdint.h>

#include <freertos/freeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_netif.h>

#include "network.h"
#include "app_bounce.h"

void app_main(void)
{
    uint8_t local_mac[6];
    esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
    uint8_t id = 0;
    uint16_t bounce_freq = 0;
    int root = 0;

    if (local_mac[1] == 0x0A) {
        // Black device:
        id = 0x21;
        bounce_freq = 600;
        root = 0;
    }
    else if (local_mac[1] == 0x62) {
        // Yellow device.
        id = 0x22;
        bounce_freq = 2000;
        root = 1; 
    }
    else {
        // If this error is emitted, you have probably forgotten to customize this
        //  function to match with _your_ device MAC addresses.
        ESP_LOGE("APP_MAIN", "Could not determine Node-id.");
        while (1) {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }

    net_init(id, root);
    app_bounce_init(id, bounce_freq);
    vTaskDelay(60000 / portTICK_RATE_MS);
    app_bounce_add_message_up((id == 0x21 ? "BLACK" : "YELLOW"), 16);
}
