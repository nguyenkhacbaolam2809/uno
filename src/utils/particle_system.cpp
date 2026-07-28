#include "particle_system.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

ParticleSystem & ParticleSystem::instance()
{
    static ParticleSystem inst;
    return inst;
}

void ParticleSystem::spawn(Particle p)
{
    m_particles.push_back(p);
}

void ParticleSystem::update(float dt)
{
    for (auto & p : m_particles)
    {
        if (!p.active) continue;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.life -= dt;
        float t = 1.0f - std::max(p.life / p.maxLife, 0.0f);
        p.color.r = (unsigned char)(p.color.r + (p.endColor.r - p.color.r) * t);
        p.color.g = (unsigned char)(p.color.g + (p.endColor.g - p.color.g) * t);
        p.color.b = (unsigned char)(p.color.b + (p.endColor.b - p.color.b) * t);
        p.color.a = (unsigned char)(p.color.a + (p.endColor.a - p.color.a) * t);
        p.size = p.size + (p.endSize - p.size) * t;
        if (p.life <= 0) p.active = false;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
            [](const Particle & p) { return !p.active; }),
        m_particles.end());
}

void ParticleSystem::render()
{
    for (auto & p : m_particles)
    {
        if (!p.active) continue;
        DrawCircleV(p.pos, p.size, p.color);
    }
}

void ParticleSystem::burst(const ParticleBurstConfig & cfg)
{
    for (int i = 0; i < cfg.count; i++)
    {
        float angle = (float)(rand() % 10000) / 10000.0f * cfg.spread * DEG2RAD;
        float speed = cfg.speedMin + (float)(rand() % 10000) / 10000.0f * (cfg.speedMax - cfg.speedMin);
        float life = cfg.lifeMin + (float)(rand() % 10000) / 10000.0f * (cfg.lifeMax - cfg.lifeMin);
        float size = cfg.sizeMin + (float)(rand() % 10000) / 10000.0f * (cfg.sizeMax - cfg.sizeMin);
        Particle p;
        p.pos = cfg.origin;
        p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.color = cfg.colorStart;
        p.endColor = cfg.colorEnd;
        p.life = life;
        p.maxLife = life;
        p.size = size;
        p.endSize = 0;
        p.active = true;
        spawn(p);
    }
}

void ParticleSystem::emit(Vector2 pos, Vector2 vel, Color color, float life, float size)
{
    Particle p;
    p.pos = pos;
    p.vel = vel;
    p.color = color;
    p.endColor = Fade(color, 0);
    p.life = life;
    p.maxLife = life;
    p.size = size;
    p.endSize = 0;
    p.active = true;
    spawn(p);
}

int ParticleSystem::activeCount() const { return (int)m_particles.size(); }
void ParticleSystem::clear() { m_particles.clear(); }
