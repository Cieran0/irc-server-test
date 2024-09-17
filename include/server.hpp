#pragma once
#include <iostream>
#include <client.hpp>
#include <cstring> // for memset
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for close
#include <vector>

class server
{
private:
    std::vector<std::unique_ptr<client>> m_clients;
    std::vector<std::thread> m_threads;
    int m_file_descriptor;
    struct sockaddr_in m_address;
    int m_port;

    void handle_clients();

public:
    server(int port);
    ~server();

    int start();
};


