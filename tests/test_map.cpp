//
// Created by etders on 05.04.2026.
//
#include <gtest/gtest.h>
#include "memory/Map.hpp"

namespace ArgusEngine::Tests {

    TEST(ArgusMapTest, BasicInsertAndFind) {
        Memory::Map<uint64_t, std::string> map(16);

        map.insert(100, "Entity_100");
        map.insert(200, "Entity_200");

        auto* val1 = map.get(100);
        auto* val2 = map.get(200);
        auto* val3 = map.get(300);

        ASSERT_NE(val1, nullptr);
        EXPECT_EQ(*val1, "Entity_100");

        ASSERT_NE(val2, nullptr);
        EXPECT_EQ(*val2, "Entity_200");

        EXPECT_EQ(val3, nullptr);
    }

    TEST(ArgusMapTest, CollisionHandling) {
        Memory::Map<uint32_t, int> map(8);

        map.insert(1, 10);
        map.insert(9, 90);
        map.insert(17, 170);

        EXPECT_EQ(*map.get(1), 10);
        EXPECT_EQ(*map.get(9), 90);
        EXPECT_EQ(*map.get(17), 170);
    }

    TEST(ArgusMapTest, AutoGrowAndRehash) {
        Memory::Map<int, int> map(4);

        for(int i = 0; i < 100; ++i) {
            map.insert(i, i * 10);
        }

        EXPECT_GE(map.capacity(), 100);

        for(int i = 0; i < 100; ++i) {
            auto* val = map.get(i);
            ASSERT_NE(val, nullptr) << "Failed at index " << i;
            EXPECT_EQ(*val, i * 10);
        }
    }

    TEST(ArgusMapTest, OverwriteExistingKey) {
        Memory::Map<std::string, int> map(16);

        map.insert("health", 100);
        map.insert("health", 80);

        auto* val = map.get("health");
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(*val, 80);
    }
}