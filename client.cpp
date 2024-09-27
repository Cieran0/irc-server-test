#include "client.hpp"
#include "server.hpp"
#include <numeric>
#include "util.hpp"
#include <message_builder.hpp>
#include <irc_numberic_replies.hpp>

#ifdef _WIN32
    #define close closesocket
#endif

client::client(int socket, std::string ip) : m_socket(socket), m_is_active(true){
    m_info.ip = ip;
}

client::~client() {
    m_is_active = false;
    if (m_read_thread.joinable()) {
        m_read_thread.join();
    }
    if (m_write_thread.joinable()) {
        m_write_thread.join();
    }
    close(m_socket);
}

void client::handle_message(std::string message){
    if(m_info.username.empty() || m_info.realname.empty()){
        if(message.starts_with("NICK ")){
            m_info.nickname = message.substr(5);
            server::add_to_client_map(m_info.nickname, this);
            std::cout << "Nickname " << m_info.nickname << std::endl;
        }
        else if(message.starts_with("USER ")){
            m_info.username = split_string(message," ",0)[1];
            int name_start = message.find_first_of(':');
            m_info.realname = message.substr(name_start+1);
            std::cout << "Real name " << m_info.realname << std::endl;
        }
        else if(message.starts_with("CAP ")){
            std::cout << message << std::endl;
            send_message(":"+server::host_name+" CAP * LS :\r\n");
        }
    }

    server::add_to_client_map(m_info.nickname, this);

        //TODO figure this out

    std::string welcome_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_WELCOME)
                            .raw(m_info.nickname, true)
                            .text("Hi, welcome to IRC",true)
                            .build();
    std::string host_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_YOURHOST)
                            .raw(m_info.nickname, true)
                            .text("Your host is ", true)
                            .hostname(false)
                            .text(", running version miniircd-2.3", false)
                            .build();
    std::string creation_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_CREATED)
                            .raw(m_info.nickname, true)
                            .text("This server was created sometime", true)
                            .build();
    std::string server_info_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_MYINFO)
                            .raw(m_info.nickname, true)
                            .hostname(false)
                            .text(" miniircd-2.3 o o", false)
                            .build();
    std::string other_users_message = message_builder()
                            .hostname(true)
                            .code(irc::RPL_LUSERCLIENT)
                            .raw(m_info.nickname, true)
                            .text("There are 2 users and 0 services on 1 server", true)
                            .build();
    std::string MOTD_message = message_builder()
                            .hostname(true)
                            .code(irc::ERR_NOMOTD)
                            .raw(m_info.nickname, true)
                            .text("MOTD File missing", true)
                            .build();

    send_message(welcome_message);
    send_message(host_message);
    send_message(creation_message);
    send_message(server_info_message);
    send_message(other_users_message);
    send_message(MOTD_message);

    {
        if(message.starts_with("JOIN ")){
            std::string channel_name = message.substr(5);
            std::cout << "Channel name [" << channel_name << "]" << std::endl;
            server::get_channel(channel_name).join(m_info.nickname);
            std::unordered_set<std::string> user_list = server::get_channel(channel_name).get_users();

            std::string join_message = ":"+m_info.nickname+"!"+m_info.username+"@::1 JOIN "+channel_name+"\r\n";

            send_message(join_message);
            send_message(":"+server::host_name+" 331 "+m_info.nickname+ " " + channel_name + " :No topic is set\r\n");

            std::string user_raw = std::accumulate(user_list.begin(), user_list.end(),std::string(),
                [](const std::string& a, const std::string& b) -> std::string{
                    return a+" "+b;
                });

            send_message(":"+server::host_name+" 353 "+m_info.nickname + " = " + channel_name+ " :" + user_raw + "\r\n");
            send_message(":"+server::host_name+" 366 "+m_info.nickname+ " " + channel_name+ " :End of NAMES list\r\n");

            for(std::string nickname : user_list) {
                if(nickname != m_info.nickname)
                    server::send_message_to_client(nickname, join_message);
            }

            for(std::string user : server::get_channel(channel_name).get_users()) {
                std::cout << "User: " << user << std::endl;
            }
        }
        else if(message.starts_with("PART ")){
            std::string channel_name = message.substr(5);
            server::get_channel(channel_name).remove_user(m_info.nickname);

            send_message(":"+m_info.nickname+"!"+m_info.username+"@::1 PART "+channel_name+"\r\n");
        }
        else if(message.starts_with("PING ")){
            std::string pong_code = message.substr(5);
            send_message(":"+server::host_name+" PONG "+server::host_name+" :" +pong_code+"\r\n");
        }
        else if(message.starts_with("WHO ")){
            std::string channel_name = message.substr(4);
            std::cout << "Channel [" << channel_name << "]" << std::endl;
            channel current_channel = server::get_channel(channel_name);

            std::string user_list;
            std::vector<client_info> clients_info;
            for (std::string user : current_channel.get_users()){
                std::cout << "USER: " << user << std::endl;
                clients_info.push_back(server::get_client_info(user));
            }
            std::string response = generate_who_response(m_info.nickname, clients_info, channel_name);
            std::cout << "Response [" << response << "]" << std::endl;
            send_message(response);
        } else if(message.starts_with("MODE ")) {
            std::string channel_name = message.substr(5);
            send_message(":" + server::host_name + " 324 "+m_info.nickname+" "+channel_name+" +\r\n");
        } else if (message.starts_with("PRIVMSG ")) {
            int text_start = message.find_first_of(":") + 1;
            std::string text = message.substr(text_start);
            std::string channel_or_user = message.substr(8, text_start - 10);
            std::cout << "ch : ["<<channel_or_user<<"]" << std::endl;
            std::cout << "txt: ["<<text<<"]" << std::endl;

            std::unordered_set<std::string> user_list;
            if(channel_or_user.starts_with("#")) {
                user_list = server::get_channel(channel_or_user).get_users();
            } else {
                user_list.emplace(channel_or_user);
            }

            std::string private_message = ":"+m_info.nickname+"!"+m_info.username+"@::1 PRIVMSG "+channel_or_user+" :" + text +"\r\n";

            for(std::string nickname : user_list) {
                if(nickname != m_info.nickname)
                    server::send_message_to_client(nickname, private_message);
            }
        }
    }
}


void client::send_message(std::string message){
    server::output_queue.emplace(m_socket, message);
}
/*
std::string client::get_next_message(){
    std::string message;
    while(m_is_active){
        {
            std::lock_guard<std::mutex> lock(m_editing_recieved_buffer);
            if(!m_recieved_buffer.empty()){
                message = m_recieved_buffer.front();
                m_recieved_buffer.pop();
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return message;
}


void client::handle_reading(client* instance) {
    char buffer[1024];
    int bytes_received = -1;

    while(instance->m_is_active) {
        bytes_received = recv(instance->m_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Received: " << buffer << std::endl;

            {
                std::lock_guard<std::mutex> lock(instance->m_editing_recieved_buffer);
                std::vector<std::string> messages = split_string(std::string(buffer), "\r\n", false);
                for (const std::string& message : messages) {
                    instance->m_recieved_buffer.push(message);
                }
            }
        } else if (bytes_received == 0) {
            // Graceful disconnect: client closed connection
            std::cout << "Client closed connection." << std::endl;
            instance->m_is_active = false;
        } else {
            // Handle socket errors
            #ifdef _WIN32
                int error_code = WSAGetLastError();
                if (error_code == WSAETIMEDOUT) {
                    std::cout << "Receive timeout (non-fatal)." << std::endl;
                    // Don't disconnect here, just continue looping
                    continue;
                } else if (error_code == WSAEWOULDBLOCK) {
                    std::cout << "Non-blocking operation could not complete (non-fatal)." << std::endl;
                    // Continue loop, this is non-fatal
                    continue;
                } else {
                    std::cerr << "Socket error: " << error_code << std::endl;
                    instance->m_is_active = false; // Fatal error
                }
            #else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::cout << "Non-blocking operation could not complete (non-fatal)." << std::endl;
                    continue;
                } else {
                    std::cerr << "Socket error: " <<  std::endl;
                    instance->m_is_active = false; // Fatal error
                }
            #endif
        }
    }
}
*/

bool client::read_from(char* buffer, size_t buffer_length) {
    bool should_close = false;
    int bytes_received = recv(m_socket, buffer, buffer_length - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << "Received from client: " << buffer << std::endl;

    } else if (bytes_received == 0) {
        // Client disconnected
        std::cout << "Client disconnected" << std::endl;
        should_close = true;

    } else {
        // Handle socket error
        std::cerr << "Socket error on client: " << m_socket << std::endl;
        should_close = true;
    }
    return should_close;
}

/*
void client::handle_reading(client* instance){
    char buffer[1024];
    int bytes_received = -1;

    while(instance->m_is_active){
        if((bytes_received = recv(instance->m_socket, buffer, sizeof(buffer) -1,0))>0){
            buffer[bytes_received] = '\0';
            std::cout << "received: " << buffer << std::endl;
            {
                std::lock_guard<std::mutex> lock(instance->m_editing_recieved_buffer);
                std::vector<std::string> messages = split_string(std::string(buffer), "\r\n", false);
                for(const std::string& message: messages){
                    instance->m_recieved_buffer.push(message);
                }
            }
        }
        else{
#ifdef _WIN32
            int error_code = WSAGetLastError();
            if (error_code == WSAETIMEDOUT) {
                std::cout << "Receive timeout" << std::endl;
            }
#endif
            instance->m_is_active = false;
            std::cout << "Client disconnected" << std::endl;
        }
    }
}

void client::handle_writing(client* instance){
    while(instance->m_is_active){
        std::string response;
        {
            std::lock_guard<std::mutex> lock(instance->m_editing_sending_buffer);
            if(!instance->m_sending_buffer.empty()){
                response = instance->m_sending_buffer.front();
                instance->m_sending_buffer.pop();
            }
        }
        if(!response.empty()){
            instance->send(response);
        }
    }
}

void client::send(const std::string& message){
    ::send(m_socket, message.c_str(), message.size(), 0);
}

*/

client_info client::get_info() {
    return m_info;
}

std::string generate_who_response(const std::string& requesting_nick, const std::vector<client_info>& clients, const std::string& channel) {
    std::ostringstream response;
    
    // Loop through each client and construct the WHO response (352)
    for (const auto& client : clients) {
        response << ":" << server::host_name << " 352 " << requesting_nick << " " 
                 << channel << " " << client.username << " " 
                 << client.ip << " " << server::host_name << " " 
                 << client.nickname << " H"  // Always 'H' for "Here"
                 << " :0 " << client.realname << "\r\n";
    }
    
    // Always send the End of WHO list response (315), even if the list is empty
    response << ":" << server::host_name << " 315 " << requesting_nick << " " 
             << channel << " :End of WHO list\r\n";

    return response.str();
}