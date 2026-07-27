#include "config.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;

GameConfig promptConfig()
{
    GameConfig config;

    string os_input;
    while (true)
    {
        cout << "Are you running on Ubuntu or Windows? [ubuntu/windows]: ";
        cin >> os_input;
        if (os_input == "ubuntu" || os_input == "u")
        {
            config.os = OS_UBUNTU;
            break;
        }
        else if (os_input == "windows" || os_input == "w")
        {
            config.os = OS_WINDOWS;
            break;
        }
        else
        {
            cout << "Invalid choice. Please type 'ubuntu' or 'windows'." << endl;
        }
    }

    string lang_input;
    while (true)
    {
        cout << "Choose language / Chon ngon ngu? [en/vi]: ";
        cin >> lang_input;
        if (lang_input == "en" || lang_input == "english")
        {
            config.lang = LANG_ENGLISH;
            break;
        }
        else if (lang_input == "vi" || lang_input == "vietnamese")
        {
            config.lang = LANG_VIETNAMESE;
            break;
        }
        else
        {
            cout << "Invalid choice. Please type 'en' or 'vi'." << endl;
        }
    }

    return config;
}

void clearScreen(const GameConfig& config)
{
    if (config.os == OS_UBUNTU)
        system("clear");
    else
        system("cls");
}

void display_intro(const GameConfig& config)
{
    string filename = (config.lang == LANG_VIETNAMESE) ? "intro_vi.txt" : "intro.txt";
    string line;
    fstream myfile;
    myfile.open(filename.c_str());
    if (myfile.is_open())
    {
        while (getline(myfile, line))
            cout << line << endl;
        myfile.close();
    }
    else
    {
        string err = (config.lang == LANG_VIETNAMESE) ? "loi khong the mo file " : "error unable to open file ";
        cout << err << filename << endl;
    }
}

string msg(const GameConfig& config, int msgId)
{
    if (config.lang == LANG_ENGLISH)
    {
        switch (msgId)
        {
            case 1: return "Please enter amount of players: ";
            case 2: return " players entering game .... ";
            case 3: return "invalid amount of players";
            case 4: return " is randomly selected to play first";
            case 5: return "PLAYER ";
            case 6: return "Confirm Player";
            case 7: return " by typing '";
            case 8: return "' and pressing enter: ";
            case 9: return ": ";
            case 10: return "Forced Draw-2";
            case 11: return "Forced Draw-4";
            case 12: return "Cards remaining for each player: ";
            case 13: return "Played Card: ";
            case 14: return "which card do you want to play? ";
            case 15: return "If you want to draw a card please enter '-1' ";
            case 16: return "Type the index of the card and press enter: ";
            case 17: return "DRAWN CARD: ";
            case 18: return "Do you want to play the drawn card [y/n] : ";
            case 19: return "Please choose a color (red, green, blue, yellow): ";
            case 20: return "invalid color";
            case 21: return "card cannot be played ";
            case 22: return "invalid index ";
            case 23: return " has won the game.";
            case 24: return "===== MAIN MENU =====";
            case 25: return "1. Single Player (vs Bot)";
            case 26: return "2. Local Multiplayer";
            case 27: return "3. Mixed (Human + Bot)";
            case 28: return "4. LAN Multiplayer";
            case 29: return "5. Exit";
            case 30: return "Choose a game mode [1-5]: ";
            case 31: return "Invalid choice!";
            case 32: return "Select bot difficulty:";
            case 33: return "1. Easy";
            case 34: return "2. Normal";
            case 35: return "3. Hard";
            case 36: return "Choose difficulty [1-3]: ";
            case 37: return "Enter player name: ";
            case 38: return "Enter number of human players (1-4): ";
            case 39: return "Enter number of bot players (1-4): ";
            case 40: return "Bot";
            case 41: return "Human";
            case 42: return "Drawing card...";
            case 43: return "Press ENTER to continue...";
            case 44: return "UNO!";
            case 45: return "Forgetting to say UNO! Draw 2 penalty!";
            case 46: return " [UNO? (y/n)]: ";
            case 47: return "Stacking +2! Next player can stack another +2.";
            case 48: return "All players pass hands to the next! (0 rule)";
            case 49: return "Choose a player to swap hands with (0-";
            case 50: return " (7 rule - swap hands)";
            case 51: return "Enable Vietnamese rules? (stack +2, 7-0 rule) [y/n]: ";
            case 52: return "Vietnamese rules enabled!";
            case 53: return "Starting LAN server on port ";
            case 54: return "Connecting to LAN server...";
            case 55: return "Server is ready! Waiting for players...";
            case 56: return "1. Host game\n2. Join game\nChoose [1-2]: ";
            case 57: return "Enter server IP: ";
            case 58: return "Enter port (default 8888): ";
            case 59: return "Waiting for game state from server...";
            default: return "";
        }
    }
    else
    {
        switch (msgId)
        {
            case 1: return "Moi nhap so luong nguoi choi: ";
            case 2: return " nguoi choi vao game.... ";
            case 3: return "so luong nguoi choi khong hop le";
            case 4: return " duoc chon ngau nhien danh truoc";
            case 5: return "NGUOI CHOI ";
            case 6: return "Xac nhan Nguoi choi";
            case 7: return " bang cach go '";
            case 8: return "' va nhan enter: ";
            case 9: return ": ";
            case 10: return "Bi phat rut 2 la";
            case 11: return "Bi phat rut 4 la";
            case 12: return "So bai con lai cua moi nguoi choi: ";
            case 13: return "Bai da danh: ";
            case 14: return "Ban muon danh la nao? ";
            case 15: return "Neu muon rut bai, hay nhap '-1' ";
            case 16: return "Nhap so thu tu va nhan enter: ";
            case 17: return "BAI RUT DUOC: ";
            case 18: return "Ban co muon danh la vua rut? [c/k] : ";
            case 19: return "Chon mau (do, xanh la, xanh duong, vang): ";
            case 20: return "mau khong hop le";
            case 21: return "la bai khong the danh ";
            case 22: return "so thu tu khong hop le ";
            case 23: return " da thang cuoc.";
            case 24: return "===== MENU CHINH =====";
            case 25: return "1. Choi don (vs Bot)";
            case 26: return "2. Nhieu nguoi (cung may)";
            case 27: return "3. Hon hop (Nguoi + Bot)";
            case 28: return "4. Choi LAN";
            case 29: return "5. Thoat";
            case 30: return "Chon che do choi [1-5]: ";
            case 31: return "Lua chon khong hop le!";
            case 32: return "Chon do kho cua Bot:";
            case 33: return "1. De";
            case 34: return "2. Binh thuong";
            case 35: return "3. Kho";
            case 36: return "Chon do kho [1-3]: ";
            case 37: return "Nhap ten nguoi choi: ";
            case 38: return "Nhap so nguoi choi that (1-4): ";
            case 39: return "Nhap so Bot (1-4): ";
            case 40: return "May";
            case 41: return "Nguoi";
            case 42: return "Dang rut bai...";
            case 43: return "Nhan ENTER de tiep tuc...";
            case 44: return "UNO!";
            case 45: return "Quen ho UNO! Phat rut 2 la!";
            case 46: return " [UNO? (c/k)]: ";
            case 47: return "Chong +2! Nguoi tiep theo co the danh tiep +2.";
            case 48: return "Tat ca chuyen bai cho nhau! (Luat so 0)";
            case 49: return "Chon nguoi choi de doi bai (0-";
            case 50: return " (Luat so 7 - doi bai)";
            case 51: return "Bat luat choi kieu Viet Nam? (chong +2, luat 7-0) [c/k]: ";
            case 52: return "Da bat luat choi Viet Nam!";
            case 53: return "Dang mo server LAN tai cong ";
            case 54: return "Dang ket noi toi server LAN...";
            case 55: return "Server san sang! Dang cho nguoi choi...";
            case 56: return "1. Mo phong\n2. Tham gia\nChon [1-2]: ";
            case 57: return "Nhap dia chi IP server: ";
            case 58: return "Nhap cong (mac dinh 8888): ";
            case 59: return "Dang cho trang thai game tu server...";
            default: return "";
        }
    }
}
