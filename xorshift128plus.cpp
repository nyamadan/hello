#include "xorshift128plus.hpp"

/* The state must be seeded so that it is not all zero */
uint64_t xorshift128plus(struct xorshift128plus_state &state) {
    uint64_t t = state.a;
    uint64_t const s = state.b;
    state.a = s;
    t ^= t << 23;        // a
    t ^= t >> 17;        // b
    t ^= s ^ (s >> 26);  // c
    state.b = t;
    return t + s;
}

double xorshift128plus01(struct xorshift128plus_state &state) {
    return static_cast<double>(xorshift128plus(state)) / UINT64_MAX;
}
