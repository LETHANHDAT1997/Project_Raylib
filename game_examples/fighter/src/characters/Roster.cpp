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

// Một số pack vẽ sẵn vệt chém trắng đặc trong frame đòn đánh. Phóng to 4-5 lần
// nó thành mảng trắng lấn át nhân vật, nên làm dịu lại: hơi trong suốt và ngả
// sang màu của nhân vật. Chỉ xử lý từ frame `slashFrom` trở đi.
struct SlashSoften { Color accent; int fromFrame; };

void SoftenSlash(Image &img, void *user)
{
    const SlashSoften &p = *static_cast<SlashSoften *>(user);
    Color *px = static_cast<Color *>(img.data);
    const int side = img.height;
    for (int y = 0; y < img.height; ++y) {
        for (int x = p.fromFrame * side; x < img.width; ++x) {
            Color &c = px[y * img.width + x];
            if (c.a > 200 && c.r > 215 && c.g > 215 && c.b > 215) {
                c.r = (unsigned char)(c.r * 0.72f + p.accent.r * 0.28f);
                c.g = (unsigned char)(c.g * 0.72f + p.accent.g * 0.28f);
                c.b = (unsigned char)(c.b * 0.72f + p.accent.b * 0.28f);
                c.a = 150;
            }
        }
    }
}

void LoadAnims(CharacterDef &def)
{
    Assets &assets = Assets::Instance();
    for (const AnimSetup &s : kAnimSetup) {
        const std::string path =
            "characters/" + def.id + "/" + AnimFileName(s.id) + ".png";

        Animation a{};
        const bool attack = s.id == AnimId::Attack1 || s.id == AnimId::Attack2;
        if (attack && def.normals.slashFrom >= 0) {
            SlashSoften soft{def.accent, def.normals.slashFrom};
            a.texture = assets.TextureProcessed(path, path + "#soft", SoftenSlash, &soft);
        } else {
            a.texture = assets.Texture(path);
        }
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

    // Ký hiệu lệnh: hướng tính theo mặt nhân vật (→ = tiến về phía đối thủ).
    // "đòn" = J/K/L (lực nhẹ/vừa/mạnh quyết định tốc độ, tầm xa của chiêu).
    // Mỗi chiêu còn có cách bấm nhanh bằng phím I (Special) + hướng.

    // ---- Mack: lãng khách, cân bằng, dễ chơi ------------------------------
    {
        CharacterDef d;
        d.id    = "samurai";
        d.name  = "MACK";
        d.title = "Lãng khách";
        d.blurb = "Đủ mọi công cụ: đạn để ép, chém vọt để chặn người nhảy, "
                  "thế phản để trừng phạt kẻ tham đòn.";
        d.lightName   = "Chém ngang";
        d.heavyName   = "Trảm hạ";
        d.specialName = "Thăng Long Trảm";
        d.superName   = "Tam Liên Trảm";
        d.moveList = {
            {"Kiếm Khí",        "↓↘→ + đòn",  "Khí kiếm bay. Lực quyết định tốc độ."},
            {"Toàn Phong Trảm", "↓↙← + đòn",  "Xoay người chém lướt tới, dùng được khi nhảy."},
            {"Thăng Long Trảm", "→↓↘ + đòn",  "Chém vọt lên, bất tử lúc bung. Chặn người nhảy."},
            {"Phản Kiếm",       "↓ ↓ + đòn",  "Thế thủ: bị đánh trúng thì chém trả cực mạnh."},
            {"Tam Liên Trảm",   "U  (1 thanh)","Lao tới năm nhát rồi phóng khí kiếm lớn."},
        };
        d.stats  = {3, 4, 3, 3};
        d.accent     = Color{255, 176, 76, 255};
        d.accentDeep = Color{146, 74, 18, 255};
        d.scale  = 3.6f;
        d.anchor = Vector2{94.0f, 122.0f};
        d.walkSpeed = 280.0f; d.backSpeed = 220.0f; d.jumpSpeed = 980.0f; d.jumpForward = 300.0f;
        d.maxHealth = 1000;   d.defenseScale = 1.00f; d.meterRate = 1.0f;
        d.normals.lightTo = 2;
        d.normals.slashFrom = 4;
        d.voicePitch = 1.0f;
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
        d.blurb = "Nhanh nhất roster, tiếp cận bằng lướt xuyên và bổ nhào. "
                  "Đổi lại máu mỏng, chịu đòn kém.";
        d.lightName   = "Liên thích";
        d.heavyName   = "Bạt đao";
        d.specialName = "Ảnh Bộ";
        d.superName   = "Loạn Ảnh Kiếm";
        d.moveList = {
            {"Phi Tiêu",      "↓↘→ + đòn", "Rất nhanh. Lực mạnh ném ba chiếc; khi nhảy ném chéo."},
            {"Ảnh Bộ",        "↓↙← + đòn", "Lướt xuyên qua người đối thủ, xuyên cả đạn."},
            {"Ưng Trảo",      "→↓↘ + đòn", "Bật cao rồi bổ nhào - đòn trên, phải ĐỨNG đỡ."},
            {"Thế Thân",      "↓ ↓ + đòn", "Biến vào khói, hiện sau lưng. Lực mạnh: rút lui."},
            {"Loạn Ảnh Kiếm", "U  (1 thanh)", "Chớp quanh đối thủ chém bảy nhát."},
        };
        d.stats  = {3, 5, 2, 2};
        d.accent     = Color{120, 200, 255, 255};
        d.accentDeep = Color{28, 82, 140, 255};
        d.scale  = 3.3f;
        d.anchor = Vector2{97.0f, 128.0f};
        d.walkSpeed = 330.0f; d.backSpeed = 270.0f; d.jumpSpeed = 1020.0f; d.jumpForward = 340.0f;
        d.maxHealth = 920;    d.defenseScale = 1.12f; d.meterRate = 1.15f;
        d.normals.lightTo = 1;
        d.normals.slashFrom = 2;
        d.normals.speed = 0.85f;
        d.normals.power = 0.9f;
        d.normals.reach = 0.92f;
        d.normals.throwDamage = 110;
        d.voicePitch = 1.18f;
        d.factory = [](const CharacterDef &def, bool right) {
            return std::unique_ptr<Fighter>(new Kenji(def, right));
        };
        Add(std::move(d));
    }

    // ---- Gareth: chậm, trâu, vật lệnh ---------------------------------------
    {
        CharacterDef d;
        d.id    = "knight";
        d.name  = "GARETH";
        d.title = "Hiệp sĩ thép";
        d.blurb = "Chậm nhưng lì: nhiều chiêu có giáp, và một cú vật lệnh "
                  "không thể đỡ. Áp sát được là thắng.";
        d.lightName   = "Vụt kiếm";
        d.heavyName   = "Cường trảm";
        d.specialName = "Khiên Xung";
        d.superName   = "Địa Chấn";
        d.moveList = {
            {"Khiên Xung",     "↓↘→ + đòn", "Húc thẳng có giáp: ăn một đòn vẫn lao tiếp."},
            {"Trảm Địa",       "↓↙← + đòn", "Nhảy bổ xuống (đòn trên) + chấn động khi đáp."},
            {"Nộ Kích",        "→↓↘ + đòn", "Chém vọt lên có giáp, hất tung đối thủ."},
            {"Địa Ngục Quăng", "↓ ↓ + đòn", "Vật lệnh: không đỡ được, không phá được."},
            {"Địa Chấn",       "U  (1 thanh)", "Nện đất, sóng chấn động quét ngang sàn."},
        };
        d.stats  = {5, 2, 3, 5};
        d.accent     = Color{214, 205, 186, 255};
        d.accentDeep = Color{96, 88, 72, 255};
        d.scale  = 3.6f;
        d.anchor = Vector2{96.0f, 115.0f};
        d.walkSpeed = 210.0f; d.backSpeed = 170.0f; d.jumpSpeed = 900.0f; d.jumpForward = 250.0f;
        d.maxHealth = 1150;   d.defenseScale = 0.84f; d.meterRate = 0.9f;
        d.normals.lightTo = 2;
        d.normals.slashFrom = 3;
        d.normals.power = 1.18f;
        d.normals.speed = 1.15f;
        d.normals.reach = 1.1f;
        d.normals.throwDamage = 140;
        d.voicePitch = 0.82f;
        d.factory = [](const CharacterDef &def, bool right) {
            return std::unique_ptr<Fighter>(new Knight(def, right));
        };
        Add(std::move(d));
    }

    // ---- Malthus: giữ khoảng cách, bắn từ xa --------------------------------
    {
        CharacterDef d;
        d.id    = "wizard";
        d.name  = "MALTHUS";
        d.title = "Hắc pháp sư";
        d.blurb = "Khống chế sân bằng cầu hắc ám và cột đất, dịch chuyển khi "
                  "bị dồn. Cận chiến yếu - đừng để bị dí.";
        d.lightName   = "Vụt trượng";
        d.heavyName   = "Quét trượng";
        d.specialName = "Hắc Cầu";
        d.superName   = "Thiên Phạt";
        d.moveList = {
            {"Hắc Cầu",     "↓↘→ + đòn", "Cầu năng lượng. Lực nhẹ bay chậm, mạnh bay nhanh."},
            {"Hắc Trụ",     "↓↙← + đòn", "Cột đen trồi lên ở xa, hất tung. Lực = khoảng cách."},
            {"Hắc Bạo",     "→↓↘ + đòn", "Bộc phá quanh người, bất tử lúc bung."},
            {"Dịch Chuyển", "↓ ↓ + đòn", "Nhẹ: sau lưng · Vừa: trước mặt · Mạnh: về xa."},
            {"Thiên Phạt",  "U  (1 thanh)", "Gọi năm tia sét giáng dọc sàn đấu."},
        };
        d.stats  = {4, 2, 5, 2};
        d.accent     = Color{186, 120, 255, 255};
        d.accentDeep = Color{74, 34, 122, 255};
        d.scale  = 2.45f;
        d.anchor = Vector2{136.0f, 167.0f};
        d.walkSpeed = 230.0f; d.backSpeed = 210.0f; d.jumpSpeed = 920.0f; d.jumpForward = 260.0f;
        d.maxHealth = 900;    d.defenseScale = 1.15f; d.meterRate = 1.2f;
        d.normals.lightTo = 3;
        d.normals.reach = 1.25f;
        d.normals.power = 0.9f;
        d.normals.speed = 1.1f;
        d.normals.throwDamage = 100;
        d.voicePitch = 0.72f;
        d.factory = [](const CharacterDef &def, bool right) {
            return std::unique_ptr<Fighter>(new Wizard(def, right));
        };
        Add(std::move(d));
    }
}

} // namespace fighter
