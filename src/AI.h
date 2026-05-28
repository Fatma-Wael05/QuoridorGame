#ifndef AI_H
#define AI_H
 
#include "QuoridorEngine.h"
#include <vector>
#include <climits>
 
enum class Difficulty { EASY, MEDIUM, HARD };
 
struct AIMove {
    bool isWall;          // true = place wall, false = move pawn
    Point target;         // destination for pawn, or wall center
    Orientation orient;   // only used if isWall = true
};
 
class AI {
public:
    AI(int playerIndex, Difficulty diff);
 
    // Call this to get the AI's chosen move
    AIMove getBestMove(QuoridorEngine& engine);
 
private:
    int playerIndex;    // 0 or 1 — which player is the AI
    Difficulty difficulty;
 
    // Minimax with alpha-beta pruning
    int minimax(QuoridorEngine& engine, int depth, int alpha, int beta, bool maximizing);
 
    // How good is the current board state for the AI player?
    int evaluate(QuoridorEngine& engine);
 
    // Get all possible pawn moves for a player
    std::vector<Point> getPawnMoves(QuoridorEngine& engine, int playerIdx);
 
    // Get candidate wall placements (not all 128 — smart selection)
    std::vector<std::pair<Point, Orientation>> getWallMoves(QuoridorEngine& engine);
 
    // BFS shortest path length from a position to a target row
    int shortestPath(const Board& board, Point start, int targetRow);
 
    int getDepth(); // returns search depth based on difficulty
};
 
#endif