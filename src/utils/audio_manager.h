#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "raylib.h"
#include <string>
#include <unordered_map>

enum class SoundId {
    CARD_SLIDE,
    CARD_DRAW,
    UNO_BUTTON,
    CATCH_UNO,
    WIN,
    LOSE,
    HOVER,
    BUTTON_CLICK,
    REVERSE,
    SKIP,
    WILD_CHOOSE
};

class AudioManager {
public:
    static AudioManager & instance();

    bool init();
    void shutdown();

    void loadSounds();
    void playSound(SoundId id);
    void playMusic(const std::string & path);

    void setMasterVolume(float vol);     // 0.0 - 1.0
    void setMusicVolume(float vol);      // 0.0 - 1.0
    void setEffectsVolume(float vol);    // 0.0 - 1.0

    float masterVolume() const { return m_masterVol; }
    float musicVolume() const { return m_musicVol; }
    float effectsVolume() const { return m_effectsVol; }

    bool isMuted() const { return m_muted; }
    void setMuted(bool muted) { m_muted = muted; }
    void toggleMute() { m_muted = !m_muted; }

    void update();

private:
    AudioManager() = default;
    ~AudioManager();

    bool m_initialized{false};
    bool m_muted{false};
    float m_masterVol{1.0f};
    float m_musicVol{1.0f};
    float m_effectsVol{1.0f};

    std::unordered_map<SoundId, Sound> m_sounds;
    Music m_music{};
    bool m_musicPlaying{false};

    std::unordered_map<SoundId, std::string> soundPaths();
    void loadSoundForId(SoundId id);
};

#endif
