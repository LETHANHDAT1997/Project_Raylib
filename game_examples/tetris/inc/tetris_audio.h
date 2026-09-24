#ifndef TETRIS_AUDIO_H
#define TETRIS_AUDIO_H

#include <stdbool.h>

void InitTetrisAudio(void);
void CloseTetrisAudio(void);

void PlaySoundMove(bool enabled);
void PlaySoundRotate(bool enabled);
void PlaySoundDrop(bool enabled);
void PlaySoundHardDrop(bool enabled);
void PlaySoundLineClear(int lines, bool enabled);
void PlaySoundGameOver(bool enabled);

#endif // TETRIS_AUDIO_H
