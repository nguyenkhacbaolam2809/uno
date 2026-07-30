#include "settings.h"
#include <fstream>
#include <algorithm>
#include <stdexcept>

Settings & Settings::instance()
{
    static Settings inst;
    return inst;
}

void Settings::load(const std::string & path)
{
    m_data.clear();
    std::ifstream f(path);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        m_data[key] = unescape(val);
    }
}

void Settings::save(const std::string & path)
{
    std::ofstream f(path);
    if (!f.is_open()) return;
    f << "# UNO Game Settings\n";
    for (auto & kv : m_data)
        f << kv.first << "=" << escape(kv.second) << "\n";
}

int Settings::getInt(const std::string & key, int fallback) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return fallback;
    try { return std::stoi(it->second); } catch (...) { return fallback; }
}

float Settings::getFloat(const std::string & key, float fallback) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return fallback;
    try { return std::stof(it->second); } catch (...) { return fallback; }
}

bool Settings::getBool(const std::string & key, bool fallback) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return fallback;
    return it->second == "true" || it->second == "1";
}

std::string Settings::getString(const std::string & key, const std::string & fallback) const
{
    auto it = m_data.find(key);
    return it != m_data.end() ? it->second : fallback;
}

void Settings::setInt(const std::string & key, int val)
{
    m_data[key] = std::to_string(val);
}

void Settings::setFloat(const std::string & key, float val)
{
    m_data[key] = std::to_string(val);
}

void Settings::setBool(const std::string & key, bool val)
{
    m_data[key] = val ? "true" : "false";
}

void Settings::setString(const std::string & key, const std::string & val)
{
    m_data[key] = val;
}

bool Settings::has(const std::string & key) const
{
    return m_data.find(key) != m_data.end();
}

std::string Settings::escape(const std::string & s) const
{
    std::string r;
    r.reserve(s.size());
    for (char c : s)
    {
        if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '=') r += "\\=";
        else r += c;
    }
    return r;
}

std::string Settings::unescape(const std::string & s) const
{
    std::string r;
    r.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            if (s[i + 1] == 'n') r += '\n';
            else if (s[i + 1] == '\\') r += '\\';
            else if (s[i + 1] == '=') r += '=';
            else { r += s[i]; r += s[i + 1]; }
            i++;
        }
        else r += s[i];
    }
    return r;
}
