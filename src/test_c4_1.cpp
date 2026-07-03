#include <iostream>
#include "common/connect_four.hpp"

int main() {
    ConnectFour game;

    // Build a board with a horizontal pair AND a vertical pair for X,
    // sharing one piece, but neither reaching 4 in a row alone
    // X X . . . . .
    // O O . . . . .
    // X . . . . . .
    // O . . . . . .
    //
    // Columns: 0,1 get pieces stacked, plus horizontal neighbors
    game.make_move(1); // X -> col 1, row 5
    game.make_move(1); // O -> col 1, row 4
    game.make_move(0); // X -> col 0, row 5 (horizontal neighbor of first X)
    game.make_move(0); // O -> col 0, row 4 (vertical neighbor, but different piece)
    game.make_move(2); // X -> col 2, row 5
    game.make_move(2); // O -> col 2, row 4

    std::cout << game.serialize() << "\n";
    std::cout << "Game over (should be 0/false): " << game.is_game_over() << "\n";

    return 0;
}