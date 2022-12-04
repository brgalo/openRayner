cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("openRayner")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

find_package(Vulkan REQUIRED)

# bootstrap device creation
add_subdirectory(libs/vk-bootstrap)


#sourcecode
add_subdirectory(src/host)

add_executable(app app.cpp)