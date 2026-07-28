#include "input_manager.h"
#include <algorithm>

void InputManager::update()
{
    m_mousePos = GetMousePosition();
    m_mouseDelta = GetMouseDelta();
    m_mouseWheel = GetMouseWheelMove();

    for (int i = 0; i < 3; i++)
    {
        m_prevMouse[i] = m_currMouse[i];
        m_currMouse[i] = IsMouseButtonDown(i);

        if (m_currMouse[i] && !m_prevMouse[i])
        {
            m_mousePressTime[i] = GetTime();
            m_lastClickPos[i] = m_mousePos;

            float timeSince = GetTime() - m_lastClickTime[i];
            float dist = Vector2Distance(m_mousePos, m_lastClickPos[i]);
            if (timeSince < 0.3f && dist < 10.0f)
                m_clickCount[i]++;
            else
                m_clickCount[i] = 1;
            m_lastClickTime[i] = GetTime();
        }
    }

    if (m_currMouse[0] && !m_prevMouse[0])
    {
        m_leftDragActive = true;
        m_dragStart = m_mousePos;
        m_dragStartTime = GetTime();
    }
    if (m_leftDragActive)
    {
        if (m_currMouse[0])
        {
            DragEvent ev;
            ev.startPos = m_dragStart;
            ev.currentPos = m_mousePos;
            ev.delta = m_mouseDelta;
            ev.button = 0;
            ev.ended = false;
            for (auto & cb : m_dragCbs) cb(ev);
        }
        else
        {
            DragEvent ev;
            ev.startPos = m_dragStart;
            ev.currentPos = m_mousePos;
            ev.delta = m_mouseDelta;
            ev.button = 0;
            ev.ended = true;
            for (auto & cb : m_dragCbs) cb(ev);
            m_leftDragActive = false;
        }
    }

    int key = GetKeyPressed();
    while (key > 0)
    {
        auto it = m_keyPressedCbs.find(key);
        if (it != m_keyPressedCbs.end())
            for (auto & cb : it->second) cb();
        key = GetKeyPressed();
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
    {
        int btn = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ? 0 : 1;
        ClickEvent ev;
        ev.pos = m_mousePos;
        ev.button = btn;
        ev.doubleClick = (m_clickCount[btn] >= 2);
        for (auto & cb : m_clickCbs) cb(ev);
    }
}

bool InputManager::isKeyDown(int key) const { return IsKeyDown(key); }
bool InputManager::isKeyPressed(int key) const { return IsKeyPressed(key); }
bool InputManager::isKeyReleased(int key) const { return IsKeyReleased(key); }
bool InputManager::isKeyRepeated(int key) const { return IsKeyPressed(key); }

bool InputManager::isMouseButtonDown(int btn) const { return m_currMouse[btn]; }
bool InputManager::isMouseButtonPressed(int btn) const
{
    return m_currMouse[btn] && !m_prevMouse[btn];
}
bool InputManager::isMouseButtonReleased(int btn) const
{
    return !m_currMouse[btn] && m_prevMouse[btn];
}
bool InputManager::isMouseDoubleClicked(int btn) const
{
    return m_clickCount[btn] >= 2;
}

bool InputManager::isLongPress(float duration) const
{
    return m_currMouse[0] && (GetTime() - m_mousePressTime[0]) >= duration;
}

void InputManager::onKeyPressed(int key, std::function<void()> callback)
{
    m_keyPressedCbs[key].push_back(std::move(callback));
}

void InputManager::onKeyReleased(int key, std::function<void()> callback)
{
    m_keyReleasedCbs[key].push_back(std::move(callback));
}

void InputManager::onClick(std::function<void(ClickEvent)> callback)
{
    m_clickCbs.push_back(std::move(callback));
}

void InputManager::onDrag(std::function<void(DragEvent)> callback)
{
    m_dragCbs.push_back(std::move(callback));
}

void InputManager::clearCallbacks()
{
    m_keyPressedCbs.clear();
    m_keyReleasedCbs.clear();
    m_clickCbs.clear();
    m_dragCbs.clear();
}
