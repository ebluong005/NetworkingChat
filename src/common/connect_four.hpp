#pragma once

#include <array>
#include <string>

class ConnectFour{

public:
    static constexpr int ROWS = 6;
    static constexpr int COLS = 7;

    //init game state
    ConnectFour() {
        board.fill('.');
        x_turn = true;
        game_over = false;
        winner = ' ';
    }

    bool make_move(int col){
        if (game_over) return false;
        if (col < 0 || col >= COLS) return false;
    

        int row = lowest_open_row(col);
        if (row == -1) return false; // column full

        char piece;
        if(x_turn){
            piece = 'X';
        }
        else{
            piece = 'O';
        }

        board[row * COLS + col] = piece;

        if (check_win(row, col, piece)){
            game_over = true;
            winner = piece;
        } else if (is_full()){
            game_over = true;
            winner = ' '; // draw
        } else {
            x_turn = !x_turn;
        }
        return true;
    }

    bool is_game_over() const {
        return game_over;
    }

    char get_winner() const {
        return winner;
    }

    bool is_x_turn() const {
        return x_turn;
    }

    std:: string serialize() const {
        return std::string(board.begin(), board.end());
    }

private:
    std::array<char, ROWS * COLS> board;
    bool x_turn;
    bool game_over;
    char winner;

    // helper functions for make move

    int lowest_open_row(int col) const {
        for(int row = ROWS - 1; row >= 0; --row) {
            if(board[row * COLS + col] == '.'){
                return row;
            }
        }
        return -1; // column is full
    }

    bool is_full() const {
        for (char c: board){
            if (c == '.'){
                return false;
            }
        }
        return true;
    }

    bool check_win(int row, int col, char piece) const{
        static const int directions[4][2] = {
            {0, 1},  // horizontal
            {1, 0},  // vertical
            {1, 1},  // diag down right
            {1, -1}, // diag down-left
        };

       

        for (auto& dir: directions){
            int dr = dir[0];
            int dc = dir[1];

            int count = 1; // resets for each direction
            count += count_direction(row, col, dr, dc, piece);
            count += count_direction(row, col, -dr, -dc, piece);

            if (count > 3) return true;
        }
        return false;
    }

    int count_direction(int row, int col, int dr, int dc, char piece) const {
        int count = 0;
        int r = row + dr;
        int c = col + dc;

        while (r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r * COLS + c] == piece) {
            count++;
            r += dr;
            c += dc;
        }
        return count;
    }

};