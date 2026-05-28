#include "Player.h"
#include <iostream>

Player::Player(int id, Point start, int boardSize)
    : ID(id), position(start), wallCount(boardSize + 1) {

    int maxIndex = 2 * boardSize - 2;

    if (start.x == 0) {
        targetRow = maxIndex; // Player starting at top wants to reach bottom row (16)
    } else if (start.x == maxIndex) {
        targetRow = 0; // Player starting at bottom wants to reach top row (0)
    } else {
        std::cerr << "Warning: Player " << id << " started at an invalid row." << std::endl;
        targetRow = -1;
    }
}

int Player::getID() const { return ID; }
Point Player::getPosition() const { return position; }
int Player::getTargetRow() const { return targetRow; }
int Player::getWallCount() const { return wallCount; }
void Player::setPosition(Point pos) { position = pos; }

bool Player::useWall() {
    if (wallCount > 0) {
        wallCount--;
        return true;
    }
    return false;
}

bool Player::hasWon() const {
    // Win condition: Player's current row matches their specific target row
    return position.x == targetRow;
}
