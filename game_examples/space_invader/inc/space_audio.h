#ifndef SPACE_AUDIO_H
#define SPACE_AUDIO_H

#include <stdbool.h>

void InitSpaceAudio(void);
void CloseSpaceAudio(void);

void PlaySoundLaser(bool enabled);
void PlaySoundAlienHit(bool enabled);
void PlaySoundPlayerHit(bool enabled);
void PlaySoundAlienMarch(int beat, bool enabled);
void PlaySoundUfo(bool enabled);
void PlaySoundBunkerHit(bool enabled);

#endif // SPACE_AUDIO_H
