add_library(EngineModules src/Render.cpp
        include/DebugUI.hpp
        include/utils/Physics.hpp)

target_include_directories(EngineModules PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(EngineModules PUBLIC
        glfw
        glad::glad
        glm::glm
        imgui::imgui
        EngineCore
)