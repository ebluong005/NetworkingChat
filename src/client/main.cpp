#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include "../common/protocol.hpp"

void receive_loop(int sock_fd){
    MessageType type;
    std::string payload;

    while(receive_message(sock_fd, type, payload)) {
        switch(type){
            case MessageType::CHAT:
                std::cout << payload << "\n";
                break;
            case MessageType::JOIN:
                std::cout << payload << " has joined\n";
                break;
            case MessageType::LEAVE:
                std::cout << payload << "has left\n";
                break;
            default:
                break;
        }
    }
    std::cout <<"\nServer disconnected\n";
}
int main() {
    //std::cout <<"Client Skeleton alive\n";

    // Create a socket

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0){
        std::cerr << "Failed to create socket \n";
        return 1;
    }


    // Specify server address to connect too

    // init sockaddr_in struct
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET; // defines ipv4
    server_address.sin_port = htons(8080); // sets port to 5000
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr); // set ip address to local host


    // Connect client socket to server (by creating tcp handshake between socket and server)
    if(connect(sock_fd, (sockaddr*)&server_address, sizeof(server_address)) < 0){
        std::cerr << "Connection failed: " << strerror(errno) << "\n";
        close(sock_fd);
        return 1;
    }

    std::cout << "Enter your username: ";
    std::string username;
    std::getline(std::cin, username);
    send_message(sock_fd, MessageType::USERNAME, username);
    
    std::cout << "Connected to server. Type messages (Ctrl+D to quit):\n";


    // Send / receive loop

    // Start 2 threads: receive message thread, sending thread
    std::thread(receive_loop, sock_fd).detach();

    
    std::string line; 
    char buffer[1024];

    while(std::getline(std::cin,line)){
        line += "\n";
        send_message(sock_fd, MessageType::CHAT, line);

    }

    close(sock_fd);
    return 0;

}