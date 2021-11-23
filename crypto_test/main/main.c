#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// prototypes
int aes_test(void);  // aes_test.c
int rsa_test( int generate_key);
int sha256_test( void );

// Main task
void main_task(void *pvParameter)
{
    while(1) {
        printf("\nCryptoblock performance test!  (TOL103M - Fall 2021)\n\n");
        vTaskDelay(2000 / portTICK_RATE_MS);
        sha256_test( );
        vTaskDelay(2000 / portTICK_RATE_MS);
        aes_test();
        vTaskDelay(2000 / portTICK_RATE_MS);
        rsa_test( 0 );
        vTaskDelay(10000 / portTICK_RATE_MS);
    }
}


// Main application
void app_main()
{
    // start the main task
    xTaskCreate(
        &main_task,  // - function ptr
        "main_task", // - arbitrary name
        8192,        // - stack size [byte]
        NULL,        // - optional data for task
        0,           // - priority, LOW as RSA keys take time
        NULL);       // - handle to task (for control)
}
