#include "Renderer.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

// ─── Constructor ──────────────────────────────────────────────────────────────
// Computes cellSize and gap dynamically so the board always fits in BOARD_AREA
// pixels regardless of boardSize.
//
// The board needs:   boardSize * cellSize + (boardSize-1) * gap  <= BOARD_AREA
// We fix gap = max(4, cellSize / 6) and solve for cellSize.
//
// Simple approach: divide BOARD_AREA by (boardSize + (boardSize-1)/6.0)
// then clamp gap to at least 4 pixels.
Renderer::Renderer(sf::RenderWindow& window, int boardSize)
    : window(window), boardSize(boardSize), internalSize(2 * boardSize - 1)
{
    // Total slots = boardSize cells + (boardSize-1) gaps
    // We want gap = cellSize / 6  (proportional)
    // So total = boardSize * cs + (boardSize-1) * cs/6
    //          = cs * (boardSize + (boardSize-1)/6.0)
    // => cs = BOARD_AREA / (boardSize + (boardSize-1)/6.0)
    float divisor = (float)boardSize + (float)(boardSize - 1) / 6.0f;
    cellSize = (int)(BOARD_AREA / divisor);
    gap      = std::max(4, cellSize / 6);

    // Recompute cellSize to use remaining space perfectly
    // total = boardSize * cellSize + (boardSize-1) * gap <= BOARD_AREA
    cellSize = (BOARD_AREA - (boardSize - 1) * gap) / boardSize;
}

bool Renderer::loadFont() {
    if (font.openFromFile("C:/Windows/Fonts/arial.ttf")) return true;
    if (font.openFromFile("arial.ttf")) return true;
    return false;
}

// ─── toPixel ─────────────────────────────────────────────────────────────────
// Converts internal (2*boardSize-1) grid coordinate to screen pixel.
// Even index = start of a cell. Odd index = start of the gap after that cell.
sf::Vector2f Renderer::toPixel(int internalRow, int internalCol) {
    // internalCol/2 = which visual column (0-based)
    // internalCol%2 = 0 means at the cell start, 1 means at the gap start
    float x = OFFSET_X + (internalCol / 2) * (cellSize + gap)
                       + (internalCol % 2) * cellSize;
    float y = OFFSET_Y + (internalRow / 2) * (cellSize + gap)
                       + (internalRow % 2) * cellSize;
    return sf::Vector2f(x, y);
}

// ─── drawBoard ────────────────────────────────────────────────────────────────
void Renderer::drawBoard(const QuoridorEngine& engine,
                         const std::vector<Point>& validMoves) {
    const Board& board = engine.getBoard();

    // Build valid-move lookup — use dynamic size (up to 27x27 for 14x14 board)
    std::vector<std::vector<bool>> isValid(internalSize, std::vector<bool>(internalSize, false));
    for (const Point& p : validMoves) {
        if (p.x >= 0 && p.x < internalSize && p.y >= 0 && p.y < internalSize)
            isValid[p.x][p.y] = true;
    }

    for (int i = 0; i < internalSize; i++) {
        for (int j = 0; j < internalSize; j++) {
            CellType     type = board.getCellType(i, j);
            sf::Vector2f pos  = toPixel(i, j);

            if (i % 2 == 0 && j % 2 == 0) {
                // ── Playable square ──────────────────────────────────────────
                sf::RectangleShape cell(sf::Vector2f((float)cellSize, (float)cellSize));
                cell.setPosition(pos);
                cell.setFillColor(isValid[i][j]
                    ? sf::Color(80, 200, 80)       // valid move: green
                    : sf::Color(240, 217, 181));   // normal: wood tone
                window.draw(cell);

                // Green dot in center of highlighted squares
                if (isValid[i][j]) {
                    float dotRadius = std::max(3.f, cellSize / 9.f);
                    sf::CircleShape dot(dotRadius);
                    dot.setFillColor(sf::Color(30, 140, 30));
                    dot.setPosition(sf::Vector2f(
                        pos.x + cellSize / 2.f - dotRadius,
                        pos.y + cellSize / 2.f - dotRadius
                    ));
                    window.draw(dot);
                }

            } else if (type == CellType::WALL) {
                // ── Placed wall ───────────────────────────────────────────────
                float w, h;
                if      (i % 2 == 1 && j % 2 == 0) { w = (float)cellSize; h = (float)gap; }
                else if (i % 2 == 0 && j % 2 == 1) { w = (float)gap;      h = (float)cellSize; }
                else                                { w = (float)gap;      h = (float)gap; }

                sf::RectangleShape wall(sf::Vector2f(w, h));
                wall.setPosition(pos);
                wall.setFillColor(sf::Color(180, 90, 20));
                window.draw(wall);
            }
        }
    }
}

// ─── drawPawns ────────────────────────────────────────────────────────────────
void Renderer::drawPawns(const QuoridorEngine& engine) {
    sf::Color colors[2] = {
        sf::Color(220, 50, 50),
        sf::Color(50, 100, 220)
    };

    for (int i = 0; i < 2; i++) {
        const Player& p    = engine.getPlayer(i);
        Point         pos  = p.getPosition();
        sf::Vector2f  pixel = toPixel(pos.x, pos.y);
        float radius = cellSize / 2.8f;

        // Drop shadow
        sf::CircleShape shadow(radius);
        shadow.setFillColor(sf::Color(0, 0, 0, 80));
        shadow.setPosition(sf::Vector2f(
            pixel.x + cellSize / 2.f - radius + 2.f,
            pixel.y + cellSize / 2.f - radius + 2.f
        ));
        window.draw(shadow);

        // Pawn
        sf::CircleShape pawn(radius);
        pawn.setFillColor(colors[i]);
        pawn.setOutlineColor(sf::Color::White);
        pawn.setOutlineThickness(2.f);
        pawn.setPosition(sf::Vector2f(
            pixel.x + cellSize / 2.f - radius,
            pixel.y + cellSize / 2.f - radius
        ));
        window.draw(pawn);

        // Player number label — scale font with cell size
        unsigned int fontSize = std::max(10u, (unsigned int)(cellSize / 4));
        sf::Text label(font, std::to_string(i + 1), fontSize);
        label.setFillColor(sf::Color::White);
        auto bounds = label.getLocalBounds();
        label.setPosition(sf::Vector2f(
            pixel.x + cellSize / 2.f - bounds.size.x / 2.f - 1.f,
            pixel.y + cellSize / 2.f - bounds.size.y / 2.f - 4.f
        ));
        window.draw(label);
    }
}

// ─── drawHUD ─────────────────────────────────────────────────────────────────
void Renderer::drawHUD(const QuoridorEngine& engine,
                       const std::string& statusMsg,
                       int statusAlpha,
                       bool aiThinking) {
    int current = engine.getCurrentIndex();

    // Turn indicator
    std::string turnStr  = aiThinking
        ? "AI is thinking..."
        : "Player " + std::to_string(current + 1) + "'s Turn";
    sf::Color turnColor = aiThinking
        ? sf::Color(255, 200, 50)
        : (current == 0 ? sf::Color(220, 50, 50) : sf::Color(50, 100, 220));

    sf::Text turnText(font, turnStr, 20);
    turnText.setFillColor(turnColor);
    turnText.setPosition(sf::Vector2f(OFFSET_X, 8.f));
    window.draw(turnText);

    // Wall counts
    for (int i = 0; i < 2; i++) {
        const Player& p    = engine.getPlayer(i);
        std::string   info = "P" + std::to_string(i + 1)
                           + " Walls: " + std::to_string(p.getWallCount());
        sf::Text wallText(font, info, 16);
        wallText.setFillColor(i == 0 ? sf::Color(220, 50, 50) : sf::Color(50, 100, 220));
        wallText.setPosition(sf::Vector2f(OFFSET_X + i * 280.f, 42.f));
        window.draw(wallText);
    }

    // Status message with fade
    if (!statusMsg.empty() && statusAlpha > 0) {
        bool isError = (statusMsg.find("Invalid")  != std::string::npos ||
                        statusMsg.find("Cannot")   != std::string::npos ||
                        statusMsg.find("No walls") != std::string::npos ||
                        (statusMsg.find("wall")    != std::string::npos &&
                         statusMsg.find("block")   != std::string::npos));

        uint8_t   alpha    = static_cast<uint8_t>(statusAlpha);
        sf::Color msgColor = isError
            ? sf::Color(255, 80,  80,  alpha)
            : sf::Color(80,  220, 80,  alpha);

        sf::Text statusText(font, statusMsg, 17);
        statusText.setFillColor(msgColor);
        auto  bounds    = statusText.getLocalBounds();
        float centeredX = (WIN_W - bounds.size.x) / 2.f;
        statusText.setPosition(sf::Vector2f(centeredX, 42.f));

        float pad = 8.f;
        sf::RectangleShape bg(sf::Vector2f(bounds.size.x + pad * 2, bounds.size.y + pad * 2));
        bg.setFillColor(sf::Color(0, 0, 0, std::min(alpha, (uint8_t)180)));
        bg.setPosition(sf::Vector2f(centeredX - pad, 42.f - pad / 2.f));
        window.draw(bg);
        window.draw(statusText);
    }

    // Controls hint — positioned below the board dynamically
    float boardBottom = OFFSET_Y
        + (boardSize - 1) * (cellSize + gap) + cellSize + 8.f;
    sf::Text hint(font, "Arrows:Move  Click gap:Wall  R:Reset  ESC:Menu", 13);
    hint.setFillColor(sf::Color(120, 120, 120));
    hint.setPosition(sf::Vector2f(OFFSET_X, boardBottom));
    window.draw(hint);
}

// ─── drawWinScreen ────────────────────────────────────────────────────────────
void Renderer::drawWinScreen(const QuoridorEngine& engine) {
    int winner = engine.getWinnerID();
    if (winner == -1) return;

    sf::RectangleShape overlay(sf::Vector2f((float)WIN_W, (float)WIN_H));
    overlay.setFillColor(sf::Color(0, 0, 0, 170));
    overlay.setPosition(sf::Vector2f(0.f, 0.f));
    window.draw(overlay);

    sf::Text winText(font, "Player " + std::to_string(winner) + " Wins!", 52);
    winText.setFillColor(winner == 1 ? sf::Color(220, 50, 50) : sf::Color(50, 100, 220));
    auto wb = winText.getLocalBounds();
    winText.setPosition(sf::Vector2f((WIN_W - wb.size.x) / 2.f, WIN_H / 2.f - 80.f));
    window.draw(winText);

    sf::Text restartText(font, "Press R to play again  |  ESC for menu", 22);
    restartText.setFillColor(sf::Color::White);
    auto rb = restartText.getLocalBounds();
    restartText.setPosition(sf::Vector2f((WIN_W - rb.size.x) / 2.f, WIN_H / 2.f));
    window.draw(restartText);
}

// ─── drawAll ─────────────────────────────────────────────────────────────────
void Renderer::drawAll(const QuoridorEngine& engine,
                       const std::vector<Point>& validMoves,
                       const std::string& statusMsg,
                       int statusAlpha,
                       bool aiThinking) {
    drawBoard(engine, validMoves);
    drawPawns(engine);
    drawHUD(engine, statusMsg, statusAlpha, aiThinking);
    if (engine.isGameOver())
        drawWinScreen(engine);
}