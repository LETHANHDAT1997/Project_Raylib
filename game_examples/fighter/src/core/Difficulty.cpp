#include "core/Difficulty.hpp"

namespace fighter {

namespace {
// Bảng độ khó. Thêm một mức mới chỉ cần thêm một dòng ở đây và một giá trị
// trong enum - AI và màn Settings tự động dùng theo.
const DifficultyProfile kProfiles[] = {
    // name        note                              react  aggr  block combo dmgTaken meter  accent
    {"Dễ",        "Máy phản ứng chậm, ít phản đòn",  0.55f, 0.35f, 0.15f, 0.10f, 1.45f, 0.70f, {126, 214, 132, 255}},
    {"Thường",    "Cân bằng, hợp để làm quen",       0.34f, 0.55f, 0.35f, 0.30f, 1.00f, 1.00f, {120, 190, 255, 255}},
    {"Khó",       "Máy đỡ đòn tốt và biết nối combo",0.20f, 0.75f, 0.58f, 0.55f, 0.80f, 1.25f, {255, 186, 92,  255}},
    {"Siêu khó",  "Phản ứng gần như tức thì",        0.11f, 0.92f, 0.76f, 0.80f, 0.62f, 1.55f, {255, 104, 104, 255}},
};
} // namespace

const DifficultyProfile &GetDifficulty(Difficulty d)
{
    int i = (int)d;
    if (i < 0 || i >= (int)Difficulty::Count) i = (int)Difficulty::Normal;
    return kProfiles[i];
}

const char *DifficultyName(Difficulty d) { return GetDifficulty(d).name; }

} // namespace fighter
