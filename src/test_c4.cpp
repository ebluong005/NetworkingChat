#include <iostream>
#include "common/connect_four.hpp"

int main() {
    ConnectFour game;
    // Simulate X winning with a vertical stack in column 0
    game.make_move(0); // X
    game.make_move(1); // O
    game.make_move(0); // X
    game.make_move(1); // O
    game.make_move(0); // X
    game.make_move(1); // O
    game.make_move(0); // X -> should win vertically

    std::cout << game.serialize() << "\n";
    std::cout << "Game over: " << game.is_game_over() << "\n";
    std::cout << "Winner: " << game.get_winner() << "\n";
    return 0;
}