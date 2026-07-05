#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <cctype>
#include "../common/protocol.hpp"

std::atomic<bool> in_game{false};

void print_board(const std::string& board) {
    std::cout << "\n";
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            std::cout << board[row * 7 + col] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "0 1 2 3 4 5 6\n\n";
}

bool is_valid_column(const std::string& s) {
    return s.size() == 1 && std::isdigit(static_cast<unsigned char>(s[0]));
}

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
            case MessageType::GAME_START:
                in_game = true;
                std::cout << "\n*** " << payload << " ***\n";
                break;
            case MessageType::GAME_UPDATE:
                print_board(payload);
                break;
            case MessageType::GAME_OVER:
                in_game = false;
                std::cout << "*** " << payload << " ***\n";
                break;
            case MessageType::GAME_ERROR:
                std::cout << "[Game] " << payload << "\n";
                break;
            default:
                break;
        }
    }
    std::cout <<"\nServer disconnected\n";
}
int main() {
    // std::cout <<"Client Skeleton alive\n";
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


    std::cout << "Connected. Type /play to start a game, or chat normally:\n";


    // Send / receive loop

    // Start 2 threads: receive message thread, sending thread
    std::thread(receive_loop, sock_fd).detach();

    
    std::string line; 
    
    while (std::getline(std::cin, line)) {
        if (line == "/play") {
            send_message(sock_fd, MessageType::PLAY_REQUEST, "");
        } else if (in_game && is_valid_column(line)) {
            send_message(sock_fd, MessageType::GAME_MOVE, line);
        } else {
            send_message(sock_fd, MessageType::CHAT, line);
        }
    }

    close(sock_fd);
    return 0;

}