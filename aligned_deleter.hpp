#pragma once

#ifdef _WIN32
#define aligned_alloc _aligned_malloc
#define aligned_free _aligned_free
#else
#define aligned_free free
#endif

template <typename T = void>
struct AlignedDeleter {
    constexpr AlignedDeleter() noexcept = default;

    template <typename U,
              typename std::enable_if<std::is_convertible<U *, T *>::value,
                                      std::nullptr_t>::type = nullptr>
    AlignedDeleter(const AlignedDeleter<U> &) noexcept {}

    void operator()(T *ptr) const { aligned_free(ptr); }
};

template <typename T>
struct AlignedDeleter<T[]> {
    constexpr AlignedDeleter() noexcept = default;

    template <typename U, typename std::enable_if<
                              std::is_convertible<U (*)[], T (*)[]>::value,
                              std::nullptr_t>::type = nullptr>
    AlignedDeleter(const AlignedDeleter<U[]> &) noexcept {}

    void operator()(T *ptr) const { aligned_free(ptr); }
};
