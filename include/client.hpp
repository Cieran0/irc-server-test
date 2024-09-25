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
#include <string>
#include <mutex>
#include <queue>

struct client_info{
    std::string username;
    std::string nickname;
    std::string realname;
    std::string ip;
};

class client{
    private:
        int m_socket;
        bool m_is_active;
        std::thread m_read_thread;
        std::thread m_write_thread;
        std::queue<std::string> m_recieved_buffer;
        std::queue<std::string> m_sending_buffer;
        std::mutex m_editing_recieved_buffer;
        std::mutex m_editing_sending_buffer;

        struct client_info m_info;

        static void handle_reading(client* instance);
        static void handle_writing(client* instance);

        void send(const std::string& message);
    public:
        client(int socket, std::string client_ip);
        ~client();

        client_info get_info();
        void handle();
        void send_message(const std::string& message);
        std::string get_next_message();
};

std::string generate_who_response(const std::string& requesting_nick, const std::vector<client_info>& clients, const std::string& channel);