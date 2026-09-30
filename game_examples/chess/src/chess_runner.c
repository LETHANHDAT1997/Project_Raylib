#include "chess_runner.h"
#include "chess_types.h"
#include "chess_game.h"
#include "chess_render.h"
#include "chess_assets.h"
#include "font_vn.h"

static ChessGame s_chessGame;
static bool s_chessInitialized = false;

void InitChessApp(void)
{
    InitVietnameseFont();
    ChessRenderInit();
    InitChessAudio();
    InitChessGame(&s_chessGame);
    s_chessInitialized = true;
}

void UpdateChessApp(float dt)
{
    // Cảnh 3D được dựng ngay trong bước này (cần đổi framebuffer cho bóng đổ).
    if (s_chessInitialized) UpdateChessGame(&s_chessGame, dt);
}

void DrawChessApp(void)
{
    if (s_chessInitialized) DrawChessGame(&s_chessGame);
}

void CloseChessApp(void)
{
    if (s_chessInitialized) {
        CloseChessGame(&s_chessGame);
        CloseChessAudio();
        ChessRenderClose();
        CloseVietnameseFont();
        s_chessInitialized = false;
    }
}
