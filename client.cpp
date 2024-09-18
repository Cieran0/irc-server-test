#include "client.hpp"
#include <iostream>
#include "util.hpp"
#include "server.hpp"

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
            m_username = message.substr(5);
            std::cout << "Username " << m_username << std::endl;
        } else if (message.starts_with("USER ")) {
            int name_start = message.find_first_of(':');
            m_real_name = message.substr(name_start+1);
            std::cout << "Real name " << m_real_name << std::endl;
        } else if (message.starts_with("CAP ")) {
            std::cout << message << std::endl;
            send_message(":DESKTOP-U8PC6T7. CAP * LS :\r\n");
        }
    }

    server_controller.client_map.emplace(m_username, this);

    //TODO figure this out
    send_message(":DESKTOP-U8PC6T7. 001 "+m_username+" :Hi, welcome to IRC\r\n");
    send_message(":DESKTOP-U8PC6T7. 002 "+m_username+" :Your host is DESKTOP-U8PC6T7., running version miniircd-2.3\r\n");
    send_message(":DESKTOP-U8PC6T7. 003 "+m_username+" :This server was created sometime\r\n");
    send_message(":DESKTOP-U8PC6T7. 004 "+m_username+" DESKTOP-U8PC6T7. miniircd-2.3 o o\r\n");
    send_message(":DESKTOP-U8PC6T7. 251 "+m_username+" :There are 2 users and 0 services on 1 server\r\n");
    send_message(":DESKTOP-U8PC6T7. 422 "+m_username+" :MOTD File is missing\r\n");

    while (true)
    {
        std::string message = get_next_message();
        //TODO: do stuff when you join
        if(message.starts_with("JOIN ")) {
            std::string channel_name = message.substr(5);
            std::cout << "Channel name [" << channel_name << "]" << std::endl;
            server_controller.get_channel(channel_name).join(m_username);

            for(std::string user : server_controller.get_channel(channel_name).get_users()) {
                std::cout << "User: " << user << std::endl;
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
