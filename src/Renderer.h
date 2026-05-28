#ifndef RENDERER_H
#define RENDERER_H

#include <SFML/Graphics.hpp>
#include "QuoridorEngine.h"
#include <string>
#include <vector>

class Renderer {
public:
    Renderer(sf::RenderWindow& window);
    bool loadFont();

    // Main draw call — called every frame
    // validMoves: squares to highlight green
    // statusMsg:  feedback message ("Invalid move!", etc.) — empty = none
    // statusAlpha: 0-255 fade value, driven by main.cpp clock
    // aiThinking:  shows yellow "AI is thinking..." indicator
    void drawAll(const QuoridorEngine& engine,
                 const std::vector<Point>& validMoves,
                 const std::string& statusMsg,
                 int statusAlpha,
                 bool aiThinking);

private:
    sf::RenderWindow& window;
    sf::Font font;

    sf::Vector2f toPixel(int internalRow, int internalCol);

    void drawBoard(const QuoridorEngine& engine,
                   const std::vector<Point>& validMoves);
    void drawPawns(const QuoridorEngine& engine);
    void drawHUD(const QuoridorEngine& engine,
                 const std::string& statusMsg,
                 int statusAlpha,
                 bool aiThinking);
    void drawWinScreen(const QuoridorEngine& engine);
};

#endif