#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "raylib.h"
#include <unordered_map>
#include <functional>
#include <vector>

struct ClickEvent {
    Vector2 pos;
    int button;
    bool doubleClick;
};

struct DragEvent {
    Vector2 startPos;
    Vector2 currentPos;
    Vector2 delta;
    int button;
    bool ended;
};

class InputManager {
public:
    static InputManager & instance();

    void update();

    bool isKeyDown(int key) const noexcept;
    bool isKeyPressed(int key) const noexcept;
    bool isKeyReleased(int key) const noexcept;
    bool isKeyRepeated(int key) const noexcept;

    bool isMouseButtonDown(int btn) const noexcept;
    bool isMouseButtonPressed(int btn) const noexcept;
    bool isMouseButtonReleased(int btn) const noexcept;
    bool isMouseDoubleClicked(int btn) const noexcept;

    Vector2 mousePosition() const noexcept { return m_mousePos; }
    Vector2 mouseDelta() const noexcept { return m_mouseDelta; }
    float mouseWheel() const noexcept { return m_mouseWheel; }

    bool isLongPress(float duration = 0.5f) const noexcept;

    void onKeyPressed(int key, std::function<void()> callback);
    void onKeyReleased(int key, std::function<void()> callback);
    void onClick(std::function<void(ClickEvent)> callback);
    void onDrag(std::function<void(DragEvent)> callback);

    void clearCallbacks();

private:
    InputManager() = default;

    Vector2 m_mousePos{};
    Vector2 m_mouseDelta{};
    float m_mouseWheel{0};
    bool m_prevMouse[3]{};
    bool m_currMouse[3]{};
    float m_mousePressTime[3]{};
    float m_lastClickTime[3]{};
    Vector2 m_lastClickPos[3]{};
    int m_clickCount[3]{};

    bool m_leftDragActive{false};
    Vector2 m_dragStart{};
    float m_dragStartTime{0};

    std::unordered_map<int, std::vector<std::function<void()>>> m_keyPressedCbs;
    std::unordered_map<int, std::vector<std::function<void()>>> m_keyReleasedCbs;
    std::vector<std::function<void(ClickEvent)>> m_clickCbs;
    std::vector<std::function<void(DragEvent)>> m_dragCbs;
};

#endif
