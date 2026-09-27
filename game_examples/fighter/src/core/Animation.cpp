#include "core/Animation.hpp"
#include <cmath>

namespace fighter {

const char *AnimFileName(AnimId id)
{
    switch (id) {
        case AnimId::Idle:    return "idle";
        case AnimId::Run:     return "run";
        case AnimId::Jump:    return "jump";
        case AnimId::Fall:    return "fall";
        case AnimId::Attack1: return "attack1";
        case AnimId::Attack2: return "attack2";
        case AnimId::TakeHit: return "takehit";
        case AnimId::Death:   return "death";
        default:              return "idle";
    }
}

Rectangle Animation::FrameRect(int index) const
{
    if (frameCount <= 0 || texture.height <= 0) return Rectangle{0, 0, 0, 0};
    if (index < 0) index = 0;
    if (index >= frameCount) index = frameCount - 1;
    const float side = (float)texture.height;
    return Rectangle{index * side, 0.0f, side, side};
}

void Animator::Play(const Animation *anim, bool resetIfSame)
{
    if (anim_ == anim && !resetIfSame) return;
    anim_     = anim;
    timer_    = 0.0f;
    frame_    = 0;
    finished_ = false;
}

void Animator::Update(float dt)
{
    if (!anim_ || anim_->frameCount <= 1) {
        frame_ = 0;
        return;
    }

    timer_ += dt * speed_;
    const float step = 1.0f / anim_->fps;

    while (timer_ >= step) {
        timer_ -= step;
        if (frame_ + 1 < anim_->frameCount) {
            ++frame_;
        } else if (anim_->loop) {
            frame_ = 0;
        } else {
            finished_ = true;
            timer_ = 0.0f;
            break;
        }
    }
}

float Animator::Progress() const
{
    if (!anim_ || anim_->frameCount <= 0) return 0.0f;
    const float perFrame = 1.0f / anim_->fps;
    const float within   = (perFrame > 0.0f) ? (timer_ / perFrame) : 0.0f;
    return ((float)frame_ + within) / (float)anim_->frameCount;
}

} // namespace fighter
