//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "util.h"

//////////////////////////////////////////////////////////////////////

static int random_seed = 0;

//////////////////////////////////////////////////////////////////////
// get next random in sequence and maintain seed
// this is a very lame prng but just about fit for purpose to flash
// some colored leds

int random_next(int *seed)
{
    int s = *seed;
    s = s * 1103515245 + 12345;
    *seed = s;
    return (s >> 16) & 0xffff;
}

//////////////////////////////////////////////////////////////////////
// this one throws the seed away

int random_from(int seed)
{
    return ((seed * 1103515245 + 12345) >> 16) & 0xffff;
}

//////////////////////////////////////////////////////////////////////
// if you don't set the seed, this will give you the same set
// of numbers each time

int random()
{
    return random_next(&random_seed);
}
