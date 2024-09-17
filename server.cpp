#include <server.hpp>
#include <memory>

server::server(int port) : m_port(port) 
{

}


int server::start()
{
    this->m_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (this->m_file_descriptor < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return EXIT_FAILURE;
    }

    int optval = 1;
    if (setsockopt(this->m_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(this->m_file_descriptor);
        return EXIT_FAILURE;
    }

    this->m_address.sin_family = AF_INET;
    this->m_address.sin_addr.s_addr = INADDR_ANY;
    this->m_address.sin_port = htons(m_port);

    std::cout<< "Port: " << m_port << std::endl;

    if (bind(this->m_file_descriptor, (struct sockaddr *)&this->m_address, sizeof(this->m_address)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(this->m_file_descriptor);
        return EXIT_FAILURE;
    }

    if (listen(this->m_file_descriptor, 3) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(this->m_file_descriptor);
        return EXIT_FAILURE;
    }

    std::cout << "Hey!" << std::endl;
    handle_clients();

    return 0;
}

server::~server()
{
}

void server::handle_clients()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket;

    std::cout << "FD: " << this->m_file_descriptor << std::endl;

    while ((client_socket = accept(this->m_file_descriptor, (struct sockaddr *)&client_addr, &client_addr_len)) >= 0) {
        // Detach a new thread to handle the client

        struct timeval timeout;
        timeout.tv_sec = 60*5; // Set your desired timeout value (5 mins)
        timeout.tv_usec = 0;
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            std::cerr << "Error setting receive timeout for client socket" << std::endl;
        }

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
