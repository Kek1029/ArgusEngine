#ifndef ARGUSENGINE_VIEW_HPP
#define ARGUSENGINE_VIEW_HPP

#include "PoolAllocator.hpp"
#include "jobSystem/JobSystem.hpp"
#include <tuple>
#include <bit>

namespace ArgusEngine::Memory {

    // TODO: а стоит ли делать ручной префетч в линейном проходе чтобы сразу грузить тонны данных, если я уверен в аппаратном префетчере на все 200?
    template <typename... Ts>
    class View {
        std::tuple<PoolAllocator<Ts>&...> pools;

    public:
        explicit View(PoolAllocator<Ts>&... target_pools) : pools(target_pools...) {}

        template <typename Func>
        void each(Func&& func) {
            iterate_range(0, get_max_chunks(), func);
        }

        template <typename Func>
        void each(Multithread::JobSystem& js, Func&& func, uint32_t batch_size = 32) {
            uint32_t total_chunks = get_max_chunks();

            struct Context {
                View* view;
                Func* f;
            } ctx { this, &func };

            auto wrapper = [](void* data, uint32_t start, uint32_t end) {
                auto* c = static_cast<Context*>(data);
                c->view->iterate_range(start, end, *c->f);
            };

            js.Dispatch(total_chunks, batch_size, wrapper, &ctx);
        }

    private:
        template <typename Func>
        void iterate_range(uint32_t start_chunk, uint32_t end_chunk, Func& func) {
            for (uint32_t c = start_chunk; c < end_chunk; ++c) {
                uint64_t mask = ~0ULL;
                bool skip = false;

                // Пересекаем битмаски всех пулов (Sparse Set Intersection)
                std::apply([&](auto&... p) {
                    ((skip |= (c >= p.get_raw().size())), ...);
                    if (!skip) {
                        ((mask &= p.get_chunk(c).bitmask), ...);
                    }
                }, pools);

                if (skip || mask == 0) continue;

                auto data_ptrs = std::apply([&](auto&... p) {
                    return std::make_tuple(p.get_chunk(c).ptr(0)...);
                }, pools);

                while (mask) {
                    uint32_t i = std::countr_zero(mask);
                    Entity e = static_cast<Entity>(c * 64 + i);

                    std::apply([&](auto*... ptr) {
                        func(e, ptr[i]...);
                    }, data_ptrs);

                    mask &= (mask - 1);
                }
            }
        }

        uint32_t get_max_chunks() const {
            return static_cast<uint32_t>(std::get<0>(pools).get_raw().size());
        }
    };
}

#endif