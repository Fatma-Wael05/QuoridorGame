#ifndef RENDERER_H
#define RENDERER_H

#include <SFML/Graphics.hpp>
#include "QuoridorEngine.h"
#include <string>
#include <vector>

class Renderer {
public:
    // Window is 700 x 720 pixels.
    // HUD takes top 70px. Board area = 700 x 640px (with 30px left margin).
    // Cell size and gap are computed from boardSize so the board always fits.
    static const int WIN_W    = 700;
    static const int WIN_H    = 720;
    static const int OFFSET_X = 30;
    static const int OFFSET_Y = 70;
    static const int BOARD_AREA = 640; // pixels available for the board

    Renderer(sf::RenderWindow& window, int boardSize);
    bool loadFont();

    void drawAll(const QuoridorEngine& engine,
                 const std::vector<Point>& validMoves,
                 const std::string& statusMsg,
                 int statusAlpha,
                 bool aiThinking);

private:
    sf::RenderWindow& window;
    sf::Font          font;
    int               boardSize;   // visual board size (e.g. 9 for 9x9)
    int               internalSize;// 2*boardSize - 1
    int               cellSize;    // computed pixel size of each square
    int               gap;         // computed pixel size of each gap

    sf::Vector2f toPixel(int internalRow, int internalCol);

    void drawBoard(const QuoridorEngine& engine, const std::vector<Point>& validMoves);
    void drawPawns(const QuoridorEngine& engine);
    void drawHUD(const QuoridorEngine& engine, const std::string& statusMsg,
                 int statusAlpha, bool aiThinking);
    void drawWinScreen(const QuoridorEngine& engine);
};

#endif