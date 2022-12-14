add_library(renderer app.hpp
                     app.cpp
                     renderer.hpp
                     renderer.cpp
                     window.hpp
                     window.cpp
                     pipeline.cpp
                     pipeline.hpp
                     device.cpp
                     device.hpp
                     swapchain.hpp
                     swapchain.cpp
                     geometry.hpp
                     geometry.cpp)
                     
target_link_libraries(renderer PRIVATE glfw
                               PRIVATE glm::glm)