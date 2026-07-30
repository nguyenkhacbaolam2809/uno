#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "raylib.h"
#include <unordered_map>
#include <memory>
#include <functional>

enum class EaseType {
    LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_OUT_BACK,
    EASE_OUT_ELASTIC,
    EASE_OUT_BOUNCE
};

float easeApply(EaseType type, float t);

struct AnimValue {
    float start;
    float end;
    float current;
};

enum class AnimState { PENDING, RUNNING, FINISHED };

class Animation {
public:
    virtual ~Animation() = default;
    virtual void update(float dt) = 0;
    virtual bool isFinished() const noexcept = 0;
    virtual AnimState state() const noexcept { return m_state; }
    virtual void reset() { m_state = AnimState::PENDING; m_elapsed = 0; }

protected:
    AnimState m_state{AnimState::PENDING};
    float m_elapsed{0};
    float m_duration{1};
    EaseType m_ease{EaseType::EASE_OUT_QUAD};
};

class FloatAnim : public Animation {
public:
    FloatAnim(float start, float end, float duration, EaseType ease = EaseType::EASE_OUT_QUAD);
    void update(float dt) override;
    bool isFinished() const noexcept override { return m_state == AnimState::FINISHED; }
    float value() const noexcept { return m_current; }
    void onFinish(std::function<void()> cb) { m_onFinish = std::move(cb); }
    void onUpdate(std::function<void(float)> cb) { m_onUpdate = std::move(cb); }
    void reset() override { Animation::reset(); m_current = m_start; }

private:
    float m_start, m_end, m_current;
    std::function<void()> m_onFinish;
    std::function<void(float)> m_onUpdate;
};

class Vec2Anim : public Animation {
public:
    Vec2Anim(Vector2 start, Vector2 end, float duration, EaseType ease = EaseType::EASE_OUT_QUAD);
    void update(float dt) override;
    bool isFinished() const noexcept override { return m_state == AnimState::FINISHED; }
    Vector2 value() const noexcept { return m_current; }
    void onFinish(std::function<void()> cb) { m_onFinish = std::move(cb); }
    void onUpdate(std::function<void(Vector2)> cb) { m_onUpdate = std::move(cb); }
    void reset() override { Animation::reset(); m_current = m_start; }

private:
    Vector2 m_current, m_start, m_end;
    std::function<void()> m_onFinish;
    std::function<void(Vector2)> m_onUpdate;
};

class ColorAnim : public Animation {
public:
    ColorAnim(Color start, Color end, float duration, EaseType ease = EaseType::LINEAR);
    void update(float dt) override;
    bool isFinished() const noexcept override { return m_state == AnimState::FINISHED; }
    Color value() const noexcept { return m_current; }
    void onUpdate(std::function<void(Color)> cb) { m_onUpdate = std::move(cb); }
    void reset() override { Animation::reset(); m_current = m_start; }

private:
    Color m_current, m_start, m_end;
    std::function<void(Color)> m_onUpdate;
};

class ShakeAnim : public Animation {
public:
    ShakeAnim(float intensity, float duration);
    void update(float dt) override;
    bool isFinished() const noexcept override { return m_state == AnimState::FINISHED; }
    Vector2 offset() const noexcept { return m_offset; }
    void onUpdate(std::function<void(Vector2)> cb) { m_onUpdate = std::move(cb); }
    void reset() override { Animation::reset(); m_offset = { 0, 0 }; }

private:
    float m_intensity;
    Vector2 m_offset;
    std::function<void(Vector2)> m_onUpdate;
};

class DelayAnim : public Animation {
public:
    explicit DelayAnim(float duration);
    void update(float dt) override;
    bool isFinished() const noexcept override { return m_state == AnimState::FINISHED; }
    void onFinish(std::function<void()> cb) { m_onFinish = std::move(cb); }
    void reset() override { Animation::reset(); }

private:
    std::function<void()> m_onFinish;
};

class SequenceAnim : public Animation {
public:
    void add(std::unique_ptr<Animation> anim);
    void update(float dt) override;
    bool isFinished() const noexcept override;
    void reset() override;

private:
    std::vector<std::unique_ptr<Animation>> m_anims;
    int m_currentIdx{0};
};

class ParallelAnim : public Animation {
public:
    void add(std::unique_ptr<Animation> anim);
    void update(float dt) override;
    bool isFinished() const noexcept override;
    void reset() override;

private:
    std::vector<std::unique_ptr<Animation>> m_anims;
};

class AnimationManager {
public:
    static AnimationManager & instance();

    void update(float dt);

    int add(std::unique_ptr<Animation> anim);
    void remove(int id);
    void clear();
    int activeCount() const noexcept;

    // Convenience helpers
    int animateFloat(float start, float end, float duration,
                     std::function<void(float)> onUpdate,
                     std::function<void()> onFinish = nullptr,
                     EaseType ease = EaseType::EASE_OUT_QUAD);

    int animateVec2(Vector2 start, Vector2 end, float duration,
                    std::function<void(Vector2)> onUpdate,
                    std::function<void()> onFinish = nullptr,
                    EaseType ease = EaseType::EASE_OUT_QUAD);

    int animateColor(Color start, Color end, float duration,
                     std::function<void(Color)> onUpdate,
                     EaseType ease = EaseType::LINEAR);

    int delay(float duration, std::function<void()> onFinish);

    int shake(float intensity, float duration,
              std::function<void(Vector2)> onUpdate);

private:
    AnimationManager() = default;

    std::unordered_map<int, std::unique_ptr<Animation>> m_animations;
    int m_nextId{1};
};

#endif
