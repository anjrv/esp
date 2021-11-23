#include <string.h>
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy_poll.h"  // random numbers
#include "freertos/FreeRTOS.h"

static const char privkey_2048_buf[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpQIBAAKCAQEAwCRfvaYtuwSMidHuNqos2amnoOBTrAt4FNa4H6RS4jcxV5l0\n"
    "nqz/cWG37nCVqm8mb/x3X4K6mZ7Tb6ptPKJnNqHAqn6tvcVuJ0QroYCU4FZyW4xG\n"
    "YujBZ52cmvMC7Aecort4IX+5FSLs4pOdGsl4uYoXfUuFd05JbW1LF+BUElKfjLbN\n"
    "ODJqDbhMtHMR/gnycNwxG/Dd3goCo/A0kgqIBaco2FGGdzYHiYup/0ZCRWzJ+oBS\n"
    "70H9borNmc/3cKm3PMnsnwL92aPHO1xy/k5kzn91qe6e7vhSQModj9hNR+kYU9ai\n"
    "H8sCaBqWWMZSHh/Dkf/3XLsEMlwm9H0x41clqwIDAQABAoIBAQC46ki53BRmyBn5\n"
    "CXCYa25+jCgnS7069k66v2q0CRE7ZKK4C5eQni24kLLTZsajIBV98Rtxb/7lmVUj\n"
    "QoGeuS1cBo/FheTvnfJFF/Zll7mvrYKhWH7k8dwwPB9bgERvo2O7uXADzUfyb4pL\n"
    "BIVOIldtONkiXGw2RcGk7mo2sE440mB3BxtgfGs+on+D5fh9QL0T0Ix/9MvDUXaH\n"
    "REugB45gTG1C/dnc1wLr2+vAxxV0/rXl9Eq/8b+7AEp5ebatuYdyQVmoBdeNbJB2\n"
    "jlADgbnicnW4cKNJX44N+f707v29CfEml4Ct5TfxaS0e5jVOQHOMx+WvP3vU9uRX\n"
    "dD44ADYBAoGBAOMoiZxcTrJqOVMisL9BguZYr1x2In2aIJLQZ/JLRevNodUWXnfi\n"
    "ZZFIVbA/oR2gYBqndkXseNPKCmkpqgspGEWM16+39Q94yCev0OAsxYpEqmuPxNMY\n"
    "lPBU0bw2x6bKyWW5eq4TS/dDFRq8V4HiSZp93UHDpxTpq/2hSUlk/YCTAoGBANiJ\n"
    "rpwcms4g6FFYMtEZjSy92dV9aD19EmACki7oAVZPSn/8ONErBVNN8bDlwRuRnXXl\n"
    "otsNtc+2KmMqH8C9C/fPFSDPSa3gxrh4U9OwFSNi0Sy8nxkkb8GXQO668MDOZkvt\n"
    "lEsMeOpSed1AIyOqaKlZgwIBWmqXD8hfs6AZTq2JAoGBAOKWIQ4TFXzqcFv5Eoz0\n"
    "b/oNJH76Z9UZ1LwdnlIVu51G9NJ1Ca5T6jRNDwxLrA69Vp+/wz5kbvHzawcTREb1\n"
    "qvxVTSA+Qvg35o+P1K6IelM9wzCfrBlVP9uq/7raebRqOxJ5PWI+ZVwzUe3lSPSK\n"
    "IBk2y2k6HIPDwoewRkYrsqJ5AoGADRL7FPfxLOL1w/uUTeXCDWKFJYvF4HiNxHhD\n"
    "RUvC0OhGskWTVKXJU4dQQYMNztFS9Yxg8eL5CEqScpxXgHapo1PAjdOWOkZpGThC\n"
    "r3RhQlq0EIRvAsCdcs3kIMXuxWVw7cKxgnqFTnieXQqDVKL0YM1DyLo2MFtgm5OT\n"
    "r+X3RykCgYEAhqRl52pLHqKrxsPm7ufej0HsbwY/ektOXqrcKbxTUBGgWhHCKL93\n"
    "cnF2A5z9+J55piZNTv3xtB4LRlZk6jw+ArO1K8Rf6/biZioCqyKaTtH8S2dSN/mO\n"
    "H6EY0sdc+mqxxDB4p3PLsqWQLVTSfFqxSLw0uYtVEhUs2+t3OEPFswI=\n"
    "-----END RSA PRIVATE KEY-----\n";

static int myrand(void *rng_state, unsigned char *output, size_t len)
{
    size_t olen;
    return mbedtls_hardware_poll(rng_state, output, len, &olen);
}

static inline int32_t cpu_cyclecount(void) {
    int32_t ticks;
    __asm__ __volatile__("rsr %0,ccount":"=a" (ticks));
    return ticks;
}

static unsigned char orig_buf[2048 / 8];
static unsigned char encrypted_buf[2048 / 8];
static unsigned char decrypted_buf[2048 / 8];

int rsa_test( int generate_key)
{
    mbedtls_pk_context  clientkey;
    mbedtls_rsa_context rsa;
    int32_t t;
    int     res;
    
    //
    // init and setup the key
    //
    if ( generate_key )
    {
        int64_t tt;
        
        mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0); // MBEDTLS_RSA_PRIVATE, 0);
        printf( "RSA: generating 2048-bit key ...\n" );
        tt = esp_timer_get_time();  // in microseconds
        if (mbedtls_rsa_gen_key(&rsa, myrand, NULL, 2048, 65537))
        {
            printf( "failed\n" );
            mbedtls_rsa_free(&rsa);
            return -1;
        }
        tt = esp_timer_get_time() - tt;
        tt /= 1000;  // to ms
        tt /= 1000;  // to sec
        printf( "  - %ld sec\n", (long)tt );
    } else {
        t   = cpu_cyclecount();
        mbedtls_pk_init(&clientkey);
        if ( mbedtls_pk_parse_key(&clientkey, (const uint8_t *)privkey_2048_buf, sizeof(privkey_2048_buf), NULL, 0) ) 
        {
            printf( "pk_parse_key failed.\n" );
            mbedtls_rsa_free(&rsa);
            return -1;
        }        
        memcpy(&rsa, mbedtls_pk_rsa(clientkey), sizeof(mbedtls_rsa_context));
        t   = cpu_cyclecount() - t;
        printf( "RSA: parsing 2048-bit key       : %d ticks, %dus\n", (int)t, (int)(t/240) );
    }

    memset(orig_buf, 0xAA, sizeof(orig_buf));
    orig_buf[0] = 0; // Ensure that orig_buf is smaller than rsa.N

    t   = cpu_cyclecount();
    res = mbedtls_rsa_public(&rsa, orig_buf, encrypted_buf); // len(buf) == 2048/8 == 256
    t   = cpu_cyclecount() - t;
    printf( "RSA: encryption with public key : %d ticks, %dus\n", (int)t, (int)(t/240) );
    
    if ( res )
    {
        if (res == MBEDTLS_ERR_MPI_NOT_ACCEPTABLE + MBEDTLS_ERR_RSA_PUBLIC_FAILED) {
            printf( "no HW support for this key length\n");
        }
        else
            printf( "failed\n");
        mbedtls_rsa_free(&rsa);
        return -1;
    }    

    // myrand for blinding
    for(int i=0; i<2; i++) 
    {
        printf( "RSA: decryption with private key: " );
        t   = cpu_cyclecount();
        res = mbedtls_rsa_private(&rsa, i?myrand:0, NULL, encrypted_buf, decrypted_buf);
        t   = cpu_cyclecount() - t;
        if ( res )
            printf( "failed\n" );
        else
            printf( "%d ticks, %dus%s\n", (int)t, (int)(t/240), i?" (blinding)":"" );
    }
    printf( "  " );
    for( int i=0; i<32; i++ )
        printf( "%02x%s", decrypted_buf[i],  ((i&0xf)!=0xf)?" ":"\n  " );
    printf( "\n" );
    mbedtls_rsa_free(&rsa);
    return 0;
}
    
