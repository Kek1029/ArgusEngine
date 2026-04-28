#include "gtest/gtest.h"
#include "memory/Chunk.hpp"
#include "memory/PoolAllocator.hpp"

using namespace ArgusEngine::Memory;

struct TestStruct {
    int x;
    TestStruct(int v = 0) : x(v) {}
};

TEST(ChunkTest, AllocFreeGet) {
    Chunk<TestStruct> chunk;

    auto [idx1, gen1] = chunk.alloc(42);
    ASSERT_NE(idx1, 0);
    ASSERT_EQ(chunk.get(idx1, gen1)->x, 42);

    chunk.free(idx1);
    ASSERT_EQ(chunk.get(idx1, gen1), nullptr);

    auto [idx2, gen2] = chunk.alloc(7);
    ASSERT_NE(idx2, 0);
    ASSERT_EQ(chunk.get(idx2, gen2)->x, 7);

    if (idx1 == idx2) {
        ASSERT_NE(gen1, gen2);
    }
}

TEST(ChunkTest, FullAndReset) {
    Chunk<int> chunk;

    std::vector<uint16_t> idxs;
    for (int i = 0; i < 64; ++i) {
        auto [idx, gen] = chunk.alloc(i);
        idxs.push_back(idx);
        ASSERT_NE(chunk.get(idx, gen), nullptr);
        ASSERT_EQ(*chunk.get(idx, gen), i);
    }
    ASSERT_TRUE(chunk.is_full());

    auto [idx, gen] = chunk.alloc(100);
    ASSERT_EQ(idx, 0);

    chunk.reset();
    ASSERT_FALSE(chunk.is_full());
}

TEST(PoolAllocatorTest, BasicAllocGetDestroy) {
    PoolAllocator<TestStruct> pool;

    auto h1 = pool.alloc(10);
    auto h2 = pool.alloc(20);

    ASSERT_NE(pool.get(h1), nullptr);
    ASSERT_NE(pool.get(h2), nullptr);
    ASSERT_EQ(pool.get(h1)->x, 10);
    ASSERT_EQ(pool.get(h2)->x, 20);

    pool.destroy(h1);
    ASSERT_EQ(pool.get(h1), nullptr);

    auto h3 = pool.alloc(30);
    ASSERT_NE(pool.get(h3), nullptr);
    ASSERT_EQ(pool.get(h3)->x, 30);
}

TEST(PoolAllocatorTest, BulkAllocAndTotalElements) {
    PoolAllocator<int> pool;

    const size_t bulk_count = 128;
    pool.allocate_bulk(bulk_count, 7);

    ASSERT_EQ(pool.total_elements(), bulk_count);
    ASSERT_GE(pool.capacity(), bulk_count);

    size_t counted = 0;
    pool.for_each_raw([&](int* data, size_t n, uint32_t) {
        for (size_t i = 0; i < n; ++i) {
            ASSERT_EQ(data[i], 7);
            counted++;
        }
    });
    ASSERT_EQ(counted, bulk_count);
}

TEST(PoolAllocatorTest, ResetGenerations) {
    PoolAllocator<int> pool;

    auto h1 = pool.alloc(5);
    auto h2 = pool.alloc(10);

    pool.reset();

    ASSERT_EQ(pool.get(h1), nullptr);
    ASSERT_EQ(pool.get(h2), nullptr);

    auto h3 = pool.alloc(15);
    ASSERT_NE(pool.get(h3), nullptr);
    ASSERT_EQ(*pool.get(h3), 15);
}

#include <random>
#include <algorithm>

TEST(PoolAllocatorTest, FragmentationAndReuse) {
    PoolAllocator<int> pool;
    std::vector<Handle> handles;
    const int count = 200;

    for (int i = 0; i < count; ++i) {
        handles.push_back(pool.alloc(i));
    }

    for (int i = 0; i < count; i += 2) {
        pool.destroy(handles[i]);
    }

    for (int i = 0; i < count; ++i) {
        if (i % 2 == 0) {
            ASSERT_EQ(pool.get(handles[i]), nullptr);
        } else {
            ASSERT_NE(pool.get(handles[i]), nullptr);
            ASSERT_EQ(*pool.get(handles[i]), i);
        }
    }

    for (int i = 0; i < count / 2; ++i) {
        auto h = pool.alloc(999);
        ASSERT_NE(pool.get(h), nullptr);
        ASSERT_EQ(*pool.get(h), 999);
    }

    ASSERT_LE(pool.capacity(), 256);
}

TEST(PoolAllocatorTest, GenerationWrapAround) {
    PoolAllocator<int> pool;

    Handle last_h;
    for (uint32_t i = 0; i < 70000; ++i) {
        auto h = pool.alloc(i);
        last_h = h;
        pool.destroy(h);
    }

    auto h_new = pool.alloc(1337);
    ASSERT_NE(pool.get(h_new), nullptr);
    ASSERT_EQ(*pool.get(h_new), 1337);
}

struct DestructorCounter {
    static int count;
    DestructorCounter() { count++; }
    ~DestructorCounter() { count--; }
};
int DestructorCounter::count = 0;

TEST(PoolAllocatorTest, DestructorCalls) {
    {
        PoolAllocator<DestructorCounter> pool;
        DestructorCounter::count = 0;

        std::vector<Handle> handles;
        for(int i = 0; i < 100; ++i) handles.push_back(pool.alloc());

        ASSERT_EQ(DestructorCounter::count, 100);

        for(int i = 0; i < 50; ++i) pool.destroy(handles[i]);
        ASSERT_EQ(DestructorCounter::count, 50);

        pool.reset();
        ASSERT_EQ(DestructorCounter::count, 0);
    }
}

TEST(PoolAllocatorTest, RandomChaos) {
    PoolAllocator<size_t> pool;
    std::vector<Handle> active_handles;
    std::mt19937 gen(42);

    for (int i = 0; i < 5000; ++i) {
        int op = std::uniform_int_distribution<>(0, 2)(gen);

        if (op == 0 || active_handles.empty()) {
            size_t val = active_handles.size();
            active_handles.push_back(pool.alloc(val));
        } else if (op == 1) {
            int idx = std::uniform_int_distribution<>(0, (int)active_handles.size()-1)(gen);
            pool.destroy(active_handles[idx]);
            active_handles.erase(active_handles.begin() + idx);
        } else {
            int idx = std::uniform_int_distribution<>(0, (int)active_handles.size()-1)(gen);
            auto* ptr = pool.get(active_handles[idx]);
            ASSERT_NE(ptr, nullptr);
        }
    }
    pool.reset();
    ASSERT_EQ(pool.total_elements(), 0);
}

