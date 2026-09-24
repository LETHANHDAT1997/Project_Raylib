#include "tetris_audio.h"
#include "raylib.h"
#include <stdlib.h>
#include <math.h>

static Sound s_sndMove = {0};
static Sound s_sndRotate = {0};
static Sound s_sndDrop = {0};
static Sound s_sndHardDrop = {0};
static Sound s_sndLine = {0};
static Sound s_sndTetris = {0};
static Sound s_sndGameOver = {0};
static bool s_audioReady = false;

#define SAMPLE_RATE 44100

static Sound GenerateTone(float startFreq, float endFreq, float duration, float volume, int waveType)
{
    int frameCount = (int)(SAMPLE_RATE * duration);
    short *samples = (short *)malloc(frameCount * sizeof(short));
    if (!samples) return (Sound){0};

    float phase = 0.0f;
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / (float)frameCount;
        float freq = startFreq + (endFreq - startFreq) * t;
        float env = 1.0f - t; // Linear decay envelope
        if (t < 0.05f) env = t / 0.05f; // Fast attack

        phase += 2.0f * PI * freq / (float)SAMPLE_RATE;
        if (phase > 2.0f * PI) phase -= 2.0f * PI;

        float val = 0.0f;
        if (waveType == 0) {
            // Square wave with mild rounding
            val = (sinf(phase) >= 0.0f) ? 0.8f : -0.8f;
        } else if (waveType == 1) {
            // Sine wave
            val = sinf(phase);
        } else if (waveType == 2) {
            // Noise / Impact
            float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            val = 0.6f * sinf(phase) + 0.4f * white;
        }

        samples[i] = (short)(val * env * volume * 32767.0f);
    }

    Wave wave = {
        .frameCount = frameCount,
        .sampleRate = SAMPLE_RATE,
        .sampleSize = 16,
        .channels = 1,
        .data = samples
    };

    Sound sound = LoadSoundFromWave(wave);
    free(samples);
    return sound;
}

void InitTetrisAudio(void)
{
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    if (IsAudioDeviceReady()) {
        s_audioReady = true;
        s_sndMove = GenerateTone(800.0f, 900.0f, 0.04f, 0.3f, 0);
        s_sndRotate = GenerateTone(500.0f, 850.0f, 0.06f, 0.35f, 0);
        s_sndDrop = GenerateTone(220.0f, 110.0f, 0.07f, 0.4f, 1);
        s_sndHardDrop = GenerateTone(350.0f, 60.0f, 0.12f, 0.55f, 2);
        s_sndLine = GenerateTone(587.0f, 880.0f, 0.22f, 0.45f, 0);
        s_sndTetris = GenerateTone(440.0f, 1046.0f, 0.40f, 0.55f, 0);
        s_sndGameOver = GenerateTone(400.0f, 65.0f, 0.55f, 0.5f, 0);
    }
}

void CloseTetrisAudio(void)
{
    if (s_audioReady) {
        if (s_sndMove.frameCount > 0) UnloadSound(s_sndMove);
        if (s_sndRotate.frameCount > 0) UnloadSound(s_sndRotate);
        if (s_sndDrop.frameCount > 0) UnloadSound(s_sndDrop);
        if (s_sndHardDrop.frameCount > 0) UnloadSound(s_sndHardDrop);
        if (s_sndLine.frameCount > 0) UnloadSound(s_sndLine);
        if (s_sndTetris.frameCount > 0) UnloadSound(s_sndTetris);
        if (s_sndGameOver.frameCount > 0) UnloadSound(s_sndGameOver);
        s_audioReady = false;
    }
}

void PlaySoundMove(bool enabled)
{
    if (s_audioReady && enabled && s_sndMove.frameCount > 0) PlaySound(s_sndMove);
}

void PlaySoundRotate(bool enabled)
{
    if (s_audioReady && enabled && s_sndRotate.frameCount > 0) PlaySound(s_sndRotate);
}

void PlaySoundDrop(bool enabled)
{
    if (s_audioReady && enabled && s_sndDrop.frameCount > 0) PlaySound(s_sndDrop);
}

void PlaySoundHardDrop(bool enabled)
{
    if (s_audioReady && enabled && s_sndHardDrop.frameCount > 0) PlaySound(s_sndHardDrop);
}

void PlaySoundLineClear(int lines, bool enabled)
{
    if (s_audioReady && enabled) {
        if (lines >= 4 && s_sndTetris.frameCount > 0) {
            PlaySound(s_sndTetris);
        } else if (s_sndLine.frameCount > 0) {
            PlaySound(s_sndLine);
        }
    }
}

void PlaySoundGameOver(bool enabled)
{
    if (s_audioReady && enabled && s_sndGameOver.frameCount > 0) {
        PlaySound(s_sndGameOver);
    }
}
