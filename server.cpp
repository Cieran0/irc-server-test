#include "server.hpp"

#include <cstring>

#ifdef _WIN32
    #define close closesocket
#endif

//extern definitions
int server::file_descriptor = 0;
struct sockaddr_in6 server::address = {};
std::map<std::string, client*> server::client_map;
std::vector<std::unique_ptr<client>> server::clients;
std::vector<std::thread> server::threads;
std::string server::host_name = "Temp_HOSTNAME";
std::map<std::string,channel> server::m_channels;
struct sockaddr_in6 server::client_addr;

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
    init();
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

    // TODO: figure out why this causes the sever to break? should get ip from client


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

    handle_clients();
    return 0;
}

void server::handle_clients(){
    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket;

    while((client_socket = accept(file_descriptor, (struct sockaddr*)&client_addr,&client_addr_len)) >= 0){
        struct timeval timeout;
        timeout.tv_sec = 60*5;
        timeout.tv_usec = 0;
        if(setsockopt(client_socket, SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout))<0){
            std::cerr << "Error setting receive timeout for client socket" << std::endl;
        }

    char ip_buffer[INET6_ADDRSTRLEN]  ={0};  

    if (client_addr.sin6_family == AF_INET6) {
        inet_ntop(AF_INET6, &client_addr.sin6_addr, ip_buffer, sizeof(ip_buffer));
    }
    std::cout << "ip: [" << ip_buffer << "]" << std::endl;

        clients.push_back(std::make_unique<client>(client_socket, std::string("::1")));
        threads.emplace_back(
            std::thread(
                [&client_pointer = clients.back()](){
                    client_pointer->handle();
                }
            )
        );
    }
}

client_info server::get_client_info(const std::string& client){
    return client_map[client]->get_info();
}

channel& server::get_channel(std::string channel_name){
    if(!m_channels.contains(channel_name)){
        m_channels.emplace(channel_name, channel(channel_name));
    }
    return m_channels[channel_name];
}

void server::add_to_client_map(std::string nickname, client* client){
    client_map.emplace(nickname, client);
}

void server::send_message_to_client(std::string nickname, std::string message){
    client_map[nickname]->send_message(message);
}