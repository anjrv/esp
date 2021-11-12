#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/******************************************************************/
#include "driver/gpio.h"
#define LED_PIN 2
/******************************************************************/
#include <stdint.h>

#define INT_LEN 10
#define BLEN 30 // word size - 2
#define MASK ((((uint32_t)1) << BLEN) - 1)

uint32_t n[INT_LEN];
int len;

//#undef __GNUC__

void f3n1(void)
{
    uint32_t r, c = 1;

    for (int i = 0; i < len; i++)
    {
        r = n[i] + (n[i] << 1) + c;
        c = r >> BLEN;
        n[i] = r & MASK;
    }
    if (c)
    {
        len++;
        if (len > INT_LEN)
        {
            fprintf(stderr, "Too large integer - aborting.\n");
            exit(-1);
        }
        n[len - 1] = c;
    }
}

void fdiv2(void)
{
    // shift whole integers
#if INT_LEN > 1
    if (n[0] == 0)
    {
        int i, k = 1;
        while (n[k] == 0)
            k++;
        i = 0;
        while (k < len)
            n[i++] = n[k++];
        len = i;
    }
#endif

    // shift the remaining bits
    {
        int k;
#if defined(__GNUC__)
        k = __builtin_ctzl(n[0]); // here uint32_t so long
#else
        k = (n[0] & 1) ^ 1;
#endif
        if (k)
        {
#if !defined(__GNUC__)
            uint32_t n0 = n[0] >> 1;
            while (!(n0 & 1))
            {
                n0 = n0 >> 1;
                k++;
            }
#endif
            // k lowest bits are zero, shift them out!
#if INT_LEN > 1
            int p = BLEN - k;
            for (int i = 1; i < len; i++)
                n[i - 1] = (n[i] << p & MASK) | (n[i - 1] >> k);
#endif
            n[len - 1] = (n[len - 1] >> k);

            while (!n[len - 1])
                len--;
        }
    }
}

/******************************************************************/

void main_task(void *pvParameter)
{
    printf("Hello TÖL301G!\n");

    while (1)
    {
        uint64_t t0;
        unsigned long count = 0;
        unsigned long i, x;

        x = 0;
        gpio_set_level(LED_PIN, x);
        vTaskDelay(5000 / portTICK_RATE_MS); // cool down!

        printf("Computing...!\n");
        t0 = esp_timer_get_time();

        for (i = 3; i <= 0xfff; i += 2)
        {
            n[0] = i;
            len = 1;

            if (((i >> 4) & 1) != x)
            {
                x ^= 1;
                gpio_set_level(LED_PIN, x);
            }

            while (n[0] != 1 || len > 1)
            {
                f3n1();
                fdiv2();
                count++;
            }
            taskYIELD();
        }
        t0 = esp_timer_get_time() - t0;
        {
            int sec, ms;
            sec = t0 / 1000000ul;
            t0 = t0 % 1000000ul;
            ms = t0 / 1000;
            printf("Numbers [2,%lu] verified, %lu rounds, "
                   "running time %d.%d sec\n",
                   i - 1, count, sec, ms);
        }
    }
}

void app_main()
{
#if defined(LED_PIN)
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
#endif

    // start the main task
    xTaskCreate(
        &main_task,  // - function ptr
        "main_task", // - arbitrary name
        2048,        // - stack size [byte]
        NULL,        // - optional data for task
        0,           // - priority (LOW)
        NULL);       // - handle to task (for control)
}
