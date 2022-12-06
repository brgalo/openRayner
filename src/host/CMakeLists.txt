add_library(renderer app.hpp
                     app.cpp
                     renderer.hpp
                     renderer.cpp
                     window.hpp
                     window.cpp)
target_link_libraries(renderer glfw)