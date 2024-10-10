#include <client.hpp>
#include "util.hpp"
#include "server.hpp"
#include <message_builder.hpp>

/*
    Send a command related error.
*/
void client::send_error_message(irc::numeric_reply error_code, const std::string& command_name, const std::string& text) {
    message error_message = message_builder()
                                .hostname(true)
                                .code(error_code)
                                .raw(command_name, true)
                                .text(text)
                                .build();
    send_message(error_message);
}

/*
    Send a channel related error.
*/
void client::send_error_message(irc::numeric_reply error_code, const std::string& nickname, const std::string& channel_name,  const std::string& text) {
    message error_message = message_builder()
                                .hostname(true)
                                .code(error_code)
                                .raw(nickname, true)
                                .raw(channel_name, true)
                                .text(text)
                                .build();
    send_message(error_message);
}

/*
    Checks if the correct number of parameters were given.
    Sends ERR_NEEDMOREPARAMS or ERR_UNKNOWNCOMMAND if false.
*/
bool client::correct_number_of_parameters(irc::client_command command, size_t expected) {

    size_t number_of_parameters = command.arguments.size();

    if(number_of_parameters < expected) {
        send_error_message(irc::ERR_NEEDMOREPARAMS, command.name, "Not enough parameters");
        return false;
    } else if (number_of_parameters > expected) {
        send_error_message(irc::ERR_UNKNOWNCOMMAND, command.name, "Too many parameters!");
        return false;
    }

    return true;
}

/*
    Send a message to a group of users.
*/
void client::broadcast_to_users(const std::unordered_set<std::string>& user_list, const message& msg, const std::string& exclude = "") {
    for (const std::string& nickname : user_list) {
        if (nickname != exclude) {
            server::send_message_to_client(nickname, msg);
        }
    }
}

/*
    Handles the NICK command: Sets the client's nickname if it's not already set.
    Checks if the nickname is already in use, if so sends an error message.
*/
void client::NICK(irc::client_command command) {
    if(!correct_number_of_parameters(command,1))
        return;

    if(!info.nickname.empty()) {
        //Ignore if nickname already set.
        return;
    }
    
    std::string nickname = command.arguments[0]; 
    if(server::client_map.contains(nickname)) {
        message name_in_use = message_builder()
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

/*
    Handles the USER command: Sets the client's username and realname.
*/
void client::USER(irc::client_command command) {
    if(!correct_number_of_parameters(command,4))
        return;

    if(!info.username.empty() && !info.realname.empty()) {
        return;
    }

    info.username = command.arguments[0];
    info.realname = command.arguments[1];
}

/*
    Handles the CAP command.
    If the command argument is "END" it does nothing. Otherwise, sends a CAP LS response.
*/
void client::CAP(irc::client_command command) {
    if( command.arguments[0] == "END") {
        return;
    }
    message cap_ls = message_builder()
                        .hostname(true)
                        .raw("CAP * LS :",false)
                        .build();

    send_message(cap_ls);
}

/*
    Handles the JOIN command: Adds a client to a channel and sends relevant join messages to users.
    Validates that the channel name starts with '#', otherwise sends an error message.
*/
void client::JOIN(irc::client_command command)
{
    if(!correct_number_of_parameters(command,1))
        return;

    std::string channel_name = command.arguments[0];

    //Checks if valid channel name
    if(channel_name[0] != '#') {
        send_error_message(irc::ERR_CANNOTSENDTOCHAN, info.nickname, channel_name, "Invalid channel name");
        return;
    }

    server::get_channel(channel_name).join(info.nickname);
    std::unordered_set<std::string> user_list = server::get_channel(channel_name).get_users();

    message join_message = message_builder()
                                   .user_details(info)
                                   .raw("JOIN", true)
                                   .raw(channel_name, false)
                                   .build();

    send_message(join_message);

    message topic_message = message_builder()
                                    .hostname(true)
                                    .code(irc::RPL_NOTOPIC)
                                    .raw(info.nickname, true)
                                    .raw(channel_name, true)
                                    .text("No topic is set")
                                    .build();

    send_message(topic_message);

    //Gets users' nicknams in a single string with spaces
    std::ostringstream user_raw_stream;
    for (const std::string& user : user_list) {
        user_raw_stream << user << " ";
    }
    std::string user_raw = user_raw_stream.str();

    message user_list_message = message_builder()
                                        .hostname(true)
                                        .code(irc::RPL_NAMREPLY)
                                        .raw(info.nickname, false)
                                        .raw(" = ", false)
                                        .raw(channel_name, true)
                                        .text(user_raw)
                                        .build();

    send_message(user_list_message);

    message user_list_end_message = message_builder()
                                            .hostname(true)
                                            .code(irc::RPL_ENDOFNAMES)
                                            .raw(info.nickname, true)
                                            .raw(channel_name, true)
                                            .text("End of NAMES list")
                                            .build();

    send_message(user_list_end_message);

    broadcast_to_users(user_list, join_message, info.nickname);
}

/*
    Handles the PART command: Removes a client from a channel and broadcasts the PART message to the channel users.
*/
void client::PART(irc::client_command command) {

    if(!correct_number_of_parameters(command,1))
        return;

    std::string channel_name = command.arguments[0];

    //Checks if valid channel name
    if(channel_name[0] != '#') {
        send_error_message(irc::ERR_CANNOTSENDTOCHAN, info.nickname, channel_name, "Invalid channel name");
        return;
    }

    if(!server::is_user_in_channel(info.nickname, channel_name)) {
        send_error_message(irc::ERR_CANNOTSENDTOCHAN, info.nickname, channel_name, "Invalid channel name");
        return;
    }

    server::get_channel(channel_name).remove_user(info.nickname);

    message part_message = message_builder()
                                .user_details(info)
                                .raw("PART",true)
                                .raw(channel_name,false)
                                .build();

    send_message(part_message);

    std::unordered_set<std::string> user_list = server::get_channel(channel_name).get_users();

    broadcast_to_users(user_list, part_message, info.nickname);
}

/*
    Handles the PING command: Sends a PONG response with the same code received.
*/
void client::PING(irc::client_command command) {
    std::string pong_code = command.arguments[0];
    message pong_message = message_builder()
                                    .hostname(true)
                                    .raw("PONG",true)
                                    .hostname(false)
                                    .text(pong_code)
                                    .build();
    send_message(pong_message);
}

/*
    Handles the WHO command: Returns a list of users in a channel to the client.
*/
void client::WHO(irc::client_command command) {

    if(!correct_number_of_parameters(command,1))
        return;

    std::string channel_name = command.arguments[0];

    if(!server::is_user_in_channel(info.nickname, channel_name)) {
        send_error_message(irc::ERR_CANNOTSENDTOCHAN, info.nickname, channel_name, "Invalid channel name");
        return;
    }

    std::unordered_set<std::string> user_list = server::get_channel(channel_name).get_users();
    std::vector<client_info> clients_info;
    //Gets information about each client in the channel
    for (std::string user : user_list){
        clients_info.push_back(server::get_client_info(user));
    }

    message response = generate_who_response(info.nickname, clients_info, channel_name);
    send_message(response);
}

/*
    Handles MODE command.
*/
void client::MODE(irc::client_command command) {
    std::string channel_name = command.arguments[0];
    message mode_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_CHANNELMODEIS)
                            .raw(info.nickname,true)
                            .raw(channel_name,false)
                            .build();

    send_message(mode_message);
}

/*
    Handles the PRIVMSG command: Sends a private message to a user or channel.
*/
void client::PRIVMSG(irc::client_command command) {
    if(!correct_number_of_parameters(command,2))
        return;

    std::string channel_or_user = command.arguments[0];
    std::string text = command.arguments[1];

    std::unordered_set<std::string> user_list;
    if(channel_or_user[0] == '#') {
        //It it a message to a channel.
        std::string channel_name = channel_or_user;
        if(!server::is_user_in_channel(info.nickname, channel_name)) {
            send_error_message(irc::ERR_CANNOTSENDTOCHAN, info.nickname, channel_name, "Invalid channel name");
            return;
        }
        user_list = server::get_channel(channel_name).get_users();

    } else {
        //It is a direct message.
        std::string recipient = channel_or_user;
        if(!server::client_map.contains(recipient)) {
            send_error_message(irc::ERR_NOSUCHNICK, info.nickname, recipient, "Nickname not in use");
            return;
        }
        user_list.emplace(recipient);

    }

    message private_message = message_builder()
                        .user_details(info)
                        .raw("PRIVMSG",true)
                        .raw(channel_or_user,true)
                        .text(text)
                        .build();

    broadcast_to_users(user_list, private_message, info.nickname);
}

/*
    Handles the QUIT command: Sends a quit message to all clients and terminates the connection.
*/
void client::QUIT(irc::client_command command) {
    if(!correct_number_of_parameters(command, 1)) {
        return;
    }

    is_active = false;

    std::string reason = command.arguments[0];

    message quit_message =  message_builder()
                        .user_details(info)
                        .raw("QUIT", true)
                        .text(reason)
                        .build();

    //Send QUIT message to all users
    for(const std::pair<const std::string, client*> &user : server::client_map) {
        server::send_message_to_client(user.first, quit_message);
    }
    
}

/*
    Handles unknown commands: Sends an error message.
*/
void client::UNKNOWN(irc::client_command command) {
    send_error_message(irc::ERR_UNKNOWNCOMMAND, command.name, "Unknown command");
}