cmake_minimum_required(VERSION 3.20)

project(ArgusEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(VCPKG_TARGET_TRIPLET STREQUAL "x64-mingw-static")
    message(STATUS "Build target: Windows (MinGW Static)")

    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -funroll-loops -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG")

    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto -s -static-libgcc -static-libstdc++ -Wl,--gc-sections")

    set(PLATFORM_LIBS opengl32 gdi32 user32 shell32 imm32 comdlg32 ole32 uuid winpthread)

elseif(VCPKG_TARGET_TRIPLET STREQUAL "x64-linux")
    message(STATUS "Build target: Linux (Native)")

    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -fno-plt -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -ffast-math -g")

    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto -Wl,--gc-sections -Wl,--as-needed")

    set(PLATFORM_LIBS pthread dl)

else()
    message(WARNING "Unknown or default triplet: ${VCPKG_TARGET_TRIPLET}")
endif()

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

set(OpenGL_GL_PREFERENCE GLVND)


message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
set(VCPKG_LIBRARY_LINKAGE static)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
add_subdirectory(deps/FastBin)
add_subdirectory(modules)
add_subdirectory(engine)
add_subdirectory(src)
add_subdirectory(tools)

option(ARGUS_BUILD_EXAMPLES "Build Argus Engine examples" OFF)

if(ARGUS_BUILD_EXAMPLES)
    message(STATUS "Examples build is ENABLED")
    add_subdirectory(examples)
endif()

enable_testing()
add_subdirectory(tests)


add_custom_target(ArgusEngine ALL
        DEPENDS ArgusEngineApp
)

if(ARGUS_BUILD_EXAMPLES)
    add_dependencies(ArgusEngine example_agstest)
endif()