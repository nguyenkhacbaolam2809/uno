#include "audio_manager.h"
#include "logger.h"
#include <algorithm>

AudioManager & AudioManager::instance()
{
    static AudioManager inst;
    return inst;
}

AudioManager::~AudioManager()
{
    shutdown();
}

bool AudioManager::init()
{
    if (m_initialized) return true;
    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        LOG_ERROR("%s", "Failed to initialize audio device");
        return false;
    }
    m_initialized = true;
    LOG_INFO("%s", "Audio device initialized");
    return true;
}

void AudioManager::shutdown()
{
    for (auto & kv : m_sounds)
        ::UnloadSound(kv.second);
    m_sounds.clear();

    if (m_musicPlaying)
    {
        ::StopMusicStream(m_music);
        ::UnloadMusicStream(m_music);
        m_musicPlaying = false;
    }

    if (m_initialized)
    {
        CloseAudioDevice();
        m_initialized = false;
    }
}

std::unordered_map<SoundId, std::string> AudioManager::soundPaths()
{
    return {
        { SoundId::CARD_SLIDE,    "assets/sounds/card_slide.wav" },
        { SoundId::CARD_DRAW,     "assets/sounds/card_draw.wav" },
        { SoundId::UNO_BUTTON,    "assets/sounds/uno.wav" },
        { SoundId::CATCH_UNO,     "assets/sounds/catch_uno.wav" },
        { SoundId::WIN,           "assets/sounds/win.wav" },
        { SoundId::LOSE,          "assets/sounds/lose.wav" },
        { SoundId::HOVER,         "assets/sounds/hover.wav" },
        { SoundId::BUTTON_CLICK,  "assets/sounds/click.wav" },
        { SoundId::REVERSE,       "assets/sounds/reverse.wav" },
        { SoundId::SKIP,          "assets/sounds/skip.wav" },
        { SoundId::WILD_CHOOSE,   "assets/sounds/wild.wav" },
    };
}

void AudioManager::loadSounds()
{
    auto paths = soundPaths();
    for (auto & kv : paths)
    {
        loadSoundForId(kv.first);
    }
}

void AudioManager::loadSoundForId(SoundId id)
{
    auto paths = soundPaths();
    auto it = paths.find(id);
    if (it == paths.end()) return;

    Sound s = ::LoadSound(it->second.c_str());
    if (s.stream.buffer != nullptr)
        m_sounds[id] = s;
    else
        LOG_WARN("Failed to load sound: %s", it->second.c_str());
}

void AudioManager::playSound(SoundId id)
{
    if (m_muted) return;
    auto it = m_sounds.find(id);
    if (it == m_sounds.end())
    {
        loadSoundForId(id);
        it = m_sounds.find(id);
        if (it == m_sounds.end()) return;
    }
    float vol = m_masterVol * m_effectsVol;
    ::SetSoundVolume(it->second, vol);
    ::PlaySound(it->second);
}

void AudioManager::playMusic(const std::string & path)
{
    if (m_musicPlaying)
    {
        ::StopMusicStream(m_music);
        ::UnloadMusicStream(m_music);
        m_musicPlaying = false;
    }

    m_music = ::LoadMusicStream(path.c_str());
    if (m_music.stream.buffer == nullptr)
    {
        LOG_WARN("Failed to load music: %s", path.c_str());
        return;
    }

    float vol = m_masterVol * m_musicVol;
    ::SetMusicVolume(m_music, vol);
    ::PlayMusicStream(m_music);
    m_musicPlaying = true;
}

void AudioManager::setMasterVolume(float vol)
{
    m_masterVol = std::clamp(vol, 0.0f, 1.0f);
    ::SetMasterVolume(m_muted ? 0 : m_masterVol);
}

void AudioManager::setMusicVolume(float vol)
{
    m_musicVol = std::clamp(vol, 0.0f, 1.0f);
    if (m_musicPlaying)
        ::SetMusicVolume(m_music, m_masterVol * m_musicVol);
}

void AudioManager::setEffectsVolume(float vol)
{
    m_effectsVol = std::clamp(vol, 0.0f, 1.0f);
}

void AudioManager::update()
{
    if (m_musicPlaying)
    {
        ::UpdateMusicStream(m_music);
        if (!::IsMusicStreamPlaying(m_music))
            m_musicPlaying = false;
    }
}
