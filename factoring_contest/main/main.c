/*
 * A host program for the 'Factoring Contest'
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

#define no_EASY_PROBLEMS

/*********************************************************************/
/*  Globals                                                          */
/*********************************************************************/
uint8_t broadcast[] = {0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF};

/*********************************************************************/
/*  Hardware-based random numbers in ESP32                           */
/*********************************************************************/
#define DR_REG_RNG_BASE  0x3ff75144

uint32_t hw_random32( void ) {
    return (uint32_t)READ_PERI_REG( DR_REG_RNG_BASE );
}

/*********************************************************************/
/*  Elementary integer arithmetic                                    */
/*********************************************************************/

// Works for all 0 < n < 2^32
uint32_t sqrt32(uint32_t n)
{
    uint32_t x0,x1 = (n>>16) + (uint32_t)(1<<14);
    do 
    {
        x0 = x1;
        x1 = (x0 + n/x0)/2;
    }
    while (x0>x1);
    return x0;
}

int is_prime(uint32_t n)
{
    if (  n < 2 )    return -1; // silly question
    if (  n < 4 )    return  0; // 2 and 3
    if ( !(n & 1) )  return  1; // even => composite
    {
        uint32_t x,x1 = sqrt32(n);
        for( x=3; x<=x1; x+=2 )
            if ( !(n % x) )
                return 1;
        return 0;
    }
}

/*********************************************************************/
/*  Our puzzle!                                                      */
/*********************************************************************/
uint64_t puzzle_p1;
uint64_t puzzle_p2;
uint64_t puzzle_ab;
int      puzzle_solved;

void new_puzzle(void)
{
    uint32_t random_prime(void) 
    {
        uint32_t r = hw_random32() | (((uint32_t)1)<<31);
#if defined(EASY_PROBLEMS)
        r = r >> 6;
#endif
        while( is_prime(r) )
            r--;
        return r;
    }
    puzzle_p1     = random_prime();
    puzzle_p2     = random_prime();
    puzzle_ab     = puzzle_p1 * puzzle_p2;
    puzzle_solved = 0;

    printf("New puzzle %llu x %llu =  %llu\n",
           (unsigned long long) puzzle_p1,
           (unsigned long long) puzzle_p2,
           (unsigned long long) puzzle_ab);
    fflush(stdout);
}

/*********************************************************************/
/*  Main task                                                        */
/*********************************************************************/

void main_task(void *pvParameter)
{
    char      buf[256];
    int       count = 0;
    
    printf("Main task started -- Generating a factoring contest soon!\n");
    fflush(stdout);
    while(1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
        
        if ( count && !puzzle_solved )
            count--;
        else 
        {
            count = 60;  // give about 5 minutes
            new_puzzle();
        }

        /* inform everyone about the current challenge! */
        sprintf( buf, "factor %llu", puzzle_ab );
        if ( esp_now_send(broadcast, (uint8_t *)buf, strlen(buf) ) != ESP_OK ) {
            printf("Error sending the data\n");
            fflush(stdout);
        }
    }
}


/*********************************************************************/
/*  WiFi and ESP-Now routines                                        */
/*********************************************************************/
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

uint64_t parse_int(const char *buf, int len)
{
    uint64_t x=0;
    for(int i=0; i<len; i++)
    {
        if ( buf[i]<'0' || buf[i]>'9'  )
            break;
        x = 10*x + (int)(buf[i] - '0');
    }
    return x;
}

void espnow_onreceive(const uint8_t *mac, const uint8_t *data, int len) {
    printf( "espnow from MAC: %02x:%02x:%02x:%02x:%02x:%02x len=%d\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len );
    fflush(stdout);
    uint64_t p = parse_int((const char*)data, len);

    if ( (p==puzzle_p1 || p==puzzle_p2) && !puzzle_solved )
    {
        puzzle_solved = 1;
        printf( " - puzzle solved!\n" );
        fflush(stdout);
    }    
}

esp_now_peer_info_t peerInfo;

void espnow_init(void)
{
    if (esp_now_init() != ESP_OK) {
        printf("Error initializing ESP-NOW\n");
        fflush(stdout);
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
        fflush(stdout);
        return;
    }
}

/*********************************************************************/
/*  Main application                                                 */
/*********************************************************************/
void app_main()
{
    esp_log_level_set("wifi", ESP_LOG_NONE);  // disable the default wifi logging    
    ESP_ERROR_CHECK(nvs_flash_init());        // initialize NVS
    wifi_init();                              // init wifi
    espnow_init();                            // init espnow

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
