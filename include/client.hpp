#pragma once

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h> 
#endif

#include <iostream>
#include <thread>

struct client_info{
    std::string username;
    std::string nickname;
    std::string realname;
    std::string ip;
};

class client{
    private:
        int m_socket;
        bool is_active;
        std::thread read_thread;
        std::thread write_thread;

        struct client_info info;

        static void handle_reading(client* instance);
        static void handle_writing(client* instance);
    public:
        client(int socket, std::string client_ip);
        ~client();

        client_info get_info();
};