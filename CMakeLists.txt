cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("openRayner")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

find_package(Vulkan REQUIRED COMPONENTS glslc)

# bootstrap device creation
add_subdirectory(libs/vk-bootstrap)


#glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)
include_directories(libs/glfw/include)


#sourcecode
add_subdirectory(src/host)


add_executable(app main.cpp)
target_link_libraries(app PUBLIC renderer Vulkan::Vulkan)
include_directories(src/host)


#shaders
add_subdirectory(src/shaders)