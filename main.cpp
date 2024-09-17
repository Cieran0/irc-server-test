#include <iostream>
#include <client.hpp>
#include <server.hpp>
#include <cstring> // for memset
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for close


int main() {
    server serve(8080);
    serve.start();
}