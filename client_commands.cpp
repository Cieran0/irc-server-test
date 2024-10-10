#include <client.hpp>
#include "util.hpp"
#include "server.hpp"
#include <message_builder.hpp>
#include <irc_numberic_replies.hpp>

bool client::correct_number_of_parameters(irc::client_command parsedCommand, size_t expected) {

    size_t got = parsedCommand.arguments.size();

    if(got < expected) {
        std::string too_few_parameters = message_builder()
                                    .hostname(true)
                                    .code(irc::ERR_NEEDMOREPARAMS)
                                    .raw(parsedCommand.command,true)
                                    .text("Not enough parameters")
                                    .build();
        send_message(too_few_parameters);
        return false;
    } else if (got > expected) {
        std::string too_many_parameters = message_builder()
                                    .hostname(true)
                                    .code(irc::ERR_UNKNOWNCOMMAND)
                                    .raw(parsedCommand.command,true)
                                    .text("Too many parameters!")
                                    .build();
        send_message(too_many_parameters);
        return false;
    }

    return true;
}

void client::NICK(irc::client_command parsedCommand) {
    if(!correct_number_of_parameters(parsedCommand,1))
        return;

    if(!info.nickname.empty()) {
        //Ignore if nickname already set.
        return;
    }
    
    std::string nickname = parsedCommand.arguments[0]; 
    if(server::client_map.contains(nickname)) {
        std::string name_in_use = message_builder()
                                    .hostname(true)
                                    .code(irc::ERR_NICKNAMEINUSE)
                                    .raw(nickname, true)
                                    .text("Nickname is already in use")
                                    .build();

        send_message(name_in_use);
        return;
    }

    info.nickname = nickname;
    server::add_to_client_map(info.nickname, this);
}

void client::USER(irc::client_command parsedCommand) {
    if(!correct_number_of_parameters(parsedCommand,4))
        return;

    if(!info.username.empty() && !info.realname.empty()) {
        return;
    }

    info.username = parsedCommand.arguments[0];
    info.realname = parsedCommand.arguments[1];
}

void client::CAP(irc::client_command parsedCommand) {
    if( parsedCommand.arguments[0] == "END") {
        return;
    }
    send_message(":"+server::host_name+" CAP * LS :\r\n");
}

void client::JOIN(irc::client_command parsedCommand)
{
    if(!correct_number_of_parameters(parsedCommand,1))
        return;

    std::string channel_name = parsedCommand.arguments[0];

    if(channel_name[0] != '#') {
    std::string bad_channel_name = message_builder()
                                    .hostname(true)
                                    .code(irc::ERR_CANNOTSENDTOCHAN)
                                    .raw(info.nickname, true)
                                    .raw(channel_name, true)
                                    .text("Invalid channel name")
                                    .build();
        send_message(bad_channel_name);
        return;
    }

    server::get_channel(channel_name).join(info.nickname);
    std::unordered_set<std::string> user_list = server::get_channel(channel_name).get_users();

    std::string join_message = message_builder()
                                   .user_details(info)
                                   .raw("JOIN", true)
                                   .raw(channel_name, false)
                                   .build();

    send_message(join_message);

    std::string topic_message = message_builder()
                                    .hostname(true)
                                    .code(irc::RPL_NOTOPIC)
                                    .raw(info.nickname, true)
                                    .raw(channel_name, true)
                                    .text("No topic is set")
                                    .build();

    send_message(topic_message);

    std::string user_raw = std::accumulate(user_list.begin(), user_list.end(), std::string(),
                                           [](const std::string &a, const std::string &b) -> std::string
                                           {
                                               return a + " " + b;
                                           });

    std::string user_list_message = message_builder()
                                        .hostname(true)
                                        .code(irc::RPL_NAMREPLY)
                                        .raw(info.nickname, false)
                                        .raw(" = ", false)
                                        .raw(channel_name, true)
                                        .text(user_raw)
                                        .build();

    send_message(user_list_message);

    std::string user_list_end_message = message_builder()
                                            .hostname(true)
                                            .code(irc::RPL_ENDOFNAMES)
                                            .raw(info.nickname, true)
                                            .raw(channel_name, true)
                                            .text("End of NAMES list")
                                            .build();

    send_message(user_list_end_message);

    for (std::string nickname : user_list)
    {
        if (nickname != info.nickname)
            server::send_message_to_client(nickname, join_message);
    }

    for (std::string user : server::get_channel(channel_name).get_users())
    {
        // std::cout << "User: " << user << std::endl;
    }
}

void client::PART(irc::client_command parsedCommand) {

    if(!correct_number_of_parameters(parsedCommand,1))
        return;

    std::string channel_name = parsedCommand.arguments[0];

    if(channel_name[0] != '#') {
        std::string bad_channel_name = message_builder()
                                        .hostname(true)
                                        .code(irc::ERR_CANNOTSENDTOCHAN)
                                        .raw(info.nickname, true)
                                        .raw(channel_name, true)
                                        .text("Invalid channel name")
                                        .build();
        send_message(bad_channel_name);
        return;
    }

    if(!server::is_user_in_channel(info.nickname, channel_name)) {
        std::string bad_channel_name = message_builder()
                                        .hostname(true)
                                        .code(irc::ERR_CANNOTSENDTOCHAN)
                                        .raw(info.nickname, true)
                                        .raw(channel_name, true)
                                        .text("Invalid channel name")
                                        .build();
        send_message(bad_channel_name);
        return;
    }

    server::get_channel(channel_name).remove_user(info.nickname);

    std::string part_message = message_builder()
                                .user_details(info)
                                .raw("PART",true)
                                .raw(channel_name,false)
                                .build();

    send_message(part_message);

    std::unordered_set<std::string> user_list = server::get_channel(channel_name).get_users();
    for(std::string nickname : user_list) {
        if(nickname != info.nickname)
            server::send_message_to_client(nickname, part_message);
    }

}

void client::PING(irc::client_command parsedCommand) {
    std::string pong_code = parsedCommand.arguments[0];
    std::string pong_message = message_builder()
                                    .hostname(true)
                                    .raw("PONG",true)
                                    .hostname(false)
                                    .text(pong_code)
                                    .build();
    send_message(pong_message);
}

void client::WHO(irc::client_command parsedCommand) {

    if(!correct_number_of_parameters(parsedCommand,1))
        return;

    std::string channel_name = parsedCommand.arguments[0];

    if(!server::is_user_in_channel(info.nickname, channel_name)) {
        std::string bad_channel_name = message_builder()
                                        .hostname(true)
                                        .code(irc::ERR_CANNOTSENDTOCHAN)
                                        .raw(info.nickname, true)
                                        .raw(channel_name, true)
                                        .text("Invalid channel name")
                                        .build();
        send_message(bad_channel_name);
        return;
    }

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
    if(!correct_number_of_parameters(parsedCommand,2))
        return;

    std::string channel_or_user = parsedCommand.arguments[0];
    std::string text = parsedCommand.arguments[1];

    std::unordered_set<std::string> user_list;
    if(channel_or_user[0] == '#') {
        //Its a channel
        if(!server::is_user_in_channel(info.nickname, channel_or_user)) {
            std::string bad_channel_name = message_builder()
                                            .hostname(true)
                                            .code(irc::ERR_CANNOTSENDTOCHAN)
                                            .raw(info.nickname, true)
                                            .raw(channel_or_user, true)
                                            .text("Invalid channel name")
                                            .build();
            send_message(bad_channel_name);
            return;
        }

        user_list = server::get_channel(channel_or_user).get_users();
    } else {
        //Its a dm
        if(!server::client_map.contains(channel_or_user)) {
            std::string bad_channel_name = message_builder()
                                            .hostname(true)
                                            .code(irc::ERR_NOSUCHNICK)
                                            .raw(info.nickname, true)
                                            .raw(channel_or_user, true)
                                            .text("Nickname not in use")
                                            .build();
            send_message(bad_channel_name);
            return;
        }
        
        user_list.emplace(channel_or_user);
    }
    std::string private_message = message_builder()
                        .user_details(info)
                        .raw("PRIVMSG",true)
                        .raw(channel_or_user,true)
                        .text(text)
                        .build();
    for(std::string nickname : user_list) {
        if(nickname != info.nickname)
            server::send_message_to_client(nickname, private_message);
    }
}

void client::QUIT(irc::client_command parsedCommand) {
    if(!correct_number_of_parameters(parsedCommand, 1)) {
        return;
    }

    is_active = false;

    std::string reason = parsedCommand.arguments[0];

    std::string quit_message =  message_builder()
                        .user_details(info)
                        .raw("QUIT", true)
                        .text(reason)
                        .build();

    for(const auto user : server::client_map) {
        server::send_message_to_client(user.first, quit_message);
    }
    
}

void client::UNKNOWN(irc::client_command parsedCommand) {
    std::string unknown_message = message_builder()
                                    .hostname(true)
                                    .code(irc::ERR_UNKNOWNCOMMAND)
                                    .raw(parsedCommand.command,true)
                                    .text("Unknown command")
                                    .build();
    send_message(unknown_message);
}