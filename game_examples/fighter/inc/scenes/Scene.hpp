#pragma once

namespace fighter {

class Game;

// ============================================================================
// Scene - một màn hình của game (Title, Settings, Battle).
//
// Game chỉ giữ con trỏ Scene* và gọi Update/Draw, nên thêm màn hình mới (bảng
// xếp hạng, chế độ luyện tập...) không phải sửa vòng lặp chính.
// ============================================================================
class Scene {
public:
    virtual ~Scene() = default;

    virtual void OnEnter(Game &game) { (void)game; }
    virtual void OnExit(Game &game)  { (void)game; }
    virtual void Update(Game &game, float dt) = 0;
    virtual void Draw(Game &game) = 0;
};

} // namespace fighter
