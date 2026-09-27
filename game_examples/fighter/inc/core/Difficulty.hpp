#pragma once
#include "raylib.h"

namespace fighter {

enum class Difficulty { Easy = 0, Normal, Hard, Expert, Count };

// ============================================================================
// Bộ tham số điều khiển độ khó. Màn Settings chỉ chọn enum, còn AI đọc toàn bộ
// hành vi của nó từ struct này -> thêm mức khó mới = thêm một dòng bảng.
// ============================================================================
struct DifficultyProfile {
    const char *name;
    const char *note;
    float reaction;     // giây AI "nghĩ" trước khi đổi quyết định
    float aggression;   // 0..1 - xu hướng lao vào tấn công
    float blockChance;  // 0..1 - khả năng đỡ đúng lúc
    float comboChance;  // 0..1 - khả năng nối đòn
    float damageTaken;  // hệ số sát thương AI PHẢI chịu (cao = AI yếu)
    float meterGain;    // hệ số hồi thanh Super của AI
    Color accent;
};

const DifficultyProfile &GetDifficulty(Difficulty d);
const char *DifficultyName(Difficulty d);

} // namespace fighter
