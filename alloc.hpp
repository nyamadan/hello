#pragma once

#if defined(_MSC_VER) || defined(__MINGW32__)
#define aligned_alloc _aligned_malloc
#define aligned_free _aligned_free
#endif