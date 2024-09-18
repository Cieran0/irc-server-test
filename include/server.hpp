#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <map>
#include <channel.hpp>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif

#include <client.hpp>

class server {
private:
    std::vector<std::unique_ptr<client>> m_clients;
    std::vector<std::thread> m_threads;
    std::map<std::string,channel> m_channels;
    int m_file_descriptor;
    struct sockaddr_in6 m_address;
    int m_port;

    void handle_clients();

public:
    server(int port);
    ~server();
    channel& get_channel(std::string channel_name);
    int start();
    std::map<std::string,client*> client_map;
};


extern server server_controller;