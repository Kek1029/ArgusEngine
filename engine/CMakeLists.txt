add_library(EngineCore
        src/core/CommandBuffer.cpp
        src/core/Engine.cpp
        src/core/EventSystem.cpp
        include/core/TileSparesSet.hpp
        include/memory/Pair.hpp
        include/utils/TileFixedPoint.hpp
        include/memory/VirtualLinearAllocator.hpp
        include/memory/FastIndexLinearAllocator.hpp
)

target_include_directories(EngineCore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(EngineCore PUBLIC
        EngineModules
        )


target_link_libraries(EngineCore PUBLIC FastBin)
