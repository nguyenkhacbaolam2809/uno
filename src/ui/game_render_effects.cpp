#include "game_view.h"
#include "card_renderer.h"
#include "colors.h"
#include "particle_system.h"
#include "animation_manager.h"
#include <cmath>

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::CARD_WIDTH;
using uno::CARD_HEIGHT;
using uno::GOLD_COLOR;

// Visual effects that enhance the game experience.
// These are lightweight on purpose — no per-frame allocations.

void GameView::renderWinConfetti(const GameEngine & engine)
{
    if (engine.getPhase() != PHASE_GAME_OVER) return;

    ParticleBurstConfig cfg;
    cfg.origin = { (float)SCREEN_W / 2, (float)SCREEN_H / 2 };
    cfg.count = 60;
    cfg.speedMin = 100;
    cfg.speedMax = 400;
    cfg.lifeMin = 1.0f;
    cfg.lifeMax = 2.5f;
    cfg.sizeMin = 3;
    cfg.sizeMax = 8;
    cfg.colorStart = GOLD_COLOR;
    cfg.colorEnd = Fade(GOLD_COLOR, 0);
    cfg.spread = 360;

    static double lastBurst = 0;
    double now = GetTime();
    if (now - lastBurst > 1.0)
    {
        ParticleSystem::instance().burst(cfg);
        lastBurst = now;
    }
}

void GameView::renderCardGlow(const card & c, int x, int y, float scale)
{
    (void)c;
    int w = static_cast<int>(CARD_WIDTH * scale);
    int h = static_cast<int>(CARD_HEIGHT * scale);
    Color glow = Fade(GOLD_COLOR, 0.15f + 0.1f * std::sin(GetTime() * 4));
    Rectangle glowRect = { (float)x - 3, (float)y - 3, (float)w + 6, (float)h + 6 };
    DrawRectangleRounded(glowRect, 0.3f, 15, glow);
}
