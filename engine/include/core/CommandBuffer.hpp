#ifndef ARGUSENGINE_COMMAND_BUFFER_HPP
#define ARGUSENGINE_COMMAND_BUFFER_HPP

#include <array>
#include <cstring>
#include <tuple>
#include "memory/Registry.hpp"
#include "memory/LinearAllocator.hpp"
#include "Types.hpp"
#include "TileSparesSet.hpp"

namespace ArgusEngine::Engine {

    enum Op : uint8_t {
        COMPONENT_ADD,
        COMPONENT_REMOVE,
        COMPONENT_SET,
        ENTITY_DESTROY,
        TILE_REGISTER,
        TILE_MOVE
    };

    struct TileData { int32_t x, z; };
    struct TileMoveData { TileData from; TileData to; };

    struct alignas(16) CommandHeader {
        uint32_t p_idx;
        Memory::Entity entity;
        Op op;
        uint8_t _pad;        
        uint16_t size;       
    };

    class CommandBuffer {
    public:
        using ApplyFunc = void(*)(WorldRegistry&, TileSparesSet&, Memory::Entity, Op, uint8_t*);

        CommandBuffer(WorldRegistry& reg, TileSparesSet& tiles)
        : reg(reg), tilespares(tiles), cmdBuffers{Memory::VirtualLinearAllocator<uint8_t>(4ULL * 1024 * 1024 * 1024),
             Memory::VirtualLinearAllocator<uint8_t>(4ULL * 1024 * 1024 * 1024)}{
        }

        void submit();

        template <typename T>
        static void apply_thunk(WorldRegistry& r, TileSparesSet&, Memory::Entity ent, Op op, uint8_t* data) {
            auto& pool = r.get_pool<T>();
            if (op == COMPONENT_REMOVE) { pool.remove(ent); return; }
            if (op == COMPONENT_SET) {
                if (auto* ptr = pool.get_ptr(ent)) std::memcpy(ptr, data, sizeof(T));
            } else if (op == COMPONENT_ADD) {
                pool.create(ent, *reinterpret_cast<const T*>(data));
            }
        }

        static void sys_destroy_thunk(WorldRegistry& r, TileSparesSet& t, Memory::Entity e, Op, uint8_t*) {
            if (auto* tr = r.get_pool<Component_Transform>().get_ptr(e)) {
                Tile tile = {tr->template Get<5>(), tr->template Get<6>()};
                t.remove(e, tile);
            }
            r.free_entity(e);
        }

        static void sys_tile_reg_thunk(WorldRegistry&, TileSparesSet& t, Memory::Entity e, Op, uint8_t* data) {
            auto* td = reinterpret_cast<TileData*>(data);
            t.create(e, Tile{td->x, td->z});
        }

        static void sys_tile_move_thunk(WorldRegistry&, TileSparesSet& t, Memory::Entity e, Op, uint8_t* data) {
            auto* md = reinterpret_cast<TileMoveData*>(data);
            t.move(e, Tile{md->from.x, md->from.z}, Tile{md->to.x, md->to.z});
        }

        static constexpr size_t SYS_OFFSET = std::tuple_size_v<AllComponents>;
        static constexpr size_t IDX_DESTROY = SYS_OFFSET + 0;
        static constexpr size_t IDX_TILE_REG = SYS_OFFSET + 1;
        static constexpr size_t IDX_TILE_MOVE = SYS_OFFSET + 2;
        static constexpr size_t TABLE_SIZE = SYS_OFFSET + 3;

        static const std::array<ApplyFunc, TABLE_SIZE> jump_table;

        template <typename T> void add(Memory::Entity ent, const T& value = T()) {
            enqueue_raw(index_of<T, AllComponents>::value, ent, COMPONENT_ADD, sizeof(T), &value);
        }

        template <typename T>
        void set(Memory::Entity ent, const T& value) {
            enqueue_raw(index_of<T, AllComponents>::value, ent, COMPONENT_SET, sizeof(T), &value);
        }

        template <typename T>
        void remove(Memory::Entity ent) {
            enqueue_raw(index_of<T, AllComponents>::value, ent, COMPONENT_REMOVE, 0, nullptr);
        }

        void destroy_entity(Memory::Entity ent) {
            enqueue_raw(IDX_DESTROY, ent, ENTITY_DESTROY, 0, nullptr);
        }

        void register_tile(Memory::Entity ent, int32_t x, int32_t z) {
            TileData td{x, z};
            enqueue_raw(IDX_TILE_REG, ent, TILE_REGISTER, sizeof(TileData), &td);
        }

        void move_tile(Memory::Entity ent, TileData from, TileData to) {
            TileMoveData md{from, to};
            enqueue_raw(IDX_TILE_MOVE, ent, TILE_MOVE, sizeof(TileMoveData), &md);
        }

    private:
        WorldRegistry& reg;
        TileSparesSet& tilespares;
        Memory::VirtualLinearAllocator<uint8_t> cmdBuffers[2];
        uint8_t active = 0;

        void enqueue_raw(size_t p_idx, Memory::Entity ent, Op op, uint16_t sz, const void* data) {
            if (p_idx >= TABLE_SIZE) return;
            auto& buffer = cmdBuffers[active];

            // выравнивание по 16
            size_t aligned_size = (sizeof(CommandHeader) + sz + 15) & ~15;
            uint8_t* dst = buffer.alloc(aligned_size);
            if (!dst) return;

            CommandHeader* h = reinterpret_cast<CommandHeader*>(dst);
            h->p_idx = static_cast<uint32_t>(p_idx);
            h->entity = ent;
            h->op = op;
            h->size = sz;

            if (sz > 0 && data) {
                std::memcpy(dst + sizeof(CommandHeader), data, sz);
            }
        }
    };
}
#endif