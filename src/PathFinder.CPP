
#include "PathFinder.h"



    // Checks if there's a wall between two adjacent player squares
  bool  PathFinder::isPathClearInBFS(const Board &board, Point current, Point next)
    {
        int midx = (current.x + next.x) / 2;
        int midy = (current.y + next.y) / 2;

        return (board.getCellType(midx, midy) != CellType::WALL);
    }

   bool PathFinder::hasPath(const Board &board, Point startPos, int targetRow)
    {
        int size = board.getSize();
        std::queue<Point> q;
        q.push(startPos);


        std::vector<std::vector<bool>> visited(size, std::vector<bool>(size, false));
        visited[startPos.x][startPos.y] = true;

        while (!q.empty())
        {
            Point current = q.front();
            q.pop();


            if (current.x == targetRow) return true;


            int dx[] = {2, -2, 0, 0};
            int dy[] = {0, 0, 2, -2};

            for (int i = 0; i < 4; i++)
            {
                Point next(current.x + dx[i], current.y + dy[i]);

                if (board.isWithinBounds(next.x, next.y) && !visited[next.x][next.y])
                {
                    if (isPathClearInBFS(board, current, next))
                    {
                        visited[next.x][next.y] = true;
                        q.push(next);
                    }
                }
            }
        }
        return false;
    }


