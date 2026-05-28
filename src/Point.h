#ifndef GAME_ELEMENTS_H
#define GAME_ELEMENTS_H

struct Point {
    int x, y;
    Point(int m = 0, int n = 0) : x(m), y(n) {}

    bool operator == (const Point &pnt) const {
        return (x == pnt.x && y == pnt.y);
    }

    bool operator != (const Point &pnt) const {
        return !(*this == pnt); // Corrected syntax
    }
};

enum class CellType {
    EMPTY, PAWN, WALL, PATH_SPACE
};

enum class Orientation {
    HORIZONTAL, VERTICAL, NONE
};

#endif
