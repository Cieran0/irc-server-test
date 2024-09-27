#include "client.hpp"
#include "server.hpp"
#include <numeric>
#include "util.hpp"
#include <message_builder.hpp>
#include <irc_numberic_replies.hpp>
#include <irc.hpp>

#ifdef _WIN32
    #define close closesocket
#endif

client::client(int socket, std::string ip) {
    info.ip = ip;
    this->socket = socket;
    is_active = true;
    welcomed=false;
}

client::~client() {
    is_active = false;
}

void client::handle_message(std::string message){
    irc::client_command parsedCommand = irc::parseClientCommand(message);


    if(info.username.empty() || info.realname.empty()){
        if("NICK" == parsedCommand.command){
            info.nickname = parsedCommand.arguments[0];
            server::add_to_client_map(info.nickname, this);
            std::cout << "Nickname " << info.nickname << std::endl;
        }
        else if("USER" == parsedCommand.command){
            info.username = parsedCommand.arguments[0];
            info.realname = parsedCommand.arguments[1];
            std::cout << "Real name " << info.realname << std::endl;
        }
        else if("CAP" == parsedCommand.command){
            std::cout << message << std::endl;
            send_message(":"+server::host_name+" CAP * LS :\r\n");
        }

        return;
    }

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
                            .text(", running version miniircd-2.3", false)
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
                            .text(" miniircd-2.3 o o", false)
                            .build();
    std::string other_users_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_LUSERCLIENT)
                            .raw(info.nickname, true)
                            .text("There are 2 users and 0 services on 1 server", true)
                            .build();
    std::string MOTD_message = message_builder()
                            .hostname(true)
                            .code(irc::ERR_NOMOTD)
                            .raw(info.nickname, true)
                            .text("MOTD File missing", true)
                            .build();

    if(!welcomed) {
        send_message(welcome_message);
        send_message(host_message);
        send_message(creation_message);
        send_message(server_info_message);
        send_message(other_users_message);
        send_message(MOTD_message);
        welcomed = true;
    }


    {
        if("JOIN" == parsedCommand.command){
            std::string channel_name = parsedCommand.arguments[0];
            std::cout << "Channel name [" << channel_name << "]" << std::endl;

            server::get_channel(channel_name).join(info.nickname);
            std::unordered_set<std::string> user_list = server::get_channel(channel_name).get_users();

            std::string join_message = message_builder()
                            .user_details(info)
                            .raw("JOIN", true)
                            .raw(channel_name,false)
                            .build();

            send_message(join_message);

            std::string topic_message = message_builder()
                                            .hostname(true)
                                            .code(irc::RPL_NOTOPIC)
                                            .raw(info.nickname, true)
                                            .raw(channel_name, true)
                                            .text("No topic is set",true)
                                            .build();
                                            
            send_message(topic_message);

            std::string user_raw = std::accumulate(user_list.begin(), user_list.end(),std::string(),
                [](const std::string& a, const std::string& b) -> std::string{
                    return a+" "+b;
                });

            std::string user_list_message = message_builder()
                                                .hostname(true)
                                                .code(irc::RPL_NAMREPLY)
                                                .raw(info.nickname, false)
                                                .raw(" = ",false)
                                                .raw(channel_name,true)
                                                .text(user_raw, true)
                                                .build();

            send_message(user_list_message);

            std::string user_list_end_message = message_builder()
                                                    .hostname(true)
                                                    .code(irc::RPL_ENDOFNAMES)
                                                    .raw(info.nickname, true)
                                                    .raw(channel_name,true)
                                                    .text("End of NAMES list", true)
                                                    .build();
            send_message(user_list_end_message);

            for(std::string nickname : user_list) {
                if(nickname != info.nickname)
                    server::send_message_to_client(nickname, join_message);
            }

            for(std::string user : server::get_channel(channel_name).get_users()) {
                std::cout << "User: " << user << std::endl;
            }
        }
        else if("PART" == parsedCommand.command){
            std::string channel_name = parsedCommand.arguments[0];
            server::get_channel(channel_name).remove_user(info.nickname);

            //TODO: send part message to everyone in channel

            std::string part_message = message_builder()
                                        .user_details(info)
                                        .raw("PART",true)
                                        .raw(channel_name,false)
                                        .build();

            send_message(part_message);
        }
        else if("PING" == parsedCommand.command){
            std::string pong_code = parsedCommand.arguments[0];

            std::string pong_message = message_builder()
                                            .hostname(true)
                                            .raw("PONG",true)
                                            .hostname(false)
                                            .text(pong_code,true)
                                            .build();

            send_message(pong_message);
        }
        else if("WHO" == parsedCommand.command){
            std::string channel_name = parsedCommand.arguments[0];

            std::cout << "Channel [" << channel_name << "]" << std::endl;
            channel current_channel = server::get_channel(channel_name);

            std::string user_list;
            std::vector<client_info> clients_info;
            for (std::string user : current_channel.get_users()){
                clients_info.push_back(server::get_client_info(user));
            }

            std::string response = generate_who_response(info.nickname, clients_info, channel_name);
            send_message(response);

        } else if("MODE" == parsedCommand.command) {

            std::string channel_name = message.substr(5);
            send_message(":" + server::host_name + " 324 "+info.nickname+" "+channel_name+" +\r\n");

        } else if ("PRIVMSG" == parsedCommand.command) {

            std::string channel_or_user = parsedCommand.arguments[0];
            std::string text = parsedCommand.arguments[1];
            std::cout << "ch : ["<<channel_or_user<<"]" << std::endl;
            std::cout << "txt: ["<<text<<"]" << std::endl;

            std::unordered_set<std::string> user_list;
            if(channel_or_user[0] == '#') {
                //Its a channel
                user_list = server::get_channel(channel_or_user).get_users();
            } else {
                //Its a dm
                user_list.emplace(channel_or_user);
            }

            std::string private_message = message_builder()
                                .user_details(info)
                                .raw("PRIVMSG",true)
                                .raw(channel_or_user,true)
                                .text(text,true)
                                .build();

            for(std::string nickname : user_list) {
                std::cout << "USER: " << nickname << std::endl;
                if(nickname != info.nickname)
                    server::send_message_to_client(nickname, private_message);
            }
        } else if ("QUIT" == parsedCommand.command) {
            is_active = false;
        }
    }
}


void client::send_message(std::string message){
    server::output_queue.emplace(socket, message);
}

bool client::read_from(char* buffer, size_t buffer_length) {
    bool should_close = false;
    int bytes_received = recv(socket, buffer, buffer_length - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << "Received from client: " << buffer << std::endl;

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