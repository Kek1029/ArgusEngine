//
// Created by etders on 09.04.2026.
//

#ifndef ARGUSENGINE_PAIR_HPP
#define ARGUSENGINE_PAIR_HPP
#include "LinearAllocator.hpp"

namespace ArgusEngine {
    namespace Memory {

        template <typename K, typename V>
        class Pair {
            K key;
            V value;
        public:
            Pair(const K& key, const V& value) : key(key), value(value) {
            }

            K get_key() const { return key; }
            V get_value() const { return value; }
        };

    } // Memory
} // ArgusEngine

#endif //ARGUSENGINE_PAIR_HPP
