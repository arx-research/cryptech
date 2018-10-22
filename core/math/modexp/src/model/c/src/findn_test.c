#include <stdio.h>
#include <stdint.h>

uint32_t findN(uint32_t length, uint32_t *E) {
       uint32_t n = -1;
       for (int i = 0; i < 32 * length; i++) {
               uint32_t ei_ = E[length - 1 - (i / 32)];
               uint32_t ei = (ei_ >> (i % 32)) & 1;

               printf("ei_ = 0x%08x, ei = 0x%08x\n", ei_, ei);
               if (ei == 1) {
                       n = i;
               }
       }
       return n + 1;
}


int main(void) {
  uint32_t my_e[4] = {0x5a00aaaa, 0x555555, 0x80808080, 0x01010101};

  printf("Result: %08d\n", findN(1, &my_e[0]));

  return 0;
}
