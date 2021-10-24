#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/**
 * Initializes random number seed with system time
 */
void initialize_noise()
{
    srand(time(NULL));
}

/**
 * Return a random integer up to lim
 * 
 * @param lim the bounds of the random number
 * @return a pseudo-random integer between -lim and lim
 */
int random_int(int lim)
{
    return (rand() % lim) * (-1 * (rand() % 1));
}

/**
 * Generate a "response" line of noise with three whitespace separated integers
 * 
 * This is the firmware NOISE datasource
 * @param buf memory allocated character array to write to
 */
void noise(char* buf)
{
    char noise_buf[50];
    snprintf(noise_buf, sizeof(noise_buf), "%d %d %d", random_int(RAND_MAX), random_int(RAND_MAX), random_int(RAND_MAX));

    strcpy(buf, noise_buf);
}