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
    //#pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

#define PORT 6667

#include "client.hpp"

namespace server{
    extern int file_descriptor;
    extern struct sockaddr_in6 address;
    extern std::vector<std::unique_ptr<client>> clients;
    extern std::vector<std::thread> threads;
    extern std::string host_name;
    extern std::map<std::string,channel> m_channels;

    void init();
    void handle_clients();
    client_info get_client_info(const std::string& client);
    channel& get_channel(std::string channel_name);
    int start();
    void add_to_client_map(std::string nickname, client* client);
    void send_message_to_client(std::string nickname, std::string message);
    extern std::map<std::string,client*> client_map;
}