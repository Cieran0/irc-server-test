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
    #include <poll.h>
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
    const int MAX_CLIENTS = 1024;

    extern int server_socket_fd;
    extern sockaddr_in6 address;
    extern sockaddr_in6 client_addr;
    extern std::map<int, client> clients;
    extern std::vector<std::thread> threads;
    extern std::string host_name;
    extern std::map<std::string,channel> channels;
    extern std::queue<std::pair<int,std::string>> output_queue;
    extern pollfd socket_fds[MAX_CLIENTS]; // the socket array
    extern int number_of_socket_fds;
    extern bool debug_mode;

    int init();
    void poll_loop();
    void handle_client_sockets();
    void check_client_timeout(int i);
    void handle_socket_error(int i);
    void listen_to_client(int i);
    void kill_client_connection(int client_socket, const std::map<int, client>::iterator &it, int i);
    void listen_for_connections();
    client_info get_client_info(const std::string &client);
    channel &get_channel(std::string channel_name);
    int main(bool debug);
    void add_to_client_map(std::string nickname, client* client);
    void send_message_to_client(std::string nickname, message to_send);
    extern std::map<std::string,client*> client_map;
    void send_all_queued_messages();
    bool is_user_in_channel(std::string nickname, std::string channel);
}