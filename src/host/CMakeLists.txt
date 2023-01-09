add_library(renderer app.hpp
                     app.cpp
                     buffer.hpp
                     buffer.cpp
                     camera.hpp
                     camera.cpp
                     descriptors.hpp
                     descriptors.cpp
                     device.cpp
                     device.hpp
                     frameinfo.hpp
                     functions.hpp
                     functions.cpp
                     geometry.hpp
                     geometry.cpp
                     keyboard.hpp
                     keyboard.cpp
                     orayobject.hpp
                     orayobject.cpp
                     pipeline.cpp
                     pipeline.hpp
                     raytracing.hpp
                     raytracing.cpp
                     renderer.hpp
                     renderer.cpp
                     rendersystem.hpp
                     rendersystem.cpp
                     swapchain.hpp
                     swapchain.cpp
                     window.hpp
                     window.cpp
                     utils.hpp)

add_library(gui gui.hpp
                gui.cpp
                ${IMGUI_INCLUDES}
                ${IMPLOT_INCLUDES})       
                     
target_link_libraries(renderer PRIVATE glfw
                               PRIVATE glm::glm
                               PRIVATE gui
                               PRIVATE Vulkan::Vulkan)
#if(MSVC)
#    target_compile_options(renderer PRIVATE /W4 /WX)
#else()
#    target_compile_options(renderer PRIVATE -Wall -Wextra -Wpedantic)
#endif()