#include "particle_system.h"
#include "rng.h"
#include <cmath>
#include <algorithm>

ParticleSystem & ParticleSystem::instance()
{
    static ParticleSystem inst;
    return inst;
}

void ParticleSystem::spawn(Particle p)
{
    for (auto & particle : m_particles)
    {
        if (!particle.active)
        {
            particle = p;
            return;
        }
    }
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
        p.color.r = (unsigned char)(p.startColor.r + (p.endColor.r - p.startColor.r) * t);
        p.color.g = (unsigned char)(p.startColor.g + (p.endColor.g - p.startColor.g) * t);
        p.color.b = (unsigned char)(p.startColor.b + (p.endColor.b - p.startColor.b) * t);
        p.color.a = (unsigned char)(p.startColor.a + (p.endColor.a - p.startColor.a) * t);
        p.size = p.startSize + (p.endSize - p.startSize) * t;
        if (p.life <= 0) p.active = false;
    }
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
        float angle = (float)randomInt(0, 9999) / 10000.0f * cfg.spread * DEG2RAD;
        float speed = cfg.speedMin + (float)randomInt(0, 9999) / 10000.0f * (cfg.speedMax - cfg.speedMin);
        float life = cfg.lifeMin + (float)randomInt(0, 9999) / 10000.0f * (cfg.lifeMax - cfg.lifeMin);
        float size = cfg.sizeMin + (float)randomInt(0, 9999) / 10000.0f * (cfg.sizeMax - cfg.sizeMin);
        Particle p;
        p.pos = cfg.origin;
        p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.color = cfg.colorStart;
        p.startColor = cfg.colorStart;
        p.endColor = cfg.colorEnd;
        p.life = life;
        p.maxLife = life;
        p.size = size;
        p.startSize = size;
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
    p.startColor = color;
    p.endColor = Fade(color, 0);
    p.life = life;
    p.maxLife = life;
    p.size = size;
    p.startSize = size;
    p.endSize = 0;
    p.active = true;
    spawn(p);
}

int ParticleSystem::activeCount() const noexcept
{
    int count = 0;
    for (auto & p : m_particles)
        if (p.active) count++;
    return count;
}

void ParticleSystem::clear() { m_particles.clear(); }
