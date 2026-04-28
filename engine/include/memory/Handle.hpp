//
// Created by etders on 24.03.2026.
//

#ifndef ARGUSENGINE_HANDLE_HPP
#define ARGUSENGINE_HANDLE_HPP

#include <cstdint>
#include <type_traits>

namespace ArgusEngine {
    namespace Memory {

        /*
            32 bit  chunk_idx
            16 bit  generation
            8 bit   element_idx
            8 bit   flags
        */
        struct Handle {
            uint32_t chunk_idx;
            uint16_t generation;
            uint8_t  element_idx;
            uint8_t  flags;

            constexpr Handle()
                : chunk_idx(0), generation(0), element_idx(0), flags(0) {}

            constexpr Handle(uint32_t c, uint16_t g, uint8_t e, uint8_t f = 0)
                : chunk_idx(c), generation(g), element_idx(e), flags(f) {}

            constexpr bool valid() const {
                return generation != 0;
            }

            bool operator==(const Handle& other) const = default;
        };

        static_assert(sizeof(Handle) == 8, "Handle must be have size 8 bytes!");
        static_assert(std::is_trivially_copyable_v<Handle>, "Handle must be POD-like");

        using Entity = uint64_t;

        static constexpr uint64_t INVALID_ENTITY = 0;
        static constexpr uint32_t ENTITY_INDEX_MASK = 0xFFFFFFFF;

        //struct Handle {
        //    /*
        //        32 bit  chunk_idx
        //        16 bit  generation
        //        8 bit   element_idx
        //        8 bit   flags
        //    */
        //
        //    uint64_t value = 0;
        //
        //    static constexpr uint64_t CHUNK_MASK = 0xFFFFFFFFull;
        //    static constexpr uint64_t GEN_MASK    = 0xFFFFull;
        //    static constexpr uint64_t ELEM_MASK   = 0xFFull;
        //    static constexpr uint64_t FLAGS_MASK  = 0xFFull;
        //
        //    static constexpr uint64_t GEN_SHIFT   = 32;
        //    static constexpr uint64_t ELEM_SHIFT  = 48;
        //    static constexpr uint64_t FLAGS_SHIFT = 56;
        //
        //    static constexpr uint64_t make_value(
        //        uint32_t chunk_idx,
        //        uint16_t generation,
        //        uint8_t element_idx,
        //        uint8_t flags
        //    ) {
        //        return
        //            (uint64_t(chunk_idx) & CHUNK_MASK) |
        //            (uint64_t(generation) << GEN_SHIFT) |
        //            (uint64_t(element_idx) << ELEM_SHIFT) |
        //            (uint64_t(flags) << FLAGS_SHIFT);
        //    }
        //
        //    constexpr uint32_t chunk() const {
        //        return static_cast<uint32_t>(value & CHUNK_MASK);
        //    }
        //
        //    constexpr uint16_t generation() const {
        //        return static_cast<uint16_t>((value >> GEN_SHIFT) & GEN_MASK);
        //    }
        //
        //    constexpr uint8_t element() const {
        //        return static_cast<uint8_t>((value >> ELEM_SHIFT) & ELEM_MASK);
        //    }
        //
        //    constexpr uint8_t flags() const {
        //        return static_cast<uint8_t>((value >> FLAGS_SHIFT) & FLAGS_MASK);
        //    }
        //
        //    constexpr bool is_valid() const {
        //        return value != 0;
        //    }
        //};
    } // namespace Memory
} // namespace ArgusEngine

#endif