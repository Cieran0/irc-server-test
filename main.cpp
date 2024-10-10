#include "server.hpp"

int main(int argc, const char** argv) {

    bool debug = false;
    if(argc == 2) {
        debug = (std::string(argv[1]) == "--debug");
    }

    return server::main(debug);
}
