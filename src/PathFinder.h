#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <queue>
#include <vector>
#include "Board.h"
#include "point.h"

class PathFinder
{
public:

    static bool isPathClearInBFS(const Board &board, Point current, Point next);

    static bool hasPath(const Board &board, Point startPos, int targetRow);
};

#endif
