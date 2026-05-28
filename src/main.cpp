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
// Returns the internal 17x17 grid point that was clicked, and whether it is a gap.
struct ClickResult { Point point; bool isGap; };

ClickResult pixelToBoard(int mouseX, int mouseY) {
    const int CELL_SIZE = 56;
    const int GAP       = 10;
    const int OFFSET_X  = 30;
    const int OFFSET_Y  = 70;

    int relX = mouseX - OFFSET_X;
    int relY = mouseY - OFFSET_Y;
    if (relX < 0 || relY < 0) return {Point(-1,-1), false};

    int totalCell = CELL_SIZE + GAP;
    int col = relX / totalCell;
    int row = relY / totalCell;
    if (col > 8 || row > 8) return {Point(-1,-1), false};

    int offX = relX % totalCell;
    int offY = relY % totalCell;

    bool inCellX = offX < CELL_SIZE;
    bool inCellY = offY < CELL_SIZE;
    bool inGapX  = offX >= CELL_SIZE;
    bool inGapY  = offY >= CELL_SIZE;

    int ir = row * 2;
    int ic = col * 2;

    if      (inCellX && inCellY)            return {Point(ir,     ic),     false};
    else if (inGapX  && inCellY && col < 8) return {Point(ir,     ic + 1), true };
    else if (inCellX && inGapY  && row < 8) return {Point(ir + 1, ic),     true };
    else if (inGapX  && inGapY)             return {Point(ir + 1, ic + 1), true };
    return {Point(-1,-1), false};
}

// ─── Menu draw ────────────────────────────────────────────────────────────────
void drawMenu(sf::RenderWindow& window, sf::Font& font) {
    window.clear(sf::Color(30, 30, 40));

    sf::Text title(font, "QUORIDOR", 60);
    title.setFillColor(sf::Color(240, 200, 100));
    title.setPosition(sf::Vector2f(220.f, 60.f));
    window.draw(title);

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
        opt.setPosition(sf::Vector2f(200.f, 200.f + i * 70.f));
        window.draw(opt);
    }

    sf::Text hint(font, "Press 1 / 2 / 3 / 4 to choose", 18);
    hint.setFillColor(sf::Color(120, 120, 120));
    hint.setPosition(sf::Vector2f(230.f, 520.f));
    window.draw(hint);

    window.display();
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    const int BOARD_SIZE = 9;

    sf::RenderWindow window(sf::VideoMode({700u, 720u}), "Quoridor");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        std::cerr << "Could not load font!" << std::endl;
        return -1;
    }

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
    // statusMsg:   the text to show ("Invalid move!", "Wall would block path!", etc.)
    // statusAlpha: 0-255, decremented each frame to fade the message out
    std::string statusMsg   = "";
    int         statusAlpha = 0;
    const int   FADE_START  = 255;  // alpha when message first appears
    const int   FADE_SPEED  = 3;    // alpha decremented per frame (~1.4 seconds to fade)

    // ── Valid moves cache ──────────────────────────────────────────────────────
    // Recomputed whenever the turn changes or a move is made.
    // During AI's turn this is empty (no highlighting needed).
    std::vector<Point> validMoves;

    auto refreshValidMoves = [&]() {
        if (!engine || engine->isGameOver()) {
            validMoves.clear();
            return;
        }
        // Show highlights only on human turns
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
        validMoves.clear();          // hide highlights while AI thinks
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

    // ── Reset / Start helpers ──────────────────────────────────────────────────
    auto resetGame = [&]() {
        while (aiThinking.load()) {}
        delete engine;   engine   = new QuoridorEngine(BOARD_SIZE);
        delete renderer; renderer = new Renderer(window);
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

    auto startGame = [&](GameMode mode, Difficulty diff) {
        gameMode = mode;
        aiDiff   = diff;
        engine   = new QuoridorEngine(BOARD_SIZE);
        renderer = new Renderer(window);
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

        // ── Event handling ──────────────────────────────────────────────────────
        while (const std::optional<sf::Event> eventOpt = window.pollEvent()) {
            const sf::Event& event = *eventOpt;

            // Window close
            if (event.is<sf::Event::Closed>()) {
                while (aiThinking.load()) {}
                window.close();
            }

            // Key press
            if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
                auto key = kp->scancode;

                // ── Menu navigation ─────────────────────────────────────────────
                if (gameMode == GameMode::MENU) {
                    if (key == sf::Keyboard::Scan::Num1) startGame(GameMode::HVH, Difficulty::EASY);
                    if (key == sf::Keyboard::Scan::Num2) startGame(GameMode::HVA, Difficulty::EASY);
                    if (key == sf::Keyboard::Scan::Num3) startGame(GameMode::HVA, Difficulty::MEDIUM);
                    if (key == sf::Keyboard::Scan::Num4) startGame(GameMode::HVA, Difficulty::HARD);

                } else {
                    // ── In-game keys ───────────────────────────────────────────
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

                    // ── Arrow key pawn movement ────────────────────────────────
                    // Only allowed on human turns, not while AI is computing
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
                                // If arrow lands on opponent, attempt straight jump
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
                                    // Determine reason for failure
                                    if (!engine->getBoard().isWithinBounds(dest.x, dest.y)) {
                                        setStatus("Invalid move! Cannot move outside the board.");
                                    } else if (engine->getBoard().getCellType(dest.x, dest.y) == CellType::PAWN) {
                                        setStatus("Invalid move! That square is occupied.");
                                    } else {
                                        setStatus("Invalid move! A wall is blocking that direction.");
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Mouse click — wall placement ────────────────────────────────────
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

                            ClickResult cr = pixelToBoard(mappedX, mappedY);

                            if (cr.isGap && cr.point.x != -1) {
                                int  r     = cr.point.x;
                                int  c     = cr.point.y;
                                bool moved = false;

                                // Determine orientation from which gap was clicked:
                                // Odd row, even col  → horizontal gap below a row → H wall
                                // Even row, odd col  → vertical gap right of col  → V wall
                                // Odd row, odd col   → corner → try both orientations
                                if (r % 2 == 1 && c % 2 == 0) {
                                    // Horizontal wall: snap center to (r, c-1) or (r, c+1)
                                    int snapC = (c > 0) ? c - 1 : c + 1;
                                    moved = engine->placeWall(Point(r, snapC), Orientation::HORIZONTAL);
                                    if (!moved) {
                                        // Try the other snap direction
                                        snapC = (c < 16) ? c + 1 : c - 1;
                                        moved = engine->placeWall(Point(r, snapC), Orientation::HORIZONTAL);
                                    }
                                } else if (r % 2 == 0 && c % 2 == 1) {
                                    // Vertical wall: snap center to (r-1, c) or (r+1, c)
                                    int snapR = (r > 0) ? r - 1 : r + 1;
                                    moved = engine->placeWall(Point(snapR, c), Orientation::VERTICAL);
                                    if (!moved) {
                                        snapR = (r < 16) ? r + 1 : r - 1;
                                        moved = engine->placeWall(Point(snapR, c), Orientation::VERTICAL);
                                    }
                                } else if (r % 2 == 1 && c % 2 == 1) {
                                    // Corner: try horizontal first, then vertical
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
                                    // Determine specific reason for wall failure
                                    if (engine->getPlayer(engine->getCurrentIndex()).getWallCount() == 0) {
                                        setStatus("No walls remaining!");
                                    } else {
                                        // The only reason placeWall fails when you have walls and
                                        // the slot is structurally available is the BFS block check.
                                        // Check if it's structural (already occupied) vs path-blocking.
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
                                            !engine->canPlaceWall(testCenter, Orientation::VERTICAL)) {
                                            setStatus("Cannot place wall here! Space already occupied.");
                                        } else {
                                            setStatus("Invalid wall! It would completely block a player's path.");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Apply AI result when thread finishes ──────────────────────────────
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

            refreshValidMoves(); // recompute highlights for human's next turn
        }

        // ── Fade status message ───────────────────────────────────────────────
        if (statusAlpha > 0) {
            statusAlpha -= FADE_SPEED;
            if (statusAlpha < 0) statusAlpha = 0;
        }

        // ── Draw ──────────────────────────────────────────────────────────────
        window.clear(sf::Color(30, 30, 40));

        if (gameMode == GameMode::MENU) {
            drawMenu(window, font);
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