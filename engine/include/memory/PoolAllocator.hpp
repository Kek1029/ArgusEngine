#ifndef ARGUSENGINE_POOLALLOCATOR_HPP
#define ARGUSENGINE_POOLALLOCATOR_HPP

#include "Handle.hpp"
#include "Chunk.hpp"
#include "LinearAllocator.hpp"
#include "VirtualLinearAllocator.hpp"
#include <bit>

namespace ArgusEngine::Memory {

template <typename T>
class PoolAllocator {
private:
    VirtualLinearAllocator<Chunk<T>> chunks;
    LinearAllocator<uint32_t> free_chunks_indices;
    LinearAllocator<Handle> entity_to_handle;
    LinearAllocator<Entity> slot_to_entity;

    void alloc_new_chunk() {
        Chunk<T>* new_chunk_ptr = chunks.alloc(1);
        if (!new_chunk_ptr) {
            std::cerr << "CRITICAL: chunks.alloc(1) returned nullptr!" << std::endl;
            std::abort();
        }
        new (new_chunk_ptr) Chunk<T>(); // Placement new... TODO: СУКА УДАЛИТЬ ЭТО И СДЕЛАТЬ БЕЗ PLACEMENT NEW!!!!

        uint32_t new_chunk_idx = static_cast<uint32_t>(chunks.size() - 1);
        free_chunks_indices.push_back(new_chunk_idx);

        uint32_t required_slots = (new_chunk_idx + 1) * 64;
        if (slot_to_entity.size() < required_slots) {
            slot_to_entity.resize(required_slots);
        }
    }

    inline uint32_t get_entity_idx(Entity e) const {
        return static_cast<uint32_t>(e & ENTITY_INDEX_MASK);
    }

    inline uint32_t get_slot_idx(uint32_t chunk_idx, uint8_t element_idx) const {
        return (chunk_idx << 6) | (static_cast<uint32_t>(element_idx) - 1);
    }

    inline uint32_t calculate_linear_index(uint32_t chunk_idx, uint8_t element_idx) const {
        return (chunk_idx << 6) | (static_cast<uint32_t>(element_idx) - 1);
    }

public:
    uint8_t id = 0;

    PoolAllocator() :
    free_chunks_indices(128),
    entity_to_handle(1024),
    slot_to_entity(64)
{
        alloc_new_chunk();
}

    template <typename... Args>
    Handle create(Entity e, Args&&... args) {
        return assign(e, std::forward<Args>(args)...);
    }

    template <typename... Args>
    Handle assign(Entity e, Args&&... args) {
        uint32_t e_idx = get_entity_idx(e);
        if (e_idx < entity_to_handle.size()) {
            Handle old = entity_to_handle[e_idx];
            if (old.element_idx != 0) remove(e);
        } else {
            entity_to_handle.resize(e_idx + 1);
        }

        if (free_chunks_indices.empty()) alloc_new_chunk();

        uint32_t chunk_idx = free_chunks_indices.back();
        auto& chunk = chunks[chunk_idx];
        auto [slot_idx, gen] = chunk.alloc(std::forward<Args>(args)...);

        if (chunk.is_full()) free_chunks_indices.pop_back();

        Handle h(chunk_idx, gen, slot_idx, id);
        entity_to_handle[e_idx] = h;
        slot_to_entity[get_slot_idx(chunk_idx, slot_idx)] = e;

        return h;
    }

    bool has(Entity e) const {
        if (e == INVALID_ENTITY) return false;
        uint32_t e_idx = get_entity_idx(e);
        if (e_idx >= entity_to_handle.size()) return false;

        Handle h = entity_to_handle[e_idx];
        if (h.element_idx == 0 || h.chunk_idx >= chunks.size()) return false;

        const auto& chunk = chunks[h.chunk_idx];
        uint32_t internal_idx = h.element_idx - 1;

        if (!(chunk.bitmask & (1ULL << internal_idx))) return false;
        if (chunk.generations[internal_idx] != h.generation) return false;

        uint32_t slot = get_slot_idx(h.chunk_idx, h.element_idx);
        return slot < slot_to_entity.size() && slot_to_entity[slot] == e;
    }

    void remove(Entity e) {
        uint32_t e_idx = get_entity_idx(e);
        if (e_idx >= entity_to_handle.size()) return;

        Handle h = entity_to_handle[e_idx];
        if (h.element_idx == 0) return;

        destroy(h);
        entity_to_handle[e_idx] = Handle();
    }

    T* get_by_entity(Entity e) {
        if (!has(e)) return nullptr;
        return get(entity_to_handle[get_entity_idx(e)]);
    }

    template <typename... Args>
    Handle alloc(Args&&... args) {
        if (free_chunks_indices.empty()) alloc_new_chunk();
        uint32_t chunk_idx = free_chunks_indices.back();
        auto& chunk = chunks[chunk_idx];

        auto [idx, gen] = chunk.alloc(std::forward<Args>(args)...);
        if (chunk.is_full()) free_chunks_indices.pop_back();

        return Handle(chunk_idx, gen, idx, id);
    }

    void destroy(const Handle& h) {
        if (h.chunk_idx >= chunks.size() || h.element_idx == 0) return;

        auto& chunk = chunks[h.chunk_idx];
        bool was_full = chunk.is_full();

        uint32_t slot = get_slot_idx(h.chunk_idx, h.element_idx);
        if (slot < slot_to_entity.size()) slot_to_entity[slot] = INVALID_ENTITY;

        chunk.free(h.element_idx);
        if (was_full) free_chunks_indices.push_back(h.chunk_idx);
    }

    T* get(Handle h) {
        if (h.chunk_idx >= chunks.size() || h.element_idx == 0) return nullptr;
        return chunks[h.chunk_idx].get(h.element_idx, h.generation);
    }

    const T* get(Handle h) const {
        if (h.chunk_idx >= chunks.size() || h.element_idx == 0) return nullptr;
        return chunks[h.chunk_idx].get(h.element_idx, h.generation);
    }

    T* get_ptr(Entity e) {
        uint32_t e_idx = get_entity_idx(e);
        if (e_idx >= entity_to_handle.size()) return nullptr;
        Handle h = entity_to_handle[e_idx];
        if (h.element_idx == 0 || h.chunk_idx >= chunks.size()) return nullptr;
        return chunks[h.chunk_idx].get(h.element_idx, h.generation);
    }

    const T* get_ptr(Entity e) const {
        uint32_t e_idx = get_entity_idx(e);
        if (e_idx >= entity_to_handle.size()) return nullptr;
        Handle h = entity_to_handle[e_idx];
        if (h.element_idx == 0 || h.chunk_idx >= chunks.size()) return nullptr;
        return chunks[h.chunk_idx].get(h.element_idx, h.generation);
    }

    VirtualLinearAllocator<Chunk<T>>& get_raw() { return chunks; }
    const VirtualLinearAllocator<Chunk<T>>& get_raw() const { return chunks; }

    Chunk<T>& get_chunk(uint32_t i) { return chunks[i]; }
    const Chunk<T>& get_chunk(uint32_t i) const { return chunks[i]; }

    Entity get_entity_at(uint32_t chunk_idx, uint8_t element_idx) const {
        uint32_t idx = calculate_linear_index(chunk_idx, element_idx);
        return (idx < slot_to_entity.size()) ? slot_to_entity[idx] : INVALID_ENTITY;
    }

    void allocate_bulk(uint32_t count, const T& default_value) {
        for(uint32_t i = 0; i < count; ++i) alloc(default_value);
    }

    template <typename Func>
    void for_each_raw(Func&& func) {
        size_t chunk_count = chunks.size();
        for (size_t c = 0; c < chunk_count; ++c) {
            auto& chunk = chunks[c];
            if (chunk.bitmask == 0) continue;

            T* base_ptr = chunk.ptr(0);

            if (chunk.is_full()) {
                func(base_ptr, 64, static_cast<uint32_t>(c));
                continue;
            }

            uint64_t mask = chunk.bitmask;
            while (mask) {
                uint32_t i = std::countr_zero(mask);
                func(base_ptr + i, 1, static_cast<uint32_t>(c));
                mask &= (mask - 1);
            }
        }
    }

    void reset() {
        free_chunks_indices.clear();
        entity_to_handle.clear();
        slot_to_entity.clear();
        size_t count = chunks.size();
        for (uint32_t i = 0; i < (uint32_t)count; ++i) {
            chunks[i].reset();
            free_chunks_indices.push_back(i);
        }
        if (chunks.size() == 0) alloc_new_chunk();
    }

    [[nodiscard]] size_t capacity() const { return chunks.size() * 64; }

    [[nodiscard]] size_t total_elements() const {
        size_t count = 0;
        size_t c_size = chunks.size();
        for (size_t i = 0; i < c_size; ++i) {
            count += std::popcount(chunks[i].bitmask);
        }
        return count;
    }
};

} // namespace ArgusEngine::Memory

#endif