#include "Renderer.h"
#include <string>
#include <vector>

// These constants must exactly match pixelToBoard() in main.cpp
const int CELL_SIZE = 56;
const int GAP       = 10;
const int OFFSET_X  = 30;
const int OFFSET_Y  = 70;

Renderer::Renderer(sf::RenderWindow& window) : window(window) {}

bool Renderer::loadFont() {
    if (font.openFromFile("C:/Windows/Fonts/arial.ttf")) return true;
    if (font.openFromFile("arial.ttf")) return true;
    return false;
}

// Converts internal 17x17 grid coordinate to screen pixel position.
// Even index = cell position. Odd index = gap after the preceding cell.
sf::Vector2f Renderer::toPixel(int internalRow, int internalCol) {
    float x = OFFSET_X + (internalCol / 2) * (CELL_SIZE + GAP) + (internalCol % 2) * CELL_SIZE;
    float y = OFFSET_Y + (internalRow / 2) * (CELL_SIZE + GAP) + (internalRow % 2) * CELL_SIZE;
    return sf::Vector2f(x, y);
}

void Renderer::drawBoard(const QuoridorEngine& engine,
                         const std::vector<Point>& validMoves) {
    const Board& board = engine.getBoard();

    // Build a fast lookup set: which internal coords are valid move destinations
    // We store them as (x*100+y) for O(1) lookup without std::set overhead
    // Max internal coord is 16, so x*100+y is always unique for 0-16 range
    bool isValid[17][17] = {};
    for (const Point& p : validMoves) {
        if (p.x >= 0 && p.x < 17 && p.y >= 0 && p.y < 17)
            isValid[p.x][p.y] = true;
    }

    for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 17; j++) {
            CellType     type = board.getCellType(i, j);
            sf::Vector2f pos  = toPixel(i, j);

            if (i % 2 == 0 && j % 2 == 0) {
                // ── Playable square ──────────────────────────────────────────
                sf::RectangleShape cell(sf::Vector2f((float)CELL_SIZE, (float)CELL_SIZE));
                cell.setPosition(pos);

                if (isValid[i][j]) {
                    // Valid move destination: bright green highlight
                    cell.setFillColor(sf::Color(80, 200, 80));
                } else {
                    cell.setFillColor(sf::Color(240, 217, 181)); // normal wood tone
                }
                window.draw(cell);

                // Small green dot in center for highlighted squares
                // (extra visibility on top of the fill color)
                if (isValid[i][j]) {
                    float dotRadius = 6.f;
                    sf::CircleShape dot(dotRadius);
                    dot.setFillColor(sf::Color(30, 140, 30));
                    dot.setPosition(sf::Vector2f(
                        pos.x + CELL_SIZE / 2.f - dotRadius,
                        pos.y + CELL_SIZE / 2.f - dotRadius
                    ));
                    window.draw(dot);
                }

            } else if (type == CellType::WALL) {
                // ── Placed wall ───────────────────────────────────────────────
                float w, h;
                if      (i % 2 == 1 && j % 2 == 0) { w = (float)CELL_SIZE; h = (float)GAP; }
                else if (i % 2 == 0 && j % 2 == 1) { w = (float)GAP;       h = (float)CELL_SIZE; }
                else                                { w = (float)GAP;       h = (float)GAP; }

                sf::RectangleShape wall(sf::Vector2f(w, h));
                wall.setPosition(pos);
                wall.setFillColor(sf::Color(180, 90, 20));
                window.draw(wall);
            }
            // PATH_SPACE with no wall: dark background shows through as the gap
        }
    }
}

void Renderer::drawPawns(const QuoridorEngine& engine) {
    sf::Color colors[2] = {
        sf::Color(220, 50, 50),   // Player 1: red
        sf::Color(50, 100, 220)   // Player 2: blue
    };

    for (int i = 0; i < 2; i++) {
        const Player& p   = engine.getPlayer(i);
        Point         pos = p.getPosition();
        sf::Vector2f pixel = toPixel(pos.x, pos.y);
        float radius = CELL_SIZE / 2.8f;

        // Drop shadow
        sf::CircleShape shadow(radius);
        shadow.setFillColor(sf::Color(0, 0, 0, 80));
        shadow.setPosition(sf::Vector2f(
            pixel.x + CELL_SIZE / 2.f - radius + 3.f,
            pixel.y + CELL_SIZE / 2.f - radius + 3.f
        ));
        window.draw(shadow);

        // Pawn circle
        sf::CircleShape pawn(radius);
        pawn.setFillColor(colors[i]);
        pawn.setOutlineColor(sf::Color::White);
        pawn.setOutlineThickness(2.f);
        pawn.setPosition(sf::Vector2f(
            pixel.x + CELL_SIZE / 2.f - radius,
            pixel.y + CELL_SIZE / 2.f - radius
        ));
        window.draw(pawn);

        // Player number label on pawn
        sf::Text label(font, std::to_string(i + 1), 14);
        label.setFillColor(sf::Color::White);
        auto bounds = label.getLocalBounds();
        label.setPosition(sf::Vector2f(
            pixel.x + CELL_SIZE / 2.f - bounds.size.x / 2.f - 1.f,
            pixel.y + CELL_SIZE / 2.f - bounds.size.y / 2.f - 4.f
        ));
        window.draw(label);
    }
}

void Renderer::drawHUD(const QuoridorEngine& engine,
                       const std::string& statusMsg,
                       int statusAlpha,
                       bool aiThinking) {
    int current = engine.getCurrentIndex();

    // ── Turn indicator ────────────────────────────────────────────────────────
    std::string turnStr;
    sf::Color   turnColor;
    if (aiThinking) {
        turnStr   = "AI is thinking...";
        turnColor = sf::Color(255, 200, 50);
    } else {
        turnStr   = "Player " + std::to_string(current + 1) + "'s Turn";
        turnColor = (current == 0) ? sf::Color(220, 50, 50) : sf::Color(50, 100, 220);
    }

    sf::Text turnText(font, turnStr, 20);
    turnText.setFillColor(turnColor);
    turnText.setPosition(sf::Vector2f(OFFSET_X, 8.f));
    window.draw(turnText);

    // ── Wall counts ───────────────────────────────────────────────────────────
    for (int i = 0; i < 2; i++) {
        const Player& p    = engine.getPlayer(i);
        std::string   info = "P" + std::to_string(i + 1)
                           + " Walls: " + std::to_string(p.getWallCount());
        sf::Text wallText(font, info, 16);
        wallText.setFillColor(i == 0 ? sf::Color(220, 50, 50) : sf::Color(50, 100, 220));
        wallText.setPosition(sf::Vector2f(OFFSET_X + i * 280.f, 42.f));
        window.draw(wallText);
    }

    // ── Status message (fading feedback) ─────────────────────────────────────
    // Shown centered below the wall counts. Fades out over time (driven by main.cpp).
    // Red  = error   (invalid move, wall blocks path, no walls left, occupied square)
    // Green = success (wall placed, move made — brief confirmation)
    if (!statusMsg.empty() && statusAlpha > 0) {
        bool isError = (statusMsg.find("Invalid")  != std::string::npos ||
                        statusMsg.find("Cannot")   != std::string::npos ||
                        statusMsg.find("No walls") != std::string::npos ||
                        statusMsg.find("blocked")  != std::string::npos ||
                        statusMsg.find("occupied") != std::string::npos ||
                        statusMsg.find("wall")     != std::string::npos && // "wall blocks" patterns
                        statusMsg.find("blocks")   != std::string::npos);

        uint8_t alpha = static_cast<uint8_t>(statusAlpha);
        sf::Color msgColor = isError
            ? sf::Color(255, 80,  80,  alpha)
            : sf::Color(80,  220, 80,  alpha);

        // Semi-transparent background pill for readability
        sf::Text statusText(font, statusMsg, 17);
        statusText.setFillColor(msgColor);
        auto bounds = statusText.getLocalBounds();
        float centeredX = (700.f - bounds.size.x) / 2.f;
        statusText.setPosition(sf::Vector2f(centeredX, 42.f));

        // Draw a dark background rectangle behind the message
        float pad = 8.f;
        sf::RectangleShape bg(sf::Vector2f(bounds.size.x + pad * 2, bounds.size.y + pad * 2));
        bg.setFillColor(sf::Color(0, 0, 0, std::min(alpha, (uint8_t)180)));
        bg.setPosition(sf::Vector2f(centeredX - pad, 42.f - pad / 2.f));
        window.draw(bg);
        window.draw(statusText);
    }

    // ── Controls hint at bottom ───────────────────────────────────────────────
    float boardBottom = OFFSET_Y + 8 * (CELL_SIZE + GAP) + CELL_SIZE + 8.f;
    sf::Text hint(font, "Arrows: Move   Click gap: Wall   R: Reset   ESC: Menu", 13);
    hint.setFillColor(sf::Color(120, 120, 120));
    hint.setPosition(sf::Vector2f(OFFSET_X, boardBottom));
    window.draw(hint);
}

void Renderer::drawWinScreen(const QuoridorEngine& engine) {
    int winner = engine.getWinnerID();
    if (winner == -1) return;

    sf::RectangleShape overlay(sf::Vector2f(700.f, 720.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 170));
    overlay.setPosition(sf::Vector2f(0.f, 0.f));
    window.draw(overlay);

    sf::Text winText(font, "Player " + std::to_string(winner) + " Wins!", 52);
    winText.setFillColor(winner == 1 ? sf::Color(220, 50, 50) : sf::Color(50, 100, 220));
    winText.setPosition(sf::Vector2f(160.f, 280.f));
    window.draw(winText);

    sf::Text restartText(font, "Press R to play again  |  ESC for menu", 22);
    restartText.setFillColor(sf::Color::White);
    restartText.setPosition(sf::Vector2f(140.f, 360.f));
    window.draw(restartText);
}

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