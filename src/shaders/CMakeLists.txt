#adapted from:
#https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396

find_program(GLSLCOMP "glslangValidator")

set(SHADER_DIR ${CURRENT_CMAKE_DIRECTORY})
file(GLOB GLSL_SOURCE_FILES
    "${SHADER_DIR}/*.frag"
    "${SHADER_DIR}/*.vert")

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILENAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/bin/spv/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/spv"
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL ${GLSL_SOURCE_FILES})

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)


