cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("openRayner")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

find_package(Vulkan REQUIRED COMPONENTS glslc)


#glfw
set(GLFW_DIR "${PROJECT_SOURCE_DIR}/libs/glfw")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL "Generate installation target" OFF)
set(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(libs/glfw)
include_directories(libs/glfw/include)

#glm
add_subdirectory(libs/glm)

#tiny obj loader
set(TINY_OBJ_PATH "${PROJECT_SOURCE_DIR}/libs/tinyobjloader")

# imGui
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/libs/imgui")
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)
set(IMGUI_INCLUDES ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
                   ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
                   ${IMGUI_DIR}/imgui.cpp
                   ${IMGUI_DIR}/imgui_draw.cpp
                   ${IMGUI_DIR}/imgui_demo.cpp
                   ${IMGUI_DIR}/imgui_tables.cpp
                   ${IMGUI_DIR}/imgui_widgets.cpp)

# imPlot
set(IMPLOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/implot)
include_directories(${IMPLOT_DIR})
set(IMPLOT_INCLUDES ${IMPLOT_DIR}/implot.h
                    ${IMPLOT_DIR}/implot_internal.h
                    ${IMPLOT_DIR}/implot.cpp   
                    ${IMPLOT_DIR}/implot_items.cpp
                    ${IMPLOT_DIR}/implot_demo.cpp)


#sourcecode
add_subdirectory(src/host)


add_executable(app main.cpp)
target_link_libraries(app PRIVATE renderer
                          PRIVATE Vulkan::Vulkan)

#if(MSVC)
#    target_compile_options(app PRIVATE /W4 /WX)
#else()
#    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
#endif()

include_directories(src/host)


#shaders
add_subdirectory(src/shaders)