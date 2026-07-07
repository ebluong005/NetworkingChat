cmake_minimum_required(VERSION 3.20)
project(ChatApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server src/server/main.cpp)
add_executable(client src/client/main.cpp)
add_executable(udp_receiver src/udp_test/receiver.cpp)
add_executable(udp_sender src/udp_test/sender.cpp)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)