#include <stdio.h>
#include <string.h>

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "serial.h"
#include "wifi.h"

#define MAX_LINKS 4

uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct
{
    uint8_t node_id;
    uint8_t mac_addr[6];
} wifi_link;

wifi_link wifi_links[MAX_LINKS];

esp_now_peer_info_t peerInfo;

int wifi_send_locate()
{
    // char wifi_msg[MSG_BUFFER_LENGTH];
    // sprintf(wifi_msg, "factor %d", 123);
    // if (esp_now_send(broadcast, (uint8_t *)wifi_msg, strlen(wifi_msg)) != ESP_OK)
    // {
    //     serial_out("error sending");
    // }

    return 0;
}

int wifi_send_status()
{

    return 0;
}

void espnow_onreceive(const uint8_t *mac, const uint8_t *data, int len)
{
    char buf[MSG_BUFFER_LENGTH];

    // Validate stuff tho...

    snprintf(buf, sizeof(buf), "espnow from MAC: %02x:%02x:%02x:%02x:%02x:%02x len=%d", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
    serial_out(buf);
}

int espnow_init(void)
{
    if (esp_now_init() != ESP_OK)
    {
        return -1;
    }
    esp_now_register_recv_cb(espnow_onreceive);
    //ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    //  register the broadcast address
    memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
    memcpy(peerInfo.peer_addr, broadcast, 6);
    peerInfo.channel = 0;
    peerInfo.ifidx = ESP_IF_WIFI_STA;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        return -2;
    }

    return 0;
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
#endif
}
