#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TASKS  5          // define the number of tasks (>=1)

#define HIGH_PRIORITY  4  // assumed high priority task
#define LOW_PRIORITY   1  // non-idling background tasks
#define MAIN_PRIORITY  0  // the main task running the show

void bg_task(void *pvParameter)
{
    //uint32_t p1 = 2132500393UL;  /* prime p1 */
    //uint32_t p2 = 2132500423UL;  /* prime p2 */
    uint64_t ab = 4547557990120166239ULL; /* product of p1 and p2 */
    
    while(1) {
        int i  = 0;
        for(uint64_t a=3; a<1ULL<<32; a+=2)
        {
            if ( !(ab % a) )
            {
                printf( "Found it!  %llu\n", a );
                break;
            }
            if ( (a & 0x7fffffUL)==1 )
            {
                i++;
                if ( i>5 )
                {
                    printf( " ... working on it!\n" );
                    i = 0;
                }
                taskYIELD();  // next please!
            }
        }
    }
}

/*  Let's check what we can "guarantee"! */
uint64_t min_delay;
uint64_t max_delay;
int      mea_done;

/*
 *   Sample high priority task
 */
void meas_task(void *pvParameter)
{
    min_delay = 0;
    max_delay = 0;
    mea_done  = 0;
    
    for(int i=0; i<100; i++)
    {
        uint64_t t = esp_timer_get_time();
        vTaskDelay(100 / portTICK_PERIOD_MS); // ask to sleep for 100ms?!
        t = esp_timer_get_time() - t;
        t = t > 0 ? t : 1;
        if ( !min_delay || min_delay > t )
            min_delay = t;
        if ( !max_delay || max_delay < t )
            max_delay = t;
    }
    mea_done = 1;
    while( 1 )
        vTaskDelay(2000 / portTICK_PERIOD_MS);
}

// Main task
void main_task(void *pvParameter)
{
    TaskHandle_t th[ TASKS ];
    printf("main task started!\n");
    while(1)
    {
        int i;
        
        vTaskDelay(1000 / portTICK_RATE_MS);

        // start the background tasks
        for(i=0; i<TASKS-1; i++) 
        {
            xTaskCreatePinnedToCore(
                &bg_task,      // - function ptr
                "bg_task",     // - arbitrary name
                2048,          // - stack size [byte]
                NULL,          // - optional data for task
                LOW_PRIORITY,  // - background jobs
                &th[i],        // - handle to task (for control)
                i & 1 );       // - use both cores explicitly!
        }
        vTaskDelay(500 / portTICK_RATE_MS);
        
        // then start the measurement task
        xTaskCreatePinnedToCore(
            &meas_task,    // - function ptr
            "measure",     // - arbitrary name
            2048,          // - stack size [byte]
            NULL,          // - optional data for task
            HIGH_PRIORITY, // - high priority
            &th[i++],      // - handle to task (for control) 
            1);

        do
            vTaskDelay(500 / portTICK_RATE_MS);
        while (!mea_done);

        // cleanup
        while(i)
        {
            i--;
            vTaskDelete( th[i] );
        }
        
        printf( "----\n" );
        printf( "min: %3lu.%lu ms\n", (unsigned long)min_delay / 1000, (unsigned long)max_delay % 1000 );
        printf( "max: %3lu.%lu ms\n", (unsigned long)max_delay / 1000, (unsigned long)max_delay % 1000 );
    }
}


// The main
void app_main()
{
    xTaskCreate(
        &main_task,    // - function ptr
        "main_task",   // - arbitrary name
        2048,          // - stack size [byte]
        NULL,          // - optional data for task
        MAIN_PRIORITY, // - priority of the main task
        NULL );        // - handle to task (for control)
}
