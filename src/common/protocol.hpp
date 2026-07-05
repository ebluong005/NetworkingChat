#pragma once

#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

enum class MessageType : uint32_t {
    USERNAME = 0,
    CHAT = 1,
    JOIN = 2,
    LEAVE = 3,
    PLAY_REQUEST = 4,   
    GAME_START   = 5,   
    GAME_MOVE    = 6,   
    GAME_UPDATE  = 7,   
    GAME_OVER    = 8,   
    GAME_ERROR   = 9,   
};


inline bool read_exact(int fd, void* buf, ssize_t len){
    char* ptr = static_cast<char*>(buf);
    size_t total_read = 0;

    while(total_read < len)
    {
        ssize_t n = read(fd, ptr + total_read, len - total_read);
        if (n <= 0){
            return false; // connection closed or error
        }
        total_read += n;
    }
    return true;
}

inline bool write_exact(int fd, const void* buf, size_t len){
    const char* ptr = static_cast<const char*>(buf);
    size_t total_written = 0;

    while(total_written < len)
    {
        ssize_t n = write(fd, ptr + total_written, len - total_written);
        if (n <= 0){
            return false; // connection closed or error
        }
        total_written += n;
    }
    return true;
}

// Sends framed message: [type][len][data]
inline bool send_message(int fd, MessageType type, const std:: string& payload){
    uint32_t net_type = htonl(static_cast<uint32_t>(type));
    uint32_t net_len = htonl(static_cast<uint32_t>(payload.size()));

    if (!write_exact(fd, &net_type, sizeof(net_type))){
        return false;
    }

    if (!write_exact(fd, &net_len, sizeof(net_len))){
        return false;
    }

    if(!payload.empty()){
       if (!write_exact(fd, payload.data(), payload.size() )){
        return false;
        } 
    }
    return true;

}

inline bool receive_message(int fd, MessageType& out_type, std::string& out_payload){
    uint32_t net_type;
    uint32_t net_len;

    if(!read_exact(fd, &net_type, sizeof(net_type))){
        return false;
    }

    if(!read_exact(fd, &net_len, sizeof(net_len))){
        return false;
    }

    uint32_t type = ntohl(net_type);
    uint32_t len = ntohl(net_len);

    // Sanity check
    if (len > 1'000'000){
        return false;
    }

    out_type = static_cast<MessageType>(type);
    out_payload.resize(len);
    if(len > 0){
        if(!read_exact(fd, out_payload.data(), len)){
            return false;
        }
    }
    return true;
}
