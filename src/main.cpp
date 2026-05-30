#include <SFML/Graphics.hpp>
#include "Point.h"
#include "QuoridorEngine.h"
#include "Renderer.h"
#include "AI.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

enum class GameMode { MENU, HVH, HVA };

// ─── Pixel → board coordinate ─────────────────────────────────────────────────
// Converts a mouse click to an internal (2*boardSize-1) grid coordinate.
// Returns isGap=true if the click landed on a gap (wall area).
struct ClickResult { Point point; bool isGap; };

// Compute the same cellSize and gap as Renderer does for this boardSize.
// Must stay in sync with Renderer constructor formula.
void computeSizes(int boardSize, int& cellSize, int& gap) {
    const int BOARD_AREA = 640;
    float divisor = (float)boardSize + (float)(boardSize - 1) / 6.0f;
    cellSize = (int)(BOARD_AREA / divisor);
    gap      = std::max(4, cellSize / 6);
    cellSize = (BOARD_AREA - (boardSize - 1) * gap) / boardSize;
}

ClickResult pixelToBoard(int mouseX, int mouseY, int boardSize) {
    const int OFFSET_X = 30;
    const int OFFSET_Y = 70;

    int cellSize, gap;
    computeSizes(boardSize, cellSize, gap);
    int totalCell = cellSize + gap;

    int relX = mouseX - OFFSET_X;
    int relY = mouseY - OFFSET_Y;
    if (relX < 0 || relY < 0) return {Point(-1,-1), false};

    int col = relX / totalCell;
    int row = relY / totalCell;
    if (col >= boardSize || row >= boardSize) return {Point(-1,-1), false};

    int offX = relX % totalCell;
    int offY = relY % totalCell;

    bool inCellX = offX < cellSize;
    bool inCellY = offY < cellSize;
    bool inGapX  = offX >= cellSize;
    bool inGapY  = offY >= cellSize;

    int ir = row * 2;
    int ic = col * 2;

    if      (inCellX && inCellY)                        return {Point(ir,     ic),     false};
    else if (inGapX  && inCellY && col < boardSize - 1) return {Point(ir,     ic + 1), true };
    else if (inCellX && inGapY  && row < boardSize - 1) return {Point(ir + 1, ic),     true };
    else if (inGapX  && inGapY)                         return {Point(ir + 1, ic + 1), true };
    return {Point(-1,-1), false};
}

// ─── Menu draw ────────────────────────────────────────────────────────────────
void drawMenu(sf::RenderWindow& window, sf::Font& font, int boardSize) {
    window.clear(sf::Color(30, 30, 40));

    // Title
    sf::Text title(font, "QUORIDOR", 60);
    title.setFillColor(sf::Color(240, 200, 100));
    title.setPosition(sf::Vector2f(220.f, 40.f));
    window.draw(title);

    // Board size selector
    sf::Text sizeLabel(font, "Board Size: " + std::to_string(boardSize) + "x" + std::to_string(boardSize), 22);
    sizeLabel.setFillColor(sf::Color(180, 220, 180));
    sizeLabel.setPosition(sf::Vector2f(250.f, 130.f));
    window.draw(sizeLabel);

    sf::Text sizeHint(font, "[ S ] Smaller       [ B ] Bigger", 17);
    sizeHint.setFillColor(sf::Color(100, 140, 100));
    sizeHint.setPosition(sf::Vector2f(225.f, 158.f));
    window.draw(sizeHint);

    // Game mode options
    const std::string options[] = {
        "1.  Human vs Human",
        "2.  Human vs AI  (Easy)",
        "3.  Human vs AI  (Medium)",
        "4.  Human vs AI  (Hard)"
    };
    const sf::Color optColors[] = {
        sf::Color(200, 200, 200),
        sf::Color(100, 220, 100),
        sf::Color(100, 180, 255),
        sf::Color(255, 100, 100)
    };
    for (int i = 0; i < 4; i++) {
        sf::Text opt(font, options[i], 28);
        opt.setFillColor(optColors[i]);
        opt.setPosition(sf::Vector2f(200.f, 210.f + i * 68.f));
        window.draw(opt);
    }

    sf::Text hint(font, "Press 1 / 2 / 3 / 4 to start", 17);
    hint.setFillColor(sf::Color(120, 120, 120));
    hint.setPosition(sf::Vector2f(240.f, 500.f));
    window.draw(hint);

    window.display();
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    sf::RenderWindow window(sf::VideoMode({700u, 720u}), "Quoridor");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        std::cerr << "Could not load font!" << std::endl;
        return -1;
    }

    // ── Runtime board size (custom board size bonus feature) ───────────────────
    // Min = 5, Max = 13, must be odd (so pawns start at center)
    int boardSize = 9;  // default standard size

    // ── Game state ─────────────────────────────────────────────────────────────
    GameMode   gameMode = GameMode::MENU;
    Difficulty aiDiff   = Difficulty::EASY;

    QuoridorEngine* engine   = nullptr;
    Renderer*       renderer = nullptr;
    AI*             ai       = nullptr;

    // ── AI threading ───────────────────────────────────────────────────────────
    std::atomic<bool> aiThinking(false);
    std::atomic<bool> aiMoveReady(false);
    AIMove            aiResult;
    std::mutex        aiMutex;

    // ── Status message state ───────────────────────────────────────────────────
    std::string statusMsg   = "";
    int         statusAlpha = 0;
    const int   FADE_START  = 255;
    const int   FADE_SPEED  = 3;

    // ── Valid moves cache ──────────────────────────────────────────────────────
    std::vector<Point> validMoves;

    auto refreshValidMoves = [&]() {
        if (!engine || engine->isGameOver()) { validMoves.clear(); return; }
        bool isHumanTurn = !(gameMode == GameMode::HVA && engine->getCurrentIndex() == 1);
        if (isHumanTurn && !aiThinking.load())
            validMoves = engine->getValidMoves();
        else
            validMoves.clear();
    };

    auto setStatus = [&](const std::string& msg) {
        statusMsg   = msg;
        statusAlpha = FADE_START;
    };

    // ── AI launcher ───────────────────────────────────────────────────────────
    auto launchAI = [&]() {
        if (aiThinking.load()) return;
        validMoves.clear();
        aiThinking  = true;
        aiMoveReady = false;

        QuoridorEngine snapshot = *engine;
        AI*            aiPtr    = ai;

        std::thread([snapshot, aiPtr, &aiResult, &aiMutex,
                     &aiThinking, &aiMoveReady]() mutable {
            AIMove move = aiPtr->getBestMove(snapshot);
            {
                std::lock_guard<std::mutex> lock(aiMutex);
                aiResult = move;
            }
            aiMoveReady = true;
            aiThinking  = false;
        }).detach();
    };

    // ── Reset helper ──────────────────────────────────────────────────────────
    auto resetGame = [&]() {
        while (aiThinking.load()) {}
        delete engine;   engine   = new QuoridorEngine(boardSize);
        delete renderer; renderer = new Renderer(window, boardSize);
        renderer->loadFont();
        if (gameMode == GameMode::HVA) {
            delete ai; ai = new AI(1, aiDiff);
        }
        aiThinking  = false;
        aiMoveReady = false;
        statusMsg   = "";
        statusAlpha = 0;
        refreshValidMoves();
    };

    // ── Start game helper ─────────────────────────────────────────────────────
    auto startGame = [&](GameMode mode, Difficulty diff) {
        gameMode = mode;
        aiDiff   = diff;
        engine   = new QuoridorEngine(boardSize);
        renderer = new Renderer(window, boardSize);
        renderer->loadFont();
        if (mode == GameMode::HVA)
            ai = new AI(1, diff);
        aiThinking  = false;
        aiMoveReady = false;
        statusMsg   = "";
        statusAlpha = 0;
        refreshValidMoves();
    };

    // ═══════════════════════════════════════════════════════════════════════════
    // MAIN LOOP
    // ═══════════════════════════════════════════════════════════════════════════
    while (window.isOpen()) {

        while (const std::optional<sf::Event> eventOpt = window.pollEvent()) {
            const sf::Event& event = *eventOpt;

            if (event.is<sf::Event::Closed>()) {
                while (aiThinking.load()) {}
                window.close();
            }

            if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
                auto key = kp->scancode;

                // ── Menu controls ────────────────────────────────────────────
                if (gameMode == GameMode::MENU) {
                    // Board size selection (S = smaller, B = bigger, odd values only)
                    if (key == sf::Keyboard::Scan::S && boardSize > 5)
                        boardSize -= 2;
                    if (key == sf::Keyboard::Scan::B && boardSize < 13)
                        boardSize += 2;

                    // Game mode selection
                    if (key == sf::Keyboard::Scan::Num1) startGame(GameMode::HVH, Difficulty::EASY);
                    if (key == sf::Keyboard::Scan::Num2) startGame(GameMode::HVA, Difficulty::EASY);
                    if (key == sf::Keyboard::Scan::Num3) startGame(GameMode::HVA, Difficulty::MEDIUM);
                    if (key == sf::Keyboard::Scan::Num4) startGame(GameMode::HVA, Difficulty::HARD);

                } else {
                    // ── In-game controls ─────────────────────────────────────
                    if (key == sf::Keyboard::Scan::R) resetGame();

                    if (key == sf::Keyboard::Scan::Escape) {
                        while (aiThinking.load()) {}
                        delete engine;   engine   = nullptr;
                        delete renderer; renderer = nullptr;
                        delete ai;       ai       = nullptr;
                        aiThinking  = false;
                        aiMoveReady = false;
                        validMoves.clear();
                        statusMsg   = "";
                        statusAlpha = 0;
                        gameMode = GameMode::MENU;
                    }

                    // ── Arrow key pawn movement ───────────────────────────────
                    if (engine && !engine->isGameOver() && !aiThinking.load()) {
                        bool isHumanTurn = !(gameMode == GameMode::HVA &&
                                             engine->getCurrentIndex() == 1);
                        if (isHumanTurn) {
                            Point pos    = engine->getPlayer(engine->getCurrentIndex()).getPosition();
                            Point oppPos = engine->getPlayer(1 - engine->getCurrentIndex()).getPosition();
                            Point dest(-1, -1);

                            if (key == sf::Keyboard::Scan::Up)    dest = Point(pos.x - 2, pos.y);
                            if (key == sf::Keyboard::Scan::Down)  dest = Point(pos.x + 2, pos.y);
                            if (key == sf::Keyboard::Scan::Left)  dest = Point(pos.x, pos.y - 2);
                            if (key == sf::Keyboard::Scan::Right) dest = Point(pos.x, pos.y + 2);

                            if (dest.x != -1) {
                                // Auto-extend to jump if destination is opponent
                                if (dest == oppPos) {
                                    if (key == sf::Keyboard::Scan::Up)    dest = Point(pos.x - 4, pos.y);
                                    if (key == sf::Keyboard::Scan::Down)  dest = Point(pos.x + 4, pos.y);
                                    if (key == sf::Keyboard::Scan::Left)  dest = Point(pos.x, pos.y - 4);
                                    if (key == sf::Keyboard::Scan::Right) dest = Point(pos.x, pos.y + 4);
                                }

                                bool moved = engine->movePlayer(dest);
                                if (moved) {
                                    setStatus("Move successful!");
                                    refreshValidMoves();
                                    if (gameMode == GameMode::HVA &&
                                        engine->getCurrentIndex() == 1 &&
                                        !engine->isGameOver())
                                        launchAI();
                                } else {
                                    if (!engine->getBoard().isWithinBounds(dest.x, dest.y))
                                        setStatus("Invalid move! Cannot move outside the board.");
                                    else if (engine->getBoard().getCellType(dest.x, dest.y) == CellType::PAWN)
                                        setStatus("Invalid move! That square is occupied.");
                                    else
                                        setStatus("Invalid move! A wall is blocking that direction.");
                                }
                            }
                        }
                    }
                }
            }

            // ── Mouse click — wall placement ──────────────────────────────────
            if (gameMode != GameMode::MENU && engine &&
                !engine->isGameOver() && !aiThinking.load()) {

                bool isHumanTurn = !(gameMode == GameMode::HVA &&
                                     engine->getCurrentIndex() == 1);
                if (isHumanTurn) {
                    if (const auto* mp = event.getIf<sf::Event::MouseButtonPressed>()) {
                        if (mp->button == sf::Mouse::Button::Left) {
                            auto  winSize = window.getSize();
                            float scaleX  = 700.f / (float)winSize.x;
                            float scaleY  = 720.f / (float)winSize.y;
                            int   mappedX = (int)(mp->position.x * scaleX);
                            int   mappedY = (int)(mp->position.y * scaleY);

                            ClickResult cr = pixelToBoard(mappedX, mappedY, boardSize);

                            if (cr.isGap && cr.point.x != -1) {
                                int  r     = cr.point.x;
                                int  c     = cr.point.y;
                                bool moved = false;
                                int  maxI  = 2 * boardSize - 2; // max internal index

                                if (r % 2 == 1 && c % 2 == 0) {
                                    int snapC = (c > 0) ? c - 1 : c + 1;
                                    moved = engine->placeWall(Point(r, snapC), Orientation::HORIZONTAL);
                                    if (!moved) {
                                        snapC = (c < maxI) ? c + 1 : c - 1;
                                        moved = engine->placeWall(Point(r, snapC), Orientation::HORIZONTAL);
                                    }
                                } else if (r % 2 == 0 && c % 2 == 1) {
                                    int snapR = (r > 0) ? r - 1 : r + 1;
                                    moved = engine->placeWall(Point(snapR, c), Orientation::VERTICAL);
                                    if (!moved) {
                                        snapR = (r < maxI) ? r + 1 : r - 1;
                                        moved = engine->placeWall(Point(snapR, c), Orientation::VERTICAL);
                                    }
                                } else if (r % 2 == 1 && c % 2 == 1) {
                                    moved = engine->placeWall(Point(r, c), Orientation::HORIZONTAL);
                                    if (!moved)
                                        moved = engine->placeWall(Point(r, c), Orientation::VERTICAL);
                                }

                                if (moved) {
                                    setStatus("Wall placed!");
                                    refreshValidMoves();
                                    if (gameMode == GameMode::HVA &&
                                        engine->getCurrentIndex() == 1 &&
                                        !engine->isGameOver())
                                        launchAI();
                                } else {
                                    if (engine->getPlayer(engine->getCurrentIndex()).getWallCount() == 0) {
                                        setStatus("No walls remaining!");
                                    } else {
                                        Point testCenter;
                                        Orientation testOrient = Orientation::HORIZONTAL;
                                        if (r % 2 == 1 && c % 2 == 0) {
                                            testCenter = Point(r, (c > 0) ? c - 1 : c + 1);
                                        } else if (r % 2 == 0 && c % 2 == 1) {
                                            testCenter = Point((r > 0) ? r - 1 : r + 1, c);
                                            testOrient = Orientation::VERTICAL;
                                        } else {
                                            testCenter = Point(r, c);
                                        }
                                        if (!engine->canPlaceWall(testCenter, testOrient) &&
                                            !engine->canPlaceWall(testCenter, Orientation::VERTICAL))
                                            setStatus("Cannot place wall here! Space already occupied.");
                                        else
                                            setStatus("Invalid wall! It would completely block a player's path.");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Apply AI result ────────────────────────────────────────────────────
        if (gameMode == GameMode::HVA && engine &&
            aiMoveReady.load() && !engine->isGameOver()) {
            AIMove move;
            {
                std::lock_guard<std::mutex> lock(aiMutex);
                move = aiResult;
            }
            aiMoveReady = false;
            if (move.isWall)
                engine->placeWall(move.target, move.orient);
            else
                engine->movePlayer(move.target);
            refreshValidMoves();
        }

        // ── Fade status message ────────────────────────────────────────────────
        if (statusAlpha > 0) {
            statusAlpha -= FADE_SPEED;
            if (statusAlpha < 0) statusAlpha = 0;
        }

        // ── Draw ──────────────────────────────────────────────────────────────
        window.clear(sf::Color(30, 30, 40));

        if (gameMode == GameMode::MENU) {
            drawMenu(window, font, boardSize);
        } else if (renderer && engine) {
            renderer->drawAll(*engine,
                              validMoves,
                              statusMsg,
                              statusAlpha,
                              aiThinking.load());
            window.display();
        }
    }

    while (aiThinking.load()) {}
    delete engine;
    delete renderer;
    delete ai;
    return 0;
}