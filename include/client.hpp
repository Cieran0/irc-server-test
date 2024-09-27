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
#include <numeric>
#include <irc.hpp>


struct client_info{
    std::string username;
    std::string nickname;
    std::string realname;
    std::string ip;
};

struct client{
        bool is_active;
        int socket;
        bool welcomed=false;
        client_info info;

        client(int socket, std::string client_ip);
        ~client();

        client_info get_info();
        void handle_message(std::string message);
        void send_message(std::string message);
        std::string get_next_message();
        bool read_from(char* buffer, size_t buffer_length);

    private:
        void welcome();
        
        void NICK(irc::client_command parsedCommand);
        void USER(irc::client_command parsedCommand);
        void CAP(irc::client_command parsedCommand);
        void JOIN(irc::client_command parsedCommand);
        void PART(irc::client_command parsedCommand);
        void PING(irc::client_command parsedCommand);
        void WHO(irc::client_command parsedCommand);
        void MODE(irc::client_command parsedCommand);
        void PRIVMSG(irc::client_command parsedCommand);
        void QUIT(irc::client_command parsedCommand);
};

std::string generate_who_response(const std::string& requesting_nick, const std::vector<client_info>& clients, const std::string& channel);