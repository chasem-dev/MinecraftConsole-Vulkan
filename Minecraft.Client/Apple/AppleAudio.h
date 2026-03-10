#pragma once
#ifdef __APPLE__

// Initialize/shutdown miniaudio engine
bool AppleAudio_Init();
void AppleAudio_Shutdown();

// Play a sound effect by name (e.g. "mob/cow", "random/click")
// soundPath: converted from wchSoundNames (dots->slashes already done by ConvertSoundPathToName)
// x,y,z: world position
// volume: 0-1, pitch: playback rate factor
// is3D: whether to apply distance attenuation
void AppleAudio_PlaySound(const char *soundPath, float x, float y, float z, float volume, float pitch, bool is3D);

// Play a UI sound effect by name (e.g. "back", "press", "craft")
void AppleAudio_PlayUISound(const char *soundName, float volume, float pitch);

// Music streaming
bool AppleAudio_StartMusic(const char *filePath);
void AppleAudio_StopMusic();
bool AppleAudio_IsMusicPlaying();
void AppleAudio_PauseMusic(bool pause);

// Volume control (0.0 - 1.0)
void AppleAudio_SetMusicVolume(float volume);
void AppleAudio_SetEffectsVolume(float volume);
float AppleAudio_GetMusicVolume();
float AppleAudio_GetEffectsVolume();

// Listener position for 3D audio
void AppleAudio_SetListenerPosition(float x, float y, float z, float frontX, float frontY, float frontZ);

// Check if audio system is initialized
bool AppleAudio_IsInitialized();

#endif // __APPLE__
