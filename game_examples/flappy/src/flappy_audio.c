#include "flappy_audio.h"
#include "flappy_assets.h"
#include "raylib.h"

// Âm thanh lấy từ các pack CC0 của Kenney - xem assets/flappy/LICENSES.md.
// Tiếng vỗ cánh có hai biến thể, luân phiên để khỏi nghe lặp lại máy móc.
static const char *s_sfxFiles[FLAPPY_SFX_COUNT] = {
    [FLAPPY_SFX_FLAP]      = "audio/flap1.ogg",
    [FLAPPY_SFX_SCORE]     = "audio/score.ogg",
    [FLAPPY_SFX_STAR]      = "audio/star.ogg",
    [FLAPPY_SFX_HIT]       = "audio/hit.ogg",
    [FLAPPY_SFX_GAME_OVER] = "audio/gameover.ogg",
    [FLAPPY_SFX_NEW_BEST]  = "audio/new_best.ogg",
    [FLAPPY_SFX_LEVEL_UP]  = "audio/level_up.ogg",
    [FLAPPY_SFX_SELECT]    = "audio/select.ogg",
    [FLAPPY_SFX_CONFIRM]   = "audio/confirm.ogg",
    [FLAPPY_SFX_PAUSE]     = "audio/pause.ogg",
};

static const float s_sfxVolume[FLAPPY_SFX_COUNT] = {
    [FLAPPY_SFX_FLAP]      = 0.55f,
    [FLAPPY_SFX_SCORE]     = 0.45f,
    [FLAPPY_SFX_STAR]      = 0.50f,
    [FLAPPY_SFX_HIT]       = 0.80f,
    [FLAPPY_SFX_GAME_OVER] = 0.55f,
    [FLAPPY_SFX_NEW_BEST]  = 0.60f,
    [FLAPPY_SFX_LEVEL_UP]  = 0.45f,
    [FLAPPY_SFX_SELECT]    = 0.50f,
    [FLAPPY_SFX_CONFIRM]   = 0.55f,
    [FLAPPY_SFX_PAUSE]     = 0.50f,
};

static Sound s_sounds[FLAPPY_SFX_COUNT];
static Sound s_flapAlt;
static bool s_flapToggle = false;
static bool s_audioReady = false;

static Sound LoadSfx(const char *relative, float volume)
{
    const char *path = FlappyAssetPath(relative);
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "FLAPPY: thiếu âm thanh '%s'", path);
        return (Sound){0};
    }
    Sound s = LoadSound(path);
    SetSoundVolume(s, volume);
    return s;
}

void InitFlappyAudio(void)
{
    if (s_audioReady) return;
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    if (!IsAudioDeviceReady()) return;

    for (int i = 0; i < FLAPPY_SFX_COUNT; i++) {
        s_sounds[i] = LoadSfx(s_sfxFiles[i], s_sfxVolume[i]);
    }
    s_flapAlt = LoadSfx("audio/flap2.ogg", s_sfxVolume[FLAPPY_SFX_FLAP]);
    s_audioReady = true;
}

void CloseFlappyAudio(void)
{
    if (!s_audioReady) return;
    for (int i = 0; i < FLAPPY_SFX_COUNT; i++) {
        if (s_sounds[i].frameCount > 0) UnloadSound(s_sounds[i]);
        s_sounds[i] = (Sound){0};
    }
    if (s_flapAlt.frameCount > 0) UnloadSound(s_flapAlt);
    s_flapAlt = (Sound){0};
    s_audioReady = false;
}

void PlayFlappySfx(FlappySfx sfx, bool enabled, float pitchJitter)
{
    if (!s_audioReady || !enabled || sfx < 0 || sfx >= FLAPPY_SFX_COUNT) return;

    Sound s = s_sounds[sfx];
    if (sfx == FLAPPY_SFX_FLAP) {
        s_flapToggle = !s_flapToggle;
        if (s_flapToggle && s_flapAlt.frameCount > 0) s = s_flapAlt;
    }
    if (s.frameCount == 0) return;

    float pitch = 1.0f;
    if (pitchJitter > 0.0f) {
        pitch += ((float)GetRandomValue(-100, 100) / 100.0f) * pitchJitter;
    }
    SetSoundPitch(s, pitch);
    PlaySound(s);
}
