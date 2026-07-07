#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in dest_address{};
    dest_address.sin_family = AF_INET;
    dest_address.sin_port = htons(9090);
    inet_pton(AF_INET, "127.0.0.1", &dest_address.sin_addr);

    std::cout << "Type messages to send (Ctrl+D to quit):\n";

    std::string line;
    char buffer[1024];
    while (std::getline(std::cin, line)) {
        sendto(sock_fd, line.c_str(), line.size(), 0,
               (sockaddr*)&dest_address, sizeof(dest_address));

        // Wait for the echo back
        sockaddr_in from_address{};
        socklen_t from_len = sizeof(from_address);
        ssize_t bytes_received = recvfrom(sock_fd, buffer, sizeof(buffer) - 1, 0,
                                            (sockaddr*)&from_address, &from_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Echo: " << buffer << "\n";
        }
    }

    close(sock_fd);
    return 0;
}