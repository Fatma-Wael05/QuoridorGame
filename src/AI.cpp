#include "AI.h"
#include <queue>
#include <vector>
#include <algorithm>

AI::AI(int playerIndex, Difficulty diff)
    : playerIndex(playerIndex), difficulty(diff) {}

int AI::getDepth() {
    switch (difficulty) {
        case Difficulty::EASY:   return 1;
        case Difficulty::MEDIUM: return 3;
        case Difficulty::HARD:   return 5;
        default:                 return 1;
    }
}

int AI::shortestPath(const Board& board, Point start, int targetRow) {
    int size = board.getSize();
    std::queue<std::pair<Point, int>> q;
    std::vector<std::vector<bool>> visited(size, std::vector<bool>(size, false));

    q.push({start, 0});
    visited[start.x][start.y] = true;

    int dx[] = {2, -2, 0, 0};
    int dy[] = {0, 0, 2, -2};

    while (!q.empty()) {
        auto [current, dist] = q.front();
        q.pop();

        if (current.x == targetRow) return dist;

        for (int i = 0; i < 4; i++) {
            Point next(current.x + dx[i], current.y + dy[i]);
            if (!board.isWithinBounds(next.x, next.y)) continue;
            if (visited[next.x][next.y]) continue;

            int midX = (current.x + next.x) / 2;
            int midY = (current.y + next.y) / 2;
            if (board.getCellType(midX, midY) == CellType::WALL) continue;

            visited[next.x][next.y] = true;
            q.push({next, dist + 1});
        }
    }
    return INT_MAX;
}

int AI::evaluate(QuoridorEngine& engine) {
    if (engine.isGameOver()) {
        int winner = engine.getWinnerID();
        return (winner == playerIndex + 1) ? 10000 : -10000;
    }

    const Board& board    = engine.getBoard();
    int          opponent = 1 - playerIndex;

    int myPath  = shortestPath(board,
        engine.getPlayer(playerIndex).getPosition(),
        engine.getPlayer(playerIndex).getTargetRow());

    int oppPath = shortestPath(board,
        engine.getPlayer(opponent).getPosition(),
        engine.getPlayer(opponent).getTargetRow());

    if (myPath  == INT_MAX) return -9000;
    if (oppPath == INT_MAX) return  9000;

    int score = (oppPath - myPath) * 10;
    score += (engine.getPlayer(playerIndex).getWallCount()
            - engine.getPlayer(opponent).getWallCount()) * 3;
    return score;
}

std::vector<Point> AI::getPawnMoves(QuoridorEngine& engine, int playerIdx) {
    std::vector<Point> moves;
    Point pos    = engine.getPlayer(playerIdx).getPosition();
    Point oppPos = engine.getPlayer(1 - playerIdx).getPosition();

    // The 4 orthogonal directions (in internal 17x17 coords, one step = 2 units)
    int dx[] = {2, -2, 0, 0};
    int dy[] = {0, 0, 2, -2};

    for (int i = 0; i < 4; i++) {
        Point next(pos.x + dx[i], pos.y + dy[i]);
        if (!engine.getBoard().isWithinBounds(next.x, next.y)) continue;

        // Check wall between current pos and next
        int midX = (pos.x + next.x) / 2;
        int midY = (pos.y + next.y) / 2;
        if (engine.getBoard().getCellType(midX, midY) == CellType::WALL) continue;

        if (engine.getBoard().getCellType(next.x, next.y) == CellType::EMPTY) {
            // Normal move — square is free
            moves.push_back(next);
            continue;
        }

        // next square is occupied by opponent
        if (next == oppPos) {
            // Try straight jump (one more step in same direction)
            Point jump(pos.x + dx[i] * 2, pos.y + dy[i] * 2);
            if (engine.getBoard().isWithinBounds(jump.x, jump.y)) {
                int jmidX = (next.x + jump.x) / 2;
                int jmidY = (next.y + jump.y) / 2;
                if (engine.getBoard().getCellType(jmidX, jmidY) != CellType::WALL &&
                    engine.getBoard().getCellType(jump.x, jump.y) == CellType::EMPTY) {
                    moves.push_back(jump);
                    continue; // straight jump available — no diagonal needed
                }
            }

            // Straight jump blocked (wall behind opp, or out of bounds)
            // Try diagonal moves — perpendicular to the direction we came from
            // BUG FIX 3: Correct perpendicular filter.
            // We want directions that are PERPENDICULAR to (dx[i], dy[i]).
            // A direction (pdx, pdy) is perpendicular if it moves on the OTHER axis.
            // If we move vertically   (dx[i]!=0, dy[i]==0) -> perp moves have dx==0
            // If we move horizontally (dx[i]==0, dy[i]!=0) -> perp moves have dy==0
            int perp_dx[] = {0, 0,  2, -2};
            int perp_dy[] = {2, -2, 0,  0};

            for (int p = 0; p < 4; p++) {
                // Skip if not truly perpendicular to our movement direction
                // Moving vertically (dx[i]!=0): perpendicular must have perp_dx==0
                // Moving horizontally (dy[i]!=0): perpendicular must have perp_dy==0
                if (dx[i] != 0 && perp_dx[p] != 0) continue; // same axis as vertical move
                if (dy[i] != 0 && perp_dy[p] != 0) continue; // same axis as horizontal move

                Point diag(next.x + perp_dx[p], next.y + perp_dy[p]);
                if (!engine.getBoard().isWithinBounds(diag.x, diag.y)) continue;

                int dmidX = (next.x + diag.x) / 2;
                int dmidY = (next.y + diag.y) / 2;
                if (engine.getBoard().getCellType(dmidX, dmidY) != CellType::WALL &&
                    engine.getBoard().getCellType(diag.x, diag.y) == CellType::EMPTY) {
                    moves.push_back(diag);
                }
            }
        }
    }
    return moves;
}

std::vector<std::pair<Point, Orientation>> AI::getWallMoves(QuoridorEngine& engine) {
    std::vector<std::pair<Point, Orientation>> walls;
    if (engine.getPlayer(playerIndex).getWallCount() == 0) return walls;

    const Board& board   = engine.getBoard();
    Point        oppPos  = engine.getPlayer(1 - playerIndex).getPosition();
    Point        myPos   = engine.getPlayer(playerIndex).getPosition();
    int          myDist  = shortestPath(board, myPos,  engine.getPlayer(playerIndex).getTargetRow());
    int          oppDist = shortestPath(board, oppPos, engine.getPlayer(1 - playerIndex).getTargetRow());

    // No point blocking if we're already winning the race
    if (myDist <= 2) return walls;

    const int MAX_CANDIDATES = (difficulty == Difficulty::HARD) ? 8 : 5;

    struct WallCandidate {
        Point       center;
        Orientation orient;
        int         impact;
    };
    std::vector<WallCandidate> candidates;

    int or_ = oppPos.x;
    int oc  = oppPos.y;

    for (int i = 1; i < 16; i += 2) {
        for (int j = 1; j < 16; j += 2) {
            if (std::abs(i - or_) > 4 && std::abs(j - oc) > 4) continue;

            for (Orientation orient : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
                if (!engine.canPlaceWall(Point(i, j), orient)) continue;

                Board testBoard = board;
                Point p1, p2, p3(i, j);
                if (orient == Orientation::HORIZONTAL) {
                    p1 = Point(i, j - 1); p2 = Point(i, j + 1);
                } else {
                    p1 = Point(i - 1, j); p2 = Point(i + 1, j);
                }
                testBoard.setCellType(p1.x, p1.y, CellType::WALL);
                testBoard.setCellType(p2.x, p2.y, CellType::WALL);
                testBoard.setCellType(p3.x, p3.y, CellType::WALL);

                if (!PathFinder::hasPath(testBoard, oppPos, engine.getPlayer(1 - playerIndex).getTargetRow())) continue;
                if (!PathFinder::hasPath(testBoard, myPos,  engine.getPlayer(playerIndex).getTargetRow())) continue;

                int newOppDist = shortestPath(testBoard, oppPos, engine.getPlayer(1 - playerIndex).getTargetRow());
                int impact = newOppDist - oppDist;

                if (impact > 0) {
                    candidates.push_back({Point(i, j), orient, impact});
                }
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(),
        [](const WallCandidate& a, const WallCandidate& b) {
            return a.impact > b.impact;
        });

    int count = std::min((int)candidates.size(), MAX_CANDIDATES);
    for (int i = 0; i < count; i++) {
        walls.push_back({candidates[i].center, candidates[i].orient});
    }

    return walls;
}

int AI::minimax(QuoridorEngine& engine, int depth, int alpha, int beta, bool maximizing) {
    if (depth == 0 || engine.isGameOver())
        return evaluate(engine);

    int currentPlayer = maximizing ? playerIndex : (1 - playerIndex);

    if (maximizing) {
        int maxScore = INT_MIN;

        for (Point move : getPawnMoves(engine, currentPlayer)) {
            QuoridorEngine copy = engine;
            if (copy.movePlayer(move)) {
                int score = minimax(copy, depth - 1, alpha, beta, false);
                maxScore  = std::max(maxScore, score);
                alpha     = std::max(alpha, score);
                if (beta <= alpha) break;
            }
        }

        if (difficulty != Difficulty::EASY) {
            for (auto& [center, orient] : getWallMoves(engine)) {
                QuoridorEngine copy = engine;
                if (copy.placeWall(center, orient)) {
                    int score = minimax(copy, depth - 1, alpha, beta, false);
                    maxScore  = std::max(maxScore, score);
                    alpha     = std::max(alpha, score);
                    if (beta <= alpha) break;
                }
            }
        }
        return maxScore;

    } else {
        int minScore = INT_MAX;

        for (Point move : getPawnMoves(engine, currentPlayer)) {
            QuoridorEngine copy = engine;
            if (copy.movePlayer(move)) {
                int score = minimax(copy, depth - 1, alpha, beta, true);
                minScore  = std::min(minScore, score);
                beta      = std::min(beta, score);
                if (beta <= alpha) break;
            }
        }

        if (difficulty != Difficulty::EASY) {
            for (auto& [center, orient] : getWallMoves(engine)) {
                QuoridorEngine copy = engine;
                if (copy.placeWall(center, orient)) {
                    int score = minimax(copy, depth - 1, alpha, beta, true);
                    minScore  = std::min(minScore, score);
                    beta      = std::min(beta, score);
                    if (beta <= alpha) break;
                }
            }
        }
        return minScore;
    }
}

AIMove AI::getBestMove(QuoridorEngine& engine) {
    int depth     = getDepth();
    int bestScore = INT_MIN;

    AIMove bestMove;
    bestMove.isWall = false;
    bestMove.target = engine.getPlayer(playerIndex).getPosition();
    bestMove.orient = Orientation::NONE;

    for (Point move : getPawnMoves(engine, playerIndex)) {
        QuoridorEngine copy = engine;
        if (copy.movePlayer(move)) {
            int score = minimax(copy, depth - 1, INT_MIN, INT_MAX, false);
            if (score > bestScore) {
                bestScore       = score;
                bestMove.isWall = false;
                bestMove.target = move;
            }
        }
    }

    if (difficulty != Difficulty::EASY) {
        for (auto& [center, orient] : getWallMoves(engine)) {
            QuoridorEngine copy = engine;
            if (copy.placeWall(center, orient)) {
                int score = minimax(copy, depth - 1, INT_MIN, INT_MAX, false);
                if (score > bestScore) {
                    bestScore       = score;
                    bestMove.isWall = true;
                    bestMove.target = center;
                    bestMove.orient = orient;
                }
            }
        }
    }

    return bestMove;
}