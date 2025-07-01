
cmake_minimum_required(VERSION 3.27)

execute_process(
    COMMAND git describe --tags
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

project(leaf LANGUAGES CXX)
add_compile_definitions(LEAF_VERSION=\"${GIT_VERSION}\")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_EXTENSIONS         OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -O2
    -Wall -Wextra -Wpedantic
    -march=native # compile for CPU
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# ------------------------------------------------------
# Dependencies -- System Installed
# ------------------------------------------------------
# program
find_package(Vulkan REQUIRED)
find_package(SDL3   REQUIRED)
find_package(glm    REQUIRED)
find_package(fmt    REQUIRED)

# ------------------------------------------------------
# Dependencies -- Submodules
# ------------------------------------------------------
add_subdirectory(lib/volk)
add_subdirectory(lib/vk-bootstrap)
add_subdirectory(lib/fastgltf)
add_subdirectory(lib/vma)

target_compile_options(volk         PRIVATE -w)
target_compile_options(vk-bootstrap PRIVATE -w)
target_compile_options(fastgltf     PRIVATE -w)

# ------------------------------------------------------
# Dependencies -- static libraries
# ------------------------------------------------------add_library(imgui_locAL STATIC
add_library(imgui_local STATIC
    ./lib/imgui/imgui_draw.cpp
    ./lib/imgui/imgui_tables.cpp
    ./lib/imgui/imgui_widgets.cpp
    ./lib/imgui/backends/imgui_impl_vulkan.cpp
    ./lib/imgui/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui_local PUBLIC
    lib/imgui
    lib/imgui/backends
)

target_compile_options(imgui_local PRIVATE -w)

# ------------------------------------------------------
# Main executable
# ------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ./src/include
)

# ------------------------------------------------------
# Link dependencies
# ------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    GPUOpen::VulkanMemoryAllocator
    Vulkan::Vulkan
    vk-bootstrap
    SDL3::SDL3
    glm::glm
    fmt::fmt
    fastgltf
    imgui_local
    volk
)


# ------------------------------------------------------
# Slang Shader Compilation
# ------------------------------------------------------

include(cmake/spirv.cmake)
