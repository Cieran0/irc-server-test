#include <client.hpp>
#include "util.hpp"
#include "server.hpp"
#include <message_builder.hpp>
#include <irc_numberic_replies.hpp>

void client::NICK(irc::client_command parsedCommand) {
    info.nickname = parsedCommand.arguments[0];
    server::add_to_client_map(info.nickname, this);
    //std::cout << "Nickname " << info.nickname << std::endl;
}

void client::USER(irc::client_command parsedCommand) {
    info.username = parsedCommand.arguments[0];
    info.realname = parsedCommand.arguments[1];
    //std::cout << "Real name " << info.realname << std::endl;
}

void client::CAP(irc::client_command parsedCommand) {
    if( parsedCommand.arguments[0] == "END") {
        return;
    }
    send_message(":"+server::host_name+" CAP * LS :\r\n");
}

void client::JOIN(irc::client_command parsedCommand) {
    std::string channel_name = parsedCommand.arguments[0];
            //std::cout << "Channel name [" << channel_name << "]" << std::endl;

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
                //std::cout << "User: " << user << std::endl;
            }
}

void client::PART(irc::client_command parsedCommand) {
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

void client::PING(irc::client_command parsedCommand) {
    std::string pong_code = parsedCommand.arguments[0];
    std::string pong_message = message_builder()
                                    .hostname(true)
                                    .raw("PONG",true)
                                    .hostname(false)
                                    .text(pong_code,true)
                                    .build();
    send_message(pong_message);
}

void client::WHO(irc::client_command parsedCommand) {
    std::string channel_name = parsedCommand.arguments[0];
    //std::cout << "Channel [" << channel_name << "]" << std::endl;
    channel current_channel = server::get_channel(channel_name);
    std::string user_list;
    std::vector<client_info> clients_info;
    for (std::string user : current_channel.get_users()){
        clients_info.push_back(server::get_client_info(user));
    }
    std::string response = generate_who_response(info.nickname, clients_info, channel_name);
    send_message(response);
}

void client::MODE(irc::client_command parsedCommand) {
    std::string channel_name = parsedCommand.arguments[0];
    send_message(":" + server::host_name + " 324 "+info.nickname+" "+channel_name+" +\r\n");
}

void client::PRIVMSG(irc::client_command parsedCommand) {
    std::string channel_or_user = parsedCommand.arguments[0];
    std::string text = parsedCommand.arguments[1];
    //std::cout << "ch : ["<<channel_or_user<<"]" << std::endl;
    //std::cout << "txt: ["<<text<<"]" << std::endl;
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
        //std::cout << "USER: " << nickname << std::endl;
        if(nickname != info.nickname)
            server::send_message_to_client(nickname, private_message);
    }
}

void client::QUIT(irc::client_command parsedCommand) {
    is_active = false;
}
