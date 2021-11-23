
#include <stdio.h>
#include <string.h>       // memset and memcpy
#include <aes/esp_aes.h>

static inline int32_t cpu_cyclecount(void) {
    int32_t ticks;
    __asm__ __volatile__("rsr %0,ccount":"=a" (ticks));
    return ticks;
}

char plaintext[16384];  // multiple of 128 bits == 16 bytes
char encrypted[16384];

int aes_test(void)
{
    esp_aes_context ctx;
    uint8_t key[32];
    uint8_t iv[16];
    int32_t t0,t1;
    float   dt;
    
    memset( iv,  0, sizeof( iv  ) ); // "random" initialization vector
    memset( key, 0, sizeof( key ) ); // and the 256bit secret key

    memset( plaintext, 0, sizeof( plaintext ) );
    strcpy( plaintext, "Hello TOL103M!" );

    esp_aes_init( &ctx );             // works always :)
    esp_aes_setkey( &ctx, key, 256 ); // 128, 192 or 256 key bits, 0=success
    t0 = cpu_cyclecount();
    esp_aes_crypt_cbc( &ctx, ESP_AES_ENCRYPT, sizeof(plaintext), iv, (uint8_t*)plaintext, (uint8_t*)encrypted );
    t1 = cpu_cyclecount();
    dt = (t1-t0)/240.0;
    printf( "AES encryption time: %.2fus  (%.2f MB/s)\n", dt, (sizeof(plaintext)*1.0)/dt );

    memset( plaintext, 0, sizeof( plaintext ) );
    memset( iv, 0, sizeof( iv ) );  

    t0 = cpu_cyclecount();
    esp_aes_crypt_cbc( &ctx, ESP_AES_DECRYPT, sizeof(encrypted), iv, (uint8_t*)encrypted, (uint8_t*)plaintext );
    t1 = cpu_cyclecount();
    dt = (t1-t0)/240.0;
    printf( "AES decryption time: %.2fus  (%.2f MB/s)\n", dt, (sizeof(plaintext)*1.0)/dt );

    // Visual verification of the output
    printf( "  " );
    for( int i=0; i<64; i++ )
        printf( "%02x[%c]%s", plaintext[i],  (plaintext[i]>31)?plaintext[i]:' ', ((i&0xf)!=0xf)?" ":"\n  " );
    printf( "\n" );
    esp_aes_free( &ctx );
    return 0;
}
