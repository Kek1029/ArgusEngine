add_executable(ArgusEngineApp
        main.cpp
)

target_link_libraries(ArgusEngineApp PRIVATE
        EngineCore
        EngineModules

        glfw
        glad::glad
        glm::glm
        imgui::imgui

        ${PLATFORM_LIBS}
)