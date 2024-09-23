#include "client.hpp"
#include "server.hpp"
#include <numeric>

#ifdef _WIN32
    #define close closesocket
#endif

client::client(int socket, std::string ip) : m_socket(socket), is_active(true){
    info.ip = ip;
    read_thread = std::thread(handle_reading,this);
    write_thread = std::thread(handle_writing,this);
}

client::~client() {
    is_active = false;
    if (read_thread.joinable()) {
        read_thread.join();
    }
    if (write_thread.joinable()) {
        write_thread.join();
    }
    close(m_socket);
}

client_info client::get_info() {
    return info;
}