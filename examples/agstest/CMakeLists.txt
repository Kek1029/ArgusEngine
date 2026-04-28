add_executable(example_agstest agstest.cpp)

if(WIN32)
    # Прямое указание линкеру использовать только статику
    target_link_options(example_agstest PRIVATE
            "-static"
            "-Wl,-Bstatic"
            "-lstdc++"
            "-lpthread"
            "-Wl,--as-needed"
    )
endif()

target_link_libraries(example_agstest PRIVATE
        EngineCore
        EngineModules
        glfw
        glad::glad
        imgui::imgui
        ${PLATFORM_LIBS}
)