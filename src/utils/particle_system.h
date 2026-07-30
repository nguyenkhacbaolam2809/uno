#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "raylib.h"
#include <vector>

struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    Color endColor;
    Color startColor;
    float life;
    float maxLife;
    float size;
    float endSize;
    float startSize;
    bool active;
};

struct ParticleBurstConfig {
    Vector2 origin;
    int count{20};
    float speedMin{50}, speedMax{200};
    float lifeMin{0.3f}, lifeMax{0.8f};
    float sizeMin{2}, sizeMax{6};
    Color colorStart{WHITE};
    Color colorEnd{Fade(WHITE, 0)};
    float spread{360};
};

class ParticleSystem {
public:
    static ParticleSystem & instance();

    void update(float dt);
    void render();

    void burst(const ParticleBurstConfig & cfg);
    void emit(Vector2 pos, Vector2 vel, Color color, float life, float size);

    int activeCount() const noexcept;
    void clear();

private:
    ParticleSystem() = default;
    std::vector<Particle> m_particles;
    void spawn(Particle p);
};

#endif
