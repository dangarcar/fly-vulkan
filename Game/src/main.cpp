#include "Game.hpp"

int main() {
    fly::Engine engine(1280, 720);

    try {
        engine.init();

        engine.setScene<Game>();

        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}