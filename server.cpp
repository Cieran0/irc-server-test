#include "server.hpp"

#include <cstring>

#ifdef _WIN32
    #define close closesocket
#endif

//extern definitions
bool server::debug_mode;
int server::server_socket_fd = 0;
sockaddr_in6 server::address = {};
std::map<std::string, client*> server::client_map;
std::map<int, client> server::clients;
std::vector<std::thread> server::threads;
std::string server::host_name;
std::map<std::string,channel> server::channels;
std::queue<std::pair<int,std::string>> server::output_queue;

pollfd server::socket_fds[MAX_CLIENTS];
int server::number_of_socket_fds = 1; // Start with only the server socket

/*
    Initialises the server socket, returns 0 for success.
*/
int server::init(){

    #ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return EXIT_FAILURE;
        }
    #endif


    // Create the server socket
    server_socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_socket_fd < 0){
        std::cerr << "Error creating socket" << std::endl;

        #ifdef _WIN32
            WSACleanup();
        #endif

        return EXIT_FAILURE;
    }

    // Makes it easier to rebind to socket after restarting program
    int optval = 1;
    if(setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) < 0){
        std::cerr << "Error setting socket options" << std::endl;
        close(server_socket_fd);

        #ifdef _WIN32
            WSACleanup();
        #endif

        return EXIT_FAILURE;
    }

    // Set the socket port to PORT
    memset(&address,0,sizeof(address));
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(PORT);

    if(bind(server_socket_fd, (struct sockaddr*)&address,sizeof(address))<0){
        std::cerr << "Error binding socket" << std::endl;
        close(server_socket_fd);

        #ifdef _WIN32
            WSACleanup();
        #endif

        return EXIT_FAILURE;
    }

    if(listen(server_socket_fd, 3)<0){
        std::cerr << "Error listening on socket" << std::endl;
        close(server_socket_fd);

        #ifdef _WIN32
            WSACleanup();
        #endif

        return EXIT_FAILURE;
    }

    char hostname_buffer[256];
    if (gethostname(hostname_buffer, sizeof(hostname_buffer)) == 0) {
        host_name = std::string(hostname_buffer);
    } else {
        std::cerr << "Error getting hostname, setting to default: HOST" << std::endl;
        host_name = "HOST";
    }

    std::cout << "Server started on port: " << PORT << " (IPv6)" << std::endl;

    socket_fds[0].fd = server_socket_fd;
    socket_fds[0].events = POLLIN;

    return 0;
}

/*
    Server entry point, if debug, server will print debug information.
*/
int server::main(bool debug){
    debug_mode = debug;
    if(init() != 0) {
        return EXIT_FAILURE;
    }

    poll_loop();

    #ifdef _WIN32
        WSACleanup(); //Windows clean up
    #endif

    return 0;
}

/*
    Handles polling connections.
*/
void server::poll_loop(){
    while (true) {
        int poll_count;
        
        #ifdef _WIN32
            poll_count = WSAPoll(fds, nfds, 5000); // Windows poll
        #else
            poll_count = poll(socket_fds, number_of_socket_fds, 5000);  // Linux poll
        #endif

        if (poll_count < 0) {
            std::cerr << "Error in poll()" << std::endl;
            break;
        }

        listen_for_connections();

        handle_client_sockets();

        send_all_queued_messages();
    }
}


/*
    Handles receiving data from clients, connection errors and timeouts.
*/
void server::handle_client_sockets()
{
    for (int i = 1; i < number_of_socket_fds; ++i)
    {
        if (socket_fds[i].revents & POLLIN)
            listen_to_client(i);
        else if (socket_fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
            handle_socket_error(i);
        else
            check_client_timeout(i);
    }
}

/*
    Checks if a client has sent a message in the last 60 seconds.
    If not disconnects the client.
*/
void server::check_client_timeout(int i)
{
    int client_socket = socket_fds[i].fd;
    std::map<int, client>::iterator it = clients.find(socket_fds[i].fd);
    bool should_close = false;

    if (it == clients.end())
    {
        return;   
    }

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<double> last_active_in_seconds = now - it->second.last_active;

    // More than 1 minute not active
    it->second.is_active = last_active_in_seconds.count() < 60;

    if (!it->second.is_active)
    {
        // Close connection, and remove client from polling.
        std::cout << "Connection timed out with client, fd = " << client_socket << std::endl;
        kill_client_connection(client_socket, it, i);
    }
}

/*
    Disconnects client if a error occurs.
*/
void server::handle_socket_error(int i)
{
    std::cerr << "Error or hang-up on socket: " << socket_fds[i].fd << std::endl;
    auto it = clients.find(socket_fds[i].fd);
    client &current_client = it->second;
    kill_client_connection(socket_fds[i].fd, it, i);
}

/*
    Reads from client, disconnecting if recieves QUIT.
*/
void server::listen_to_client(int i)
{
    char buffer[1024];
    int client_socket = socket_fds[i].fd;
    std::map<int, client>::iterator it = clients.find(socket_fds[i].fd);
    bool should_close = false;

    if (it == clients.end())
    {
        return;
    }

    client &current_client = it->second;
    should_close = current_client.read_from(buffer, sizeof(buffer));

    if (should_close)
    {
        kill_client_connection(client_socket, it, i);
        return;
        
    }

    std::string messages_recieved(buffer);
    std::vector<std::string> messages = split_string(messages_recieved, "\r\n", false);
    for (const std::string &raw_message : messages)
    {
        current_client.handle_message(raw_message);
    }

    if (!current_client.is_active)
    {
        kill_client_connection(client_socket, it, i);
    }
}


/*
    Kills the connection with a client.
    Cleans up clients and client_map.
*/
void server::kill_client_connection(int client_socket, const std::map<int, client>::iterator &client_it, int i)
{
    // Close connection, and remove client from polling.
    std::string nickname = client_it->second.get_info().nickname;
    close(client_socket);
    for (auto &ch : channels)
    {
        ch.second.remove_user(nickname);
    }
    clients.erase(client_it);
    client_map.erase(nickname);

    if (i != number_of_socket_fds - 1)
    {
        socket_fds[i].fd = socket_fds[number_of_socket_fds - 1].fd;
        socket_fds[i].events = socket_fds[number_of_socket_fds - 1].events;
    }
    number_of_socket_fds--;
    std::cout << "Closed connection with client socket: " << client_socket << std::endl;
}

/*
    Listens on the Server socket, accepting new clients.
*/
void server::listen_for_connections()
{
    if (socket_fds[0].revents & POLLIN)
    {
        struct sockaddr_in6 client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket < 0)
        {
            std::cerr << "Error accepting client" << std::endl;
            return;
        }

        if (number_of_socket_fds < MAX_CLIENTS)
        {
            socket_fds[number_of_socket_fds].fd = client_socket;
            socket_fds[number_of_socket_fds].events = POLLIN; 
            number_of_socket_fds++;

            char ip_buffer[INET6_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET6, &client_addr.sin6_addr, ip_buffer, sizeof(ip_buffer));
            std::cout << "New connection from IP: [" << ip_buffer << "]" << std::endl;
            clients.try_emplace(client_socket, client_socket, std::string(ip_buffer));
        }
        else
        {
            std::cerr << "Too many clients" << std::endl;
            close(client_socket);
        }
    }
}

/*
    Empties the output queue, sending messages to clients.
*/
void server::send_all_queued_messages() {

    while (!output_queue.empty())
    {
        int client_socket = output_queue.front().first;
        message to_send = output_queue.front().second;

        output_queue.pop();

        std::map<int,client>::iterator it = clients.find(client_socket);

        if (it == clients.end()){
            //Ignore messages to users who have left.
            continue;
        }

        
        if(server::debug_mode) {
            std::cout << "S [" << it->second.get_info().ip << "]: \"" << decode(to_send) << "\"" << std::endl;
        }

        send(client_socket, to_send.c_str(), to_send.size(), 0);

    }
}

client_info server::get_client_info(const std::string& client){
    return client_map[client]->get_info();
}

/*
    If no channel with channel_name exists, create one.
    Returns the channel with channel_name.
*/
channel& server::get_channel(std::string channel_name){
    if(!channels.contains(channel_name)){
        channels.emplace(channel_name, channel(channel_name));
    }
    return channels[channel_name];
}

void server::add_to_client_map(std::string nickname, client* client){
    client_map.emplace(nickname, client);
}

void server::send_message_to_client(std::string nickname, message to_send){
    client_map[nickname]->send_message(to_send);
}

bool server::is_user_in_channel(std::string user, std::string channel) {
    return get_channel(channel).has_user(user);
}