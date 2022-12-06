cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("openRayner")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

find_package(Vulkan REQUIRED)

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

#shaders
add_subdirectory(src/shaders)

add_executable(app main.cpp)
target_link_libraries(app renderer)
include_directories(src/host)

#auto compile shaders and copy to bin/shaders
#see src/shaders/CMakeLists.txt for more details
add_dependencies(app Shaders)