#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);  // SOCK_DGRAM = UDP, not SOCK_STREAM
    if (sock_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9090);  // separate port from your TCP chat (8080)

    if (bind(sock_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << "\n";
        close(sock_fd);
        return 1;
    }

    std::cout << "UDP receiver listening on port 9090...\n";

    char buffer[1024];
    while (true) {
        sockaddr_in sender_address{};
        socklen_t sender_len = sizeof(sender_address);

        ssize_t bytes_received = recvfrom(sock_fd, buffer, sizeof(buffer) - 1, 0,
                                            (sockaddr*)&sender_address, &sender_len);
        if (bytes_received < 0) {
            std::cerr << "recvfrom failed\n";
            continue;
        }
        buffer[bytes_received] = '\0';

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender_address.sin_addr, sender_ip, sizeof(sender_ip));
        std::cout << "Received from " << sender_ip << ":" << ntohs(sender_address.sin_port)
                  << " -> " << buffer << "\n";

        // Echo it back to whoever sent it
        sendto(sock_fd, buffer, bytes_received, 0,
               (sockaddr*)&sender_address, sender_len);
    }

    close(sock_fd);
    return 0;
}