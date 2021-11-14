#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

/*  DHT22 */
#include <esp_timer.h>   // esp_timer_get_time(..)
#include <driver/gpio.h> // IO via GPIO pins

/******************************************************************/
/*  UPDATE THESE -- AND NOTHING ELSE!                             */
/******************************************************************/
/*
 * Alternatively, define your network SSID and PSK (WPA2) in my_network.h
 *
 */
#define WIFI_SSID "NETGEAR76"
#define WIFI_PASS "newdiamond274"
#ifndef WIFI_SSID
#include "my_network.h"
#endif

#define MY_LATITUDE round(10000 * 63.926900)
#define MY_LONGITUDE round(10000 * -20.990400)

/*****************************************************************
 *                                                               *
 *  Node data                                                    *
 *                                                               *
 *****************************************************************/

typedef struct
{
    int changed;     /* sender clears   */
    long lat;        /* 10000 x         */
    long lon;        /* 10000 x         */
    int temperature; /* 10 x in Celcius */
    int humidity;    /* in percentages  */
} node_data_t;

static SemaphoreHandle_t mutex = NULL;
static unsigned char mac_addr[6];
static node_data_t node_data =
    {
        .changed = 0,
        .lat = MY_LATITUDE,
        .lon = MY_LONGITUDE,
};

/*****************************************************************/

/*
 *  Logging system, we use two channels
 */
static const char *WTAG = "wifi"; // WiFi related info
static const char *MTAG = "meas"; // Measurement related info

/*
 *  Host system somewhere in Gróska
 */
#define HOST_IP_ADDR "130.208.113.193"; // RPi of the course
#define HOST_IP_PORT 7654

/*
 *  Wifi
 */
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

/*****************************************************************
 *                                                               *
 *  DHT22 code (that could go to its own file)                   *
 *                                                               *
 *****************************************************************/

// Some definitions for clarity.
#define PIN_LOW 0
#define PIN_HIGH 1
#define RISING PIN_HIGH
#define FALLING PIN_LOW
#define DHT_DATA GPIO_NUM_18
#define DHT_MASK (1ull << DHT_DATA)
#define TIMEOUT_MICROS (20000ll)
#define THRESHOLD_MICROS (40ll)

// Structure to store data from DHT22 sensor.
typedef struct DhtRead_s
{
    int16_t temperature; // x10, fixed point
    int16_t humidity;    // x10, fixed point
} DhtRead_t;

void dht_setup()
{
    gpio_config_t io_mix = {};
    io_mix.intr_type = GPIO_INTR_DISABLE;
    io_mix.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    io_mix.pull_up_en = 0;
    io_mix.pull_down_en = 0;
    io_mix.pin_bit_mask = DHT_MASK;
    gpio_config(&io_mix);
}

// Sample the DHT22 sensor device. Returns 0 on success, non-zero otherwise.
int dht_scan(DhtRead_t *out)
{
    assert(out != NULL);
    int16_t temp = 0;  // DHT22 data frame:
    int16_t humid = 0; // [temperature|humidity|checksum]
    int16_t check = 0;
    int last = PIN_HIGH;
    int edge = RISING;
    int64_t t0 = 0;
    int64_t t_last = 0;
    int64_t t_now = 0;

    // Signal the request for sensor data.
    gpio_set_level(DHT_DATA, PIN_LOW);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(DHT_DATA, PIN_HIGH);

    // Initialize time-keeping to discern 0-bits from 1-bits.
    t0 = esp_timer_get_time(); //  microsecond resolution
    t_last = t0;
    t_now = t0;
    // Ignore the first 2 falling edges, these correspond to the sensor's
    // response & prepare signal.
    int ignore = 1;
    // Keep track of the data bits we've received; this will be used to fill
    // the correct components of the read.
    int bits = 0;
    int16_t *target = NULL;
    // Within the TIMEOUT window, track the signal edges on the data wire
    // which indicate transmission from the DHT22.
    while (t_now - t0 < TIMEOUT_MICROS && ignore > -41)
    {
        t_now = esp_timer_get_time();
        edge = gpio_get_level(DHT_DATA);
        if (last != edge)
        {
            last = edge;
            if (edge == FALLING)
            {
                if (ignore < 0)
                {
                    if (bits < 16)
                        target = &humid;
                    else if (bits < 32)
                        target = &temp;
                    else
                        target = &check;
                    *target = *target << 1;
                    if (t_now - t_last > THRESHOLD_MICROS)
                    {
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
    // expected number of bits from DHT22.
    if (t_now - t0 >= TIMEOUT_MICROS)
    {
        ESP_LOGE(MTAG, "Sensor read timed out.");
        return 1;
    }
    // Verify the checksum
    check ^= ((temp & 0x00FF) + (temp >> 8) + (humid & 0x00FF) + (humid >> 8)) & 0x00FF;
    if (check)
    {
        ESP_LOGE(MTAG, "Sensor read corrupted.");
        return 2;
    }
    out->humidity = humid;
    out->temperature = temp;
    return 0;
}

/***************************************************************** 
 *                                                               * 
 *  Measurement task                                             * 
 *                                                               * 
 *****************************************************************/
void meas_task(void *pvParameter)
{
    static DhtRead_t dht;

    dht_setup();
    while (1)
    {
        vTaskDelay(9000 / portTICK_RATE_MS);
        if (!dht_scan(&dht))
        {
            node_data_t *nd = &node_data; // for the convenience

            ESP_LOGI(MTAG, "temp: %d.%d, humi %d.%d %%",
                     dht.temperature / 10, dht.temperature % 10,
                     dht.humidity / 10, dht.humidity % 10);
            xSemaphoreTake(mutex, portMAX_DELAY);
            int temp = dht.temperature;
            int humi = (dht.humidity + 5) / 10;
            if (temp != nd->temperature ||
                humi != nd->humidity)
            {

                nd->temperature = temp;
                nd->humidity = humi;
                nd->changed = 1;
            }
            xSemaphoreGive(mutex);
        }
    }
}

/*****************************************************************
 *                                                               *
 *  Task to send the data back to home every 10 seconds          *
 *                                                               *
 *****************************************************************/

int send_message(const char *payload)
{
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    int status = -1;
    int sock;
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(host_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(HOST_IP_PORT);

    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
        ESP_LOGE(WTAG, "Unable to create socket: errno %d", errno);
    else if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6)) != 0)
        ESP_LOGE(WTAG, "Socket unable to connect: errno %d", errno);
    else
        ESP_LOGI(WTAG, "Sending data");
    if (send(sock, payload, strlen(payload), 0) < 0)
        ESP_LOGE(WTAG, "Error occurred during sending: errno %d", errno);
    else
        status = 0; // all seems OK!

    vTaskDelay(500 / portTICK_PERIOD_MS);

    if (sock != -1)
    {
        ESP_LOGI(WTAG, "Closing the socket");
        shutdown(sock, 0);
        close(sock);
    }
    return status;
}

#define BUFLEN 200
static void comm_task(void *pvParameters)
{
    char payload[BUFLEN];
    int counter = 0;

    ESP_LOGI(MTAG, "Comm task: waiting for WiFi ...");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    while (1)
    {
        vTaskDelay(14500 / portTICK_PERIOD_MS);
        counter++;

        xSemaphoreTake(mutex, portMAX_DELAY);
        if (!node_data.changed && counter < 5)
            xSemaphoreGive(mutex); // nothing to report!
        else
        {
            node_data_t *nd = &node_data; // for the convenience
            int len = snprintf(payload, BUFLEN,
                               "hello tol103m-nov21\n"
                               "mac %02x:%02x:%02x:%02x:%02x:%02x\n"
                               "lat %ld\n"
                               "lon %ld\n"
                               "temperature %d\n"
                               "humidity %d\n",
                               mac_addr[0], mac_addr[1], mac_addr[2],
                               mac_addr[3], mac_addr[4], mac_addr[5],
                               nd->lat, nd->lon,
                               nd->temperature, nd->humidity);
            nd->changed = 0; // information forwarded (soon!)
            xSemaphoreGive(mutex);
            counter = 0;
            if (len >= BUFLEN)
            {
                ESP_LOGE(WTAG, "Send buffer too short!");
                payload[BUFLEN - 1] = '\0';
            }
            send_message(payload);
        }
    }
    vTaskDelete(NULL);
}

/*****************************************************************
 *                                                               *
 *  WiFi routines                                                *
 *                                                               *
 *****************************************************************/

void wifi_handler(void *arg, esp_event_base_t base, int32_t id, void *ev_data)
{
    static int retry_num = 0;

    if (base == WIFI_EVENT)
    {
        switch (id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGE(WTAG, "Got STA start");
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGE(WTAG, "Got STA_stop");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGE(WTAG, "Got STA_conn");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (++retry_num >= 5)
            {
                ESP_LOGE(WTAG, "Could not connect - reboot!");
                esp_restart(); // the lazy solution!
            }
            ESP_LOGE(WTAG, "Got disconnect -- trying to reconnect!");
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        default:
            break;
        }
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)ev_data;
        ESP_LOGI(WTAG, "Got IP addr:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        retry_num = 0;
    }
}

static void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate(); // notification for WiFi availability

    ESP_ERROR_CHECK(esp_netif_init());                // init the tcp stack
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // WiFi events go through this
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        wifi_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        wifi_handler,
                                                        NULL,
                                                        NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("Connecting to %s\n", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_connect());
    printf("Connected?!\n");
}

/*****************************************************************
 *                                                               *
 *  Main application                                             *
 *                                                               *
 *****************************************************************/
void app_main()
{
    /*  Setup the logging levels ( NONE, WARN or VERBOSE )  */
    esp_log_level_set("wifi", ESP_LOG_VERBOSE); // enable wifi logging
    esp_log_level_set("meas", ESP_LOG_VERBOSE); // enable measurement logging

    ESP_ERROR_CHECK(nvs_flash_init());   // initialize NVS (wrong place?!)
    mutex = xSemaphoreCreateMutex();     // to guard measurement variables
    wifi_init();                         // Start connection to AP
    esp_efuse_mac_get_default(mac_addr); // MAC is our identity for now!
    esp_read_mac(mac_addr, ESP_MAC_WIFI_STA);

    xTaskCreate(&meas_task, "meas", 2048, NULL, 2, NULL);
    xTaskCreate(&comm_task, "comm", 2048, NULL, 4, NULL);
}
