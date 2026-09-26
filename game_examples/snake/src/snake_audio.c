#include "snake_audio.h"
#include "raylib.h"
#include <stdlib.h>
#include <math.h>

#define SAMPLE_RATE 44100

static Sound s_sndEat = {0};
static Sound s_sndGolden = {0};
static Sound s_sndFrost = {0};
static Sound s_sndTurn = {0};
static Sound s_sndDie = {0};
static Sound s_sndClick = {0};
static Sound s_sndPause = {0};
static bool s_audioReady = false;

static Sound GenerateTone(float startFreq, float endFreq, float duration, float volume, int waveType)
{
    int frameCount = (int)(SAMPLE_RATE * duration);
    if (frameCount <= 0) return (Sound){0};

    short *samples = (short *)malloc(frameCount * sizeof(short));
    if (!samples) return (Sound){0};

    float phase = 0.0f;
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / (float)frameCount;
        float freq = startFreq + (endFreq - startFreq) * t;
        float env = 1.0f - t; // Linear decay envelope
        if (t < 0.08f) env = t / 0.08f; // Smooth attack

        phase += 2.0f * PI * freq / (float)SAMPLE_RATE;
        if (phase > 2.0f * PI) phase -= 2.0f * PI;

        float val = 0.0f;
        if (waveType == 0) {
            // Sine wave
            val = sinf(phase);
        } else if (waveType == 1) {
            // Rounded square
            val = (sinf(phase) >= 0.0f) ? 0.75f : -0.75f;
        } else if (waveType == 2) {
            // Triangle wave
            val = 2.0f * fabsf(2.0f * (phase / (2.0f * PI) - floorf(phase / (2.0f * PI) + 0.5f))) - 1.0f;
        } else if (waveType == 3) {
            // Noise + Sine impact
            float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            val = 0.5f * sinf(phase) + 0.5f * white;
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

static Sound GenerateArpeggio(const float *freqs, int noteCount, float noteDuration, float volume)
{
    int totalFrames = (int)(SAMPLE_RATE * noteDuration * noteCount);
    if (totalFrames <= 0) return (Sound){0};

    short *samples = (short *)malloc(totalFrames * sizeof(short));
    if (!samples) return (Sound){0};

    int noteFrames = (int)(SAMPLE_RATE * noteDuration);
    for (int n = 0; n < noteCount; n++) {
        float freq = freqs[n];
        float phase = 0.0f;
        for (int i = 0; i < noteFrames; i++) {
            int globalIndex = n * noteFrames + i;
            if (globalIndex >= totalFrames) break;

            float t = (float)i / (float)noteFrames;
            float env = 1.0f - t * 0.8f;
            if (t < 0.1f) env = t / 0.1f;

            phase += 2.0f * PI * freq / (float)SAMPLE_RATE;
            if (phase > 2.0f * PI) phase -= 2.0f * PI;

            // Sine + soft harmonic
            float val = 0.8f * sinf(phase) + 0.2f * sinf(phase * 2.0f);
            samples[globalIndex] = (short)(val * env * volume * 32767.0f);
        }
    }

    Wave wave = {
        .frameCount = totalFrames,
        .sampleRate = SAMPLE_RATE,
        .sampleSize = 16,
        .channels = 1,
        .data = samples
    };

    Sound sound = LoadSoundFromWave(wave);
    free(samples);
    return sound;
}

void InitSnakeAudio(void)
{
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    if (IsAudioDeviceReady()) {
        s_audioReady = true;

        // Eat sound: Crisp ascending tone
        s_sndEat = GenerateTone(520.0f, 880.0f, 0.08f, 0.40f, 0);

        // Golden food: Joyous 4-note arpeggio (C5, E5, G5, C6)
        const float goldNotes[] = {523.25f, 659.25f, 783.99f, 1046.50f};
        s_sndGolden = GenerateArpeggio(goldNotes, 4, 0.07f, 0.45f);

        // Frost food: Cool descending chime
        s_sndFrost = GenerateTone(950.0f, 480.0f, 0.16f, 0.40f, 2);

        // Turn sound: Very brief soft blip
        s_sndTurn = GenerateTone(420.0f, 540.0f, 0.035f, 0.20f, 0);

        // Die sound: Heavy impact + descending drop
        s_sndDie = GenerateTone(260.0f, 50.0f, 0.45f, 0.55f, 3);

        // Click / UI: Snappy click
        s_sndClick = GenerateTone(700.0f, 900.0f, 0.03f, 0.30f, 1);

        // Pause sound: Dual-tone pulse
        s_sndPause = GenerateTone(440.0f, 330.0f, 0.10f, 0.30f, 0);
    }
}

void CloseSnakeAudio(void)
{
    if (s_audioReady) {
        if (s_sndEat.frameCount > 0) UnloadSound(s_sndEat);
        if (s_sndGolden.frameCount > 0) UnloadSound(s_sndGolden);
        if (s_sndFrost.frameCount > 0) UnloadSound(s_sndFrost);
        if (s_sndTurn.frameCount > 0) UnloadSound(s_sndTurn);
        if (s_sndDie.frameCount > 0) UnloadSound(s_sndDie);
        if (s_sndClick.frameCount > 0) UnloadSound(s_sndClick);
        if (s_sndPause.frameCount > 0) UnloadSound(s_sndPause);
        s_audioReady = false;
    }
}

void PlaySoundSnakeEat(bool enabled)
{
    if (s_audioReady && enabled && s_sndEat.frameCount > 0) PlaySound(s_sndEat);
}

void PlaySoundSnakeGolden(bool enabled)
{
    if (s_audioReady && enabled && s_sndGolden.frameCount > 0) PlaySound(s_sndGolden);
}

void PlaySoundSnakeFrost(bool enabled)
{
    if (s_audioReady && enabled && s_sndFrost.frameCount > 0) PlaySound(s_sndFrost);
}

void PlaySoundSnakeTurn(bool enabled)
{
    if (s_audioReady && enabled && s_sndTurn.frameCount > 0) PlaySound(s_sndTurn);
}

void PlaySoundSnakeDie(bool enabled)
{
    if (s_audioReady && enabled && s_sndDie.frameCount > 0) PlaySound(s_sndDie);
}

void PlaySoundSnakeClick(bool enabled)
{
    if (s_audioReady && enabled && s_sndClick.frameCount > 0) PlaySound(s_sndClick);
}

void PlaySoundSnakePause(bool enabled)
{
    if (s_audioReady && enabled && s_sndPause.frameCount > 0) PlaySound(s_sndPause);
}
