#ifdef __APPLE__

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <dirent.h>

// Client root path (defined as a compile-time macro)
#ifndef MCE_CLIENT_ROOT
#define MCE_CLIENT_ROOT "."
#endif

static ma_engine g_audioEngine;
static bool g_audioInitialized = false;
static float g_musicVolume = 1.0f;
static float g_effectsVolume = 1.0f;
static ma_sound g_musicSound;
static bool g_musicLoaded = false;
static std::mutex g_audioMutex;

struct ActiveSound
{
    ma_sound sound;
};

static std::vector<std::unique_ptr<ActiveSound>> g_activeSounds;

// Sound index: maps XWB base name (e.g. "mob_cow") to list of WAV file paths
static std::unordered_map<std::string, std::vector<std::string>> g_soundIndex;

// Strip trailing digits from a filename to get the base name for variant grouping.
// e.g. "mob_cow1" -> "mob_cow", "random_click" -> "random_click"
static std::string stripTrailingDigits(const std::string &name)
{
    size_t end = name.size();
    while (end > 0 && name[end - 1] >= '0' && name[end - 1] <= '9')
        end--;
    return (end > 0) ? name.substr(0, end) : name;
}

// Scan the sounds/ directory and build the variant index
static void buildSoundIndex()
{
    g_soundIndex.clear();
    std::string soundsDir = std::string(MCE_CLIENT_ROOT) + "/sounds";
    DIR *dir = opendir(soundsDir.c_str());
    if (!dir)
    {
        printf("[AppleAudio] WARNING: sounds/ directory not found at %s\n", soundsDir.c_str());
        return;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename(entry->d_name);
        if (filename.size() < 5) continue; // skip . and ..
        size_t extPos = filename.rfind(".wav");
        if (extPos == std::string::npos || extPos + 4 != filename.size()) continue;

        std::string nameNoExt = filename.substr(0, extPos);
        std::string baseName = stripTrailingDigits(nameNoExt);
        std::string fullPath = soundsDir + "/" + filename;

        g_soundIndex[baseName].push_back(fullPath);
        // Also index the exact name (for entries without trailing digits)
        if (baseName != nameNoExt)
            g_soundIndex[nameNoExt].push_back(fullPath);
        count++;
    }
    closedir(dir);
    printf("[AppleAudio] Sound index: %d WAV files, %d base names\n", count, (int)g_soundIndex.size());
}

// Build full path from relative sound path (e.g. "mob/cow" -> picks random mob_cow*.wav)
static std::string buildSoundPath(const char *relativePath)
{
    // Convert slashes to underscores to match XWB naming
    std::string xwbName;
    for (const char *p = relativePath; *p; p++)
        xwbName += (*p == '/') ? '_' : *p;

    // Look up in sound index
    auto it = g_soundIndex.find(xwbName);
    if (it != g_soundIndex.end() && !it->second.empty())
    {
        const auto &variants = it->second;
        return variants[rand() % variants.size()];
    }
    return "";
}

static std::string buildUIPath(const char *uiName)
{
    // Map UI sound names to actual WAV files
    std::string base = std::string(MCE_CLIENT_ROOT) + "/Common/Media/Sound/";
    if (strcmp(uiName, "back") == 0) return base + "btn_Back.wav";
    if (strcmp(uiName, "press") == 0 || strcmp(uiName, "craft") == 0 || strcmp(uiName, "focus") == 0) return base + "click.wav";
    if (strcmp(uiName, "scroll") == 0) return base + "pop.wav";
    if (strcmp(uiName, "craftfail") == 0) return base + "wood click.wav";
    return "";
}

static std::string buildMusicPath(const char *filePath)
{
    // filePath is like "music/music/calm1.binka" or "music/cds/cat.binka"
    // Try OGG first (user-converted), then WAV, then print info about binka
    std::string pathStr(filePath);

    // Strip .binka extension if present
    std::string baseName = pathStr;
    size_t extPos = baseName.rfind(".binka");
    if (extPos != std::string::npos) baseName = baseName.substr(0, extPos);

    static const char *extensions[] = { ".ogg", ".wav", ".mp3", ".flac", nullptr };

    // Try relative to MCE_CLIENT_ROOT
    for (int i = 0; extensions[i]; ++i)
    {
        std::string fullPath = std::string(MCE_CLIENT_ROOT) + "/" + baseName + extensions[i];
        FILE *f = fopen(fullPath.c_str(), "rb");
        if (f) { fclose(f); return fullPath; }
    }

    // Try relative to Durango layout (where the binka files are)
    for (int i = 0; extensions[i]; ++i)
    {
        std::string fullPath = std::string(MCE_CLIENT_ROOT) + "/Durango/Layout/Image/Loose/" + baseName + extensions[i];
        FILE *f = fopen(fullPath.c_str(), "rb");
        if (f) { fclose(f); return fullPath; }
    }

    return "";
}

static float clampNonNegative(float value)
{
    return value < 0.0f ? 0.0f : value;
}

static float sanitizePitch(float pitch)
{
    return pitch > 0.01f ? pitch : 1.0f;
}

static void cleanupFinishedSoundsLocked()
{
    g_activeSounds.erase(
        std::remove_if(
            g_activeSounds.begin(),
            g_activeSounds.end(),
            [](std::unique_ptr<ActiveSound> &activeSound) {
                if (!activeSound)
                {
                    return true;
                }

                if (ma_sound_at_end(&activeSound->sound))
                {
                    ma_sound_uninit(&activeSound->sound);
                    return true;
                }

                return false;
            }),
        g_activeSounds.end());
}

static void clearActiveSoundsLocked()
{
    for (size_t i = 0; i < g_activeSounds.size(); ++i)
    {
        if (g_activeSounds[i])
        {
            ma_sound_uninit(&g_activeSounds[i]->sound);
        }
    }
    g_activeSounds.clear();
}

static void playManagedSound(
    const std::string &path,
    float volume,
    float pitch,
    bool is3D,
    float x,
    float y,
    float z)
{
    if (path.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(g_audioMutex);
    cleanupFinishedSoundsLocked();

    std::unique_ptr<ActiveSound> activeSound(new ActiveSound());
    std::memset(&activeSound->sound, 0, sizeof(activeSound->sound));

    ma_uint32 flags = 0;
    if (!is3D)
    {
        flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    }

    ma_result result = ma_sound_init_from_file(
        &g_audioEngine,
        path.c_str(),
        flags,
        NULL,
        NULL,
        &activeSound->sound);
    if (result != MA_SUCCESS)
    {
        std::printf("[AppleAudio] Failed to load SFX %s (error %d)\n", path.c_str(), result);
        return;
    }

    ma_sound_set_volume(&activeSound->sound, clampNonNegative(volume));
    ma_sound_set_pitch(&activeSound->sound, sanitizePitch(pitch));
    ma_sound_set_spatialization_enabled(&activeSound->sound, is3D ? MA_TRUE : MA_FALSE);

    if (is3D)
    {
        ma_sound_set_positioning(&activeSound->sound, ma_positioning_absolute);
        ma_sound_set_position(&activeSound->sound, x, y, z);
        ma_sound_set_attenuation_model(&activeSound->sound, ma_attenuation_model_inverse);
        ma_sound_set_rolloff(&activeSound->sound, 1.0f);
        ma_sound_set_min_distance(&activeSound->sound, 1.0f);
        ma_sound_set_max_distance(&activeSound->sound, 32.0f);
    }

    result = ma_sound_start(&activeSound->sound);
    if (result != MA_SUCCESS)
    {
        std::printf("[AppleAudio] Failed to start SFX %s (error %d)\n", path.c_str(), result);
        ma_sound_uninit(&activeSound->sound);
        return;
    }

    g_activeSounds.push_back(std::move(activeSound));
}

bool AppleAudio_Init()
{
    if (g_audioInitialized) return true;

    ma_engine_config config = ma_engine_config_init();
    config.channels = 2;
    config.sampleRate = 44100;
    config.listenerCount = 1;

    ma_result result = ma_engine_init(&config, &g_audioEngine);
    if (result != MA_SUCCESS)
    {
        printf("[AppleAudio] Failed to initialize audio engine: %d\n", result);
        return false;
    }

    g_audioInitialized = true;
    g_musicLoaded = false;
    buildSoundIndex();
    printf("[AppleAudio] Audio engine initialized successfully\n");
    return true;
}

void AppleAudio_Shutdown()
{
    if (!g_audioInitialized) return;

    if (g_musicLoaded)
    {
        ma_sound_uninit(&g_musicSound);
        g_musicLoaded = false;
    }

    {
        std::lock_guard<std::mutex> lock(g_audioMutex);
        clearActiveSoundsLocked();
    }

    ma_engine_uninit(&g_audioEngine);
    g_audioInitialized = false;
    printf("[AppleAudio] Audio engine shut down\n");
}

void AppleAudio_PlaySound(const char *soundPath, float x, float y, float z, float volume, float pitch, bool is3D)
{
    if (!g_audioInitialized || !soundPath || soundPath[0] == '\0') return;

    std::string path = buildSoundPath(soundPath);
    if (path.empty()) return; // Sound file not found, silent

    playManagedSound(path, volume, pitch, is3D, x, y, z);
}

void AppleAudio_PlayUISound(const char *soundName, float volume, float pitch)
{
    if (!g_audioInitialized || !soundName || soundName[0] == '\0') return;

    std::string path = buildUIPath(soundName);
    if (path.empty()) return;

    playManagedSound(path, volume, pitch, false, 0.0f, 0.0f, 0.0f);
}

// Forward declaration needed since StopMusic is called by StartMusic
void AppleAudio_StopMusic();

bool AppleAudio_StartMusic(const char *filePath)
{
    if (!g_audioInitialized || !filePath) return false;

    // Stop any currently playing music
    AppleAudio_StopMusic();

    std::string path = buildMusicPath(filePath);
    if (path.empty())
    {
        printf("[AppleAudio] Music file not found: %s (try converting .binka to .ogg)\n", filePath);
        return false;
    }

    ma_result result = ma_sound_init_from_file(&g_audioEngine, path.c_str(),
        MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION,
        NULL, NULL, &g_musicSound);
    if (result != MA_SUCCESS)
    {
        printf("[AppleAudio] Failed to load music: %s (error %d)\n", path.c_str(), result);
        return false;
    }

    ma_sound_set_volume(&g_musicSound, g_musicVolume);
    ma_sound_start(&g_musicSound);
    g_musicLoaded = true;
    printf("[AppleAudio] Playing music: %s\n", path.c_str());
    return true;
}

void AppleAudio_StopMusic()
{
    if (!g_audioInitialized || !g_musicLoaded) return;

    ma_sound_stop(&g_musicSound);
    ma_sound_uninit(&g_musicSound);
    g_musicLoaded = false;
}

bool AppleAudio_IsMusicPlaying()
{
    if (!g_audioInitialized || !g_musicLoaded) return false;
    return ma_sound_is_playing(&g_musicSound) != 0;
}

void AppleAudio_PauseMusic(bool pause)
{
    if (!g_audioInitialized || !g_musicLoaded) return;
    if (pause)
        ma_sound_stop(&g_musicSound);
    else
        ma_sound_start(&g_musicSound);
}

void AppleAudio_SetMusicVolume(float volume)
{
    g_musicVolume = volume;
    if (g_audioInitialized && g_musicLoaded)
    {
        ma_sound_set_volume(&g_musicSound, volume);
    }
}

void AppleAudio_SetEffectsVolume(float volume)
{
    g_effectsVolume = volume;
}

float AppleAudio_GetMusicVolume() { return g_musicVolume; }
float AppleAudio_GetEffectsVolume() { return g_effectsVolume; }

void AppleAudio_SetListenerPosition(float x, float y, float z, float frontX, float frontY, float frontZ)
{
    if (!g_audioInitialized) return;
    std::lock_guard<std::mutex> lock(g_audioMutex);
    cleanupFinishedSoundsLocked();
    ma_engine_listener_set_position(&g_audioEngine, 0, x, y, z);
    ma_engine_listener_set_direction(&g_audioEngine, 0, frontX, frontY, frontZ);
    ma_engine_listener_set_world_up(&g_audioEngine, 0, 0.0f, 1.0f, 0.0f);
}

bool AppleAudio_IsInitialized()
{
    return g_audioInitialized;
}

#endif // __APPLE__
