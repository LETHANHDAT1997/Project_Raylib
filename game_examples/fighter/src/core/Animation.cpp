#include "core/Animation.hpp"
#include <algorithm>

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
    index = std::clamp(index, 0, frameCount - 1);
    const float side = (float)texture.height;
    return Rectangle{index * side, 0.0f, side, side};
}

int Animator::First() const
{
    if (!anim_) return 0;
    return std::clamp(mode_.from, 0, anim_->frameCount - 1);
}

int Animator::Last() const
{
    if (!anim_) return 0;
    const int last = (mode_.to < 0) ? anim_->frameCount - 1 : mode_.to;
    return std::clamp(last, First(), anim_->frameCount - 1);
}

int Animator::ClipLength() const
{
    return anim_ ? (Last() - First() + 1) : 1;
}

void Animator::Play(const Animation *anim, bool restart)
{
    PlayMode m;
    m.loop = anim ? anim->loop : true;
    Play(anim, m, restart);
}

void Animator::Play(const Animation *anim, const PlayMode &mode, bool restart)
{
    const bool same = anim_ == anim && mode_.from == mode.from && mode_.to == mode.to &&
                      mode_.loop == mode.loop && mode_.reverse == mode.reverse;
    if (same && !restart) return;

    anim_     = anim;
    mode_     = mode;
    timer_    = 0.0f;
    finished_ = false;
    frame_    = mode.reverse ? Last() : First();
}

void Animator::Update(float dt)
{
    if (!anim_ || ClipLength() <= 1) {
        frame_ = First();
        return;
    }

    timer_ += dt * speed_;
    const float step = 1.0f / anim_->fps;

    while (timer_ >= step && !finished_) {
        timer_ -= step;
        if (!mode_.reverse) {
            if (frame_ < Last())      ++frame_;
            else if (mode_.loop)      frame_ = First();
            else                      finished_ = true;
        } else {
            if (frame_ > First())     --frame_;
            else if (mode_.loop)      frame_ = Last();
            else                      finished_ = true;
        }
    }
}

} // namespace fighter
