#pragma once
#include "raylib.h"
#include <vector>

namespace fighter {

// ============================================================================
// Background - sân đấu nhiều lớp parallax.
//
// Sáu lớp ảnh (trời, mây xa, núi xa, núi, mây gần, rừng cây) cuộn với tốc độ
// khác nhau theo vị trí camera, cộng thêm sàn đấu và hạt bụi bay để khung hình
// có chiều sâu thay vì một nền phẳng.
// ============================================================================
class Background {
public:
    void Load();
    void Update(float cameraX, float dt);
    void Draw() const;

    // Đổi tông màu theo nhân vật/độ khó (ví dụ trận Expert ngả đỏ).
    void SetTint(Color tint) { tint_ = tint; }

private:
    struct Layer {
        Texture2D tex{};
        float speed  = 0.1f;
        float offset = 0.0f;
        float yScale = 1.0f;
    };

    void DrawLayer(const Layer &l) const;
    void DrawFloor() const;

    std::vector<Layer> layers_;
    float time_   = 0.0f;
    float floorScroll_ = 0.0f;
    Color tint_   = WHITE;
    bool  loaded_ = false;
};

} // namespace fighter
