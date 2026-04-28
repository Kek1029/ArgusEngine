add_executable(ArgusConverter converter.cpp)

target_link_libraries(ArgusConverter PRIVATE
        EngineCore
        ${PLATFORM_LIBS}
        FastBin
)