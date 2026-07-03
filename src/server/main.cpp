#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

// Shared list of connected client sockets
std::vector<int> client_fds;
std::mutex clients_mutex; 

void broadcast_message(const char* buffer, ssize_t len, int sender_fd){
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int fd: client_fds){
        //broadcast all messages except client sending
        if (fd != sender_fd){
            write(fd, buffer, len);
        }
    }
}

void remove_client(int client_fd){
    std::lock_guard<std::mutex> lock(clients_mutex);
    // erase remove block

    client_fds.erase(
        std::remove(client_fds.begin(), client_fds.end(), client_fd), 
        client_fds.end()
    );

    // condensed erase remove only for c++ 20+  :(
    //std::erase(client_fds, client_fd);
}


void handle_client(int client_fd){
    // connect client fd to client fds vector
    {
        std::lock_guard<std::mutex> lock(clients_mutex); // client_fds unlocked after client is added
        client_fds.push_back(client_fd); // adds new client to back of vector
    }

    std::cout << "Client " << client_fd << " is connected. Total clients: " << client_fds.size() << "\n";


    //read and write data
    char buffer[1024];
    while (true){
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

        if(bytes_read < 0){
            std::cerr << "Client " << client_fd << " disconnected\n";
            break;
        }

        buffer[bytes_read] = '\0'; //null terminator
        
        std::cout << "Received from " << client_fd << ": " << buffer;
        // Tag the message with sender identity before broadcasting
        std::string tagged = "Client " + std::to_string(client_fd) + ": " + buffer;
        broadcast_message(buffer, bytes_read, client_fd);
    }

    remove_client(client_fd);
    close(client_fd);
}

int main() {
    // Create a socket fer server
    // function socket(int domain, int type, int protocol) creates an endpoint server side  
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        std::cerr <<"Failed to create socket\n";
        return 1; //exit code
    }

    // Allow quick restart of ther server without "address already in use errors"
    // function int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
    // used to confgiure network timeouts, buffersizes and routing
 
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    // Specify Server Address to connect to 
    // Create struct to o store sin_family, sin_addr, and port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // listen on all local interfaces/accepts connections on any ip
    address.sin_port = htons(8080); //converts port to network byteorder

    // Checking binding   
    if(bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0){
        std::cerr << "Bind failed: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }


    // Listen for incoming connections from clients
    if (listen(server_fd,1) < 0){
        std::cerr <<"Listen failed\n";
        close(server_fd);
        return 1;
    }

    std::cout<<"Server is listening on port 8080...\n";

 

    // read from client and print/send

    while(true){
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_fd, (sockaddr*)&client_address, &client_len);
        if (client_fd < 0){
            std::cerr << "Accept failed\n";
            continue;
        }

        std::thread(handle_client, client_fd).detach();
    }

    
    close(server_fd);
    return 0;


}