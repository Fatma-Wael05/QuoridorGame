#include "QuoridorEngine.h"
#include <cmath>
#include <vector>

QuoridorEngine::QuoridorEngine(int boardSize) :
    board(boardSize),
    currentIndex(0),
    pawns{Player(1, Point(0, (2*boardSize-1)/2), boardSize),
          Player(2, Point(2*boardSize-2, (2*boardSize-1)/2), boardSize)}
{
    board.setCellType(pawns[0].getPosition().x, pawns[0].getPosition().y, CellType::PAWN);
    board.setCellType(pawns[1].getPosition().x, pawns[1].getPosition().y, CellType::PAWN);
}

bool QuoridorEngine::isPathClear(Point start, Point end) {
    int midx = (start.x + end.x) / 2;
    int midy = (start.y + end.y) / 2;
    return (board.getCellType(midx, midy) != CellType::WALL);
}

void QuoridorEngine::changePos(Point currentPos, Point newPos) {
    board.setCellType(currentPos.x, currentPos.y, CellType::EMPTY);
    board.setCellType(newPos.x, newPos.y, CellType::PAWN);
    pawns[currentIndex].setPosition(newPos);
}

void QuoridorEngine::switchTurn() {
    // Only switch if game is not over — keeps currentIndex pointing at winner
    if (!pawns[currentIndex].hasWon()) {
        currentIndex = (currentIndex == 0) ? 1 : 0;
    }
}

// ─── getValidMoves ────────────────────────────────────────────────────────────
// Returns all squares the current player's pawn can legally move to.
// Handles: standard moves, straight jumps, diagonal jumps.
std::vector<Point> QuoridorEngine::getValidMoves() const {
    std::vector<Point> result;

    Point pos    = pawns[currentIndex].getPosition();
    Point oppPos = pawns[1 - currentIndex].getPosition();

    int dx[] = {-2, 2, 0, 0};
    int dy[] = {0,  0, -2, 2};

    for (int i = 0; i < 4; i++) {
        Point next(pos.x + dx[i], pos.y + dy[i]);
        if (!board.isWithinBounds(next.x, next.y)) continue;

        // Check wall between current and next
        int midX = (pos.x + next.x) / 2;
        int midY = (pos.y + next.y) / 2;
        if (board.getCellType(midX, midY) == CellType::WALL) continue;

        CellType nextType = board.getCellType(next.x, next.y);

        if (nextType == CellType::EMPTY) {
            // Standard move — square is free
            result.push_back(next);

        } else if (next == oppPos) {
            // Opponent is adjacent — try straight jump first
            Point jump(pos.x + dx[i] * 2, pos.y + dy[i] * 2);
            bool straightPossible = false;

            if (board.isWithinBounds(jump.x, jump.y)) {
                int jmidX = (next.x + jump.x) / 2;
                int jmidY = (next.y + jump.y) / 2;
                if (board.getCellType(jmidX, jmidY) != CellType::WALL &&
                    board.getCellType(jump.x, jump.y) == CellType::EMPTY) {
                    result.push_back(jump);
                    straightPossible = true;
                }
            }

            // If straight jump blocked (wall or boundary), add valid diagonal moves
            if (!straightPossible) {
                // Perpendicular directions relative to direction of approach
                int perp_dx[] = {0,  0,  -2, 2};
                int perp_dy[] = {-2, 2,   0, 0};

                for (int p = 0; p < 4; p++) {
                    // Only keep truly perpendicular directions
                    if (dx[i] != 0 && perp_dx[p] != 0) continue;
                    if (dy[i] != 0 && perp_dy[p] != 0) continue;

                    Point diag(next.x + perp_dx[p], next.y + perp_dy[p]);
                    if (!board.isWithinBounds(diag.x, diag.y)) continue;

                    int dmidX = (next.x + diag.x) / 2;
                    int dmidY = (next.y + diag.y) / 2;
                    if (board.getCellType(dmidX, dmidY) != CellType::WALL &&
                        board.getCellType(diag.x, diag.y) == CellType::EMPTY) {
                        result.push_back(diag);
                    }
                }
            }
        }
    }
    return result;
}

// ─── movePlayer ───────────────────────────────────────────────────────────────
bool QuoridorEngine::movePlayer(Point newPos) {
    if (isGameOver()) return false;
    if (!board.isWithinBounds(newPos.x, newPos.y)) return false;
    if (board.getCellType(newPos.x, newPos.y) != CellType::EMPTY) return false;

    Point currentPos = pawns[currentIndex].getPosition();
    Point otherPos   = pawns[1 - currentIndex].getPosition();
    int dx = std::abs(newPos.x - currentPos.x);
    int dy = std::abs(newPos.y - currentPos.y);

    // Case 1: Standard move
    if ((dx == 2 && dy == 0) || (dx == 0 && dy == 2)) {
        if (newPos == otherPos) return false;
        if (isPathClear(currentPos, newPos)) {
            changePos(currentPos, newPos);
            switchTurn();
            return true;
        }
    }
    // Case 2: Straight jump over opponent
    else if ((dx == 4 && dy == 0) || (dx == 0 && dy == 4)) {
        Point mid((currentPos.x + newPos.x) / 2, (currentPos.y + newPos.y) / 2);
        if (mid == otherPos &&
            isPathClear(currentPos, otherPos) &&
            isPathClear(otherPos, newPos)) {
            changePos(currentPos, newPos);
            switchTurn();
            return true;
        }
    }
    // Case 3: Diagonal jump
    else if (dx == 2 && dy == 2) {
        Point midOption1(currentPos.x, newPos.y);
        Point midOption2(newPos.x, currentPos.y);
        Point mid = (midOption1 == otherPos) ? midOption1 : midOption2;

        if (mid == otherPos) {
            Point behindOther(mid.x + (mid.x - currentPos.x),
                              mid.y + (mid.y - currentPos.y));
            bool straightBlocked =
                !board.isWithinBounds(behindOther.x, behindOther.y) ||
                !isPathClear(otherPos, behindOther);

            if (isPathClear(currentPos, otherPos) &&
                isPathClear(otherPos, newPos) &&
                straightBlocked) {
                changePos(currentPos, newPos);
                switchTurn();
                return true;
            }
        }
    }

    return false;
}

// ─── placeWall ────────────────────────────────────────────────────────────────
bool QuoridorEngine::placeWall(Point center, Orientation orient) {
    if (isGameOver()) return false;

    Point p1, p2, p3 = center;
    if (orient == Orientation::HORIZONTAL) {
        p1 = Point(center.x, center.y - 1);
        p2 = Point(center.x, center.y + 1);
    } else {
        p1 = Point(center.x - 1, center.y);
        p2 = Point(center.x + 1, center.y);
    }

    if (pawns[currentIndex].getWallCount() > 0 &&
        board.isWithinBounds(p1.x, p1.y) && board.isWithinBounds(p2.x, p2.y)) {

        if (board.getCellType(p1.x, p1.y) == CellType::PATH_SPACE &&
            board.getCellType(p2.x, p2.y) == CellType::PATH_SPACE &&
            board.getCellType(p3.x, p3.y) == CellType::PATH_SPACE) {

            board.setCellType(p1.x, p1.y, CellType::WALL);
            board.setCellType(p2.x, p2.y, CellType::WALL);
            board.setCellType(p3.x, p3.y, CellType::WALL);

            if (PathFinder::hasPath(board, pawns[0].getPosition(), pawns[0].getTargetRow()) &&
                PathFinder::hasPath(board, pawns[1].getPosition(), pawns[1].getTargetRow())) {
                pawns[currentIndex].useWall();
                switchTurn();
                return true;
            } else {
                board.setCellType(p1.x, p1.y, CellType::PATH_SPACE);
                board.setCellType(p2.x, p2.y, CellType::PATH_SPACE);
                board.setCellType(p3.x, p3.y, CellType::PATH_SPACE);
            }
        }
    }
    return false;
}

// ─── canPlaceWall (structural check only, no BFS) ────────────────────────────
bool QuoridorEngine::canPlaceWall(Point center, Orientation orient) const {
    Point p1, p2, p3 = center;
    if (orient == Orientation::HORIZONTAL) {
        p1 = Point(center.x, center.y - 1);
        p2 = Point(center.x, center.y + 1);
    } else {
        p1 = Point(center.x - 1, center.y);
        p2 = Point(center.x + 1, center.y);
    }

    if (pawns[currentIndex].getWallCount() == 0) return false;
    if (!board.isWithinBounds(p1.x, p1.y)) return false;
    if (!board.isWithinBounds(p2.x, p2.y)) return false;
    if (!board.isWithinBounds(p3.x, p3.y)) return false;
    if (board.getCellType(p1.x, p1.y) != CellType::PATH_SPACE) return false;
    if (board.getCellType(p2.x, p2.y) != CellType::PATH_SPACE) return false;
    if (board.getCellType(p3.x, p3.y) != CellType::PATH_SPACE) return false;

    return true;
}
