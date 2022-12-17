cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("openRayner")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

find_package(Vulkan REQUIRED COMPONENTS glslc)


#glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)
include_directories(libs/glfw/include)

#glm
add_subdirectory(libs/glm)


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