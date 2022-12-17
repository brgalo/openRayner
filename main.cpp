#include "app.hpp"
#include "iostream"


int main(int argc, char *argv[]) {
    oray::Application app{};
    try {
        app.run();
    } catch (const std::exception &e) {
      std::cerr << e.what();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}