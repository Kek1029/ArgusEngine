//
// Created by etders on 26.03.2026.
//

#ifndef ARGUSENGINE_REGISTRY_HPP
#define ARGUSENGINE_REGISTRY_HPP

#include "PoolAllocator.hpp"
#include "View.hpp"
#include "utils/Utils.hpp"

namespace ArgusEngine {
    namespace Memory {
        template <typename... BaseTypes>
        class Registry {
        private:
            std::tuple<PoolAllocator<BaseTypes>...> pools;
            Entity next_entity = 0;
            LinearAllocator<Entity> free_entities;

        public:

            template <typename T>
            static constexpr size_t pool_id_v = index_of<T, std::tuple<BaseTypes...>>::value;

            using ComponentsTuple = std::tuple<BaseTypes...>;

            Registry() {
                uint8_t id_gen = 0;
                std::apply([&](auto&... p) { ((p.id = id_gen++), ...); }, pools);
            }

            Entity create_entity() {
                if (!free_entities.empty()) {
                    Entity e = free_entities.back();
                    free_entities.pop_back();
                    return e;
                }
                Entity e = next_entity++;
                return e;
            }

            void free_entity(Entity e) {
                if (e == 0) return;

                std::apply([e](auto&... pool) {
                    (pool.remove(e), ...);
                }, pools);

                free_entities.push_back(e);
            }

            template <typename T, typename... Args>
            T& assign(Entity e, Args&&... args) {
                return *get_pool<T>().get(get_pool<T>().assign(e, std::forward<Args>(args)...));
            }

            template <typename T>
            void remove(Entity e) {
                get_pool<T>().remove(e);
            }

            template <typename... Comps>
            Entity assemblage(Comps&&... components) {
                Entity e = create_entity();
                (assign<std::decay_t<Comps>>(e, std::forward<Comps>(components)), ...);
                return e;
            }

            template <typename T>
            auto& get_pool()
            {
                return std::get<pool_id_v<T>>(pools);
            }

            template <typename T>
            const auto& get_pool() const
            {
                return std::get<pool_id_v<T>>(pools);
            }

            template <size_t I>
            auto& get_pool_at() {
                return std::get<I>(pools);
            }

            template <size_t I>
            const auto& get_pool_at() const {
                return std::get<I>(pools);
            }

            template <typename T>
            auto get_pool_id() {
                return std::get<static_cast<std::size_t>(pool_id_v<T>)>(pools);
            }

            template <typename T>
            const auto get_pool() const {
                return std::get<pool_id_v<T>>(pools);
            }

            template <typename T, typename... Args>
            Handle create(Args&&... args) {
                return get_pool<T>().alloc(std::forward<Args>(args)...);
            }

            template <size_t I, typename... Args>
            Handle create_at(Args&&... args) {
                return get_pool_at<I>().alloc(std::forward<Args>(args)...);
            }

            template <typename T>
            const T* get(const Handle& h) const {
                return get_pool<T>().get(h);
            }

            template <typename T>
            T* get(const Handle& h) {
                return get_pool<T>().get(h);
            }

            template <typename T>
            void destroy(const Handle& h) { get_pool<T>().destroy(h); }

            template <typename T>
            T* get_by_handle(const Handle& h) {
                return get_pool<T>().get(h);
            }

            template <typename T>
            void set_by_handle(const Handle& h, const T& val) {
                if (auto* ptr = get_by_handle<T>(h)) {
                    *ptr = val;
                }
            }

            template <typename T, typename Func>
            void for_each_raw(Func&& func) {
                auto& pool = get_pool<T>();

                pool.for_each_raw(std::forward<Func>(func));
            }

            template <typename Func>
            void for_each_raw_all(Func&& func) {
                std::apply([&](auto&... pool) {
                    (pool.for_each_raw(func), ...);
                }, pools);
            }

            template <typename... Ts>
            auto view() {
                return View<Ts...>(get_pool<Ts>()...);
            }

            size_t total_entities() const { return next_entity; }
        };
    }
}

#endif //ARGUSENGINE_REGISTRY_HPP