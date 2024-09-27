#include "server.hpp"

#include <cstring>

#ifdef _WIN32
    #define close closesocket
#endif

//extern definitions
int server::file_descriptor = 0;
struct sockaddr_in6 server::address = {};
std::map<std::string, client*> server::client_map;
std::map<int, client> server::clients;
std::vector<std::thread> server::threads;
std::string server::host_name = "Temp_HOSTNAME";
std::map<std::string,channel> server::m_channels;
std::queue<std::pair<int,std::string>> server::output_queue;

int server::init(){
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return (EXIT_FAILURE);
    }
#endif

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

int server::main(){
    if(init() != 0) {
        return EXIT_FAILURE;
    }

    handle_clients();
    return 0;
}

void server::handle_clients(){
    const int MAX_CLIENTS = 1024;
    struct pollfd fds[MAX_CLIENTS]; //TODO set this properly
    int nfds = 1;  // Initially only 1 (the server socket)
    
    // Initialize the poll structure
    fds[0].fd = file_descriptor;  // Server socket
    fds[0].events = POLLIN;  // Monitor for incoming connections

    while (true) {

        int poll_count = poll(fds, nfds, 16);  // Wait indefinitely for events

        if (poll_count < 0) {
            std::cerr << "Error in poll()" << std::endl;
            break;
        }

        // Check the server socket (listen for new connections)
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in6 client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_socket = accept(file_descriptor, (struct sockaddr*)&client_addr, &client_addr_len);

            if (client_socket < 0) {
                std::cerr << "Error accepting client" << std::endl;
                continue;
            }

            // Add new client socket to poll set
            if (nfds < MAX_CLIENTS) {
                fds[nfds].fd = client_socket;
                fds[nfds].events = POLLIN;  // Monitor for incoming data
                nfds++;

                char ip_buffer[INET6_ADDRSTRLEN] = {0}; 
                inet_ntop(AF_INET6, &client_addr.sin6_addr, ip_buffer, sizeof(ip_buffer));
                std::cout << "New connection from IP: [" << ip_buffer << "]" << std::endl;
                clients.try_emplace(client_socket, client_socket, std::string(ip_buffer));
            } else {
                std::cerr << "Too many clients" << std::endl;
                close(client_socket);
            }
        }

        // Check all other client sockets for incoming data
        for (int i = 1; i < nfds; ++i) {
            if (fds[i].revents & POLLIN) {
                char buffer[1024];
                int client_socket = fds[i].fd;
                std::map<int,client>::iterator it = clients.find(fds[i].fd);
                bool should_close = false;

                if (it == clients.end()){
                    continue;
                }

                client& current_client = it->second;
                should_close = current_client.read_from(buffer, sizeof(buffer));

                if(should_close) {

                    //close connection, and remove client from polling.

                    close(client_socket);
                    
                    for (auto& ch : m_channels)
                    {
                        ch.second.remove_user(current_client.get_info().nickname);
                    }
                    
                    clients.erase(it);
                    if (i != nfds - 1) {
                        fds[i].fd = fds[nfds - 1].fd;
                        fds[i].events = fds[nfds - 1].events;
                     }
                    nfds--;
                    std::cout << "Closed connection with client socket: " << client_socket << std::endl;
                    
                    continue;
                }   
                std::string messages_recieved(buffer);

                std::vector<std::string> messages = split_string(messages_recieved, "\r\n", false);
                for(const std::string& message: messages){
                    current_client.handle_message(message);
                }

                if(!current_client.is_active) {
                    //close connection, and remove client from polling.
                    close(client_socket);
                    for (auto& ch : m_channels)
                    {
                        ch.second.remove_user(current_client.get_info().nickname);
                    }
                    clients.erase(it);
                    if (i != nfds - 1) {
                        fds[i].fd = fds[nfds - 1].fd;
                        fds[i].events = fds[nfds - 1].events;
                     }
                    nfds--;
                    std::cout << "Closed connection with client socket: " << client_socket << std::endl;
                }
                
            } else if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                std::cerr << "Error or hang-up on socket: " << fds[i].fd << std::endl;
                close(fds[i].fd);
                auto it = clients.find(fds[i].fd);
                client& current_client = it->second;
                for (auto& ch : m_channels)
                {
                        ch.second.remove_user(current_client.get_info().nickname);
                }
                clients.erase(fds[i].fd);

                if (i != nfds - 1) {
                    fds[i].fd = fds[nfds - 1].fd;
                    fds[i].events = fds[nfds - 1].events;
                }
                nfds--;
            }
        }
    
        send_all_queued_messages();
    }
}

void server::send_all_queued_messages() {

    while (!output_queue.empty())
    {
        int client_socket = output_queue.front().first;
        std::string message = output_queue.front().second;

        output_queue.pop();

        std::map<int,client>::iterator it = clients.find(client_socket);

        if (it == clients.end()){
            //Ignore messages to users who have left.
            continue;
        }

        send(client_socket, message.c_str(), message.size(), 0);

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