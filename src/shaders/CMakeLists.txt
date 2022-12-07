#adapted from:
#https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396
include(CMakePrintHelpers)

set(SHADER_DIR ${CURRENT_CMAKE_DIRECTORY})
set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})

# add shader files to list
file(GLOB GLSL_SOURCE_FILES
    "*.frag"
    "*.vert")

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILENAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/spv/${FILENAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/spv"
        COMMAND glslc ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL ${GLSL_SOURCE_FILES})

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)



#auto compile shaders and copy to bin/shaders
#see src/shaders/CMakeLists.txt for more details
add_dependencies(app Shaders)

