//
// Created by etders on 24.03.2026.
//

#ifndef ARGUSENGINE_CHUNK_HPP
#define ARGUSENGINE_CHUNK_HPP

#include "Handle.hpp"
#include <cstdint>
#include <new>
#include <utility>
#include <bit>
#include <cstring>

namespace ArgusEngine {
    namespace Memory {
        template <typename T>
            struct Chunk {
            static constexpr uint64_t FULL = ~0ULL;

            alignas(T) std::byte data[64 * sizeof(T)];
            uint64_t bitmask = 0;
            uint8_t generations[64] = {};

            T* ptr(uint8_t i) {
                return std::launder(reinterpret_cast<T*>(data + i * sizeof(T)));
            }

            const T* ptr(uint8_t i) const {
                return std::launder(reinterpret_cast<const T*>(data + i * sizeof(T)));
            }

            template <typename... Args>
            std::pair<uint8_t, uint8_t> alloc(Args&&... args) {
                if (bitmask == FULL) [[unlikely]]
                    return {0, 0};

                uint8_t idx = std::countr_zero(~bitmask);

                if (++generations[idx] == 0) [[unlikely]]
                    generations[idx] = 1; // 0 зарезервирован под невалидный индекс

                new (ptr(idx)) T(std::forward<Args>(args)...);

                bitmask |= (1ULL << idx);

                return {uint8_t(idx + 1), generations[idx]};
            }

            void free(uint8_t idx) {
                if (idx == 0) [[unlikely]] return;
                uint8_t i = idx - 1;

                if (!(bitmask & (1ULL << i))) [[unlikely]] return;

                if constexpr (!std::is_trivially_destructible_v<T>) {
                    ptr(i)->~T();
                }
                bitmask &= ~(1ULL << i);
                generations[i]++;
            }

            T* get(uint8_t idx, uint8_t gen) {
                if (idx == 0) [[unlikely]] return nullptr;

                uint8_t i = idx - 1;

                if (!(bitmask & (1ULL << i)) || generations[i] != gen)
                    return nullptr;


                return ptr(i);
            }

            const T* get(uint8_t idx, uint8_t gen) const {
                if (idx == 0) [[unlikely]] return nullptr;
                uint8_t i = idx - 1;
                if (!(bitmask & (1ULL << i)) || generations[i] != gen) [[unlikely]]
                    return nullptr;
                return ptr(i);
            }

            std::pair<uint8_t, uint16_t> reserve_empty() {
                uint8_t idx = static_cast<uint8_t>(std::countr_one(bitmask)) + 1;
                if (idx > 64) return {0, 0};

                uint64_t mask = 1ULL << (idx - 1);
                bitmask |= mask;
                bitmask &= ~mask;

                generations[idx - 1]++;
                return {idx, generations[idx - 1]};
            }

            void reset() {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    uint64_t temp_mask = bitmask;
                    while (temp_mask) {
                        uint8_t i = (uint8_t)std::countr_zero(temp_mask);
                        ptr(i)->~T();
                        temp_mask &= ~(1ULL << i);
                    }
                }
                bitmask = 0;
                for (auto& gen : generations) {
                    gen++;
                }
            }

            bool is_full() const {
                return bitmask == FULL;
            }
        };
    }
}

#endif //ARGUSENGINE_CHUNK_HPP