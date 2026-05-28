#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <iostream>
#include "point.h"

class Board {
private:
    int size;
    std::vector<std::vector<CellType>> grid;
    std::vector<Point> walls;

public:

    Board(int boardSize = 17);

    void initialization();
    bool isWithinBounds(int x, int y) const;

    // Setters and Getters
    void setCellType(int x, int y, CellType cell);
    CellType getCellType(int x, int y) const;
    int getSize() const;
};

#endif
