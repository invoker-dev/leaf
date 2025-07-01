#
#
#
#
#
#
# ###################################
# Not my code
# Generated with Claude Sonnet 4
# ###################################
#
#
#
#
#
#
#

# Find slangc compiler
find_program(SLANGC_EXECUTABLE slangc 
    HINTS "${CMAKE_SOURCE_DIR}/opt/slang/bin/"
)

# Shader directories
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")

# Create output directory
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

# Find all shader files
file(GLOB_RECURSE SHADER_FILES 
    "${SHADER_SOURCE_DIR}/*.slang"
    "${SHADER_SOURCE_DIR}/*.vert" 
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.comp"
    "${SHADER_SOURCE_DIR}/*.geom"
    "${SHADER_SOURCE_DIR}/*.tesc"
    "${SHADER_SOURCE_DIR}/*.tese"
)

# Function to determine shader stage from filename/extension
function(get_shader_stage SHADER_FILE OUT_STAGE)
    get_filename_component(FILENAME ${SHADER_FILE} NAME)
    
    if(FILENAME MATCHES "\\.(vert|vertex)\\.")
        set(${OUT_STAGE} "vertex" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.(frag|fragment|pixel)\\.")
        set(${OUT_STAGE} "fragment" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.(comp|compute)\\.")
        set(${OUT_STAGE} "compute" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.(geom|geometry)\\.")
        set(${OUT_STAGE} "geometry" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.(tesc|tesscontrol)\\.")
        set(${OUT_STAGE} "tesscontrol" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.(tese|tesseval)\\.")
        set(${OUT_STAGE} "tesseval" PARENT_SCOPE)
    else()
        # Try to infer from extension
        get_filename_component(EXT ${SHADER_FILE} LAST_EXT)
        if(EXT STREQUAL ".vert")
            set(${OUT_STAGE} "vertex" PARENT_SCOPE)
        elseif(EXT STREQUAL ".frag")
            set(${OUT_STAGE} "fragment" PARENT_SCOPE)
        elseif(EXT STREQUAL ".comp")
            set(${OUT_STAGE} "compute" PARENT_SCOPE)
        elseif(EXT STREQUAL ".geom")
            set(${OUT_STAGE} "geometry" PARENT_SCOPE)
        elseif(EXT STREQUAL ".tesc")
            set(${OUT_STAGE} "tesscontrol" PARENT_SCOPE)
        elseif(EXT STREQUAL ".tese")
            set(${OUT_STAGE} "tesseval" PARENT_SCOPE)
        else()
            # Default to vertex for .slang files or unknown
            set(${OUT_STAGE} "vertex" PARENT_SCOPE)
        endif()
    endif()
endfunction()

# Compile each shader
set(COMPILED_SHADERS "")
foreach(SHADER_FILE ${SHADER_FILES})
    # Get relative path and output name
    file(RELATIVE_PATH REL_PATH "${SHADER_SOURCE_DIR}" "${SHADER_FILE}")
    get_filename_component(NAME_WE ${REL_PATH} NAME_WE)
    get_filename_component(DIR_PATH ${REL_PATH} DIRECTORY)
    
    # Create output path
    if(DIR_PATH)
        set(OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${DIR_PATH}/${NAME_WE}.spv")
        file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}/${DIR_PATH}")
    else()
        set(OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${NAME_WE}.spv")
    endif()
    
    # Get shader stage
    get_shader_stage(${SHADER_FILE} STAGE)
    
    # Add compilation command
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${SLANGC_EXECUTABLE}
            -target spirv
            -stage ${STAGE}
            -o ${OUTPUT_FILE}
            ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling shader: ${REL_PATH} (${STAGE})"
        VERBATIM
    )
    
    list(APPEND COMPILED_SHADERS ${OUTPUT_FILE})
endforeach()

# Create target for all shaders
if(COMPILED_SHADERS)
    add_custom_target(shaders ALL DEPENDS ${COMPILED_SHADERS})
    
    # Only add dependency if PROJECT_NAME target exists
    if(TARGET ${PROJECT_NAME})
        add_dependencies(${PROJECT_NAME} shaders)
    endif()
    
    list(LENGTH SHADER_FILES SHADER_COUNT)
    message(STATUS "Found ${SHADER_COUNT} shader files to compile")
else()
    message(STATUS "No shader files found in ${SHADER_SOURCE_DIR}")
endif()
