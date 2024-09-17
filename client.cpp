#include "client.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/time.h>  // For struct timeval

#define msleep(x) usleep(x * 1000)

client::client(int socket) : m_socket(socket), m_is_active(true) {

    m_last_active = std::chrono::system_clock::now();
    m_read_thread = std::thread(handle_reading, this);
    m_write_thread = std::thread(handle_writing, this);
}

client::~client() {
    m_is_active = false;
    if (m_read_thread.joinable())
        m_read_thread.join();
    if (m_write_thread.joinable())
        m_write_thread.join();
    close(m_socket);
}

void client::handle() {
    while (m_is_active) {
        std::string message = get_next_message();
        //if (!message.empty()) {
        //    send_message(message);
        //}
    }
}

void client::send_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_editing_sending_buffer);
    m_sending_buffer.push_back(message);
}

std::string client::get_next_message() {
    std::string message;

    while (m_is_active) {
        {
            std::lock_guard<std::mutex> lock(m_editing_recieved_buffer);
            if (!m_recieved_buffer.empty()) {
                message = m_recieved_buffer.back();
                m_recieved_buffer.pop_back();
                break;
            }
        }
        msleep(10);
    }

    return message;
}

void client::handle_reading(client* instance) {
    char buffer[1024];
    int bytes_received = -1;

    while (instance->m_is_active) {
        if ((bytes_received = recv(instance->m_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Received: {" << std::endl << buffer << "}" << std::endl;
            instance->m_last_active = std::chrono::system_clock::now();

            {
                std::lock_guard<std::mutex> lock(instance->m_editing_recieved_buffer);
                instance->m_recieved_buffer.push_back(std::string(buffer));
            }
        } else {
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
                response = instance->m_sending_buffer.back();
                instance->m_sending_buffer.pop_back();
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
