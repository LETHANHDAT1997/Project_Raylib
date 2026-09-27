#include "render/Background.hpp"
#include "core/Assets.hpp"
#include "core/Config.hpp"
#include <cmath>

namespace fighter {

namespace {
// Lớp càng gần người xem thì cuộn càng nhanh - đó là toàn bộ mẹo parallax.
struct LayerSetup { const char *file; float speed; };
const LayerSetup kSetup[] = {
    {"sky.png",            0.00f},
    {"far-clouds.png",     0.05f},
    {"far-mountains.png",  0.12f},
    {"mountains.png",      0.22f},
    {"near-clouds.png",    0.32f},
    {"trees.png",          0.48f},
};
} // namespace

void Background::Load()
{
    if (loaded_) return;
    loaded_ = true;

    Assets &assets = Assets::Instance();
    for (const LayerSetup &s : kSetup) {
        Layer l;
        l.tex   = assets.Texture(std::string("backgrounds/mountain_dusk/") + s.file);
        l.speed = s.speed;
        layers_.push_back(l);
    }
}

void Background::Update(float cameraX, float dt)
{
    time_ += dt;
    for (auto &l : layers_) {
        // Mây trôi nhẹ kể cả khi camera đứng yên.
        const float drift = (l.speed > 0.0f && l.speed < 0.35f) ? time_ * 6.0f * l.speed : 0.0f;
        l.offset = cameraX * l.speed + drift;
    }
    floorScroll_ = cameraX * 0.72f;
}

void Background::DrawLayer(const Layer &l) const
{
    if (l.tex.id == 0 || l.tex.height <= 0) return;

    // Kéo ảnh cho cao bằng canvas rồi lát ngang cho đủ chiều rộng.
    const float scale = (float)kCanvasHeight / (float)l.tex.height;
    const float w     = l.tex.width * scale;
    if (w <= 1.0f) return;

    float start = -std::fmod(l.offset, w);
    if (start > 0.0f) start -= w;

    for (float x = start; x < kCanvasWidth; x += w) {
        DrawTexturePro(l.tex,
                       Rectangle{0.0f, 0.0f, (float)l.tex.width, (float)l.tex.height},
                       Rectangle{x, 0.0f, w, (float)kCanvasHeight},
                       Vector2{0.0f, 0.0f}, 0.0f, tint_);
    }
}

void Background::DrawFloor() const
{
    // Sàn đấu: một dải gỗ có vân chạy ngang, tối dần về phía dưới.
    const int top = (int)kGroundY;
    const int h   = kCanvasHeight - top;

    for (int y = 0; y < h; ++y) {
        const float t = (float)y / (float)h;
        Color c{
            (unsigned char)(96 - 46 * t),
            (unsigned char)(64 - 30 * t),
            (unsigned char)(48 - 22 * t),
            255
        };
        DrawRectangle(0, top + y, kCanvasWidth, 1, c);
    }

    // Vân ván sàn cuộn theo camera cho có cảm giác di chuyển.
    const float plank = 96.0f;
    float start = -std::fmod(floorScroll_, plank);
    if (start > 0.0f) start -= plank;
    for (float x = start; x < kCanvasWidth; x += plank) {
        DrawLine((int)x, top, (int)(x - 22.0f), kCanvasHeight, Color{40, 26, 20, 90});
    }

    // Viền sáng ở mép sàn cho nhân vật "đứng" rõ ràng hơn.
    DrawRectangle(0, top - 3, kCanvasWidth, 3, Color{176, 140, 96, 120});
    DrawRectangle(0, top, kCanvasWidth, 2, Color{38, 24, 18, 180});
}

void Background::Draw() const
{
    ClearBackground(Color{18, 16, 30, 255});
    for (const auto &l : layers_) DrawLayer(l);

    // Phủ tối phần trời phía trên để HUD nổi lên.
    DrawRectangleGradientV(0, 0, kCanvasWidth, 190, Color{6, 6, 18, 170}, Color{0, 0, 0, 0});

    DrawFloor();

    // Làm tối hai mép màn hình (vignette) cho khung hình có chiều sâu.
    DrawRectangleGradientH(0, 0, 160, kCanvasHeight, Color{0, 0, 0, 120}, Color{0, 0, 0, 0});
    DrawRectangleGradientH(kCanvasWidth - 160, 0, 160, kCanvasHeight,
                           Color{0, 0, 0, 0}, Color{0, 0, 0, 120});
}

} // namespace fighter
