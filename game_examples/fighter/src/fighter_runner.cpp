#include "fighter_runner.h"
#include "Game.hpp"
#include <memory>

// Cầu nối giữa Arcade Hub (C) và game (C++). Hub tự lo cửa sổ, canvas và
// letterbox, ở đây chỉ cần chuyển tiếp init/update/draw/close.

namespace {
std::unique_ptr<fighter::Game> g_game;
}

extern "C" void InitFighterApp(void)
{
    if (!g_game) g_game.reset(new fighter::Game());
    g_game->Init();
}

extern "C" void UpdateFighterApp(float dt)
{
    if (g_game) g_game->Update(dt);
}

extern "C" void DrawFighterApp(void)
{
    if (g_game) g_game->Draw();
}

extern "C" void CloseFighterApp(void)
{
    if (g_game) {
        g_game->Shutdown();
        g_game.reset();
    }
}
