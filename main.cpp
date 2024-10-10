#include "server.hpp"

int main(int argc, const char** argv) {

    //Check if debug mode is enabled
    bool debug = false;
    if(argc == 2) {
        debug = (std::string(argv[1]) == "--debug");
    }

    return server::main(debug);
}
