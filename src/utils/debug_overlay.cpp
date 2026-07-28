#include "debug_overlay.h"
#include "logger.h"
#include <algorithm>

DebugOverlay & DebugOverlay::instance()
{
    static DebugOverlay inst;
    return inst;
}

void DebugOverlay::toggle()
{
    m_visible = !m_visible;
    LOG_DEBUG("Debug overlay %s", m_visible ? "shown" : "hidden");
}

void DebugOverlay::update(float dt)
{
    (void)dt;
    m_fps = GetFPS();
    m_frameTime = GetFrameTime() * 1000.0f;

    addHistoryPoint((float)m_fps);
}

void DebugOverlay::render()
{
    if (!m_visible) return;

    int x = 10, y = 10, lineH = 20;

    // Background
    int w = 320;
    int h = (int)m_info.size() * lineH + lineH * 2 + 40;
    Color bgColor = { 0, 0, 0, 180 };
    Color borderColor = { 100, 100, 100, 200 };
    Color graphBg = { 0, 0, 0, 100 };
    DrawRectangle(x, y, w, h, bgColor);
    DrawRectangleLines(x, y, w, h, borderColor);

    y += 10;

    DrawText(TextFormat("FPS: %d", m_fps), x + 10, y, 16, GREEN);
    DrawText(TextFormat("Frame: %.2f ms", m_frameTime), x + 130, y, 16, WHITE);
    y += lineH;

    if (m_ping > 0)
    {
        DrawText(TextFormat("Ping: %d ms", m_ping), x + 10, y, 16, WHITE);
        y += lineH;
    }

    for (auto & kv : m_info)
    {
        Color infoColor = { 245, 245, 245, 255 };
        DrawText(TextFormat("%s: %s", kv.first.c_str(), kv.second.c_str()),
                 x + 10, y, 14, infoColor);
        y += lineH;
    }

    // Mini FPS graph
    if (!m_fpsHistory.empty())
    {
        int gx = x + 10, gy = y + 10, gw = w - 20, gh = 50;
        DrawRectangle(gx, gy, gw, gh, graphBg);
        int n = (int)m_fpsHistory.size();
        float stepX = (float)gw / std::max(n, 2);
        float prevY = (float)gy;
        for (int i = 0; i < n; i++)
        {
            float val = m_fpsHistory[i] / 120.0f;
            float py = (float)(gy + gh) - val * gh;
            if (i > 0)
                DrawLine((int)(gx + (i - 1) * stepX), (int)prevY,
                         (int)(gx + i * stepX), (int)py, GREEN);
            prevY = py;
        }
    }
}

void DebugOverlay::setInfo(const std::string & key, const std::string & value)
{
    for (auto & kv : m_info)
    {
        if (kv.first == key)
        {
            kv.second = value;
            return;
        }
    }
    m_info.emplace_back(key, value);
}

void DebugOverlay::addHistoryPoint(float value)
{
    m_fpsHistory.push_back(value);
    if (m_fpsHistory.size() > HISTORY_MAX)
        m_fpsHistory.pop_front();
}

void DebugOverlay::renderGraph(const char * label, float minVal, float maxVal, Color col)
{
    (void)label;
    (void)minVal;
    (void)maxVal;
    (void)col;
}
