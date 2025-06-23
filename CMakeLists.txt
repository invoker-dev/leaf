cmake_minimum_required(VERSION 4.0)
project(exort LANGUAGES CXX)

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
find_package(Vulkan REQUIRED)
find_package(SDL3   REQUIRED)
find_package(glm    REQUIRED)
find_package(fmt    REQUIRED)

# ------------------------------------------------------
# Dependencies -- Submodules
# ------------------------------------------------------
add_subdirectory(extern/volk)
add_subdirectory(extern/vk-bootstrap)
add_subdirectory(extern/fastgltf)
add_subdirectory(extern/vma)

target_compile_options(volk         PRIVATE -w)
target_compile_options(vk-bootstrap PRIVATE -w)
target_compile_options(fastgltf     PRIVATE -w)
target_compile_options(vma          PRIVATE -w)

# ------------------------------------------------------
# Dependencies -- static libraries
# ------------------------------------------------------
add_library(imgui
    ./extern/imgui/imgui.cpp
    ./extern/imgui/imgui_draw.cpp
    ./extern/imgui/imgui_tables.cpp
    ./extern/imgui/imgui_widgets.cpp
    ./extern/imgui/backends/imgui_impl_vulkan.cpp
    ./extern/imgui/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui PUBLIC
    extern/imgui
    extern/imgui/backends
)

target_compile_options(imgui PRIVATE -w)
# ------------------------------------------------------
# Main executable
# ------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

# ------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ./src/include
)

# ------------------------------------------------------
# Link dependencies
# ------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3
    glm::glm
    fmt::fmt
    volk
    vk-bootstrap
    fastgltf
    imgui
)
