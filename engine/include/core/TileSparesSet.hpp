//
// Created by etders on 09.04.2026.
//

#ifndef ARGUSENGINE_TILESPARESSET_HPP
#define ARGUSENGINE_TILESPARESSET_HPP

#include "../memory/Registry.hpp"
#include "Types.hpp"
#include "memory/Map.hpp"
#include "memory/Pair.hpp"

namespace ArgusEngine {
    namespace Engine {

        class TileSparesSet {
        private:
            Memory::Map<int64_t, Memory::LinearAllocator<Memory::Entity>> spares_set;

            static inline int64_t tile_to_64(const Tile& tile) {
                return static_cast<int64_t>(static_cast<uint32_t>(tile.x)) << 32 | static_cast<uint32_t>(tile.y);
            }

            static inline Tile tile_from_64(const int64_t key) {
                return Tile {
                    static_cast<int32_t>(static_cast<uint64_t>(key) >> 32),
                    static_cast<int32_t>(static_cast<uint32_t>(key & 0xFFFFFFFF))
                };
            }
        public:

            TileSparesSet() : spares_set(64) {

            }

            void create(Memory::Entity entity, const Tile& tile) {
                int64_t key = tile_to_64(tile);
                auto* entities = spares_set.get(key);
                if (!entities) {
                    spares_set.insert(key, Memory::LinearAllocator<Memory::Entity>(128));
                    entities = spares_set.get(key);
                }
                entities->push_back(entity);
            }

            bool remove(Memory::Entity entity, const Tile& tile) {
                auto* entities = spares_set.get(tile_to_64(tile));
                if (entities) {
                    for (size_t i = 0; i < entities->size(); ++i) {
                        if ((*entities)[i] == entity) {
                            entities->fast_erase(i);
                            return true;
                        }
                    }
                }
                return false;
            }


            void move(Memory::Entity entity, const Tile& old_t, const Tile& new_t) {
                if (old_t.x == new_t.x && old_t.y == new_t.y) return;
                remove(entity, old_t);
                create(entity, new_t);
            }

            struct TileProxy {
                int32_t x, y;
                Memory::LinearAllocator<Memory::Entity>* entities;

                Memory::Pair<Memory::Entity*, size_t> getRawEntities() const {
                    return Memory::Pair<Memory::Entity*, size_t>(entities->data(), entities->size());
                }
            };


            TileProxy at(const Tile& tile) {
                return TileProxy {tile.x, tile.y, spares_set.get(tile_to_64(tile))};
            }
        };
    } // Engine
} // ArgusEngine

#endif //ARGUSENGINE_TILESPARESSET_HPP
