#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <unordered_map>

class Settings {
public:
    static Settings & instance();

    void load(const std::string & path = "settings.cfg");
    void save(const std::string & path = "settings.cfg");

    int getInt(const std::string & key, int fallback = 0) const;
    float getFloat(const std::string & key, float fallback = 0.0f) const;
    bool getBool(const std::string & key, bool fallback = false) const;
    std::string getString(const std::string & key, const std::string & fallback = "") const;

    void setInt(const std::string & key, int val);
    void setFloat(const std::string & key, float val);
    void setBool(const std::string & key, bool val);
    void setString(const std::string & key, const std::string & val);

    bool has(const std::string & key) const;

private:
    Settings() = default;
    std::unordered_map<std::string, std::string> m_data;

    std::string escape(const std::string & s) const;
    std::string unescape(const std::string & s) const;
};

#endif
