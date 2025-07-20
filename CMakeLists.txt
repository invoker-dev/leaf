
cmake_minimum_required(VERSION 3.27)

execute_process(
    COMMAND git describe --tags
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

project(leaf LANGUAGES CXX)
add_compile_definitions(LEAF_VERSION=\"${GIT_VERSION}\")


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_EXTENSIONS         OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        -g3
        -O0
        -fno-omit-frame-pointer
        -fno-inline
        -Wall
        -Wextra
        -Wpedantic
        -march=native
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -O3
        -DNDEBUG

    )
endif()

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
target_compile_options(VulkanMemoryAllocator INTERFACE -w)

# ------------------------------------------------------
# Dependencies -- static libraries
# ------------------------------------------------------add_library(imgui_locAL STATIC
add_library(imgui STATIC
    ./lib/imgui/imgui.cpp
    ./lib/imgui/imgui_demo.cpp
    ./lib/imgui/imgui_draw.cpp
    ./lib/imgui/imgui_tables.cpp
    ./lib/imgui/imgui_widgets.cpp
    ./lib/imgui/backends/imgui_impl_vulkan.cpp
    ./lib/imgui/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui PUBLIC
    lib/imgui
    lib/imgui/backends
)

target_compile_options(imgui PRIVATE -w)

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
    imgui
    volk
)



# ------------------------------------------------------
# Compile shaders
# ------------------------------------------------------

set(COMPILED_SHADER_FILES "")

# stolen from vk-bootstrap
macro(compile_shader SHADER_NAME)
    set(SHADER_SOURCE ${CMAKE_SOURCE_DIR}/src/shaders/${SHADER_NAME})
    set(SHADER_SPIRV_NAME ${SHADER_NAME}.spv)
    set(SHADER_SPIRV_PATH ${CMAKE_SOURCE_DIR}/build/shaders/${SHADER_SPIRV_NAME})

    add_custom_command(
        OUTPUT ${SHADER_SPIRV_PATH}
        COMMAND glslang -V ${SHADER_SOURCE} -o ${SHADER_SPIRV_PATH}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Shader ${SHADER_NAME} compiled"
    )

    list(APPEND COMPILED_SHADER_FILES ${SHADER_SPIRV_PATH})
endmacro()

compile_shader(triangle.vert)
compile_shader(triangleMesh.vert)
compile_shader(triangle.frag)

add_custom_target(compile_shaders ALL
    DEPENDS ${COMPILED_SHADER_FILES}
)

