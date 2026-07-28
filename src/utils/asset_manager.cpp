#include "asset_manager.h"
#include "logger.h"
#include <algorithm>

AssetManager & AssetManager::instance()
{
    static AssetManager inst;
    return inst;
}

AssetManager::~AssetManager()
{
    unloadAll();
}

Result<Texture2D> AssetManager::loadTexture(const std::string & path)
{
    auto it = m_textures.find(path);
    if (it != m_textures.end())
    {
        it->second.refCount++;
        return it->second.tex;
    }

    Texture2D tex = ::LoadTexture(path.c_str());
    if (tex.id == 0)
        return Result<Texture2D>::fail("Failed to load texture: " + path);

    m_textures[path] = { tex, 1 };
    return tex;
}

Result<Font> AssetManager::loadFont(const std::string & path, int fontSize)
{
    std::string key = path + "@" + std::to_string(fontSize);
    auto it = m_fonts.find(key);
    if (it != m_fonts.end())
    {
        it->second.refCount++;
        return it->second.font;
    }

    Font f = ::LoadFontEx(path.c_str(), fontSize, nullptr, 0);
    if (f.texture.id == 0)
        return Result<Font>::fail("Failed to load font: " + path);

    m_fonts[key] = { f, 1 };
    return f;
}

Result<Sound> AssetManager::loadSound(const std::string & path)
{
    auto it = m_sounds.find(path);
    if (it != m_sounds.end())
    {
        it->second.refCount++;
        return it->second.sound;
    }

    Sound s = ::LoadSound(path.c_str());
    if (s.stream.buffer == nullptr)
        return Result<Sound>::fail("Failed to load sound: " + path);

    m_sounds[path] = { s, 1 };
    return s;
}

Result<Music> AssetManager::loadMusic(const std::string & path)
{
    auto it = m_music.find(path);
    if (it != m_music.end())
    {
        it->second.refCount++;
        return it->second.music;
    }

    Music m = ::LoadMusicStream(path.c_str());
    if (m.stream.buffer == nullptr)
        return Result<Music>::fail("Failed to load music: " + path);

    m_music[path] = { m, 1 };
    return m;
}

void AssetManager::unloadTexture(const std::string & path)
{
    auto it = m_textures.find(path);
    if (it == m_textures.end()) return;
    if (--it->second.refCount <= 0)
    {
        ::UnloadTexture(it->second.tex);
        m_textures.erase(it);
    }
}

void AssetManager::unloadFont(const std::string & path)
{
    auto it = m_fonts.find(path);
    if (it == m_fonts.end()) return;
    if (--it->second.refCount <= 0)
    {
        ::UnloadFont(it->second.font);
        m_fonts.erase(it);
    }
}

void AssetManager::unloadSound(const std::string & path)
{
    auto it = m_sounds.find(path);
    if (it == m_sounds.end()) return;
    if (--it->second.refCount <= 0)
    {
        ::UnloadSound(it->second.sound);
        m_sounds.erase(it);
    }
}

void AssetManager::unloadMusic(const std::string & path)
{
    auto it = m_music.find(path);
    if (it == m_music.end()) return;
    if (--it->second.refCount <= 0)
    {
        ::UnloadMusicStream(it->second.music);
        m_music.erase(it);
    }
}

Texture2D * AssetManager::getTexture(const std::string & path)
{
    auto it = m_textures.find(path);
    return it != m_textures.end() ? &it->second.tex : nullptr;
}

Font * AssetManager::getFont(const std::string & path)
{
    auto it = m_fonts.find(path);
    return it != m_fonts.end() ? &it->second.font : nullptr;
}

Sound * AssetManager::getSound(const std::string & path)
{
    auto it = m_sounds.find(path);
    return it != m_sounds.end() ? &it->second.sound : nullptr;
}

Music * AssetManager::getMusic(const std::string & path)
{
    auto it = m_music.find(path);
    return it != m_music.end() ? &it->second.music : nullptr;
}

bool AssetManager::hasTexture(const std::string & path) const
{
    return m_textures.find(path) != m_textures.end();
}

bool AssetManager::hasFont(const std::string & path) const
{
    return m_fonts.find(path) != m_fonts.end();
}

void AssetManager::preloadCommon()
{
    Result<Font> f = loadFont("assets/fonts/main.ttf", 24);
    if (f.isFail())
        LOG_WARN("Common font not loaded: %s", f.error().c_str());
}

void AssetManager::unloadAll()
{
    for (auto & kv : m_textures)
        ::UnloadTexture(kv.second.tex);
    m_textures.clear();

    for (auto & kv : m_fonts)
        ::UnloadFont(kv.second.font);
    m_fonts.clear();

    for (auto & kv : m_sounds)
        ::UnloadSound(kv.second.sound);
    m_sounds.clear();

    for (auto & kv : m_music)
        ::UnloadMusicStream(kv.second.music);
    m_music.clear();

    clearTextCache();
}

AssetStats AssetManager::stats() const
{
    AssetStats s;
    s.textureCount = (int)m_textures.size();
    s.fontCount = (int)m_fonts.size();
    s.audioCount = (int)(m_sounds.size() + m_music.size());
    for (auto & kv : m_textures)
        s.estimatedMemoryBytes += kv.second.tex.width * kv.second.tex.height * 4;
    s.estimatedMemoryBytes += m_textCache.size() * 128 * 32 * 4;
    return s;
}

const AssetManager::CachedText * AssetManager::getCachedText(
    const std::string & text, int fontSize, Color color)
{
    TextCacheKey key{ text, fontSize, color };
    auto it = m_textCache.find(key);
    if (it != m_textCache.end())
        return &it->second;

    Image img = ImageTextEx(GetFontDefault(), text.c_str(), (float)fontSize, 1, color);
    if (img.data == nullptr)
    {
        img = ::GenImageColor(1, 1, BLANK);
    }
    CachedText ct;
    ct.tex = ::LoadTextureFromImage(img);
    ct.width = img.width;
    ct.height = img.height;
    ::UnloadImage(img);

    m_textCache[key] = ct;
    return &m_textCache[key];
}

void AssetManager::clearTextCache()
{
    for (auto & kv : m_textCache)
        ::UnloadTexture(kv.second.tex);
    m_textCache.clear();
}
