#ifndef QUORIDORENGINE_H
#define QUORIDORENGINE_H

#include "Board.h"
#include "Player.h"
#include "PathFinder.h"
#include <vector>

class QuoridorEngine {
private:
    Player pawns[2];
    Board board;
    int currentIndex;

    void changePos(Point currentPos, Point newPos);
    void switchTurn();
    bool isPathClear(Point start, Point end);

public:
    QuoridorEngine(int boardSize);
    bool movePlayer(Point newPos);
    bool placeWall(Point center, Orientation orient);

    // Read-only access
    const Board&  getBoard()          const { return board; }
    int           getCurrentIndex()   const { return currentIndex; }
    const Player& getPlayer(int index) const { return pawns[index]; }
    bool          isGameOver()        const { return pawns[0].hasWon() || pawns[1].hasWon(); }
    int           getWinnerID()       const {
        if (pawns[0].hasWon()) return 1;
        if (pawns[1].hasWon()) return 2;
        return -1;
    }

    // Returns all valid destination squares for the current player's pawn.
    // Used by the renderer to highlight reachable squares.
    std::vector<Point> getValidMoves() const;

    // Checks if a wall CAN be placed structurally (no BFS check).
    // Used by AI to filter candidates cheaply before running Minimax.
    bool canPlaceWall(Point center, Orientation orient) const;
};

#endif
