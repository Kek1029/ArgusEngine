//
// Created by etders on 26.03.2026.
//

#ifndef ARGUSENGINE_UTILS_HPP
#define ARGUSENGINE_UTILS_HPP

#include <tuple>
#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <cstdint>

constexpr size_t align_up(size_t offset, size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

// Layout
template<typename... Ts>
struct Layout {
    static constexpr size_t count = sizeof...(Ts);

    static constexpr std::array<size_t, count> offsets = [] {
        std::array<size_t, count> offs{};
        size_t offset = 0;
        size_t i = 0;

        ((offset = align_up(offset, alignof(Ts)),
          offs[i++] = offset,
          offset += sizeof(Ts)), ...);

        return offs;
    }();

    static constexpr size_t total_size = [] {
        size_t offset = 0;
        ((offset = align_up(offset, alignof(Ts)),
          offset += sizeof(Ts)), ...);

        return offset;
    }();
};

template<typename... Ts>
inline constexpr bool all_trivially_copyable_v =
    (std::is_trivially_copyable_v<Ts> && ...);

#define MAKE_COMPONENT(name, ...) \
struct Component_##name { \
    using Types = std::tuple<__VA_ARGS__>; \
    using L = Layout<__VA_ARGS__>; \
\
    static_assert(all_trivially_copyable_v<__VA_ARGS__>, \
        "Components must be trivially copyable"); \
\
    alignas(16) unsigned char data[L::total_size]; \
\
    template<typename... Args> \
    static Component_##name Make(Args... args) { \
        static_assert(sizeof...(Args) == std::tuple_size_v<Types>); \
        Component_##name c; \
        size_t i = 0; \
        ([&](auto&& val) {\
        using T = std::decay_t<decltype(val)>; \
        std::memcpy(c.data + L::offsets[i++], &val, sizeof(T));\
            }(std::forward<Args>(args)), ...);\
        return c;\
    } \
\
    template<size_t Index> \
    auto& Get() { \
        using T = std::tuple_element_t<Index, Types>; \
        return *std::launder(reinterpret_cast<T*>(data + L::offsets[Index])); \
    } \
\
    template<size_t Index> \
    const auto& Get() const { \
        using T = std::tuple_element_t<Index, Types>; \
        return *std::launder(reinterpret_cast<const T*>(data + L::offsets[Index])); \
    } \
template<size_t Index, typename T> \
void Set(T&& value) { \
using TargetT = std::tuple_element_t<Index, Types>; \
static_assert(std::is_assignable_v<TargetT&, T>, \
"Type mismatch in Component::Set"); \
*std::launder(reinterpret_cast<TargetT*>(data + L::offsets[Index])) = std::forward<T>(value); \
} \
};

#define MAKE_EVENT(name, ...) \
struct Event_##name { \
using Types = std::tuple<__VA_ARGS__>; \
using L = Layout<__VA_ARGS__>; \
static_assert(all_trivially_copyable_v<__VA_ARGS__>, "Events must be trivially copyable"); \
\
uint8_t data[L::total_size]; \
\
template<size_t Index> \
auto& get() { \
using T = std::tuple_element_t<Index, Types>; \
return *std::launder(reinterpret_cast<T*>(data + L::offsets[Index])); \
} \
};

template <typename T, typename Tuple>
struct index_of;

template <typename T, typename... Rest>
struct index_of<T, std::tuple<T, Rest...>> {
    static constexpr std::size_t value = 0;
};

template <typename T, typename Head, typename... Rest>
struct index_of<T, std::tuple<Head, Rest...>> {
    static constexpr std::size_t value = 1 + index_of<T, std::tuple<Rest...>>::value;
};

template <typename T>
struct index_of<T, std::tuple<>> {
    static_assert(sizeof(T) == 0, "Type not found in component list!");
};

inline constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
inline constexpr uint64_t FNV_PRIME = 0x100000001b3;

#include <string>

template <typename T>
uint64_t hash_fnv1a(const T& value) {
    if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
        uint64_t hash = FNV_OFFSET_BASIS;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        for (size_t i = 0; i < sizeof(T); ++i) {
            hash ^= bytes[i];
            hash *= FNV_PRIME;
        }
        return hash;
    } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
        uint64_t hash = FNV_OFFSET_BASIS;
        for (char c : value) {
            hash ^= static_cast<uint8_t>(c);
            hash *= FNV_PRIME;
        }
        return hash;
    } else {
        static_assert(sizeof(T) == 0, "Unsupported hash type!");
    }
}

#include <cstdint>

namespace ArgusEngine {

    class TypeRegistry {
    private:
        static inline uint32_t nextId = 0;
    public:
        template<typename T>
        static uint32_t get_id() {
            static uint32_t id = nextId++;
            return id;
        }
    };

} // namespace ArgusEngine

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

#endif //ARGUSENGINE_UTILS_HPP