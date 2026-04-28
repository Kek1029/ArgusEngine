//
// Created by etders on 14.04.2026.
//

#ifndef ARGUSENGINE_FASTINDEXLINEARALLOCATOR_HPP
#define ARGUSENGINE_FASTINDEXLINEARALLOCATOR_HPP
#include "VirtualLinearAllocator.hpp"

namespace ArgusEngine {
    namespace Memory {
        template <typename T>
        class [[deprecated]] FastIndexLinearAllocator {
            struct BulkInstance {
                T data[16];
                uint16_t bitmask;
            };

        public:
            VirtualLinearAllocator<T> data;

        public:
            void bulk_set(const BulkInstance& instance, uint32_t offset) {
                uint16_t m = instance.bitmask;
                while (m) {
                    uint32_t i = __builtin_ctz(m);
                    data[offset + i] = instance.data[i];
                    m &= (m - 1);
                }
            }
        };

    } // Memory
} // ArgusEngine

#endif //ARGUSENGINE_FASTINDEXLINEARALLOCATOR_HPP
