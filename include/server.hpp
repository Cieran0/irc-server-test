#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <map>
#include <poll.h>
#include <channel.hpp>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #error "Not working on windows"
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
#include "util.hpp"


namespace server{
    extern int file_descriptor;
    extern struct sockaddr_in6 address;
    extern struct sockaddr_in6 client_addr;
    extern std::map<int, client> clients;
    extern std::vector<std::thread> threads;
    extern std::string host_name;
    extern std::map<std::string,channel> m_channels;
    extern std::queue<std::pair<int,std::string>> output_queue;


    int init();
    void handle_clients();
    client_info get_client_info(const std::string& client);
    channel& get_channel(std::string channel_name);
    int main();
    void add_to_client_map(std::string nickname, client* client);
    void send_message_to_client(std::string nickname, std::string message);
    extern std::map<std::string,client*> client_map;
    void send_all_queued_messages();
}