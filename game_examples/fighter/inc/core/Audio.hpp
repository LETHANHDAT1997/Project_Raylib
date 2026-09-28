#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace fighter {

// Hiệu ứng âm thanh. Mỗi mục là một "ngân hàng" nhiều biến thể; Play() chọn
// ngẫu nhiên một biến thể và đổi cao độ nhẹ để nghe không bị lặp máy móc.
enum class Sfx {
    SwingLight, SwingMedium, SwingHeavy,
    HitLight, HitMedium, HitHeavy,
    Block, Counter, Throw, Tech, Land, Jump, Dash, Knockdown,
    Magic, Clash, Thunder, Teleport, SuperFlash, KOBell,
    UiMove, UiConfirm, UiBack,
    VoiceShout, VoiceSpecial, VoiceHurt, VoiceKO,
    Count
};

// Câu của người xướng ngôn.
enum class Line {
    Round1, Round2, Round3, Round4, Round5, FinalRound, Fight,
    YouWin, YouLose, Winner, Player1, Player2, Flawless, Time, Tie, ChooseCharacter,
    Count
};

enum class MusicTrack { None, Menu, Battle };

// ============================================================================
// Audio - toàn bộ âm thanh của game.
//
// Tiếng va chạm, xướng ngôn, tiếng hét là file CC0 (xem LICENSES.md). Riêng
// tiếng gió vung đòn, phép thuật, sét... được TỔNG HỢP bằng code lúc khởi động
// (nhiễu trắng qua bộ lọc quét tần số) vì không có sẵn trong gói miễn phí.
// ============================================================================
class Audio {
public:
    static Audio &Instance();

    void Init();
    void Shutdown();
    void Update(float dt);          // gọi mỗi frame: stream nhạc + hàng đợi xướng ngôn

    void Play(Sfx s, float volume = 1.0f, float pitch = 1.0f);
    void Say(Line l, float delay = 0.0f);
    void PlayMusic(MusicTrack t);
    void StopMusic();

    bool Ready() const { return ready_; }

private:
    Audio() = default;

    struct Bank {
        std::vector<Sound> sources;          // sở hữu dữ liệu
        std::vector<Sound> voices;           // alias để phát chồng
        int next = 0;
    };

    void LoadFiles(Sfx s, const std::vector<std::string> &files, float baseVolume);
    void AddSynth(Sfx s, Wave w, float baseVolume);

    Bank  banks_[(int)Sfx::Count];
    float bankVolume_[(int)Sfx::Count] = {};
    Sound lines_[(int)Line::Count] = {};

    struct Queued { Line line; float delay; };
    std::vector<Queued> queue_;

    Music music_{};
    bool  musicLoaded_ = false;
    MusicTrack track_ = MusicTrack::None;
    float musicVolume_ = 0.0f;
    float musicTarget_ = 0.0f;

    bool ready_ = false;
    bool ownsDevice_ = false;
};

} // namespace fighter
