#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <map>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

#include "client.hpp"

namespace server{
    int file_descriptor;
    struct sockaddr_in6 address;

    void init();
    int start();   
}