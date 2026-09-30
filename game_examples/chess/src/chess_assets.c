#include "chess_assets.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

// Mô hình + vân gỗ: Poly Haven (CC0). Âm thanh: Kenney (CC0).
// Xem assets/chess/LICENSES.md.

static char s_root[256] = "";

static void ResolveRoot(void)
{
    if (s_root[0] != '\0') return;
    // Game có thể chạy từ thư mục build, từ gốc repo hoặc từ Arcade Hub.
    static const char *candidates[] = {
        "assets/chess/",
        "../assets/chess/",
        "../../assets/chess/",
        "../../../assets/chess/",
        "game_examples/assets/chess/",
        "../game_examples/assets/chess/",
        "../../game_examples/assets/chess/",
        NULL
    };
    for (int i = 0; candidates[i] != NULL; i++) {
        char probe[300];
        snprintf(probe, sizeof(probe), "%smodel", candidates[i]);
        if (DirectoryExists(probe)) {
            strncpy(s_root, candidates[i], sizeof(s_root) - 1);
            return;
        }
    }
    TraceLog(LOG_WARNING, "CHESS: không tìm thấy thư mục assets/chess");
    strncpy(s_root, "assets/chess/", sizeof(s_root) - 1);
}

const char *ChessAssetPath(const char *relative)
{
    // Bộ đệm xoay vòng: cho phép gọi vài lần liền nhau trong cùng một biểu thức.
    static char buffers[4][512];
    static int next = 0;
    ResolveRoot();
    char *buf = buffers[next];
    next = (next + 1) % 4;
    snprintf(buf, sizeof(buffers[0]), "%s%s", s_root, relative);
    return buf;
}

static const char *s_sfxFiles[CHESS_SFX_COUNT] = {
    [CHESS_SFX_MOVE]    = "audio/move.ogg",
    [CHESS_SFX_CAPTURE] = "audio/capture.ogg",
    [CHESS_SFX_CASTLE]  = "audio/castle.ogg",
    [CHESS_SFX_CHECK]   = "audio/check.ogg",
    [CHESS_SFX_PROMOTE] = "audio/promote.ogg",
    [CHESS_SFX_WIN]     = "audio/win.ogg",
    [CHESS_SFX_LOSE]    = "audio/lose.ogg",
    [CHESS_SFX_DRAW]    = "audio/draw.ogg",
    [CHESS_SFX_SELECT]  = "audio/select.ogg",
    [CHESS_SFX_CONFIRM] = "audio/confirm.ogg",
    [CHESS_SFX_ERROR]   = "audio/error.ogg",
    [CHESS_SFX_UNDO]    = "audio/undo.ogg",
    [CHESS_SFX_HINT]    = "audio/hint.ogg",
};

static const float s_sfxVolume[CHESS_SFX_COUNT] = {
    [CHESS_SFX_MOVE]    = 0.80f,
    [CHESS_SFX_CAPTURE] = 0.85f,
    [CHESS_SFX_CASTLE]  = 0.70f,
    [CHESS_SFX_CHECK]   = 0.60f,
    [CHESS_SFX_PROMOTE] = 0.55f,
    [CHESS_SFX_WIN]     = 0.60f,
    [CHESS_SFX_LOSE]    = 0.55f,
    [CHESS_SFX_DRAW]    = 0.55f,
    [CHESS_SFX_SELECT]  = 0.40f,
    [CHESS_SFX_CONFIRM] = 0.50f,
    [CHESS_SFX_ERROR]   = 0.40f,
    [CHESS_SFX_UNDO]    = 0.55f,
    [CHESS_SFX_HINT]    = 0.50f,
};

static Sound s_sounds[CHESS_SFX_COUNT];
static bool s_audioReady = false;

void InitChessAudio(void)
{
    if (s_audioReady) return;
    if (!IsAudioDeviceReady()) InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    for (int i = 0; i < CHESS_SFX_COUNT; i++) {
        const char *path = ChessAssetPath(s_sfxFiles[i]);
        s_sounds[i] = (Sound){0};
        if (!FileExists(path)) {
            TraceLog(LOG_WARNING, "CHESS: thiếu âm thanh '%s'", path);
            continue;
        }
        s_sounds[i] = LoadSound(path);
        SetSoundVolume(s_sounds[i], s_sfxVolume[i]);
    }
    s_audioReady = true;
}

void CloseChessAudio(void)
{
    if (!s_audioReady) return;
    for (int i = 0; i < CHESS_SFX_COUNT; i++) {
        if (s_sounds[i].frameCount > 0) UnloadSound(s_sounds[i]);
        s_sounds[i] = (Sound){0};
    }
    s_audioReady = false;
}

void PlayChessSfx(ChessSfx sfx, bool enabled, float pitchJitter)
{
    if (!s_audioReady || !enabled || sfx < 0 || sfx >= CHESS_SFX_COUNT) return;
    Sound s = s_sounds[sfx];
    if (s.frameCount == 0) return;
    float pitch = 1.0f;
    if (pitchJitter > 0.0f) pitch += ((float)GetRandomValue(-100, 100) / 100.0f) * pitchJitter;
    SetSoundPitch(s, pitch);
    PlaySound(s);
}
