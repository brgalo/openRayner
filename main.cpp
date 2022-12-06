#include "app.hpp"


int main(int argc, char *argv[]) {
    oray::Application app{};
    try {
        app.run();
    } catch (const std::exception &e) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}