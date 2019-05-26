#include <stdint.h>

struct xorshift128plus_state {
    uint64_t a, b;
};

uint64_t xorshift128plus(struct xorshift128plus_state *state);
