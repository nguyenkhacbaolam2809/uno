#include "animation_manager.h"
#include "rng.h"
#include <algorithm>
#include <cmath>

float easeApply(EaseType type, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    switch (type)
    {
        case EaseType::LINEAR:
            return t;
        case EaseType::EASE_IN_QUAD:
            return t * t;
        case EaseType::EASE_OUT_QUAD:
            return t * (2 - t);
        case EaseType::EASE_IN_OUT_QUAD:
            return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
        case EaseType::EASE_OUT_BACK:
        {
            float c = 1.70158f;
            return 1 + (c + 1) * std::pow(t - 1, 3) + c * std::pow(t - 1, 2);
        }
        case EaseType::EASE_OUT_ELASTIC:
        {
            if (t == 0 || t == 1) return t;
            return std::pow(2, -10 * t) * std::sin((t - 0.075) * (2 * 3.14159f) / 0.3f) + 1;
        }
        case EaseType::EASE_OUT_BOUNCE:
        {
            if (t < 1 / 2.75f) return 7.5625f * t * t;
            else if (t < 2 / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; }
            else if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; }
            else { t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f; }
        }
        default:
            return t;
    }
}

// FloatAnim
FloatAnim::FloatAnim(float start, float end, float duration, EaseType ease)
{
    m_current = start;
    m_duration = duration;
    m_ease = ease;
    m_start = start;
    m_end = end;
}

void FloatAnim::update(float dt)
{
    if (m_state != AnimState::RUNNING) return;
    m_elapsed += dt;
    float t = std::min(m_elapsed / m_duration, 1.0f);
    m_current = m_start + (m_end - m_start) * easeApply(m_ease, t);
    if (t >= 1.0f)
    {
        m_state = AnimState::FINISHED;
        if (m_onFinish) m_onFinish();
    }
}

// Vec2Anim
Vec2Anim::Vec2Anim(Vector2 start, Vector2 end, float duration, EaseType ease)
    : m_current(start), m_start(start), m_end(end)
{
    m_duration = duration;
    m_ease = ease;
}

void Vec2Anim::update(float dt)
{
    if (m_state != AnimState::RUNNING) return;
    m_elapsed += dt;
    float t = std::min(m_elapsed / m_duration, 1.0f);
    float e = easeApply(m_ease, t);
    m_current.x = m_start.x + (m_end.x - m_start.x) * e;
    m_current.y = m_start.y + (m_end.y - m_start.y) * e;
    if (t >= 1.0f)
    {
        m_state = AnimState::FINISHED;
        if (m_onFinish) m_onFinish();
    }
}

// ColorAnim
ColorAnim::ColorAnim(Color start, Color end, float duration, EaseType ease)
    : m_current(start), m_start(start), m_end(end)
{
    m_duration = duration;
    m_ease = ease;
}

void ColorAnim::update(float dt)
{
    if (m_state != AnimState::RUNNING) return;
    m_elapsed += dt;
    float t = std::min(m_elapsed / m_duration, 1.0f);
    float e = easeApply(m_ease, t);
    m_current.r = (unsigned char)(m_start.r + (m_end.r - m_start.r) * e);
    m_current.g = (unsigned char)(m_start.g + (m_end.g - m_start.g) * e);
    m_current.b = (unsigned char)(m_start.b + (m_end.b - m_start.b) * e);
    m_current.a = (unsigned char)(m_start.a + (m_end.a - m_start.a) * e);
    if (t >= 1.0f) m_state = AnimState::FINISHED;
}

// ShakeAnim
ShakeAnim::ShakeAnim(float intensity, float duration)
    : m_intensity(intensity)
{
    m_duration = duration;
}

void ShakeAnim::update(float dt)
{
    if (m_state != AnimState::RUNNING) return;
    m_elapsed += dt;
    float t = std::min(m_elapsed / m_duration, 1.0f);
    float decay = 1.0f - t;
    float angle = (float)(randomInt(0, 627)) / 100.0f;
    m_offset.x = std::cos(angle) * m_intensity * decay;
    m_offset.y = std::sin(angle) * m_intensity * decay;
    if (t >= 1.0f)
    {
        m_offset = { 0, 0 };
        m_state = AnimState::FINISHED;
    }
}

// DelayAnim
DelayAnim::DelayAnim(float duration)
{
    m_duration = duration;
}

void DelayAnim::update(float dt)
{
    if (m_state != AnimState::RUNNING) return;
    m_elapsed += dt;
    if (m_elapsed >= m_duration)
    {
        m_state = AnimState::FINISHED;
        if (m_onFinish) m_onFinish();
    }
}

// SequenceAnim
void SequenceAnim::add(std::unique_ptr<Animation> anim)
{
    m_anims.push_back(std::move(anim));
}

void SequenceAnim::update(float dt)
{
    if (m_state != AnimState::RUNNING) return;
    if (m_currentIdx >= (int)m_anims.size())
    {
        m_state = AnimState::FINISHED;
        return;
    }
    if (m_anims[m_currentIdx]->state() == AnimState::PENDING)
        m_anims[m_currentIdx]->reset();
    m_anims[m_currentIdx]->update(dt);
    if (m_anims[m_currentIdx]->isFinished())
        m_currentIdx++;
}

bool SequenceAnim::isFinished() const
{
    return m_currentIdx >= (int)m_anims.size();
}

void SequenceAnim::reset()
{
    Animation::reset();
    m_currentIdx = 0;
    for (auto & a : m_anims) a->reset();
}

// ParallelAnim
void ParallelAnim::add(std::unique_ptr<Animation> anim)
{
    m_anims.push_back(std::move(anim));
}

void ParallelAnim::update(float dt)
{
    if (m_state != AnimState::RUNNING) return;
    bool allDone = true;
    for (auto & a : m_anims)
    {
        if (a->state() == AnimState::PENDING)
            a->reset();
        if (!a->isFinished())
        {
            a->update(dt);
            if (!a->isFinished()) allDone = false;
        }
    }
    if (allDone) m_state = AnimState::FINISHED;
}

bool ParallelAnim::isFinished() const
{
    for (auto & a : m_anims)
        if (!a->isFinished()) return false;
    return true;
}

void ParallelAnim::reset()
{
    Animation::reset();
    for (auto & a : m_anims) a->reset();
}

// AnimationManager
AnimationManager & AnimationManager::instance()
{
    static AnimationManager inst;
    return inst;
}

void AnimationManager::update(float dt)
{
    for (auto & entry : m_animations)
    {
        if (entry.anim->state() == AnimState::PENDING)
            entry.anim->reset();
        if (!entry.anim->isFinished())
            entry.anim->update(dt);
    }
    m_animations.erase(
        std::remove_if(m_animations.begin(), m_animations.end(),
            [](const Entry & e) { return e.anim->isFinished(); }),
        m_animations.end());
}

int AnimationManager::add(std::unique_ptr<Animation> anim)
{
    int id = m_nextId++;
    m_animations.push_back({ id, std::move(anim) });
    return id;
}

void AnimationManager::remove(int id)
{
    m_animations.erase(
        std::remove_if(m_animations.begin(), m_animations.end(),
            [id](const Entry & e) { return e.id == id; }),
        m_animations.end());
}

void AnimationManager::clear() { m_animations.clear(); }
int AnimationManager::activeCount() const { return (int)m_animations.size(); }

int AnimationManager::animateFloat(float start, float end, float duration,
    std::function<void(float)> onUpdate, std::function<void()> onFinish, EaseType ease)
{
    auto anim = std::make_unique<FloatAnim>(start, end, duration, ease);
    anim->onFinish([onUpdate, onFinish, animPtr = anim.get()]() {
        onUpdate(animPtr->value());
        if (onFinish) onFinish();
    });
    int id = m_nextId++;
    m_animations.push_back({ id, std::move(anim) });
    return id;
}

int AnimationManager::animateVec2(Vector2 start, Vector2 end, float duration,
    std::function<void(Vector2)> onUpdate, std::function<void()> onFinish, EaseType ease)
{
    auto anim = std::make_unique<Vec2Anim>(start, end, duration, ease);
    anim->onFinish([onUpdate, onFinish, animPtr = anim.get()]() {
        onUpdate(animPtr->value());
        if (onFinish) onFinish();
    });
    int id = m_nextId++;
    m_animations.push_back({ id, std::move(anim) });
    return id;
}

int AnimationManager::animateColor(Color start, Color end, float duration,
    std::function<void(Color)> onUpdate, EaseType ease)
{
    (void)onUpdate;
    auto anim = std::make_unique<ColorAnim>(start, end, duration, ease);
    int id = m_nextId++;
    // Poll-based: caller reads value each frame
    m_animations.push_back({ id, std::move(anim) });
    return id;
}

int AnimationManager::delay(float duration, std::function<void()> onFinish)
{
    auto anim = std::make_unique<DelayAnim>(duration);
    anim->onFinish(std::move(onFinish));
    int id = m_nextId++;
    m_animations.push_back({ id, std::move(anim) });
    return id;
}

int AnimationManager::shake(float intensity, float duration,
    std::function<void(Vector2)> onUpdate)
{
    (void)onUpdate;
    auto anim = std::make_unique<ShakeAnim>(intensity, duration);
    int id = m_nextId++;
    m_animations.push_back({ id, std::move(anim) });
    return id;
}
