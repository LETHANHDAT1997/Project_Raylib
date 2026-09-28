#include "core/Audio.hpp"
#include "core/Assets.hpp"
#include <cmath>
#include <cstdlib>

namespace fighter {

namespace {

constexpr int kSynthRate = 22050;
constexpr int kAliases   = 4;     // tối đa 4 lần phát chồng cho mỗi biến thể

float Frand() { return (float)GetRandomValue(0, 100000) / 100000.0f; }

// Tạo Wave 16-bit mono từ một hàm sinh mẫu f(t, i) trả về [-1, 1].
template <typename Fn>
Wave MakeWave(float seconds, Fn fn)
{
    Wave w{};
    w.frameCount = (unsigned int)(seconds * kSynthRate);
    w.sampleRate = kSynthRate;
    w.sampleSize = 16;
    w.channels   = 1;
    short *data  = (short *)MemAlloc(w.frameCount * sizeof(short));
    for (unsigned int i = 0; i < w.frameCount; ++i) {
        const float t = (float)i / kSynthRate;
        float v = fn(t, seconds);
        v = std::fmax(-1.0f, std::fmin(1.0f, v));
        data[i] = (short)(v * 32000.0f);
    }
    w.data = data;
    return w;
}

// Tiếng gió vung đòn: nhiễu trắng qua bộ lọc thông thấp có tần số cắt quét
// xuống, biên độ lên nhanh rồi tắt dần. Đòn càng nặng càng dài và trầm.
Wave Whoosh(float len, float fHigh, float fLow)
{
    float lp = 0.0f, lp2 = 0.0f;
    return MakeWave(len, [&](float t, float L) {
        const float k = t / L;
        const float cutoff = fHigh + (fLow - fHigh) * k;
        const float a = 1.0f - std::exp(-2.0f * PI * cutoff / kSynthRate);
        const float n = Frand() * 2.0f - 1.0f;
        lp  += a * (n - lp);
        lp2 += a * (lp - lp2);
        const float env = std::sin(PI * std::pow(k, 0.55f)) * (1.0f - k * 0.4f);
        return (lp - lp2 * 0.6f) * env * 3.2f;
    });
}

} // namespace

Audio &Audio::Instance()
{
    static Audio a;
    return a;
}

void Audio::LoadFiles(Sfx s, const std::vector<std::string> &files, float baseVolume)
{
    Bank &b = banks_[(int)s];
    bankVolume_[(int)s] = baseVolume;
    const std::string root = Assets::Instance().Root() + "audio/";
    for (const std::string &f : files) {
        const std::string path = root + f;
        if (!FileExists(path.c_str())) {
            TraceLog(LOG_WARNING, "FIGHTER: thiếu âm thanh '%s'", path.c_str());
            continue;
        }
        Sound src = LoadSound(path.c_str());
        if (src.frameCount == 0) continue;
        b.sources.push_back(src);
        for (int i = 0; i < kAliases; ++i) b.voices.push_back(LoadSoundAlias(src));
    }
}

void Audio::AddSynth(Sfx s, Wave w, float baseVolume)
{
    Bank &b = banks_[(int)s];
    bankVolume_[(int)s] = baseVolume;
    Sound src = LoadSoundFromWave(w);
    UnloadWave(w);
    b.sources.push_back(src);
    for (int i = 0; i < kAliases; ++i) b.voices.push_back(LoadSoundAlias(src));
}

void Audio::Init()
{
    if (ready_) return;
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        ownsDevice_ = true;
    }
    if (!IsAudioDeviceReady()) return;   // máy không có thiết bị âm thanh: chơi câm
    ready_ = true;

    auto seq = [](const char *fmt, int from, int to) {
        std::vector<std::string> v;
        for (int i = from; i <= to; ++i) v.push_back(TextFormat(fmt, i));
        return v;
    };

    // --- Tổng hợp ---------------------------------------------------------
    for (int i = 0; i < 3; ++i) AddSynth(Sfx::SwingLight,  Whoosh(0.11f + 0.01f * i, 5200.0f, 1800.0f), 0.55f);
    for (int i = 0; i < 3; ++i) AddSynth(Sfx::SwingMedium, Whoosh(0.17f + 0.02f * i, 3800.0f, 900.0f),  0.65f);
    for (int i = 0; i < 3; ++i) AddSynth(Sfx::SwingHeavy,  Whoosh(0.26f + 0.03f * i, 2600.0f, 380.0f),  0.8f);

    // Phép thuật / đạn: âm quét lên có rung, phủ lớp nhiễu lấp lánh.
    AddSynth(Sfx::Magic, MakeWave(0.45f, [](float t, float L) {
        const float k = t / L;
        const float f = 260.0f + 900.0f * k;
        const float tone = std::sin(2.0f * PI * f * t + 3.0f * std::sin(2.0f * PI * 18.0f * t));
        const float env = std::exp(-4.0f * k) * std::fmin(1.0f, t * 60.0f);
        return (tone * 0.55f + (Frand() * 2 - 1) * 0.25f * (1 - k)) * env;
    }), 0.55f);

    // Hai đạn triệt tiêu: tiếng nổ trầm.
    AddSynth(Sfx::Clash, MakeWave(0.55f, [](float t, float L) {
        const float k = t / L;
        const float body = std::sin(2.0f * PI * (90.0f - 50.0f * k) * t);
        return (body * 0.7f + (Frand() * 2 - 1) * 0.6f * std::exp(-9.0f * k)) * std::exp(-5.0f * k);
    }), 0.8f);

    // Sét: tiếng nổ lách tách + ầm ì.
    AddSynth(Sfx::Thunder, MakeWave(0.9f, [](float t, float L) {
        const float k = t / L;
        const float crack = (Frand() * 2 - 1) * std::exp(-14.0f * k);
        const float rumble = std::sin(2.0f * PI * 48.0f * t) * std::sin(2.0f * PI * 3.0f * t) * std::exp(-3.0f * k);
        return crack * 0.9f + rumble * 0.6f;
    }), 0.75f);

    // Dịch chuyển: "vút" quét xuống.
    AddSynth(Sfx::Teleport, MakeWave(0.28f, [](float t, float L) {
        const float k = t / L;
        const float f = 1600.0f * std::exp(-4.0f * k) + 120.0f;
        return std::sin(2.0f * PI * f * t) * 0.5f * std::sin(PI * k) + (Frand() * 2 - 1) * 0.15f * (1 - k);
    }), 0.6f);

    // Tung siêu chiêu: nhịp trống trầm + âm dâng lên.
    AddSynth(Sfx::SuperFlash, MakeWave(0.8f, [](float t, float L) {
        const float k = t / L;
        const float boom = std::sin(2.0f * PI * (70.0f - 30.0f * k) * t) * std::exp(-5.0f * k);
        const float rise = std::sin(2.0f * PI * (300.0f + 700.0f * k * k) * t) * 0.25f * k * (1.0f - k) * 4.0f;
        return boom * 0.9f + rise * 0.4f;
    }), 0.9f);

    // --- File CC0 ---------------------------------------------------------
    LoadFiles(Sfx::HitLight,  seq("sfx/impactPunch_medium_%03d.ogg", 0, 4), 0.55f);
    LoadFiles(Sfx::HitMedium, seq("sfx/impactPunch_heavy_%03d.ogg", 0, 4), 0.7f);
    LoadFiles(Sfx::HitHeavy,  seq("sfx/impactSoft_heavy_%03d.ogg", 0, 2), 1.0f);
    LoadFiles(Sfx::Block,     seq("sfx/impactPlate_light_%03d.ogg", 0, 3), 0.6f);
    LoadFiles(Sfx::Counter,   seq("sfx/impactMetal_heavy_%03d.ogg", 0, 1), 0.7f);
    LoadFiles(Sfx::Throw,     {"sfx/drawKnife1.ogg", "sfx/chop.ogg"}, 0.7f);
    LoadFiles(Sfx::Tech,      seq("sfx/impactPlate_light_%03d.ogg", 0, 1), 0.8f);
    LoadFiles(Sfx::Land,      seq("sfx/footstep_wood_%03d.ogg", 0, 2), 0.45f);
    LoadFiles(Sfx::Jump,      {"sfx/cloth1.ogg", "sfx/cloth2.ogg"}, 0.35f);
    LoadFiles(Sfx::Dash,      {"sfx/cloth3.ogg", "sfx/cloth4.ogg"}, 0.5f);
    LoadFiles(Sfx::Knockdown, seq("sfx/impactSoft_heavy_%03d.ogg", 0, 2), 0.8f);
    LoadFiles(Sfx::KOBell,    seq("sfx/impactBell_heavy_%03d.ogg", 0, 1), 1.0f);

    LoadFiles(Sfx::UiMove,    seq("ui/select_%03d.ogg", 1, 3), 0.45f);
    LoadFiles(Sfx::UiConfirm, seq("ui/confirmation_%03d.ogg", 1, 2), 0.6f);
    LoadFiles(Sfx::UiBack,    seq("ui/back_%03d.ogg", 1, 2), 0.5f);

    LoadFiles(Sfx::VoiceShout,   seq("voice/shout_%02d.wav", 0, 11), 0.5f);
    LoadFiles(Sfx::VoiceSpecial, seq("voice/special_%02d.wav", 0, 7), 0.62f);
    LoadFiles(Sfx::VoiceHurt,    seq("voice/hurt_%02d.wav", 0, 5), 0.55f);
    LoadFiles(Sfx::VoiceKO,      seq("voice/ko_%02d.wav", 0, 3), 0.75f);

    static const char *kLineFiles[(int)Line::Count] = {
        "round_1", "round_2", "round_3", "round_4", "round_5", "final_round", "fight",
        "you_win", "you_lose", "winner", "player_1", "player_2", "flawless_victory",
        "time", "tie", "choose_your_character",
    };
    const std::string root = Assets::Instance().Root() + "audio/announcer/";
    for (int i = 0; i < (int)Line::Count; ++i) {
        const std::string p = root + kLineFiles[i] + ".ogg";
        if (FileExists(p.c_str())) lines_[i] = LoadSound(p.c_str());
    }

    const std::string mus = Assets::Instance().Root() + "audio/music/battle.qoa";
    if (FileExists(mus.c_str())) {
        music_ = LoadMusicStream(mus.c_str());
        music_.looping = true;
        musicLoaded_ = music_.frameCount > 0;
    }
}

void Audio::Shutdown()
{
    if (!ready_) return;
    StopMusic();
    if (musicLoaded_) UnloadMusicStream(music_);
    musicLoaded_ = false;

    for (Bank &b : banks_) {
        for (Sound &s : b.voices)  UnloadSoundAlias(s);
        for (Sound &s : b.sources) UnloadSound(s);
        b.voices.clear();
        b.sources.clear();
        b.next = 0;
    }
    for (Sound &s : lines_) { if (s.frameCount) UnloadSound(s); s = Sound{}; }
    queue_.clear();

    if (ownsDevice_) CloseAudioDevice();
    ownsDevice_ = false;
    ready_ = false;
}

void Audio::Play(Sfx s, float volume, float pitch)
{
    if (!ready_) return;
    Bank &b = banks_[(int)s];
    if (b.voices.empty()) return;

    // Chọn ngẫu nhiên một biến thể, rồi lấy alias kế tiếp của biến thể đó.
    const int variants = (int)b.sources.size();
    const int v = GetRandomValue(0, variants - 1);
    b.next = (b.next + 1) % kAliases;
    Sound &snd = b.voices[v * kAliases + b.next];

    SetSoundVolume(snd, volume * bankVolume_[(int)s]);
    SetSoundPitch(snd, pitch * (0.94f + 0.12f * Frand()));
    PlaySound(snd);
}

void Audio::Say(Line l, float delay)
{
    if (!ready_ || lines_[(int)l].frameCount == 0) return;
    queue_.push_back({l, delay});
}

void Audio::PlayMusic(MusicTrack t)
{
    if (!ready_ || !musicLoaded_) return;
    if (t == MusicTrack::None) { musicTarget_ = 0.0f; track_ = t; return; }
    if (!IsMusicStreamPlaying(music_)) {
        PlayMusicStream(music_);
        musicVolume_ = 0.0f;
    }
    track_ = t;
    // Menu dùng chung bản nhạc nhưng nhỏ hơn hẳn để không át tiếng giao diện.
    musicTarget_ = (t == MusicTrack::Battle) ? 0.42f : 0.16f;
}

void Audio::StopMusic()
{
    if (musicLoaded_ && IsMusicStreamPlaying(music_)) StopMusicStream(music_);
    track_ = MusicTrack::None;
    musicVolume_ = musicTarget_ = 0.0f;
}

void Audio::Update(float dt)
{
    if (!ready_) return;

    if (musicLoaded_ && IsMusicStreamPlaying(music_)) {
        musicVolume_ += (musicTarget_ - musicVolume_) * (1.0f - std::exp(-3.0f * dt));
        SetMusicVolume(music_, musicVolume_);
        UpdateMusicStream(music_);
        if (track_ == MusicTrack::None && musicVolume_ < 0.01f) StopMusicStream(music_);
    }

    for (size_t i = 0; i < queue_.size();) {
        queue_[i].delay -= dt;
        if (queue_[i].delay <= 0.0f) {
            Sound &s = lines_[(int)queue_[i].line];
            SetSoundVolume(s, 0.95f);
            PlaySound(s);
            queue_.erase(queue_.begin() + (long)i);
        } else {
            ++i;
        }
    }
}

} // namespace fighter
