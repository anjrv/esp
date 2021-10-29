/*
 * An elementary client for the 'Factoring Contest'
 *
 * v0.0  2021-07-08  Esa
 */
#include <stdio.h>
#include <string.h>  // memcpy
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_log.h"

/*********************************************************************/
/*  HW random numbers                                                */
/*********************************************************************/
# define DR_REG_RNG_BASE   0x3ff75144    

uint32_t hw_random32( void ) {
    return (uint32_t)READ_PERI_REG( DR_REG_RNG_BASE );
}

/*********************************************************************/
/*  Globals                                                          */
/*********************************************************************/
uint8_t broadcast[] = {0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF};

/*********************************************************************/
/*  Elementary integer arithmetic                                    */
/*********************************************************************/

uint64_t sqrt64(uint64_t n)  // Works for all 0 < n < 2^64
{
    uint64_t x0,x1 = (n>>32) + (((uint64_t)1)<<30);
    do 
    {
        x0 = x1;
        x1 = (x0 + n/x0)/2;
    }
    while (x0>x1);
    return x0;
}

/*********************************************************************/
/*  Computing job: factor a 'big' number                             */
/*********************************************************************/
uint64_t puzzle_ab = 0;  // the computing job
char     sbuf[256];      // send buffer

void factor_it(void) 
{
    uint64_t ab    = puzzle_ab;
    uint64_t n1    = sqrt64( ab );

    while( 1 ) 
    {
        uint32_t count = 500000;               // "block size"
        uint64_t n0    = hw_random32( ) % n1;  // random start
        uint64_t n;
        
        n0 = n0 < 2 ? 2 : n0;

        for( n=n0; n<=n1; n++)
        {
            if ( !(ab % n) )
            {
                if ( ab==puzzle_ab ) 
                {
                    uint64_t m = ab / n;
                    printf("Factor found! %llu x %llu = %llu\n",
                           (unsigned long long)n,
                           (unsigned long long)m,
                           (unsigned long long)ab );
                    sprintf( sbuf, "%llu", (unsigned long long)n );
                    if ( esp_now_send(broadcast, (uint8_t *)sbuf, strlen(sbuf) ) != ESP_OK )
                        printf("Error sending the data\n");
                    puzzle_ab = 0;
                    return;
                }
                printf("Factor found too late!\n" );
                return;  // we are done for now!
            }
            if ( (--count)==0 )
                break;
        }

        vTaskDelay( 10 );     // feed the watchdog and have a break        
        printf("checked [%llu ... %llu]  max=%llu\n",
               (unsigned long long)n0,
               (unsigned long long)n,
               (unsigned long long)n1 );
    }
    // never reached?!
}

/*********************************************************************/
/*  Main task                                                        */
/*********************************************************************/

void main_task(void *pvParameter)
{
    printf("Main task started - Waiting for a factoring task!\n");
    while(1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
        if ( puzzle_ab > 3 )
            factor_it();
    }
}


/*************************************************************/

uint64_t parse_int(const char *buf, int len)
{
    uint64_t x=0;
    for(int i=0; i<len; i++)
    {
        if ( buf[i]<'0' || buf[i]>'9' )
            break;
        x = 10*x + (int)(buf[i] - '0');
    }
    return x;
}

void espnow_onreceive(const uint8_t *mac, const uint8_t *data, int len) {
    const char *buf = (char *)data;
    
    printf("espnow from MAC %02x:%02x:%02x:%02x:%02x:%02x: ",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if ( len > 7 && !strncmp(buf, "factor ", 7 ) ) 
    {
        uint64_t ab = parse_int( buf+7, len-7 );
        if ( ab==puzzle_ab )
            printf( " the old puzzle re-announced\n");
        else
        {
            puzzle_ab = ab;
            printf( " a new puzzle received: %llu\n", (unsigned long long)puzzle_ab );
        }
    }    
}

/*********************************************************************/
/*  WiFi and ESP-Now setup routines (from docs)                      */
/*********************************************************************/

esp_now_peer_info_t peerInfo;

void espnow_init(void)
{
    if (esp_now_init() != ESP_OK) {
        printf("Error initializing ESP-NOW\n");
        return;
    }
    esp_now_register_recv_cb(espnow_onreceive);
    //ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    //  register the broadcast address
    memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
    memcpy(peerInfo.peer_addr, broadcast, 6);
    peerInfo.channel = 0;  
    peerInfo.ifidx   = ESP_IF_WIFI_STA;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        printf("Failed to add peer\n");
        return;
    }
}


static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}


/*********************************************************************/
/*  Main application                                                 */
/*********************************************************************/

void app_main()
{
    esp_log_level_set("wifi", ESP_LOG_NONE);  // disable the default wifi logging
    ESP_ERROR_CHECK(nvs_flash_init());        // initialize NVS
    wifi_init();                              // init wifi
    espnow_init();                            // ... and espnow

    // start the main task
    xTaskCreate(
        &main_task,  // - function ptr
        "main_task", // - arbitrary name
        2048,        // - stack size [byte]
        NULL,        // - optional data for task
        5,           // - priority
        NULL);       // - handle to task (for control)

    // xTaskCreatePinnedToCore( ... )
    // vTaskDelete( handle );
}
