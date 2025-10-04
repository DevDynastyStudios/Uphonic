
#pragma once

#include <algorithm>

static inline float uph_smooth_lerp(float current, float target, float speed, float dt)
{
    return current + (target - current) * std::min(1.0f, speed * dt);
}