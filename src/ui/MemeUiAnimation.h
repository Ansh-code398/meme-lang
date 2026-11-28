#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

namespace MemeUI {

struct SpringConfig {
    float stiffness = 220.0f;
    float damping = 24.0f;
    float mass = 1.0f;
};

struct AnimatedScalar {
    float current = 0.0f;
    float velocity = 0.0f;
    float target = 0.0f;
    bool initialized = false;
    bool active = false;
};

inline bool stepSpring(AnimatedScalar &s, float dtSec, const SpringConfig &cfg = {}) {
    if (!s.initialized) {
        s.current = s.target;
        s.velocity = 0.0f;
        s.initialized = true;
        s.active = false;
        return false;
    }
    float dt = std::clamp(dtSec, 0.001f, 0.05f);
    float disp = s.current - s.target;
    if (std::abs(disp) < 0.001f && std::abs(s.velocity) < 0.002f) {
        s.current = s.target;
        s.velocity = 0.0f;
        s.active = false;
        return false;
    }
    float force = -cfg.stiffness * disp - cfg.damping * s.velocity;
    float accel = force / std::max(0.05f, cfg.mass);
    s.velocity += accel * dt;
    s.current += s.velocity * dt;
    s.active = true;
    return true;
}

inline float evalEase(const std::string &curve, float t) {
    float x = std::clamp(t, 0.0f, 1.0f);
    if (curve == "linear") return x;
    if (curve == "ease-in") return x * x * x;
    if (curve == "ease-out") {
        float inv = 1.0f - x;
        return 1.0f - inv * inv * inv;
    }
    // ease-in-out cubic smoothstep
    return x < 0.5f ? 4.0f * x * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 3.0f) * 0.5f;
}

class UiAnimationStore {
public:
    std::unordered_map<std::string, AnimatedScalar> scalars;
    double elapsedSec = 0.0;
    bool hasLoopingKeyframe = false;

    void setTarget(const std::string &key, float target, float initialVal, bool hasInitial) {
        auto &s = scalars[key];
        if (!s.initialized) {
            s.current = hasInitial ? initialVal : target;
            s.target = target;
            s.velocity = 0.0f;
            s.initialized = true;
            s.active = (std::abs(s.current - s.target) > 0.001f);
        } else if (std::abs(s.target - target) > 0.0005f) {
            s.target = target;
            s.active = true;
        }
    }

    float getValue(const std::string &key, float fallback) const {
        auto it = scalars.find(key);
        return (it != scalars.end() && it->second.initialized) ? it->second.current : fallback;
    }

    bool tickAll(double dtMs, const SpringConfig &cfg = {}) {
        float dtSec = static_cast<float>(dtMs / 1000.0);
        elapsedSec += std::max(0.001, dtMs / 1000.0);
        bool anyActive = hasLoopingKeyframe;
        for (auto &[k, sc] : scalars) {
            if (sc.active || std::abs(sc.current - sc.target) > 0.001f) {
                if (stepSpring(sc, dtSec, cfg)) anyActive = true;
            }
        }
        return anyActive;
    }
};

} // namespace MemeUI
