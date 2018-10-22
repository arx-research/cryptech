#include <stdio.h>

#include "cc20_prng.h"

int main()
{
    uint32_t i;
    i = chacha20_prng_self_test();
    printf("test result: %i (%s)\n", i, (i == 0)?"FAIL":"SUCCESS");
}
