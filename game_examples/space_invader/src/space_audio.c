#include "space_audio.h"
#include "raylib.h"
#include <stdlib.h>
#include <math.h>

#define SAMPLE_RATE 44100

static Sound s_sndLaser = {0};
static Sound s_sndAlienHit = {0};
static Sound s_sndPlayerHit = {0};
static Sound s_sndMarch[4] = {{0}};
static Sound s_sndUfo = {0};
static Sound s_sndBunker = {0};
static bool s_audioReady = false;

static Sound GenTone(float startFreq, float endFreq, float duration, float volume, int waveType)
{
    int frameCount = (int)(SAMPLE_RATE * duration);
    short *samples = (short *)malloc(frameCount * sizeof(short));
    if (!samples) return (Sound){0};

    float phase = 0.0f;
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / (float)frameCount;
        float freq = startFreq + (endFreq - startFreq) * t;
        float env = 1.0f - t;
        if (t < 0.05f) env = t / 0.05f;

        phase += 2.0f * PI * freq / (float)SAMPLE_RATE;
        if (phase > 2.0f * PI) phase -= 2.0f * PI;

        float val = 0.0f;
        if (waveType == 0) { // Square
            val = (sinf(phase) >= 0.0f) ? 0.75f : -0.75f;
        } else if (waveType == 1) { // Sine
            val = sinf(phase);
        } else if (waveType == 2) { // Noise burst
            float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            val = white * (1.0f - t);
        } else if (waveType == 3) { // FM / Warble
            float mod = sinf(2.0f * PI * 35.0f * t) * 120.0f;
            phase += 2.0f * PI * (freq + mod) / (float)SAMPLE_RATE;
            val = sinf(phase);
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

void InitSpaceAudio(void)
{
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    if (IsAudioDeviceReady()) {
        s_audioReady = true;

        s_sndLaser = GenTone(1250.0f, 250.0f, 0.09f, 0.40f, 0);
        s_sndAlienHit = GenTone(300.0f, 80.0f, 0.12f, 0.50f, 2);
        s_sndPlayerHit = GenTone(160.0f, 40.0f, 0.45f, 0.65f, 2);

        // 4 classic descending march beats
        const float marchFreqs[4] = { 174.6f, 155.6f, 138.6f, 123.5f };
        for (int i = 0; i < 4; i++) {
            s_sndMarch[i] = GenTone(marchFreqs[i], marchFreqs[i] * 0.95f, 0.06f, 0.45f, 0);
        }

        s_sndUfo = GenTone(650.0f, 850.0f, 0.30f, 0.35f, 3);
        s_sndBunker = GenTone(220.0f, 140.0f, 0.05f, 0.30f, 0);
    }
}

void CloseSpaceAudio(void)
{
    if (s_audioReady) {
        if (s_sndLaser.frameCount > 0) UnloadSound(s_sndLaser);
        if (s_sndAlienHit.frameCount > 0) UnloadSound(s_sndAlienHit);
        if (s_sndPlayerHit.frameCount > 0) UnloadSound(s_sndPlayerHit);
        for (int i = 0; i < 4; i++) {
            if (s_sndMarch[i].frameCount > 0) UnloadSound(s_sndMarch[i]);
        }
        if (s_sndUfo.frameCount > 0) UnloadSound(s_sndUfo);
        if (s_sndBunker.frameCount > 0) UnloadSound(s_sndBunker);
        s_audioReady = false;
    }
}

void PlaySoundLaser(bool enabled)
{
    if (s_audioReady && enabled && s_sndLaser.frameCount > 0) PlaySound(s_sndLaser);
}

void PlaySoundAlienHit(bool enabled)
{
    if (s_audioReady && enabled && s_sndAlienHit.frameCount > 0) PlaySound(s_sndAlienHit);
}

void PlaySoundPlayerHit(bool enabled)
{
    if (s_audioReady && enabled && s_sndPlayerHit.frameCount > 0) PlaySound(s_sndPlayerHit);
}

void PlaySoundAlienMarch(int beat, bool enabled)
{
    if (s_audioReady && enabled && beat >= 0 && beat < 4) {
        if (s_sndMarch[beat].frameCount > 0) PlaySound(s_sndMarch[beat]);
    }
}

void PlaySoundUfo(bool enabled)
{
    if (s_audioReady && enabled && s_sndUfo.frameCount > 0) PlaySound(s_sndUfo);
}

void PlaySoundBunkerHit(bool enabled)
{
    if (s_audioReady && enabled && s_sndBunker.frameCount > 0) PlaySound(s_sndBunker);
}
