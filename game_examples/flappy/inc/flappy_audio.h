#ifndef FLAPPY_AUDIO_H
#define FLAPPY_AUDIO_H

#include <stdbool.h>

typedef enum {
    FLAPPY_SFX_FLAP = 0,
    FLAPPY_SFX_SCORE,
    FLAPPY_SFX_STAR,
    FLAPPY_SFX_HIT,
    FLAPPY_SFX_GAME_OVER,
    FLAPPY_SFX_NEW_BEST,
    FLAPPY_SFX_LEVEL_UP,
    FLAPPY_SFX_SELECT,
    FLAPPY_SFX_CONFIRM,
    FLAPPY_SFX_PAUSE,
    FLAPPY_SFX_COUNT
} FlappySfx;

void InitFlappyAudio(void);
void CloseFlappyAudio(void);

// Phát hiệu ứng âm thanh; pitchJitter > 0 làm mỗi lần phát nghe hơi khác nhau.
void PlayFlappySfx(FlappySfx sfx, bool enabled, float pitchJitter);

#endif // FLAPPY_AUDIO_H
