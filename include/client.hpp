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
#include <chrono>
#include <message_builder.hpp>

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
        bool info_empty();
        void handle_message(std::string raw_message);
        void send_message(message to_send);
        bool read_from(char* buffer, size_t buffer_length);

        std::chrono::time_point<std::chrono::system_clock> last_active;

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
        void UNKNOWN(irc::client_command parsedCommand);

        bool correct_number_of_parameters(irc::client_command parsedCommand, size_t expected);
};

std::string generate_who_response(const std::string& requesting_nick, const std::vector<client_info>& clients, const std::string& channel);