#include "characters/Roster.hpp"
#include "characters/Characters.hpp"
#include "core/Assets.hpp"
#include <algorithm>
#include <cassert>

namespace fighter {

namespace {

// Nhịp phát mặc định cho từng animation. Số frame KHÔNG khai báo ở đây: nó được
// suy ra từ kích thước ảnh (mỗi frame là hình vuông cạnh = chiều cao sheet),
// nên thay pack sprite khác số frame vẫn chạy đúng.
struct AnimSetup { AnimId id; float fps; bool loop; };

const AnimSetup kAnimSetup[] = {
    {AnimId::Idle,     8.0f,  true },
    {AnimId::Run,     12.0f,  true },
    {AnimId::Jump,     9.0f,  false},
    {AnimId::Fall,     9.0f,  false},
    {AnimId::Attack1, 15.0f,  false},
    {AnimId::Attack2, 14.0f,  false},
    {AnimId::TakeHit, 12.0f,  false},
    {AnimId::Death,    9.0f,  false},
};

void LoadAnims(CharacterDef &def)
{
    Assets &assets = Assets::Instance();
    for (const AnimSetup &s : kAnimSetup) {
        const std::string path =
            "characters/" + def.id + "/" + AnimFileName(s.id) + ".png";

        Animation a{};
        a.texture = assets.Texture(path);
        a.fps     = s.fps;
        a.loop    = s.loop;
        a.frameCount = (a.texture.height > 0)
                     ? std::max(1, a.texture.width / a.texture.height)
                     : 1;
        def.anims[(int)s.id] = a;
    }
}

} // namespace

Roster &Roster::Instance()
{
    static Roster instance;
    return instance;
}

const CharacterDef &Roster::At(int index) const
{
    assert(!chars_.empty() && "Roster chưa được Load()");
    if (index < 0 || index >= (int)chars_.size()) index = 0;
    return chars_[index];
}

int Roster::IndexOf(const std::string &id) const
{
    for (int i = 0; i < (int)chars_.size(); ++i) {
        if (chars_[i].id == id) return i;
    }
    return -1;
}

std::unique_ptr<Fighter> Roster::Create(int index, bool facingRight) const
{
    const CharacterDef &def = At(index);
    return def.factory(def, facingRight);
}

void Roster::Unload()
{
    chars_.clear();
    loaded_ = false;
}

void Roster::Add(CharacterDef def)
{
    LoadAnims(def);
    chars_.push_back(std::move(def));
}

// ============================================================================
// THÊM NHÂN VẬT MỚI
// ----------------------------------------------------------------------------
// 1. Chép sprite vào assets/fighter/characters/<id>/ với 8 file:
//    idle, run, jump, fall, attack1, attack2, takehit, death (.png).
// 2. Viết một lớp con của Fighter trong characters/Characters.hpp + .cpp.
// 3. Thêm một khối Add({...}) bên dưới.
// Màn chọn nhân vật tự chia trang nên không cần sửa giao diện.
//
// Hai giá trị cần canh bằng mắt khi thêm pack mới:
//   anchor - toạ độ (trong frame gốc) ứng với điểm giữa hai bàn chân
//   scale  - phóng to bao nhiêu lần để nhân vật cao ~190px trên màn hình
// ============================================================================
void Roster::Load()
{
    if (loaded_) return;
    loaded_ = true;

    // ---- Mack: lãng khách, cân bằng, dễ chơi ------------------------------
    {
        CharacterDef d;
        d.id    = "samurai";
        d.name  = "MACK";
        d.title = "Lãng khách";
        d.blurb = "Cân bằng giữa công và thủ, dễ làm quen. Chiêu chém vọt lên "
                  "trị nhân vật hay nhảy.";
        d.lightName   = "Chém ngang";
        d.heavyName   = "Trảm hạ";
        d.specialName = "Thăng Long Trảm";
        d.superName   = "Tam Liên Trảm";
        d.stats  = {3, 4, 3, 3};
        d.accent     = Color{255, 176, 76, 255};
        d.accentDeep = Color{146, 74, 18, 255};
        d.scale  = 3.6f;
        d.anchor = Vector2{94.0f, 122.0f};
        d.walkSpeed = 300.0f; d.backSpeed = 245.0f; d.jumpSpeed = 940.0f;
        d.maxHealth = 1000;   d.defenseScale = 1.00f; d.meterRate = 1.0f;
        d.factory = [](const CharacterDef &def, bool right) {
            return std::unique_ptr<Fighter>(new Samurai(def, right));
        };
        Add(std::move(d));
    }

    // ---- Kenji: áp sát, nhanh, giấy mỏng ----------------------------------
    {
        CharacterDef d;
        d.id    = "kenji";
        d.name  = "KENJI";
        d.title = "Kiếm ảnh";
        d.blurb = "Nhanh nhất roster, ép sát liên tục. Đổi lại máu mỏng và "
                  "chịu đòn kém.";
        d.lightName   = "Liên thích";
        d.heavyName   = "Bạt đao";
        d.specialName = "Ảnh Bộ";
        d.superName   = "Loạn Ảnh Kiếm";
        d.stats  = {3, 5, 2, 2};
        d.accent     = Color{120, 200, 255, 255};
        d.accentDeep = Color{28, 82, 140, 255};
        d.scale  = 3.3f;
        d.anchor = Vector2{97.0f, 128.0f};
        d.walkSpeed = 352.0f; d.backSpeed = 292.0f; d.jumpSpeed = 985.0f;
        d.maxHealth = 920;    d.defenseScale = 1.12f; d.meterRate = 1.15f;
        d.factory = [](const CharacterDef &def, bool right) {
            return std::unique_ptr<Fighter>(new Kenji(def, right));
        };
        Add(std::move(d));
    }

    // ---- Knight: chậm, trâu, đòn nặng có armor -----------------------------
    {
        CharacterDef d;
        d.id    = "knight";
        d.name  = "GARETH";
        d.title = "Hiệp sĩ thép";
        d.blurb = "Đi chậm nhưng nhiều máu, đòn nặng có armor ăn đòn vẫn vung "
                  "tiếp. Hợp lối đánh lì.";
        d.lightName   = "Vụt kiếm";
        d.heavyName   = "Cường trảm";
        d.specialName = "Khiên Xung";
        d.superName   = "Địa Chấn";
        d.stats  = {5, 2, 3, 5};
        d.accent     = Color{214, 205, 186, 255};
        d.accentDeep = Color{96, 88, 72, 255};
        d.scale  = 3.6f;
        d.anchor = Vector2{96.0f, 115.0f};
        d.walkSpeed = 228.0f; d.backSpeed = 178.0f; d.jumpSpeed = 840.0f;
        d.maxHealth = 1150;   d.defenseScale = 0.84f; d.meterRate = 0.9f;
        d.factory = [](const CharacterDef &def, bool right) {
            return std::unique_ptr<Fighter>(new Knight(def, right));
        };
        Add(std::move(d));
    }

    // ---- Wizard: giữ khoảng cách, bắn từ xa --------------------------------
    {
        CharacterDef d;
        d.id    = "wizard";
        d.name  = "MALTHUS";
        d.title = "Hắc pháp sư";
        d.blurb = "Đứng xa ném cầu hắc ám, ép đối thủ phải lao vào. Cận chiến "
                  "yếu, đừng để bị dí.";
        d.lightName   = "Vụt trượng";
        d.heavyName   = "Quét trượng";
        d.specialName = "Hắc Cầu";
        d.superName   = "Tam Hắc Cầu";
        d.stats  = {4, 2, 5, 2};
        d.accent     = Color{186, 120, 255, 255};
        d.accentDeep = Color{74, 34, 122, 255};
        d.scale  = 2.0f;
        d.anchor = Vector2{136.0f, 167.0f};
        d.walkSpeed = 242.0f; d.backSpeed = 214.0f; d.jumpSpeed = 880.0f;
        d.maxHealth = 900;    d.defenseScale = 1.15f; d.meterRate = 1.2f;
        d.factory = [](const CharacterDef &def, bool right) {
            return std::unique_ptr<Fighter>(new Wizard(def, right));
        };
        Add(std::move(d));
    }
}

} // namespace fighter
