#include <stdlib.h>

#pragma once
#if defined(_MSC_VER)
#define aligned_alloc _aligned_malloc
#define aligned_free _aligned_free
#endif