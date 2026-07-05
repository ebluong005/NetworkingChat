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
#include <unordered_map>
#include <memory>
#include "../common/protocol.hpp"
#include "../common/connect_four.hpp"


std::vector<int> client_fds;
std::unordered_map<int, std::string> client_names;
std::mutex clients_mutex; 

std::mutex write_mutex;

struct Game {
    int player_x_fd;
    int player_o_fd;
    ConnectFour board;
    std::mutex board_mutex; // protects this game's board from concurrent access by both players' threads
};

std::vector<int> waiting_queue;
std::mutex queue_mutex;

std::unordered_map<int, std::shared_ptr<Game>> fd_to_game;
std::mutex games_mutex;

void safe_send(int fd, MessageType type, const std::string& payload) {
    std::lock_guard<std::mutex> lock(write_mutex);
    send_message(fd, type, payload);
}

std::string get_username(int fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = client_names.find(fd);
    return it != client_names.end() ? it->second : "Unknown";
}

void broadcast(MessageType type, const std::string& payload, int sender_fd){
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int fd: client_fds){
        //broadcast all messages except client sending
        if (fd != sender_fd){
            safe_send(fd, type, payload);
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
    client_names.erase(client_fd);

    // condensed erase remove only for c++ 20+  :(
    //std::erase(client_fds, client_fd);
}

void handle_play_request(int fd) {
    std::shared_ptr<Game> new_game;

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (std::find(waiting_queue.begin(), waiting_queue.end(), fd) != waiting_queue.end()) {
            return; // already queued, ignore duplicate request
        }
        waiting_queue.push_back(fd);

        if (waiting_queue.size() >= 2) {
            int player_x = waiting_queue[0];
            int player_o = waiting_queue[1];
            waiting_queue.erase(waiting_queue.begin(), waiting_queue.begin() + 2);

            new_game = std::make_shared<Game>();
            new_game->player_x_fd = player_x;
            new_game->player_o_fd = player_o;
        }
    }

    if (!new_game) {
        safe_send(fd, MessageType::GAME_ERROR, "Waiting for an opponent...");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(games_mutex);
        fd_to_game[new_game->player_x_fd] = new_game;
        fd_to_game[new_game->player_o_fd] = new_game;
    }

    std::string x_name = get_username(new_game->player_x_fd);
    std::string o_name = get_username(new_game->player_o_fd);

    safe_send(new_game->player_x_fd, MessageType::GAME_START, "You are X. Opponent: " + o_name);
    safe_send(new_game->player_o_fd, MessageType::GAME_START, "You are O. Opponent: " + x_name);

    std::string board_str = new_game->board.serialize();
    safe_send(new_game->player_x_fd, MessageType::GAME_UPDATE, board_str);
    safe_send(new_game->player_o_fd, MessageType::GAME_UPDATE, board_str);
}

void handle_game_move(int fd, const std::string& payload) {
    std::shared_ptr<Game> game;
    {
        std::lock_guard<std::mutex> lock(games_mutex);
        auto it = fd_to_game.find(fd);
        if (it == fd_to_game.end()) {
            safe_send(fd, MessageType::GAME_ERROR, "You're not in a game. Type /play to start one.");
            return;
        }
        game = it->second;
    }

    std::lock_guard<std::mutex> board_lock(game->board_mutex); // protects everything below from the opponent's thread doing the same

    bool is_x = (fd == game->player_x_fd);
    if (game->board.is_x_turn() != is_x) {
        safe_send(fd, MessageType::GAME_ERROR, "Not your turn.");
        return;
    }

    int col;
    try {
        col = std::stoi(payload);
    } catch (...) {
        safe_send(fd, MessageType::GAME_ERROR, "Invalid move format.");
        return;
    }

    if (!game->board.make_move(col)) {
        safe_send(fd, MessageType::GAME_ERROR, "Illegal move.");
        return;
    }

    std::string board_str = game->board.serialize();
    safe_send(game->player_x_fd, MessageType::GAME_UPDATE, board_str);
    safe_send(game->player_o_fd, MessageType::GAME_UPDATE, board_str);

    if (game->board.is_game_over()) {
        char winner = game->board.get_winner();
        std::string result = (winner == ' ') ? "Draw!" : (winner == 'X' ? "X wins!" : "O wins!");

        safe_send(game->player_x_fd, MessageType::GAME_OVER, result);
        safe_send(game->player_o_fd, MessageType::GAME_OVER, result);

        std::lock_guard<std::mutex> lock(games_mutex);
        fd_to_game.erase(game->player_x_fd);
        fd_to_game.erase(game->player_o_fd);
    }
}



void handle_client(int client_fd){
    MessageType type;
    std::string payload;

    // create username from 1st message

    if(!receive_message(client_fd, type, payload) || type!= MessageType::USERNAME){
        close(client_fd);
        return;
    }

    std::string username = payload;

    // add client fd to client fds vector
    {
        std::lock_guard<std::mutex> lock(clients_mutex); // client_fds unlocked after client is added
        client_fds.push_back(client_fd); // adds new client to back of vector
        client_names[client_fd] = username;
    }

    std::cout << username << " connected (fd" << client_fd << ") . Total clients: " << client_fds.size() << "\n";
    broadcast(MessageType::JOIN, username, client_fd);
    

    // read and write data
    while (receive_message(client_fd, type, payload)) {
        if (type == MessageType::CHAT) {
            std::cout << username << ": " << payload << "\n";
            broadcast(MessageType::CHAT, username + ": " + payload, client_fd);
        } else if (type == MessageType::PLAY_REQUEST) {
            handle_play_request(client_fd);
        } else if (type == MessageType::GAME_MOVE) {
            handle_game_move(client_fd, payload);
        }
    }

    std::cout << username + " has left the chat\n";
    remove_client(client_fd);
    broadcast(MessageType::LEAVE, username, -1);
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
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    // Specify Server Address to connect to 
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
    if (listen(server_fd, 10) < 0){
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