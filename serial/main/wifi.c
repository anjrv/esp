#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "serial.h"
#include "wifi.h"
#include "noise.h"
#include "utils.h"
#include "serial.h"
#include "tasks.h"

#define MAX_LINKS 4
#define FRAME_SIZE 152
// Some packet constants
#define VERSION 0x11
#define BROADCAST 0xFF
#define LOCATE 0x01
#define LINK 0x02
#define STATUS 0x03

static const TickType_t LOCATE_DELAY = 5000 / portTICK_PERIOD_MS;

static uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t wifi_msg[FRAME_SIZE] = {0};
static uint8_t own_id = 0xFF;
static uint8_t identifier = 0xFF;
static int curr_links = 0;
static int added_peers = 0;
static int active_nodes = 0;
static int inactive_nodes = 0;

typedef struct
{
    uint8_t node_id;
    uint8_t mac_addr[6];
} wifi_link;

static esp_now_peer_info_t peerInfo;

static wifi_link wifi_links[MAX_LINKS] = {0};
static time_t link_timers[MAX_LINKS];
static time_t status_timers[MAX_LINKS];
static time_t locate_timer;

static SemaphoreHandle_t links_access;
static SemaphoreHandle_t link_timers_access;
static SemaphoreHandle_t status_timers_access;

void wifi_net_table()
{
    while (xSemaphoreTake(links_access, WAIT_QUEUE) != pdTRUE)
    {
        vTaskDelay(DELAY);
    }

    char buf[40];
    int results = 0;
    for (int i = 0; i < MAX_LINKS; i++)
    {
        if (wifi_links[i].mac_addr[0] != 0)
        {
            results++;
            snprintf(buf, sizeof(buf), "%d %02X %02X:%02X:%02X:%02X:%02X:%02X",
                     i,
                     wifi_links[i].node_id,
                     wifi_links[i].mac_addr[0],
                     wifi_links[i].mac_addr[1],
                     wifi_links[i].mac_addr[2],
                     wifi_links[i].mac_addr[3],
                     wifi_links[i].mac_addr[4],
                     wifi_links[i].mac_addr[5]);
            serial_out(buf);
        }
    }

    if (!results) {
        serial_out("empty table");
    }

    xSemaphoreGive(links_access);
}

void wifi_net_reset()
{
    while (xSemaphoreTake(links_access, WAIT_QUEUE) != pdTRUE)
    {
        vTaskDelay(DELAY);
    }

    for (int i = 0; i < MAX_LINKS; i++)
    {
        wifi_links[i].node_id = 0;
        wifi_links[i].mac_addr[0] = 0;
        wifi_links[i].mac_addr[1] = 0;
        // Rest isn't actually checked
    }

    xSemaphoreGive(links_access);
    serial_out("node reset");
}

int wifi_send_locate()
{
    while (xSemaphoreTake(links_access, WAIT_QUEUE) != pdTRUE)
    {
        vTaskDelay(DELAY);
    }

    int has_room = 0;
    for (int i = 0; i < MAX_LINKS; i++)
    {
        if (wifi_links[i].mac_addr[0] == 0)
        {
            has_room++;
            break;
        }
    }

    xSemaphoreGive(links_access);

    if (!has_room)
    {
        return -3;
    }

    wifi_msg[0] = VERSION;
    wifi_msg[1] = own_id;
    wifi_msg[2] = BROADCAST;
    // 3 is checksum
    wifi_msg[4] = LOCATE;

    identifier = random_int(RAND_MAX);
    wifi_msg[5] = identifier;
    wifi_msg[3] = wifi_msg[0] + wifi_msg[1] + wifi_msg[2] + wifi_msg[4] + wifi_msg[5];
    // Rest of the message is initialized to 0

    if (esp_now_send(broadcast, (uint8_t *)wifi_msg, FRAME_SIZE) != ESP_OK)
    {
        return -1;
    }

    added_peers = 0;
    time(&locate_timer);
    vTaskDelay(LOCATE_DELAY);

    return added_peers;
}

int wifi_send_status()
{

    return 0;
}

int send_link(const uint8_t node, const uint8_t id)
{
    while (xSemaphoreTake(links_access, WAIT_QUEUE) != pdTRUE)
    {
        vTaskDelay(DELAY);
    }

    int i;
    for (i = 0; i < MAX_LINKS; i++)
    {
        if (wifi_links[i].node_id == node)
        {
            // Already exists
            xSemaphoreGive(links_access);
            return -3;
        }
    }

    for (i = 0; i < MAX_LINKS; i++)
    {
        if (wifi_links[i].mac_addr[0] == 0)
        {
            while (xSemaphoreTake(link_timers_access, WAIT_QUEUE) != pdTRUE)
            {
                vTaskDelay(DELAY);
            }

            // Temp store
            wifi_links[i].node_id = node;
            wifi_links[i].mac_addr[1] = id;
            time(&link_timers[i]);

            xSemaphoreGive(link_timers_access);
            break;
        }
    }

    xSemaphoreGive(links_access);

    wifi_msg[0] = VERSION;
    wifi_msg[1] = own_id;
    wifi_msg[2] = node;
    wifi_msg[4] = LINK;
    wifi_msg[5] = id;
    wifi_msg[3] = wifi_msg[0] + wifi_msg[1] + wifi_msg[2] + wifi_msg[4] + wifi_msg[5];

    if (esp_now_send(broadcast, (uint8_t *)wifi_msg, FRAME_SIZE) != ESP_OK)
    {
        return -1;
    }

    return 0;
}

int process_link(const uint8_t node, const uint8_t id, const uint8_t *mac)
{
    if (identifier == id)
    {
        if (difftime(time(NULL), locate_timer) > 1)
        {
            // Response to locate too slow
            return -4;
        }
        else
        {
            while (xSemaphoreTake(links_access, WAIT_QUEUE) != pdTRUE)
            {
                vTaskDelay(DELAY);
            }

            int i;
            for (i = 0; i < MAX_LINKS; i++)
            {
                if (wifi_links[i].node_id == node)
                {
                    // Already exists
                    xSemaphoreGive(links_access);
                    return -3;
                }
            }

            for (i = 0; i < MAX_LINKS; i++)
            {
                if (wifi_links[i].mac_addr[0] == 0)
                {
                    // Perm store
                    wifi_links[i].node_id = node;
                    memcpy(wifi_links[i].mac_addr, mac, sizeof(uint8_t) * 6);
                    added_peers++;
                    break;
                }
            }

            xSemaphoreGive(links_access);
        }

        wifi_msg[0] = VERSION;
        wifi_msg[1] = own_id;
        wifi_msg[2] = node;
        wifi_msg[4] = LINK;
        wifi_msg[5] = id;
        wifi_msg[3] = wifi_msg[0] + wifi_msg[1] + wifi_msg[2] + wifi_msg[4] + wifi_msg[5];

        if (esp_now_send(broadcast, (uint8_t *)wifi_msg, FRAME_SIZE) != ESP_OK)
        {
            return -1;
        }

        return 0;
    }
    else
    {
        while (xSemaphoreTake(links_access, WAIT_QUEUE) != pdTRUE)
        {
            vTaskDelay(DELAY);
        }

        int i;
        for (i = 0; i < MAX_LINKS; i++)
        {
            if (wifi_links[i].node_id == node)
            {
                while (xSemaphoreTake(link_timers_access, WAIT_QUEUE) != pdTRUE)
                {
                    vTaskDelay(DELAY);
                }

                if (difftime(time(NULL), link_timers[i]) > 2)
                {
                    // Response to link too slow
                    xSemaphoreGive(link_timers_access);

                    return -4;
                }

                memcpy(wifi_links[i].mac_addr, mac, sizeof(uint8_t) * 6);
                xSemaphoreGive(link_timers_access);
                break;
            }
        }

        xSemaphoreGive(links_access);
        return 0;
    }

    return -1;
}

void espnow_onreceive(const uint8_t *mac, const uint8_t *data, int len)
{
    if (len != FRAME_SIZE)
    {
        // Length wrong
        return;
    }

    uint8_t chk = 0x00;
    int i = 0;
    for (i = 0; i < len; i++)
    {
        if (i != 3)
        {
            chk += data[i];
        }
    }

    if (chk ^ data[3] || VERSION ^ data[0] || (data[2] ^ BROADCAST && data[2] ^ own_id))
    {
        // They're truthy for a reason...
        return;
    }

    switch (data[4])
    {
    case LOCATE:
        send_link(data[1], data[5]);
        break;
    case LINK:
        process_link(data[1], data[5], mac);
        break;
    case STATUS:
        break;
    }
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

    uint8_t curr_mac[6];
    esp_efuse_mac_get_default(curr_mac);
    own_id = curr_mac[0] == 0x30 ? 0x1e : 0x1d;

    links_access = xSemaphoreCreateBinary();
    xSemaphoreGive(links_access);

    link_timers_access = xSemaphoreCreateBinary();
    xSemaphoreGive(link_timers_access);

    status_timers_access = xSemaphoreCreateBinary();
    xSemaphoreGive(status_timers_access);

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
