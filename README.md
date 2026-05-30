# Quoridor Game

A complete implementation of the **Quoridor** board game built in **C++ with SFML 3.0**, featuring a graphical interface, local multiplayer, and an AI opponent with three difficulty levels.

> 🎥 **Demo Video:** [INSERT VIDEO LINK HERE]

---

## Game Description

Quoridor is an abstract strategy board game invented by Mirko Marchesi (1997), winner of the Mensa Mind Game award. Two players each control a pawn and take turns either moving their pawn or placing a wall. The first player to reach the opposite side of the board wins.

**Rules summary:**
- Player 1 (red) starts at the top-center and must reach the bottom row
- Player 2 (blue) starts at the bottom-center and must reach the top row
- Each player has 10 walls to place (scales with board size)
- Pawns move one square orthogonally per turn
- If the opponent is adjacent with no wall between, you can jump over them
- If the jump is blocked by a wall, you can move diagonally around the opponent
- Walls cannot completely cut off any player's path to their goal

---

## Screenshots

| Main Menu | Game in Progress |
|-----------|-----------------|
| ![Menu](screenshots/menu.png) | ![Gameplay](screenshots/gameplay.png) |

| Valid Move Highlights | Win Screen |
|----------------------|------------|
| ![Highlights](screenshots/highlights.png) | ![Win](screenshots/win.png) |

> 📷 Screenshots are located in the `screenshots/` folder in this repository.

---

## Installation and Running Instructions

### Prerequisites
- Windows 10/11 (64-bit)
- [MSYS2](https://www.msys2.org) installed at `C:\msys64\`
- [Visual Studio Code](https://code.visualstudio.com) with the **C/C++ extension**
- GCC and SFML installed via MSYS2

### Step 1 — Install MSYS2
Download and install from [msys2.org](https://www.msys2.org). Keep the default path (`C:\msys64\`).

### Step 2 — Install GCC and SFML
Open the **MSYS2 MINGW64** terminal and run:
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-sfml
```

### Step 3 — Clone the Repository
```bash
git clone https://github.com/Fatma-Wael05/QuoridorGamr.git
cd QuoridorGamr
```

### Step 4 — Copy DLLs
Open the **MSYS2 MINGW64** terminal and run:
```bash
cp /c/msys64/mingw64/bin/*.dll bin/
```

---

## Method 1: Build and Run from VS Code (Recommended)

### Step 5 — Open in VS Code
1. Open VS Code
2. Click **File → Open Folder** and select the `QuoridorGame` folder

### Step 6 — Build
Press **Ctrl + Shift + B**

This runs the pre-configured "Build Quoridor" task which compiles all source files automatically using MSYS2's g++.

> ✅ The compiled executable will be at `bin/Quoridor.exe`

### Step 7 — Run
Open the VS Code terminal (**Ctrl + `**) and run:
```bash
cd bin
./Quoridor.exe
```

---

## Method 2: Build and Run from MSYS2 Terminal

### Step 5 — Navigate to the project
Open **MSYS2 MINGW64** terminal:
```bash
cd /c/Users/YourUsername/C++/QuoridorGame
```

### Step 6 — Compile
```bash
g++ -std=c++17 -O2 \
    -I C:/msys64/mingw64/include \
    src/main.cpp src/AI.cpp src/Board.cpp src/PathFinder.CPP \
    src/Player.cpp src/QuoridorEngine.cpp src/Renderer.cpp \
    -L C:/msys64/mingw64/lib \
    -lsfml-graphics -lsfml-window -lsfml-system -lpthread \
    -o bin/Quoridor.exe
```

### Step 7 — Run
```bash
cd bin
./Quoridor.exe
```

---

## Controls

### Main Menu
| Key | Action |
|-----|--------|
| `S` | Decrease board size (min 5×5) |
| `B` | Increase board size (max 13×13) |
| `1` | Start Human vs Human |
| `2` | Start Human vs AI (Easy) |
| `3` | Start Human vs AI (Medium) |
| `4` | Start Human vs AI (Hard) |

### In-Game
| Input | Action |
|-------|--------|
| `Arrow Keys` | Move your pawn (up / down / left / right) |
| `Left Mouse Click` on a gap between squares | Place a wall |
| `R` | Reset the current game |
| `ESC` | Return to the main menu |

### Notes
- Arrow keys automatically jump over the opponent if they are adjacent
- Clicking directly on a square (not a gap) does nothing — walls are gap-only
- Valid moves are highlighted in **green** on your turn
- Status messages appear briefly after every action to confirm success or explain errors

---

## Bonus Features

### AI Difficulty Levels
Three AI difficulty levels using Minimax with Alpha-Beta Pruning:
- **Easy** — Depth 1, pawn moves only, greedy
- **Medium** — Depth 3, uses walls strategically
- **Hard** — Depth 5, deep lookahead, strongest opponent

### Custom Board Sizes
The game supports custom board sizes from 5×5 to 13×13. Use **S** and **B** keys on the main menu to change the board size before starting a game. All game rules, wall counts, and AI scale automatically.

---

## Project Structure

```
QuoridorGame/
├── src/
│   ├── Point.h                    # Core types: Point, CellType, Orientation
│   ├── Board.h / Board.cpp        # 17×17 internal grid representation
│   ├── Player.h / Player.cpp      # Player state management
│   ├── PathFinder.h / PathFinder.CPP  # BFS path validation
│   ├── QuoridorEngine.h / QuoridorEngine.cpp  # Game rules engine
│   ├── AI.h / AI.cpp              # Minimax with Alpha-Beta pruning
│   ├── Renderer.h / Renderer.cpp  # SFML rendering pipeline
│   └── main.cpp                   # Entry point and game loop
├── bin/                           # Compiled executable + DLLs
├── .vscode/
│   └── tasks.json                 # VS Code build task
├── screenshots/                   # Screenshots for README
├── .gitignore
└── README.md
```

---

## Team Members

| Name | Student ID |
|------|-----------|
| [Fatma Wael Taher] | [2300974] |
| [Rahma Yosry Mohamed] | [2300976] |
