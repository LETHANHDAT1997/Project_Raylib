# Raylib Game Projects

This repository contains several game examples and utilities built using the Raylib library, demonstrating various game development concepts from basic graphics and input to more complex mechanics like collision detection, AI, and state management.

## Game List

### 1. Tetris
Classic falling block puzzle game.
- **Controls**:
    - Left/Right: Move piece
    - Up: Rotate piece
    - Down: Soft drop
    - Space: Hard drop
    - P: Pause
    - M: Toggle sound

### 2. Space Invaders Clone
A modern take on the classic arcade shooter.
- **Features**:
    - Player movement and shooting
    - Descending alien formation
    - Bunker defense system
    - Scoring system
    - Sound effects (laser, explosion, game over)
    - Screen shake on impact
    - High score tracking
- **Controls**:
    - Left/Right: Move player
    - Space/Up/W: Shoot
    - M: Toggle sound
    - P: Pause/Resume game
    - Escape: Exit game
    - R: Restart game

## Building and Running

Follow these steps to build and run the games on your Linux system.

### Prerequisites
- **Raylib Library**:
  - **Bundled Version**: **Raylib 6.1-dev** (included directly via the `raylib/` git submodule).
  - **Supported Versions**: Fully compatible with **Raylib 5.0, 5.5, and 6.0+**.
  *(Note: You do not need to install Raylib system-wide; the build system automatically compiles the bundled submodule).*
- **Build Tools**:
  - **CMake**: 3.16 or higher.
  - **C Compiler**: GCC or Clang with C99 support.

### Building

You can build all games at once or build each game individually:

**Option 1: Build All Games**
```bash
cd game_examples
chmod +x build_all.sh
./build_all.sh
```
Executables will be placed in `game_examples/build/tetris/tetris` and `game_examples/build/space_invader/space_invader`.

**Option 2: Build & Run Individually**
- For Tetris:
  ```bash
  cd game_examples/tetris
  ./build_and_run.sh
  ```
- For Space Invaders:
  ```bash
  cd game_examples/space_invader
  ./build_and_run.sh
  ```

### Running
After a successful build, you can run the games using the following commands:

**Tetris**:
```bash
./build/tetris/tetris
```

**Space Invaders**:
```bash
./build/space_invader/space_invader
```