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
#include <irc_numberic_replies.hpp>
#include <util.hpp>
#include <chrono>

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

        void handle_message(std::string raw_message);
        void send_message(message to_send);
        bool read_from(char* buffer, size_t buffer_length);

        std::chrono::time_point<std::chrono::system_clock> last_active;

    private:

        void welcome();
        bool info_empty();
        
        void NICK(irc::client_command command);
        void USER(irc::client_command command);
        void CAP(irc::client_command command);
        void JOIN(irc::client_command command);
        void PART(irc::client_command command);
        void PING(irc::client_command command);
        void WHO(irc::client_command command);
        void MODE(irc::client_command command);
        void PRIVMSG(irc::client_command command);
        void QUIT(irc::client_command command);
        void UNKNOWN(irc::client_command command);

        bool correct_number_of_parameters(irc::client_command command, size_t expected);
        void send_error_message(irc::numeric_reply error_code, const std::string& command_name, const std::string& text);
        void send_error_message(irc::numeric_reply error_code, const std::string& nickname, const std::string& channel_name,  const std::string& text);
        void broadcast_to_users(const std::unordered_set<std::string>& user_list, const message& msg, const std::string& exclude);
};

std::string generate_who_response(const std::string& requesting_nick, const std::vector<client_info>& clients, const std::string& channel);