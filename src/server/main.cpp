#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>

int main() {
    // Create a socket fer server
    // function socket(int domain, int type, int protocol) creates an endpoint server side
    // Parameters of socket () : AF_INET ; Address Family: Internet - use IPV4 addressing
    // Parameters of socket () : SOCK_STREAM ; gives me a stream-oriented approach, connection based socket (this is what makes it tcp **note## SOCK_DGRAM gives UDP)
    // Parameters of socket () : 0 ; picks the dfault for this combination which would AF_INET + SOCK_STREAM 
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        std::cerr <<"Failed to create socket\n";
        return 1; //exit code
    }

    // Allow quick restart of ther server without "address already in use errors"
    // function int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
    // used to confgiure network timeouts, buffersizes and routing
    // Parameter of setsockopt(): sockfd; The file descriptor of the socket you want to configure.
    // Parameter setsockopt(): level; The protocol layer where the option resides (e.g., `SOL_SOCKET` for general socket options, `IPPROTO_TCP` for TCP-specific options).
    // Parameter setsockopt(): optname; The specific setting you want to change (e.g., `SO_REUSEADDR`).
    // Parameter setsockopt(): optval; A pointer to the value you want to assign to the option.
    // Parameter setsockopt(): optlen; The size of the memory space pointed to by `optval`.
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    // Specify Server Address to connect to 
    // Create struct to o store sin_family, sin_addr, and port
    sockaddr_in address{};
    address.sin_family = AF_INET;

    // binds all network interfaces to this machine
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

    // Accept one client connection
    sockaddr_in client_address{};
    socklen_t client_len = sizeof(client_address);
    int client_fd = accept(server_fd, (sockaddr*)&client_address , &client_len);
    if(client_fd < 0){
        std::cerr <<"Accept failed\n";
        close(server_fd);
        return 1;
    }

    std::cout<<"Client connected\n";

    // read from client and print/send

    char buffer[1024];

    while(true){
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) -1);
        if(bytes_read <= 0){
            std::cout << "Client disconnected\n";
            break;
        }
        buffer[bytes_read] = '\0';
        std::cout << "Received: " << buffer;

        //print out what client said
        write(client_fd, buffer, bytes_read);
    }

    close(client_fd);
    close(server_fd);
    return 0;


}