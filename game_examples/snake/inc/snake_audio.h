#ifndef SNAKE_AUDIO_H
#define SNAKE_AUDIO_H

#include <stdbool.h>

void InitSnakeAudio(void);
void CloseSnakeAudio(void);

void PlaySoundSnakeEat(bool enabled);
void PlaySoundSnakeGolden(bool enabled);
void PlaySoundSnakeFrost(bool enabled);
void PlaySoundSnakeTurn(bool enabled);
void PlaySoundSnakeDie(bool enabled);
void PlaySoundSnakeClick(bool enabled);
void PlaySoundSnakePause(bool enabled);

#endif // SNAKE_AUDIO_H
