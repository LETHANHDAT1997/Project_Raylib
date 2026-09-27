#pragma once
#include "raylib.h"
#include "core/Animation.hpp"
#include "entities/Fighter.hpp"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fighter {

// Chỉ số hiển thị trên màn chọn nhân vật, thang 1..5.
struct CharacterStats {
    int power   = 3;
    int speed   = 3;
    int range   = 3;
    int defense = 3;
};

// ============================================================================
// CharacterDef - toàn bộ dữ liệu tĩnh của một nhân vật.
//
// Thêm nhân vật mới = thêm một CharacterDef vào Roster::Load() + một lớp con
// của Fighter. Màn chọn nhân vật tự chia trang theo Roster::Count(), không cần
// sửa gì thêm ở giao diện.
// ============================================================================
struct CharacterDef {
    std::string id;          // trùng tên thư mục trong assets/fighter/characters
    std::string name;        // tên hiển thị
    std::string title;       // danh hiệu ngắn
    std::string blurb;       // một câu giới thiệu lối đánh
    std::string lightName, heavyName, specialName, superName;

    CharacterStats stats{};
    Color accent     = SKYBLUE;
    Color accentDeep = DARKBLUE;

    // Hình hoạ ---------------------------------------------------------------
    float   scale  = 3.4f;   // hệ số phóng sprite gốc
    Vector2 anchor = {100.0f, 122.0f};  // điểm trong frame gốc ứng với giữa 2 bàn chân

    // Thông số chiến đấu -----------------------------------------------------
    float walkSpeed    = 290.0f;
    float backSpeed    = 230.0f;
    float jumpSpeed    = 900.0f;
    int   maxHealth    = 1000;
    float defenseScale = 1.0f;   // <1 = lì đòn hơn
    float meterRate    = 1.0f;

    std::array<Animation, (int)AnimId::Count> anims{};

    // Hàm dựng lớp con tương ứng.
    std::function<std::unique_ptr<Fighter>(const CharacterDef &, bool)> factory;

    const Animation &Anim(AnimId id) const { return anims[(int)id]; }
};

// ============================================================================
// Roster - danh mục nhân vật, nạp một lần lúc vào game.
// ============================================================================
class Roster {
public:
    static Roster &Instance();

    void Load();                    // nạp sprite + dựng bảng nhân vật
    // Quên bảng nhân vật. Bắt buộc gọi trước khi Assets::UnloadAll(): các
    // Animation ở đây giữ handle texture, không xoá thì lần vào game sau sẽ
    // vẽ bằng handle đã chết (Arcade Hub vào/ra game nhiều lần).
    void Unload();
    bool Loaded() const { return loaded_; }

    int Count() const { return (int)chars_.size(); }
    const CharacterDef &At(int index) const;
    int IndexOf(const std::string &id) const;

    std::unique_ptr<Fighter> Create(int index, bool facingRight) const;

private:
    Roster() = default;
    void Add(CharacterDef def);

    std::vector<CharacterDef> chars_;
    bool loaded_ = false;
};

} // namespace fighter
