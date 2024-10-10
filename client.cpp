#include "client.hpp"
#include "server.hpp"
#include "util.hpp"
#include <message_builder.hpp>
#include <irc_numberic_replies.hpp>


#ifdef _WIN32
    #define close closesocket
#endif

const std::string VERSION = "IRC-SERVER-1.0";

/*
    Creates the client, setting last_active to now.
*/
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

/*
    Sends the welcome messages to client.
    Sets welcomed to true.
*/
void client::welcome() {
    
    std::string welcome_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_WELCOME)
                            .raw(info.nickname, true)
                            .text("Hi, welcome to IRC")
                            .build();
    std::string host_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_YOURHOST)
                            .raw(info.nickname, true)
                            .raw("Your host is", true)
                            .hostname(true)
                            .raw(", running version " + VERSION, false)
                            .build();
    std::string creation_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_CREATED)
                            .raw(info.nickname, true)
                            .text("This server was created sometime")
                            .build();
    std::string server_info_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_MYINFO)
                            .raw(info.nickname, true)
                            .hostname(false)
                            .raw(VERSION +" o o", false)
                            .build();

    std::string clients_message = "There are " + std::to_string(server::clients.size());
    std::string other_users_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_LUSERCLIENT)
                            .raw(info.nickname, true)
                            .text(clients_message + " users and 0 services on 1 server")
                            .build();
    std::string MOTD_message = message_builder()
                            .hostname(true)
                            .code(irc::ERR_NOMOTD)
                            .raw(info.nickname, true)
                            .text("MOTD File missing")
                            .build();

    
    send_message(welcome_message);
    send_message(host_message);
    send_message(creation_message);
    send_message(server_info_message);
    send_message(other_users_message);
    send_message(MOTD_message);
    welcomed = true;
    
}

/*
    Returns false if username, realname or nickname are not set.
    Otherwise returns true.
*/
bool client::info_empty() {
    return info.username.empty() || info.realname.empty() || info.nickname.empty();
}

/*
    Takes a message from a client and responds to it appropriately.
*/
void client::handle_message(std::string message){
    irc::client_command command = irc::parse_client_command(message);

    if(info_empty()){
        if("NICK" == command){
            NICK(command);
        }
        else if("USER" == command){
            USER(command);
        }
        else if("CAP" == command){
            CAP(command);
        }

        if(!info_empty() && !welcomed) {
            welcome();
        }
        return;
    }

    //Checks which command client has sent

    if("JOIN" == command) {
        JOIN(command);
    }
    else if("PART" == command) {
        PART(command);
    }
    else if("PING" == command) {
       PING(command);
    }
    else if("WHO" == command) {
        WHO(command);
    } 
    else if("MODE" == command) {
        MODE(command);
    } 
    else if ("PRIVMSG" == command) {
        PRIVMSG(command);
    } 
    else if ("QUIT" == command) {
        QUIT(command);
    } 
    else if("CAP" == command) {
        CAP(command);
    }
    else if("NICK" == command){
        NICK(command);
    }
    else if("USER" == command){
        USER(command);
    }
    //If command is unknown.
    else {
        UNKNOWN(command);
    }
}

void client::send_message(std::string message){
    server::output_queue.emplace(socket, message);
}

/*
    Reads from the client socket, returns data in buffer.
*/
bool client::read_from(char* buffer, size_t buffer_length) {
    last_active = std::chrono::system_clock::now();
    is_active = true;
    bool should_close = false;
    int bytes_received = recv(socket, buffer, buffer_length - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        if(server::debug_mode) {
            std::cout << "R [" << info.ip << "]: \"" << decode(std::string(buffer)) << "\"" << std::endl;
        }

    } else if (bytes_received == 0) {
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

/*
    Generate response to WHO #channel
*/
std::string generate_who_response(const std::string& requesting_nick, const std::vector<client_info>& clients, const std::string& channel) {
    message_builder response_builder;

    for (const client_info& client : clients) {
        response_builder.hostname(true).code(irc::RPL_WHOREPLY).raw(requesting_nick,true) 
                .raw(channel,true).raw(client.username,true)
                .raw(client.ip,true).hostname(false) 
                .raw(client.nickname,true).raw("H",true)  //'H' for "Here"
                .raw(":0",true).raw(client.realname,false).raw("\r\n",false);
    }

    response_builder.hostname(true).code(irc::RPL_ENDOFWHO).raw(requesting_nick,true)
                    .raw(channel,true).text("End of WHO list");

    return response_builder.build();
}