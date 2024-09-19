#include "server.hpp"
#include <memory>
#include <cstring>

#ifdef _WIN32
    #define close closesocket
#endif

server::server(int port) : m_port(port) {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

int server::start() {
    // Create an IPv6 socket (AF_INET6)
    this->m_file_descriptor = socket(AF_INET6, SOCK_STREAM, 0);
    if (this->m_file_descriptor < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return EXIT_FAILURE;
    }
    host_name = "HOSTNAME";

    int optval = 1;
    if (setsockopt(this->m_file_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(this->m_file_descriptor);
        return EXIT_FAILURE;
    }

    // Set up the IPv6 address struct
    memset(&this->m_address, 0, sizeof(this->m_address));
    this->m_address.sin6_family = AF_INET6;
    this->m_address.sin6_addr = in6addr_any; // Equivalent to INADDR_ANY for IPv4, but for IPv6
    this->m_address.sin6_port = htons(m_port);

    // Bind the socket to the specified port and address
    if (bind(this->m_file_descriptor, (struct sockaddr*)&this->m_address, sizeof(this->m_address)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(this->m_file_descriptor);
        return EXIT_FAILURE;
    }

    // Start listening for incoming connections
    if (listen(this->m_file_descriptor, 3) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(this->m_file_descriptor);
        return EXIT_FAILURE;
    }

    std::cout << "Server started on port: " << m_port << " (IPv6)" << std::endl;
    handle_clients();
    return 0;
}

server::~server() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void server::handle_clients() {
    struct sockaddr_in6 client_addr;  // Use sockaddr_in6 for IPv6 clients
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket;

    while ((client_socket = accept(this->m_file_descriptor, (struct sockaddr *)&client_addr, &client_addr_len)) >= 0) {
        // Set a timeout for the client socket
        struct timeval timeout;
        timeout.tv_sec = 60 * 5;  // 5 minutes
        timeout.tv_usec = 0;
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
            std::cerr << "Error setting receive timeout for client socket" << std::endl;
        }

        // Create a client object and handle it in a separate thread
        this->m_clients.push_back(std::make_unique<client>(client_socket));
        m_threads.emplace_back(
            std::thread(
                [&client_pointer = m_clients.back()]() {
                    client_pointer->handle();
                }
            )
        );
    }
}

channel& server::get_channel(std::string channel_name) {
    if(!m_channels.contains(channel_name)) {
        m_channels.emplace(channel_name, channel(channel_name));
    }
    
    return m_channels[channel_name];
}