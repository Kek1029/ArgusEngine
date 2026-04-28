#ifndef ARGUSENGINE_MAP_HPP
#define ARGUSENGINE_MAP_HPP

#include "LinearAllocator.hpp"
#include "utils/Utils.hpp"
#include <cstdint>
#include <algorithm>
#include <utility>
#include <cstring> // Для memset

namespace ArgusEngine::Memory {

    template <typename K, typename V>
    class Map {
        struct Entry {
            K key;
            V value;
            uint32_t psl = 0;
            bool occupied = false;

            Entry() : key{}, value{}, psl(0), occupied(false) {}

            Entry(Entry&& other) noexcept
                : key(std::move(other.key)),
                  value(std::move(other.value)),
                  psl(other.psl),
                  occupied(other.occupied)
            {
                other.occupied = false;
                other.psl = 0;
            }
            Entry& operator=(Entry&& other) noexcept {
                if (this != &other) {
                    key = std::move(other.key);
                    value = std::move(other.value);
                    psl = other.psl;
                    occupied = other.occupied;
                    other.occupied = false;
                }
                return *this;
            }

        };

    private:
        LinearAllocator<Entry> table;
        size_t _size = 0;
        size_t _capacity = 0;

        uint32_t getIdealIndex(const K& key) const {
            if (_capacity == 0) return 0;
            return static_cast<uint32_t>(hash_fnv1a(key) & (_capacity - 1));
        }

        void clear_table_memory() {
            if (table.data()) {
                std::memset(table.data(), 0, _capacity * sizeof(Entry));
            }
        }

    public:
        Map(size_t reserve) : table(reserve) {
            _capacity = (reserve < 8) ? 8 : std::bit_ceil(reserve);
            table.resize(_capacity);
            clear_table_memory();
        }

        void insert(const K& key, V&& value) {
            if (_size * 10 >= _capacity * 7 || _capacity == 0) {
                rehash();
            }

            Entry current;
            current.key = key;
            current.value = std::move(value);
            current.psl = 0;
            current.occupied = true;

            insert_internal(std::move(current));
        }

        void remove(const K& key) {
            if (_size == 0) return;

            uint32_t idx = getIdealIndex(key);
            uint32_t dist = 0;

            while (table[idx].occupied) {
                if (dist > table[idx].psl) return;
                if (table[idx].key == key) break;

                idx = (idx + 1) & (_capacity - 1);
                dist++;
            }

            if (!table[idx].occupied) return;

            uint32_t curr = idx;
            uint32_t next = (curr + 1) & (_capacity - 1);

            while (table[next].occupied && table[next].psl > 0) {
                table[curr] = std::move(table[next]);
                table[curr].psl--;

                curr = next;
                next = (curr + 1) & (_capacity - 1);
            }

            table[curr].occupied = false;
            table[curr].psl = 0;

            _size--;
        }

    private:
        void rehash() {
            size_t old_cap = _capacity;
            LinearAllocator<Entry> old_table = std::move(table);

            _capacity = (old_cap == 0) ? 8 : old_cap * 2;
            _size = 0;

            table.resize(_capacity);
            clear_table_memory();

            for (size_t i = 0; i < old_cap; ++i) {
                if (old_table[i].occupied) {
                    Entry current = std::move(old_table[i]);
                    current.psl = 0;
                    insert_internal(std::move(current));
                }
            }
        }

        void insert_internal(Entry&& current) {
            uint32_t idx = getIdealIndex(current.key);

            while (true) {
                if (!table[idx].occupied) {
                    table[idx] = std::move(current);
                    _size++;
                    return;
                }

                if (table[idx].key == current.key) {
                    table[idx].value = std::move(current.value);
                    return;
                }

                if (current.psl > table[idx].psl) {
                    std::swap(current, table[idx]);
                }

                idx = (idx + 1) & (_capacity - 1);
                current.psl++;
            }
        }

    public:
        V* get(const K& key) {
            if (_capacity == 0) return nullptr;
            uint32_t idx = getIdealIndex(key);
            uint32_t dist = 0;

            while (table[idx].occupied) {
                if (dist > table[idx].psl) return nullptr;
                if (table[idx].key == key) return &table[idx].value;

                idx = (idx + 1) & (_capacity - 1);
                dist++;
            }
            return nullptr;
        }

        size_t size() const { return _size; }
        size_t capacity() const { return _capacity; }
    };
}

#endif