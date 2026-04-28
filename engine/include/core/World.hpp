#ifndef ARGUSENGINE_WORLD_HPP
#define ARGUSENGINE_WORLD_HPP

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <cmath>
#include <type_traits>

#include "memory/Registry.hpp"
#include "CommandBuffer.hpp"
#include "Types.hpp"
#include "TileSparesSet.hpp"

namespace ArgusEngine::Engine {

    constexpr float TILE_METER_SIZE = static_cast<float>(CHUNK_SIZE);


    inline glm::vec3 getGlobalPos(const Component_Transform& t) {
        const auto& pos = t.template Get<1>(); // Vector3Fixed<24>
        return {
            pos.x.to_float() + (static_cast<float>(t.template Get<5>()) * TILE_METER_SIZE),
            pos.y.to_float(),
            pos.z.to_float() + (static_cast<float>(t.template Get<6>()) * TILE_METER_SIZE)
        };
    }

    struct WorldPosProxy {
        Memory::Entity entity;
        Component_Transform& trans;
        CommandBuffer& cb;

        glm::vec3 get() const { return getGlobalPos(trans); }

        void set(const glm::vec3& v) {
            int32_t tx = static_cast<int32_t>(std::floor(v.x / TILE_METER_SIZE));
            int32_t tz = static_cast<int32_t>(std::floor(v.z / TILE_METER_SIZE));

            Tile tile = static_cast<Tile>(trans.template Get<6>());
            int32_t old_tx = tile.x;
            int32_t old_tz = tile.y;

            if (tx != old_tx || tz != old_tz) {
                cb.move_tile(entity, {old_tx, old_tz}, {tx, tz});

                trans.template Set<5>(tx);
                trans.template Set<6>(tz);
            }

            float lx = v.x - (static_cast<float>(tx) * TILE_METER_SIZE);
            float lz = v.z - (static_cast<float>(tz) * TILE_METER_SIZE);

            trans.template Set<1>(Math::Vector3Fixed<24>(lx, v.y, lz));
        }
    };

    namespace ProxyMagic {
        template <typename T>
        struct Wrapper {
            static T& wrap(Memory::Entity e, T& comp, CommandBuffer& cb) { return comp; }
        };
        template <>
        struct Wrapper<Component_Transform> {
            static WorldPosProxy wrap(Memory::Entity e, Component_Transform& comp, CommandBuffer& cb) {
                return WorldPosProxy{e, comp, cb};
            }
        };
    }

    class World {
    private:
        WorldRegistry reg;
        TileSparesSet tilespares;
        CommandBuffer cmd;
        Multithread::JobSystem js;
        uint64_t currentFrame = 0;

    public:
        World() : cmd(reg, tilespares) { js.Initialize(); }

        Memory::Entity create_entity() { return reg.create_entity(); }
        void destroy_entity(Memory::Entity e) { cmd.destroy_entity(e); }

        struct EntityProxy {
            Memory::Entity e;
            CommandBuffer& cb;

            template <typename T> EntityProxy& add(const T& val = T()) {
                cb.add<T>(e, val);
                if constexpr (std::is_same_v<T, Component_Transform>) {
                    cb.register_tile(e, val.template Get<5>(), val.template Get<6>());
                }
                return *this;
            }
            template <typename T> EntityProxy& set(const T& val) { cb.set<T>(e, val); return *this; }
            template <typename T> EntityProxy& remove() { cb.remove<T>(e); return *this; }
        };

        EntityProxy at(Memory::Entity e) { return { e, cmd }; }

        template <typename... Tn, typename F>
        void update_systems(const float dt, F&& systemFunc) {
            currentFrame++;

            // Вьюха теперь собирает только нужные компоненты
            auto view = reg.view<Tn...>();

            view.each(js, [&](Memory::Entity e, Tn&... comps) {
                systemFunc(ProxyMagic::Wrapper<Tn>::wrap(e, comps, cmd)..., dt);
            }, 4096);
        }

        void sync() { cmd.submit(); }

        WorldRegistry& registry() { return reg; }
        TileSparesSet& get_tiles() { return tilespares; }
        uint64_t get_current_frame() const { return currentFrame; }
        size_t total_entities() const { return reg.total_entities(); }
    };
}

#endif // ARGUSENGINE_WORLD_HPP