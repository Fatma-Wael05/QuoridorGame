#ifndef PLAYER_H
#define PLAYER_H

#include "point.h"

class Player {
private:
    int ID;
    Point position;
    int targetRow;
    int wallCount;

public:

    Player(int id, Point start, int boardSize);

    // Getters
    int getID() const;
    Point getPosition() const;
    int getTargetRow() const;
    int getWallCount() const;

    // Setters
    void setPosition(Point pos);

    bool useWall();
    bool hasWon() const;
};

#endif
