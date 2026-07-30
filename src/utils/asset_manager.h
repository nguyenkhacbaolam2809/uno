#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "raylib.h"
#include "result.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

struct AssetStats {
    int textureCount{0};
    int fontCount{0};
    int audioCount{0};
    size_t estimatedMemoryBytes{0};
};

class AssetManager {
public:
    static AssetManager & instance();

    Result<Texture2D> loadTexture(const std::string & path);
    Result<Font> loadFont(const std::string & path, int fontSize = 24);
    Result<Sound> loadSound(const std::string & path);
    Result<Music> loadMusic(const std::string & path);

    void unloadTexture(const std::string & path);
    void unloadFont(const std::string & path);
    void unloadSound(const std::string & path);
    void unloadMusic(const std::string & path);

    Texture2D * getTexture(const std::string & path);
    Font * getFont(const std::string & path);
    Sound * getSound(const std::string & path);
    Music * getMusic(const std::string & path);

    bool hasTexture(const std::string & path) const;
    bool hasFont(const std::string & path) const;

    void preloadCommon();
    void unloadAll();

    AssetStats stats() const;

    // Text texture caching
    struct CachedText {
        Texture2D tex;
        int width;
        int height;
    };
    const CachedText * getCachedText(const std::string & text, int fontSize, Color color);
    void drawCachedText(const std::string & text, int posX, int posY, int fontSize, Color color);
    Vector2 measureCachedText(const std::string & text, int fontSize) const;
    void clearTextCache();

private:
    AssetManager() = default;
    ~AssetManager();
    AssetManager(const AssetManager &) = delete;
    AssetManager & operator=(const AssetManager &) = delete;

    struct RefCountedTexture {
        Texture2D tex;
        int refCount{1};
    };
    struct RefCountedFont {
        Font font;
        int refCount{1};
    };
    struct RefCountedSound {
        Sound sound;
        int refCount{1};
    };
    struct RefCountedMusic {
        Music music;
        int refCount{1};
    };

    std::unordered_map<std::string, RefCountedTexture> m_textures;
    std::unordered_map<std::string, RefCountedFont> m_fonts;
    std::unordered_map<std::string, RefCountedSound> m_sounds;
    std::unordered_map<std::string, RefCountedMusic> m_music;

    struct TextCacheKey {
        std::string text;
        int fontSize;
        Color color;
        bool operator==(const TextCacheKey & o) const {
            return text == o.text && fontSize == o.fontSize &&
                   color.r == o.color.r && color.g == o.color.g &&
                   color.b == o.color.b && color.a == o.color.a;
        }
    };
    struct TextCacheKeyHash {
        size_t operator()(const TextCacheKey & k) const {
            return std::hash<std::string>()(k.text) ^
                   std::hash<int>()(k.fontSize) ^
                   k.color.r ^ (k.color.g << 8) ^ (k.color.b << 16) ^ (k.color.a << 24);
        }
    };
    std::unordered_map<TextCacheKey, CachedText, TextCacheKeyHash> m_textCache;
};

#endif
