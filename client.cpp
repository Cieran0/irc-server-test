#include "client.hpp"
#include "server.hpp"
#include "util.hpp"
#include <message_builder.hpp>
#include <irc_numberic_replies.hpp>


#ifdef _WIN32
    #define close closesocket
#endif

const std::string VERSION = "IRC-SERVER-1.0";

client::client(int socket, std::string ip) {
    last_active = std::chrono::system_clock::now();
    info.ip = ip;
    this->socket = socket;
    is_active = true;
    welcomed=false;
}

client::~client() {
    is_active = false;
}

void client::welcome() {
    
    std::string welcome_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_WELCOME)
                            .raw(info.nickname, true)
                            .text("Hi, welcome to IRC",true)
                            .build();
    std::string host_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_YOURHOST)
                            .raw(info.nickname, true)
                            .text("Your host is ", true)
                            .hostname(false)
                            .text(", running version " + VERSION, false)
                            .build();
    std::string creation_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_CREATED)
                            .raw(info.nickname, true)
                            .text("This server was created sometime", true)
                            .build();
    std::string server_info_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_MYINFO)
                            .raw(info.nickname, true)
                            .hostname(false)
                            .text(VERSION +" o o", false)
                            .build();

    std::string clients_message = "There are " + std::to_string(server::clients.size());
    std::string other_users_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_LUSERCLIENT)
                            .raw(info.nickname, true)
                            .text(clients_message + " users and 0 services on 1 server", true)
                            .build();
    std::string MOTD_message = message_builder()
                            .hostname(true)
                            .code(irc::ERR_NOMOTD)
                            .raw(info.nickname, true)
                            .text("MOTD File missing", true)
                            .build();

    
    send_message(welcome_message);
    send_message(host_message);
    send_message(creation_message);
    send_message(server_info_message);
    send_message(other_users_message);
    send_message(MOTD_message);
    welcomed = true;
    
}

void client::handle_message(std::string message){
    //std::cout << message << std::endl;
    irc::client_command parsedCommand = irc::parseClientCommand(message);

    //TODO: clean up this mess!
    if(info.username.empty() || info.realname.empty() || info.nickname.empty()){
        if("NICK" == parsedCommand.command){
            NICK(parsedCommand);
        }
        else if("USER" == parsedCommand.command){
            USER(parsedCommand);
        }
        else if("CAP" == parsedCommand.command){
            CAP(parsedCommand);
        }
        std::cout << "username: " << info.username << ", realname: " << info.realname << ", nickname: " << info.nickname << std::endl;

        if(!(info.username.empty() || info.realname.empty() || info.nickname.empty()) && !welcomed) {
            welcome();
        }
        return;
    }


    if("JOIN" == parsedCommand.command) {
        JOIN(parsedCommand);
    }
    else if("PART" == parsedCommand.command) {
        PART(parsedCommand);
    }
    else if("PING" == parsedCommand.command) {
       PING(parsedCommand);
    }
    else if("WHO" == parsedCommand.command) {
        WHO(parsedCommand);
    } 
    else if("MODE" == parsedCommand.command) {
        MODE(parsedCommand);
    } 
    else if ("PRIVMSG" == parsedCommand.command) {
        PRIVMSG(parsedCommand);
    } 
    else if ("QUIT" == parsedCommand.command) {
        QUIT(parsedCommand);
    } 
    else if("CAP" == parsedCommand.command) {
        CAP(parsedCommand);
    }
    else if("NICK" == parsedCommand.command){
        NICK(parsedCommand);
    }
    else if("USER" == parsedCommand.command){
        USER(parsedCommand);
    }
    else {
        UNKNOWN(parsedCommand);
    }
}


void client::send_message(std::string message){
    server::output_queue.emplace(socket, message);
}

bool client::read_from(char* buffer, size_t buffer_length) {
    last_active = std::chrono::system_clock::now();
    is_active = true;
    bool should_close = false;
    int bytes_received = recv(socket, buffer, buffer_length - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << "R[" << info.ip << "]: \"" << decode(std::string(buffer)) << "\"" << std::endl;

    } else if (bytes_received == 0) {
        // Client disconnected
        std::cout << "Client disconnected" << std::endl;
        should_close = true;

    } else {
        std::cerr << "Socket error on client: " << socket << std::endl;
        should_close = true;
    }
    return should_close;
}


client_info client::get_info() {
    return info;
}

std::string generate_who_response(const std::string& requesting_nick, const std::vector<client_info>& clients, const std::string& channel) {
    //TODO: clean this up
    std::ostringstream response;
    
    for (const client_info& client : clients) {
        response << ":" << server::host_name << " 352 " << requesting_nick << " " 
                 << channel << " " << client.username << " " 
                 << client.ip << " " << server::host_name << " " 
                 << client.nickname << " H"  //'H' for "Here"
                 << " :0 " << client.realname << "\r\n";
    }
    
    response << ":" << server::host_name << " 315 " << requesting_nick << " " 
             << channel << " :End of WHO list\r\n";

    return response.str();
}