#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <queue>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h> 
#endif

class client {
public:
    client(int socket);
    ~client();

    void handle();
    void send_message(const std::string& message);
    std::string get_next_message();

private:
    static void handle_reading(client* instance);
    static void handle_writing(client* instance);

    int m_socket;
    bool m_is_active;
    std::thread m_write_thread;
    std::thread m_read_thread;
    std::queue<std::string> m_recieved_buffer;
    std::queue<std::string> m_sending_buffer;
    std::mutex m_editing_recieved_buffer;
    std::mutex m_editing_sending_buffer;

    std::string m_username;
    std::string m_real_name;

    void send(const std::string& message);
};
