#include "client.hpp"
#include <iostream>
#include "util.hpp"
#include "server.hpp"
#include <numeric>

#ifdef _WIN32
    #define close closesocket
#endif

client::client(int socket) : m_socket(socket), m_is_active(true) {
    m_read_thread = std::thread(handle_reading, this);
    m_write_thread = std::thread(handle_writing, this);
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

void client::handle() {
    while(m_username.empty() || m_real_name.empty()) {
        std::string message = get_next_message();
        if(message.starts_with("NICK ")) {
            m_nickname= message.substr(5);
            std::cout << "Username " << m_nickname << std::endl;
        } else if (message.starts_with("USER ")) {
            m_username = split_string(message," ", 0)[1];
            int name_start = message.find_first_of(':');
            m_real_name = message.substr(name_start+1);
            std::cout << "Real name " << m_real_name << std::endl;
        } else if (message.starts_with("CAP ")) {
            std::cout << message << std::endl;
            send_message(":"+server_controller.host_name+" CAP * LS :\r\n");
        }
    }

    server_controller.client_map.emplace(m_nickname, this);

    //TODO figure this out
    send_message(":"+server_controller.host_name+" 001 "+m_nickname+" :Hi, welcome to IRC\r\n");
    send_message(":"+server_controller.host_name+" 002 "+m_nickname+" :Your host is "+server_controller.host_name+", running version miniircd-2.3\r\n");
    send_message(":"+server_controller.host_name+" 003 "+m_nickname+" :This server was created sometime\r\n");
    send_message(":"+server_controller.host_name+" 004 "+m_nickname+" "+server_controller.host_name+" miniircd-2.3 o o\r\n");
    send_message(":"+server_controller.host_name+" 251 "+m_nickname+" :There are 2 users and 0 services on 1 server\r\n");
    send_message(":"+server_controller.host_name+" 422 "+m_nickname+" :MOTD File is missing\r\n");

    while (true)
    {
        std::string message = get_next_message();
        //TODO: do stuff when you join
        if(message.starts_with("JOIN ")) {
            std::string channel_name = message.substr(5);
            std::cout << "Channel name [" << channel_name << "]" << std::endl;
            server_controller.get_channel(channel_name).join(m_nickname);

            send_message(":"+m_nickname+"!"+m_username+"@::1 JOIN "+channel_name+"\r\n");
            send_message(":"+server_controller.host_name+" 331 "+m_nickname+ " " + channel_name + " :No topic is set\r\n");
            std::unordered_set<std::string> user_list = server_controller.get_channel(channel_name).get_users();

            std::string user_raw = std::accumulate(user_list.begin(), user_list.end(),std::string(),
                [](const std::string& a, const std::string& b) -> std::string{
                    return a+b;
                });

            send_message(":"+server_controller.host_name+" 353 "+m_nickname + " = " + channel_name+ ":" + user_raw + "\r\n");
            send_message(":"+server_controller.host_name+" 366 "+m_nickname+ " " + channel_name+ ":End of NAMES list\r\n");

            for(std::string user : server_controller.get_channel(channel_name).get_users()) {
                std::cout << "User: " << user << std::endl;
            }
        }
        else if(message.starts_with("PING ")){
            std::string pong_code = message.substr(5);
            send_message(":"+server_controller.host_name+" PONG "+server_controller.host_name+" :" +pong_code+"\r\n");
        }
        else if(message.starts_with("WHO ")){
            std::string channel_name = message.substr(5);
            channel current_channel = server_controller.get_channel(channel_name);

            std::string user_list;
            for (std::string user : current_channel.get_users()){
                //user_list = user_list + +" "+channel_name+" "+m_username
            }
        }
    }
    
}

void client::send_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_editing_sending_buffer);
    m_sending_buffer.push(message);
}

std::string client::get_next_message() {
    std::string message;
    while (m_is_active) {
        {
            std::lock_guard<std::mutex> lock(m_editing_recieved_buffer);
            if (!m_recieved_buffer.empty()) {
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

    while (instance->m_is_active) {
        if ((bytes_received = recv(instance->m_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            {
                std::lock_guard<std::mutex> lock(instance->m_editing_recieved_buffer);
                std::vector<std::string> messages = split_string(std::string(buffer), "\r\n", false);
                for(const std::string& message : messages) {
                    instance->m_recieved_buffer.push(message);
                }
            }
        } else {
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

void client::handle_writing(client* instance) {
    while (instance->m_is_active) {
        std::string response;
        {
            std::lock_guard<std::mutex> lock(instance->m_editing_sending_buffer);
            if (!instance->m_sending_buffer.empty()) {
                response = instance->m_sending_buffer.front();
                instance->m_sending_buffer.pop();
            }
        }

        if (!response.empty()) {
            instance->send(response);
        }
    }
}

void client::send(const std::string& message) {
    ::send(m_socket, message.c_str(), message.size(), 0);
}
