#pragma once

#include <string>

namespace Audio {
    // Initialize the audio system (optional).
    bool Init();

    // Preload a sound file for later playback. Returns true if successful.
    // The sound is stored by name and can be played via PlayCached().
    bool Preload(const std::string& name, const std::string& path, bool loop = false);

    // Play a preloaded sound by name. If it's not preloaded, nothing happens.
    void PlayCached(const std::string& name);

    // Stop a preloaded sound by name.
    void StopCached(const std::string& name);

    // Play a sound file. Path is relative to executable or absolute.
    // If loop is true, attempt to loop playback (best-effort depending on platform/player).
    void Play(const std::string& path, bool loop = false);

    // Stop all playback started via this helper.
    void StopAll();

    // Set global volume (0.0 - 1.0). May be a no-op depending on platform.
    void SetVolume(float v);
}
