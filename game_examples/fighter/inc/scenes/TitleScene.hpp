#pragma once
#include "scenes/Scene.hpp"
#include "core/Animation.hpp"

namespace fighter {

// Màn tiêu đề: logo, bốn nhân vật đứng idle phía sau, menu chính.
class TitleScene final : public Scene {
public:
    void OnEnter(Game &game) override;
    void Update(Game &game, float dt) override;
    void Draw(Game &game) override;

private:
    void DrawMenu() const;

    float time_    = 0.0f;
    int   cursor_  = 0;
    float blink_   = 0.0f;
    Animator demo_[4];
};

} // namespace fighter
