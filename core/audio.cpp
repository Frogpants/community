#include "audio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

namespace fs = std::filesystem;

std::mutex s_mutex;
ma_engine s_engine;
bool s_engineInitialized = false;
std::vector<std::unique_ptr<ma_sound>> s_loopingSounds;
std::unordered_map<std::string, std::unique_ptr<ma_sound>> s_cachedSounds;
float s_volume = 1.0f;

std::string GetExeDir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return ".";
    }

    std::string path(buffer, buffer + length);
    size_t pos = path.find_last_of("\\/");
    return (pos == std::string::npos) ? "." : path.substr(0, pos);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        std::string path(buffer.data());
        size_t pos = path.find_last_of('/');
        return (pos == std::string::npos) ? "." : path.substr(0, pos);
    }
    return ".";
#else
    char buffer[4096];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length <= 0) {
        return ".";
    }

    buffer[length] = '\0';
    std::string path(buffer);
    size_t pos = path.find_last_of('/');
    return (pos == std::string::npos) ? "." : path.substr(0, pos);
#endif
}

std::string ResolveSoundPath(const std::string& path) {
    fs::path input(path);
    std::vector<fs::path> candidates;

    candidates.push_back(input);
    candidates.push_back(fs::current_path() / input);

    const std::string exeDir = GetExeDir();
    if (!exeDir.empty()) {
        fs::path exePath(exeDir);
        candidates.push_back(exePath / input);
        candidates.push_back(exePath / ".." / input);
    }

    for (const fs::path& candidate : candidates) {
        if (!candidate.empty() && fs::exists(candidate) && fs::is_regular_file(candidate)) {
            return candidate.lexically_normal().string();
        }
    }

    return path;
}

bool EnsureEngineLocked() {
    if (s_engineInitialized) {
        return true;
    }

    ma_engine_config config = ma_engine_config_init();
    if (ma_engine_init(&config, &s_engine) != MA_SUCCESS) {
        return false;
    }

    s_engineInitialized = true;
    ma_engine_set_volume(&s_engine, s_volume);
    return true;
}

void ShutdownEngineLocked() {
    for (std::unique_ptr<ma_sound>& sound : s_loopingSounds) {
        if (sound) {
            ma_sound_uninit(sound.get());
        }
    }
    s_loopingSounds.clear();

    for (auto& pair : s_cachedSounds) {
        if (pair.second) {
            ma_sound_uninit(pair.second.get());
        }
    }
    s_cachedSounds.clear();

    if (s_engineInitialized) {
        ma_engine_uninit(&s_engine);
        s_engineInitialized = false;
    }
}

} // namespace

namespace Audio {

bool Init() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return EnsureEngineLocked();
}

void Play(const std::string& path, bool loop) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!EnsureEngineLocked()) {
        return;
    }

    const std::string resolvedPath = ResolveSoundPath(path);

    if (!loop) {
        ma_engine_play_sound(&s_engine, resolvedPath.c_str(), nullptr);
        return;
    }

    auto sound = std::make_unique<ma_sound>();
    if (ma_sound_init_from_file(&s_engine, resolvedPath.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound.get()) != MA_SUCCESS) {
        return;
    }

    ma_sound_set_looping(sound.get(), MA_TRUE);
    if (ma_sound_start(sound.get()) != MA_SUCCESS) {
        ma_sound_uninit(sound.get());
        return;
    }

    s_loopingSounds.push_back(std::move(sound));
}

void StopAll() {
    std::lock_guard<std::mutex> lock(s_mutex);
    ShutdownEngineLocked();
}

void SetVolume(float volume) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_volume = std::clamp(volume, 0.0f, 1.0f);

    if (!EnsureEngineLocked()) {
        return;
    }

    ma_engine_set_volume(&s_engine, s_volume);
}

bool Preload(const std::string& name, const std::string& path, bool loop) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!EnsureEngineLocked()) {
        return false;
    }

    const std::string resolvedPath = ResolveSoundPath(path);

    auto sound = std::make_unique<ma_sound>();
    if (ma_sound_init_from_file(&s_engine, resolvedPath.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound.get()) != MA_SUCCESS) {
        return false;
    }

    if (loop) {
        ma_sound_set_looping(sound.get(), MA_TRUE);
    }

    s_cachedSounds[name] = std::move(sound);
    return true;
}

void PlayCached(const std::string& name) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!EnsureEngineLocked()) {
        return;
    }

    auto it = s_cachedSounds.find(name);
    if (it == s_cachedSounds.end() || !it->second) {
        return;
    }

    ma_sound_start(it->second.get());
}

void StopCached(const std::string& name) {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_cachedSounds.find(name);
    if (it == s_cachedSounds.end() || !it->second) {
        return;
    }

    ma_sound_stop(it->second.get());
}

} // namespace Audio
