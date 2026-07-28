#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#include "raylib.h"
#include <string>
#include <functional>
#include <deque>

class DebugOverlay {
public:
    static DebugOverlay & instance();

    void toggle();
    bool isVisible() const { return m_visible; }

    void update(float dt);
    void render();

    void setFPS(int fps) { m_fps = fps; }
    void setFrameTime(float ms) { m_frameTime = ms; }
    void setPing(int ms) { m_ping = ms; }
    void setInfo(const std::string & key, const std::string & value);

    void addHistoryPoint(float value);
    void renderGraph(const char * label, float minVal, float maxVal, Color col);

private:
    DebugOverlay() = default;

    bool m_visible{false};
    int m_fps{0};
    float m_frameTime{0};
    int m_ping{0};
    std::vector<std::pair<std::string, std::string>> m_info;

    std::deque<float> m_fpsHistory;
    static constexpr int HISTORY_MAX = 120;
};

#endif
