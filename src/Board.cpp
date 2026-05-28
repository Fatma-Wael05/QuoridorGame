#include "Board.h"

Board::Board(int boardSize)  {
    this->size=2*boardSize-1 ;
    // Dynamic resizing: grid[size][size]
    grid.resize(size, std::vector<CellType>(size, CellType::EMPTY));
    initialization();
}


void Board::initialization() {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {

            if (i % 2 == 0 && j % 2 == 0) {
                grid[i][j] = CellType::EMPTY;
            } else {
                grid[i][j] = CellType::PATH_SPACE;
            }
        }
    }
}


bool Board::isWithinBounds(int x, int y) const {
    return (x >= 0 && x < size && y >= 0 && y < size);
}


void Board::setCellType(int x, int y, CellType cell) {
    if (isWithinBounds(x, y)) {
        grid[x][y] = cell;
    } else {
        std::cerr << "Board Error: Set out of bounds at (" << x << "," << y << ")" << std::endl;
    }
}


CellType Board::getCellType(int x, int y) const {
    if (isWithinBounds(x, y)) {
        return grid[x][y];
    }
    return CellType::EMPTY;
}

int Board::getSize() const {
    return size;
}
