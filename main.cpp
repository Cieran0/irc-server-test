#include <iostream>
#include <client.hpp>
#include <server.hpp>
#include <cstring> // for memset
#include <sys/types.h>
#include <unistd.h> // for close


server server_controller(6667);

int main() {
    server_controller.start();
}