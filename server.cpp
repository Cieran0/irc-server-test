#include "server.hpp"

#include <cstring>

#ifdef _WIN32
    #define close closesocket
#endif

#define PORT 6667

void server::init(){
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

int server::start(){
    //Create an IPv6 socket (AF_INET6)
    file_descriptor = socket(AF_INET6, SOCK_STREAM, 0);
    if(file_descriptor < 0){
        std::cerr << "Error creating socket" << std::endl;
        return EXIT_FAILURE;
    }

    int optval = 1;
    if(setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) < 0){
        std::cerr << "Error setting socket options" << std::endl;
        close(file_descriptor);
        return EXIT_FAILURE;
    }

    // Set up the IPv6 address struct
    memset(&address,0,sizeof(address));
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(PORT);

    if(bind(file_descriptor, (struct sockaddr*)&address,sizeof(address))<0){
        std::cerr << "Error binding socket" << std::endl;
        close(file_descriptor);
        return EXIT_FAILURE;
    }

    if(listen(file_descriptor,3)<0){
        std::cerr << "Error listening on socket" << std::endl;
        close(file_descriptor);
        return EXIT_FAILURE;
    }

    std::cout<< "Server started on port: " << PORT << " (IPv6)" << std::endl;

    return 0;
}

client_info server::get_client_info(const std::string& client){
    return client_map[client]->get_info();
}