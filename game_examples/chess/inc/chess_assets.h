/**
 * chess_assets.h - Đường dẫn tài nguyên và âm thanh của Cờ Vua 3D.
 */
#ifndef CHESS_ASSETS_H
#define CHESS_ASSETS_H

#include <stdbool.h>

typedef enum {
    CHESS_SFX_MOVE = 0,
    CHESS_SFX_CAPTURE,
    CHESS_SFX_CASTLE,
    CHESS_SFX_CHECK,
    CHESS_SFX_PROMOTE,
    CHESS_SFX_WIN,
    CHESS_SFX_LOSE,
    CHESS_SFX_DRAW,
    CHESS_SFX_SELECT,
    CHESS_SFX_CONFIRM,
    CHESS_SFX_ERROR,
    CHESS_SFX_UNDO,
    CHESS_SFX_HINT,
    CHESS_SFX_COUNT
} ChessSfx;

const char *ChessAssetPath(const char *relative);

void InitChessAudio(void);
void CloseChessAudio(void);
void PlayChessSfx(ChessSfx sfx, bool enabled, float pitchJitter);

#endif // CHESS_ASSETS_H
