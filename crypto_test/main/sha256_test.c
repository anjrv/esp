#include <stdio.h>
#include <stdint.h>

#include "mbedtls/sha256.h"

mbedtls_sha256_context sha256_ctx;

static inline int32_t cpu_cyclecount(void) {
    int32_t ticks;
    __asm__ __volatile__("rsr %0,ccount":"=a" (ticks));
    return ticks;
}

#define TEST_LEN 16384
static unsigned char  test_str_a[TEST_LEN];
static unsigned char  test_str_b[3] = {'a','b','c'};

static unsigned char  sha256[32];  // the result

int sha256_test(void) 
{
    int try_it(int len)
    {
        int32_t t;
        t = cpu_cyclecount();        
        mbedtls_sha256_init(&sha256_ctx);
        if ( mbedtls_sha256_starts_ret(&sha256_ctx, 0) )
        {
            mbedtls_sha256_free(&sha256_ctx);
            return -1;
        }
        
        if ( len==3 ) 
        {
            if ( mbedtls_sha256_update_ret(&sha256_ctx, test_str_b, 3 ) )
            {
                mbedtls_sha256_free(&sha256_ctx);
                return -2;
            }
        }
        else
        {
            if ( mbedtls_sha256_update_ret(&sha256_ctx, test_str_a, len ) )
            {
                mbedtls_sha256_free(&sha256_ctx);
                return -2;
            }
        }
        
        if ( mbedtls_sha256_finish_ret(&sha256_ctx, sha256) )
        {
            mbedtls_sha256_free(&sha256_ctx);
            return -3;
        }
        mbedtls_sha256_free(&sha256_ctx);
        t = cpu_cyclecount() - t;
        {
            float dt = t / 240.0;
            printf( "SHA-256: " );
            for(int i=0; i<32; i++)
                printf( "%02x", sha256[i] );
            printf( " %7.2fus (%6.3fMB/s), input %5d bytes\n", dt, len / dt, len );
        }
        return 0;
    }

    for(int i=0; i<TEST_LEN; i++)
        test_str_a[i] = i & 0xff;
    
    if ( try_it( 3 ) || try_it( 256 ) || try_it( 3 ) || try_it( TEST_LEN ) )
        printf( "Error with SHA-256 computation\n" );
    else
        printf("\n");
    return 0;
}
