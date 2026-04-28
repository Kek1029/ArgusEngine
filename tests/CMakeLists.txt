include(GoogleTest)

add_executable(Tests
        test_chunk.cpp
        test_map.cpp)

target_link_libraries(Tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main EngineCore)

gtest_discover_tests(Tests)