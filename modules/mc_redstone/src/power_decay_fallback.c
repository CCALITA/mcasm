/*
 * power_decay_fallback.c -- C fallback for power_decay.asm functions.
 *
 * On x86_64, the NASM version (power_decay.asm) is used instead.
 * This file compiles on all architectures and provides identical behavior.
 */

#include <stdint.h>

#ifdef __x86_64__
/* On x86_64 the ASM version is linked; avoid duplicate symbols. */
#else

uint8_t mc_redstone_decay_power(uint8_t current_power, uint8_t hops)
{
    int result = (int)current_power - (int)hops;
    return (result < 0) ? 0 : (uint8_t)result;
}

uint8_t mc_redstone_max_neighbor_power(const uint8_t *powers, uint32_t count)
{
    uint8_t max_val = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (powers[i] > max_val) {
            max_val = powers[i];
        }
    }
    return max_val;
}

#endif /* !__x86_64__ */
