#pragma once
#include <thread>
#include <mutex>
#include <cstring> // for memset
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for close
#include <chrono>
#include <vector>
#include <string>

typedef std::chrono::_V2::system_clock::time_point time_stamp;

struct user_info {
    std::string username;
    std::string real_name;
};

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
    std::chrono::time_point<std::chrono::system_clock> m_last_active;
    std::vector<std::string> m_recieved_buffer;
    std::vector<std::string> m_sending_buffer;
    std::mutex m_editing_recieved_buffer;
    std::mutex m_editing_sending_buffer;

    void send(const std::string& message);
};