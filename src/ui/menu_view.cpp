#include "menu_view.h"
#include "colors.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::BG_DARK;
using uno::GOLD_COLOR;
using uno::WHITE_SMOKE;

MenuView::MenuView(const GameConfig & cfg) : config(cfg)
{
}

const char * MenuView::msg(int id) const
{
    if (config.lang == Language::English)
    {
        switch (id)
        {
            case 1: return "UNO!";
            case 2: return "Single Player";
            case 3: return "Local Multiplayer";
            case 4: return "Mixed (Human + Bot)";
            case 5: return "LAN Multiplayer";
            case 6: return "Exit";
            case 7: return "Select Difficulty";
            case 8: return "Easy";
            case 9: return "Normal";
            case 10: return "Hard";
            case 11: return "Enter player name:";
            case 12: return "Number of players:";
            case 13: return "Host Game";
            case 14: return "Join Game";
            case 15: return "Server IP:";
            case 16: return "Port:";
            case 17: return "Back";
            case 18: return "Start Game";
            case 19: return "Vietnamese Rules? (Y/N)";
            case 20: return "Choose a color:";
            case 21: return "Number of bots:";
            case 22: return "Number of humans:";
            default: return "";
        }
    }
    switch (id)
    {
        case 1: return "UNO!";
        case 2: return "Choi Don";
        case 3: return "Nhieu Nguoi (cung may)";
        case 4: return "Hon Hop (Nguoi + Bot)";
        case 5: return "Choi LAN";
        case 6: return "Thoat";
        case 7: return "Chon Do Kho";
        case 8: return "De";
        case 9: return "Binh Thuong";
        case 10: return "Kho";
        case 11: return "Nhap ten:";
        case 12: return "So nguoi choi:";
        case 13: return "Mo phong";
        case 14: return "Tham gia";
        case 15: return "IP Server:";
        case 16: return "Cong:";
        case 17: return "Quay lai";
        case 18: return "Bat dau";
        case 19: return "Luat Viet Nam? (C/K)";
        case 20: return "Chon mau:";
        case 21: return "So Bot:";
        case 22: return "So nguoi:";
        default: return "";
    }
}

void MenuView::drawTitle(const char * text, int y, int fontSize, Color color)
{
    int tw = MeasureText(text, fontSize);
    DrawText(text, (SCREEN_W - tw) / 2, y, fontSize, color);
}

void MenuView::drawTextCentered(const char * text, int y, int fontSize, Color color)
{
    int tw = MeasureText(text, fontSize);
    DrawText(text, (SCREEN_W - tw) / 2, y, fontSize, color);
}

MenuView::Button MenuView::makeButton(int x, int y, int w, int h, const std::string & text, Color color)
{
    Button btn;
    btn.rect = { (float)x, (float)y, (float)w, (float)h };
    btn.text = text;
    btn.color = color;
    btn.hovered = false;
    return btn;
}

bool MenuView::drawButton(Button & btn)
{
    Vector2 m = GetMousePosition();
    btn.hovered = CheckCollisionPointRec(m, btn.rect);
    Color c = btn.hovered ? Fade(btn.color, 0.7f) : btn.color;

    DrawRectangleRounded(btn.rect, 0.3f, 10, c);
    DrawRectangleRoundedLines(btn.rect, 0.3f, 10, 2, Fade(WHITE, 0.3f));

    int fontSize = 22;
    int tw = MeasureText(btn.text.c_str(), fontSize);
    DrawText(btn.text.c_str(),
             (int)(btn.rect.x + (btn.rect.width - tw) / 2),
             (int)(btn.rect.y + (btn.rect.height - fontSize) / 2),
             fontSize, WHITE);

    return btn.hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

MenuResult MenuView::show()
{
    return showMainMenu();
}

MenuResult MenuView::showMainMenu()
{
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_DARK);

        drawTitle(msg(1), 60, 80, GOLD_COLOR);
        drawTextCentered("Multiplayer Card Game", 140, 24, WHITE_SMOKE);

        int bw = 300, bh = 50, bx = (SCREEN_W - bw) / 2, by = 200, gap = 15;

        Color redBtn = { 237, 28, 36, 255 };
        Color blueBtn = { 0, 114, 188, 255 };
        Color greenBtn = { 0, 155, 72, 255 };
        Color brownBtn = { 180, 100, 20, 255 };
        Color grayBtn = { 80, 80, 80, 255 };
        std::vector<Button> btns;
        btns.push_back(makeButton(bx, by, bw, bh, msg(2), redBtn));
        btns.push_back(makeButton(bx, by + (bh + gap) * 1, bw, bh, msg(3), blueBtn));
        btns.push_back(makeButton(bx, by + (bh + gap) * 2, bw, bh, msg(4), greenBtn));
        btns.push_back(makeButton(bx, by + (bh + gap) * 3, bw, bh, msg(5), brownBtn));
        btns.push_back(makeButton(bx, by + (bh + gap) * 4, bw, bh, msg(6), grayBtn));

        for (auto & btn : btns)
        {
            if (drawButton(btn))
            {
                EndDrawing();
                if (btn.text == msg(2)) return showDifficultySelect();
                if (btn.text == msg(3)) return showLocalSetup();
                if (btn.text == msg(4)) return showMixedSetup();
                if (btn.text == msg(5)) return showLanMenu();
                if (btn.text == msg(6))
                {
                    MenuResult r;
                    r.action = -1;
                    r.confirmed = true;
                    return r;
                }
            }
        }

        EndDrawing();
    }

    MenuResult r;
    r.action = -1;
    r.confirmed = true;
    return r;
}

MenuResult MenuView::showDifficultySelect()
{
    MenuResult result;
    result.action = 1;
    result.botDifficulty = 1;
    result.numHumans = 1;
    result.numBots = 1;
    result.vietRules = false;
    result.confirmed = false;
    result.port = 8888;

    char nameBuf[64] = "Player";
    int nameLen = (int)std::strlen(nameBuf);
    bool editingName = false;

    int diffChoice = 1;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_DARK);

        drawTitle(msg(2), 40, 48, GOLD_COLOR);

        int bw = 260, bh = 44, gap = 12;
        int startY = 120;

        drawTextCentered(msg(7), startY, 22, WHITE_SMOKE);
        std::vector<Button> diffBtns;
        Color diffEasy = { 0, 155, 72, 255 };
        Color diffMed = { 180, 140, 20, 255 };
        Color diffHard = { 237, 28, 36, 255 };
        diffBtns.push_back(makeButton((SCREEN_W - bw) / 2, startY + 40, bw, bh, msg(8), diffEasy));
        diffBtns.push_back(makeButton((SCREEN_W - bw) / 2, startY + 40 + (bh + gap), bw, bh, msg(9), diffMed));
        diffBtns.push_back(makeButton((SCREEN_W - bw) / 2, startY + 40 + (bh + gap) * 2, bw, bh, msg(10), diffHard));

        for (int i = 0; i < 3; i++)
        {
            if (drawButton(diffBtns[i]))
            {
                diffChoice = i + 1;
                result.botDifficulty = i + 1;
            }
            if (i == diffChoice - 1)
            {
                DrawRectangleRoundedLines(diffBtns[i].rect, 0.3f, 10, 3, GOLD_COLOR);
            }
        }

        drawTextCentered(msg(11), startY + 200, 20, WHITE_SMOKE);

        Rectangle nameRect = { (float)(SCREEN_W / 2 - 150), (float)(startY + 230), 300, 40 };
        Color nameRectColor = { 60, 60, 70, 255 };
        DrawRectangleRounded(nameRect, 0.3f, 10, nameRectColor);
        if (editingName)
            DrawRectangleRoundedLines(nameRect, 0.3f, 10, 2, GOLD_COLOR);

        DrawText(nameBuf, (int)nameRect.x + 10, (int)nameRect.y + 8, 22, WHITE);

        if (IsKeyPressed(KEY_ENTER)) editingName = !editingName;
        if (editingName)
        {
            int k = GetCharPressed();
            while (k > 0)
            {
                if (k >= 32 && k <= 126 && nameLen < 62)
                {
                    nameBuf[nameLen] = (char)k;
                    nameLen++;
                    nameBuf[nameLen] = 0;
                }
                k = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && nameLen > 0)
            {
                nameLen--;
                nameBuf[nameLen] = 0;
            }
        }

        int numBots = result.numBots;
        drawTextCentered(msg(21), startY + 290, 20, WHITE_SMOKE);
        DrawText(TextFormat("%d", numBots), SCREEN_W / 2 - 10, startY + 320, 28, WHITE);

        Rectangle minusRect2 = { (float)(SCREEN_W / 2 - 60), (float)(startY + 320), 40, 30 };
        Rectangle plusRect2 = { (float)(SCREEN_W / 2 + 20), (float)(startY + 320), 40, 30 };
        Color minusColor = { 200, 60, 60, 255 };
        Color plusColor = { 60, 200, 60, 255 };
        DrawRectangleRounded(minusRect2, 0.3f, 10, minusColor);
        DrawRectangleRounded(plusRect2, 0.3f, 10, plusColor);
        DrawText("-", (int)minusRect2.x + 13, (int)minusRect2.y + 2, 24, WHITE);
        DrawText("+", (int)plusRect2.x + 12, (int)plusRect2.y + 2, 24, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), minusRect2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            numBots = std::max(1, numBots - 1);
        if (CheckCollisionPointRec(GetMousePosition(), plusRect2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            numBots = std::min(4, numBots + 1);

        result.numBots = numBots;
        result.numHumans = 1;

        Rectangle vietBtn = { (float)(SCREEN_W / 2 - 150), (float)(startY + 380), 300, 40 };
        Color vietOn = { 0, 200, 100, 255 };
        Color vietOff = { 80, 80, 90, 255 };
        Color vietCol = result.vietRules ? vietOn : vietOff;
        DrawRectangleRounded(vietBtn, 0.3f, 10, vietCol);
        drawTextCentered(msg(19), startY + 383, 18, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), vietBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            result.vietRules = !result.vietRules;

        Rectangle startBtn = { (float)(SCREEN_W / 2 - 100), (float)(startY + 440), 200, 50 };
        Color startBtnColor = { 0, 155, 72, 255 };
        DrawRectangleRounded(startBtn, 0.3f, 10, startBtnColor);
        drawTextCentered(msg(18), startY + 445, 22, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), startBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            result.playerName = nameBuf;
            result.confirmed = true;
            EndDrawing();
            return result;
        }

        EndDrawing();
    }

    result.confirmed = false;
    result.action = -1;
    return result;
}

MenuResult MenuView::showLocalSetup()
{
    MenuResult result;
    result.action = 2;
    result.numHumans = 2;
    result.numBots = 0;
    result.vietRules = false;
    result.confirmed = false;
    result.port = 8888;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_DARK);

        drawTitle(msg(3), 40, 48, GOLD_COLOR);

        drawTextCentered(msg(12), 130, 22, WHITE_SMOKE);

        int nPlayers = result.numHumans;

        Rectangle minusRect = { (float)(SCREEN_W / 2 - 60), 170, 40, 40 };
        Rectangle plusRect = { (float)(SCREEN_W / 2 + 20), 170, 40, 40 };

        Color minusColor = { 200, 60, 60, 255 };
        Color plusColor = { 60, 200, 60, 255 };
        DrawRectangleRounded(minusRect, 0.3f, 10, minusColor);
        DrawRectangleRounded(plusRect, 0.3f, 10, plusColor);
        DrawText("-", (int)minusRect.x + 12, (int)minusRect.y + 6, 28, WHITE);
        DrawText("+", (int)plusRect.x + 10, (int)plusRect.y + 6, 28, WHITE);
        DrawText(TextFormat("%d", nPlayers), SCREEN_W / 2 - 12, 175, 32, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), minusRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            nPlayers = std::max(2, nPlayers - 1);
        if (CheckCollisionPointRec(GetMousePosition(), plusRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            nPlayers = std::min(4, nPlayers + 1);
        result.numHumans = nPlayers;

        Rectangle vietBtn = { (float)(SCREEN_W / 2 - 150), 240, 300, 40 };
        Color vietOn = { 0, 200, 100, 255 };
        Color vietOff = { 80, 80, 90, 255 };
        Color vietCol = result.vietRules ? vietOn : vietOff;
        DrawRectangleRounded(vietBtn, 0.3f, 10, vietCol);
        drawTextCentered(msg(19), 243, 18, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), vietBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            result.vietRules = !result.vietRules;

        Rectangle startBtn = { (float)(SCREEN_W / 2 - 100), 310, 200, 50 };
        Color startBtnColor = { 0, 155, 72, 255 };
        DrawRectangleRounded(startBtn, 0.3f, 10, startBtnColor);
        drawTextCentered(msg(18), 315, 22, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), startBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            result.confirmed = true;
            EndDrawing();
            return result;
        }

        EndDrawing();
    }

    result.action = -1;
    result.confirmed = false;
    return result;
}

MenuResult MenuView::showMixedSetup()
{
    MenuResult result;
    result.action = 3;
    result.numHumans = 1;
    result.numBots = 1;
    result.botDifficulty = 1;
    result.vietRules = false;
    result.confirmed = false;
    result.port = 8888;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_DARK);

        drawTitle(msg(4), 40, 48, GOLD_COLOR);

        drawTextCentered(msg(22), 120, 20, WHITE_SMOKE);
        drawTextCentered(msg(21), 200, 20, WHITE_SMOKE);

        int nHumans = result.numHumans;
        int nBots = result.numBots;

        DrawText(TextFormat("%d", nHumans), SCREEN_W / 2 - 60, 140, 32, WHITE);
        DrawText(TextFormat("%d", nBots), SCREEN_W / 2 - 60, 220, 32, WHITE);

        Rectangle hMinus = { (float)(SCREEN_W / 2 - 100), 140, 30, 30 };
        Rectangle hPlus  = { (float)(SCREEN_W / 2 - 30), 140, 30, 30 };
        Rectangle bMinus = { (float)(SCREEN_W / 2 - 100), 220, 30, 30 };
        Rectangle bPlus  = { (float)(SCREEN_W / 2 - 30), 220, 30, 30 };
        Color minusColor = { 200, 60, 60, 255 };
        Color plusColor = { 60, 200, 60, 255 };
        DrawRectangleRounded(hMinus, 0.3f, 10, minusColor);
        DrawRectangleRounded(hPlus, 0.3f, 10, plusColor);
        DrawRectangleRounded(bMinus, 0.3f, 10, minusColor);
        DrawRectangleRounded(bPlus, 0.3f, 10, plusColor);
        DrawText("-", (int)hMinus.x + 8, (int)hMinus.y + 2, 24, WHITE);
        DrawText("+", (int)hPlus.x + 7, (int)hPlus.y + 2, 24, WHITE);
        DrawText("-", (int)bMinus.x + 8, (int)bMinus.y + 2, 24, WHITE);
        DrawText("+", (int)bPlus.x + 7, (int)bPlus.y + 2, 24, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), hMinus) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            nHumans = std::max(1, nHumans - 1);
        if (CheckCollisionPointRec(GetMousePosition(), hPlus) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            nHumans = std::min(4, nHumans + 1);
        if (CheckCollisionPointRec(GetMousePosition(), bMinus) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            nBots = std::max(0, nBots - 1);
        if (CheckCollisionPointRec(GetMousePosition(), bPlus) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            nBots = std::min(4, nBots + 1);
        result.numHumans = nHumans;
        result.numBots = nBots;

        drawTextCentered(msg(7), 280, 20, WHITE_SMOKE);

        std::vector<Button> diffBtns;
        int bw = 200, bh = 36, gap = 8;
        int startX = (SCREEN_W - bw) / 2;
        Color diffEasy = { 0, 155, 72, 255 };
        Color diffMed = { 180, 140, 20, 255 };
        Color diffHard = { 237, 28, 36, 255 };
        diffBtns.push_back(makeButton(startX, 310, bw, bh, msg(8), diffEasy));
        diffBtns.push_back(makeButton(startX, 310 + (bh + gap), bw, bh, msg(9), diffMed));
        diffBtns.push_back(makeButton(startX, 310 + (bh + gap) * 2, bw, bh, msg(10), diffHard));

        for (int i = 0; i < 3; i++)
        {
            if (drawButton(diffBtns[i])) result.botDifficulty = i + 1;
            if (i == result.botDifficulty - 1)
                DrawRectangleRoundedLines(diffBtns[i].rect, 0.3f, 10, 3, GOLD_COLOR);
        }

        Rectangle vietBtn = { (float)(SCREEN_W / 2 - 150), 430, 300, 40 };
        Color vietOn = { 0, 200, 100, 255 };
        Color vietOff = { 80, 80, 90, 255 };
        Color vietCol = result.vietRules ? vietOn : vietOff;
        DrawRectangleRounded(vietBtn, 0.3f, 10, vietCol);
        drawTextCentered(msg(19), 433, 18, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), vietBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            result.vietRules = !result.vietRules;

        Rectangle startBtn = { (float)(SCREEN_W / 2 - 100), 490, 200, 50 };
        Color startBtnColor = { 0, 155, 72, 255 };
        DrawRectangleRounded(startBtn, 0.3f, 10, startBtnColor);
        drawTextCentered(msg(18), 495, 22, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), startBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            result.confirmed = true;
            EndDrawing();
            return result;
        }

        EndDrawing();
    }

    result.action = -1;
    result.confirmed = false;
    return result;
}

MenuResult MenuView::showColorPicker()
{
    MenuResult result;
    result.confirmed = true;
    result.action = 6;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_DARK);

        drawTitle(msg(20), SCREEN_H / 2 - 160, 36, WHITE_SMOKE);

        Color redCol = { 237, 28, 36, 255 };
        Color blueCol = { 0, 114, 188, 255 };
        Color greenCol = { 0, 155, 72, 255 };
        Color yellowCol = { 255, 205, 0, 255 };
        struct { int id; Color c; const char * label; } colors[4] = {
            { 1, redCol, "RED" },
            { 2, blueCol, "BLUE" },
            { 3, greenCol, "GREEN" },
            { 4, yellowCol, "YELLOW" }
        };

        int btnW = 120, btnH = 80, gap = 20;
        int totalW = 4 * btnW + 3 * gap;
        int startX = (SCREEN_W - totalW) / 2;
        int startY = SCREEN_H / 2 - btnH / 2;

        for (int i = 0; i < 4; i++)
        {
            Rectangle r = { (float)(startX + i * (btnW + gap)), (float)startY, (float)btnW, (float)btnH };
            Color c = colors[i].c;
            if (CheckCollisionPointRec(GetMousePosition(), r))
                c = Fade(c, 0.7f);
            DrawRectangleRounded(r, 0.3f, 10, c);

            int lw = MeasureText(colors[i].label, 16);
            DrawText(colors[i].label, (int)(r.x + (r.width - lw) / 2), (int)(r.y + (r.height - 16) / 2), 16, WHITE);

            if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                result.action = colors[i].id;
                EndDrawing();
                return result;
            }
        }

        Rectangle backBtn = { (float)(SCREEN_W / 2 - 70), (float)(startY + btnH + 40), 140, 40 };
        DrawRectangleRounded(backBtn, 0.3f, 10, Color{ 80, 80, 80, 255 });
        drawTextCentered(msg(17), startY + btnH + 44, 20, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            result.action = -1;
            result.confirmed = false;
            EndDrawing();
            return result;
        }

        EndDrawing();
    }

    result.action = -1;
    result.confirmed = false;
    return result;
}

MenuResult MenuView::showLanMenu()
{
    MenuResult result;
    result.action = 4;
    result.confirmed = false;
    result.port = 8888;
    result.serverIp = "127.0.0.1";

    char ipBuf[64] = "127.0.0.1";
    int ipLen = (int)std::strlen(ipBuf);
    char portBuf[8] = "8888";
    int portLen = 4;
    bool editingIp = false, editingPort = false;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_DARK);

        drawTitle(msg(5), 40, 48, GOLD_COLOR);

        int bw = 260, bh = 50, gap = 20;
        int bx = (SCREEN_W - bw) / 2;
        int by = 160;

        Color hostBtnColor = { 0, 114, 188, 255 };
        Color joinBtnColor = { 0, 155, 72, 255 };
        Button hostBtn = makeButton(bx, by, bw, bh, msg(13), hostBtnColor);
        Button joinBtn = makeButton(bx, by + bh + gap, bw, bh, msg(14), joinBtnColor);

        if (drawButton(hostBtn))
        {
            result.action = 4;
            result.confirmed = true;
            EndDrawing();
            return result;
        }

        if (drawButton(joinBtn))
        {
            result.action = 5;
            result.confirmed = true;
            result.port = std::atoi(portBuf);
            result.serverIp = ipBuf;
            EndDrawing();
            return result;
        }

        drawTextCentered(msg(15), by + 140, 20, WHITE_SMOKE);
        Rectangle ipRect = { (float)(SCREEN_W / 2 - 150), (float)(by + 170), 300, 36 };
        Color ipRectColor = { 60, 60, 70, 255 };
        DrawRectangleRounded(ipRect, 0.3f, 10, ipRectColor);
        if (editingIp) DrawRectangleRoundedLines(ipRect, 0.3f, 10, 2, GOLD_COLOR);
        DrawText(ipBuf, (int)ipRect.x + 10, (int)ipRect.y + 6, 20, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), ipRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        { editingIp = true; editingPort = false; }

        drawTextCentered(msg(16), by + 230, 20, WHITE_SMOKE);
        Rectangle portRect = { (float)(SCREEN_W / 2 - 150), (float)(by + 260), 300, 36 };
        Color portRectColor = { 60, 60, 70, 255 };
        DrawRectangleRounded(portRect, 0.3f, 10, portRectColor);
        if (editingPort) DrawRectangleRoundedLines(portRect, 0.3f, 10, 2, GOLD_COLOR);
        DrawText(portBuf, (int)portRect.x + 10, (int)portRect.y + 6, 20, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), portRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        { editingPort = true; editingIp = false; }

        if (editingIp)
        {
            int k = GetCharPressed();
            while (k > 0)
            {
                if (k >= 32 && k <= 126 && ipLen < 62)
                { ipBuf[ipLen] = (char)k; ipLen++; ipBuf[ipLen] = 0; }
                k = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && ipLen > 0)
            { ipLen--; ipBuf[ipLen] = 0; }
            if (IsKeyPressed(KEY_ENTER)) editingIp = false;
        }

        if (editingPort)
        {
            int k = GetCharPressed();
            while (k > 0)
            {
                if (k >= '0' && k <= '9' && portLen < 6)
                { portBuf[portLen] = (char)k; portLen++; portBuf[portLen] = 0; }
                k = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && portLen > 0)
            { portLen--; portBuf[portLen] = 0; }
            if (IsKeyPressed(KEY_ENTER)) editingPort = false;
        }

        EndDrawing();
    }

    result.action = -1;
    result.confirmed = false;
    return result;
}
