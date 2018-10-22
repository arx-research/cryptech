#ifndef __STM32_CHACHA20_H
#define __STM32_CHACHA20_H

#include <stdint.h>

#define CHACHA20_MAX_BLOCK_COUNTER	0xffffffff
#define CHACHA20_NUM_WORDS              16
#define CHACHA20_BLOCK_SIZE		(CHACHA20_NUM_WORDS * 4)

struct cc20_state {
    uint32_t i[CHACHA20_NUM_WORDS];
};

extern void chacha20_prng_reseed(struct cc20_state *cc, uint32_t *entropy);
extern void chacha20_prng_block(struct cc20_state *cc, uint32_t block_counter, struct cc20_state *out);
extern int chacha20_prng_self_test();

#endif /* __STM32_CHACHA20_H */
