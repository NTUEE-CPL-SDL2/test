#pragma once

#include <SDL2/SDL_mixer.h>
#include <string>
#include <map>
#include <iostream>

class MusicManager {
private:
    Mix_Music* bgMusic;
    std::map<std::string, Mix_Chunk*> sfxMap;
    bool initialized;
    int musicVolume;
    int sfxVolume;
    Uint32 musicStartTime;
    bool paused;

public:
    MusicManager() 
        : bgMusic(nullptr), initialized(false), 
          musicVolume(MIX_MAX_VOLUME), sfxVolume(MIX_MAX_VOLUME),
          musicStartTime(0), paused(false) {
        init();
    }

    ~MusicManager() {
        cleanup();
    }

    bool init() {
        if (initialized) return true;

        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "[ERROR] SDL_mixer init failed: " << Mix_GetError() << std::endl;
            return false;
        }

        Mix_AllocateChannels(16);
        initialized = true;
        std::cout << "[OK] MusicManager initialized" << std::endl;
        return true;
    }

    bool loadMusic(const std::string& filepath) {
        if (bgMusic) {
            Mix_FreeMusic(bgMusic);
            bgMusic = nullptr;
        }

        bgMusic = Mix_LoadMUS(filepath.c_str());
        if (!bgMusic) {
            std::cerr << "[ERROR] Failed to load music: " << filepath << std::endl;
            std::cerr << "        SDL_mixer: " << Mix_GetError() << std::endl;
            return false;
        }

        std::cout << "[OK] Music loaded: " << filepath << std::endl;
        return true;
    }

    void playMusic(int loops = -1) {
        if (!bgMusic) {
            std::cerr << "[ERROR] No music loaded" << std::endl;
            return;
        }

        if (Mix_PlayMusic(bgMusic, loops) == -1) {
            std::cerr << "[ERROR] Music play failed: " << Mix_GetError() << std::endl;
            return;
        }

        musicStartTime = SDL_GetTicks();
        paused = false;
        std::cout << "[INFO] Music started" << std::endl;
    }

    void pauseMusic() {
        if (Mix_PlayingMusic() && !paused) {
            Mix_PauseMusic();
            paused = true;
        }
    }

    void resumeMusic() {
        if (paused) {
            Mix_ResumeMusic();
            paused = false;
        }
    }

    void stopMusic() {
        Mix_HaltMusic();
        paused = false;
    }

    void setMusicVolume(int volume) {
        musicVolume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : volume);
        Mix_VolumeMusic(musicVolume);
    }

    Uint32 getMusicTime() const {
        if (!Mix_PlayingMusic() || paused) return 0;
        return SDL_GetTicks() - musicStartTime;
    }

    bool loadSoundEffect(const std::string& name, const std::string& filepath) {
        Mix_Chunk* sound = Mix_LoadWAV(filepath.c_str());
        if (!sound) {
            std::cerr << "[ERROR] Failed to load SFX: " << filepath << std::endl;
            return false;
        }

        auto it = sfxMap.find(name);
        if (it != sfxMap.end()) {
            Mix_FreeChunk(it->second);
        }

        sfxMap[name] = sound;
        std::cout << "[OK] SFX loaded: " << name << std::endl;
        return true;
    }

    void playSoundEffect(const std::string& name, int loops = 0) {
        auto it = sfxMap.find(name);
        if (it == sfxMap.end()) {
            std::cerr << "[ERROR] SFX not found: " << name << std::endl;
            return;
        }

        Mix_PlayChannel(-1, it->second, loops);
    }

    void setSFXVolume(int volume) {
        sfxVolume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : volume);
        for (auto& pair : sfxMap) {
            Mix_VolumeChunk(pair.second, sfxVolume);
        }
    }

    bool isMusicPlaying() const {
        return Mix_PlayingMusic() && !paused;
    }

    void fadeInMusic(int ms, int loops = -1) {
        if (!bgMusic) return;
        Mix_FadeInMusic(bgMusic, loops, ms);
        musicStartTime = SDL_GetTicks();
        paused = false;
    }

    void fadeOutMusic(int ms) {
        Mix_FadeOutMusic(ms);
    }

    void cleanup() {
        if (bgMusic) {
            Mix_FreeMusic(bgMusic);
            bgMusic = nullptr;
        }

        for (auto& pair : sfxMap) {
            Mix_FreeChunk(pair.second);
        }
        sfxMap.clear();

        if (initialized) {
            Mix_CloseAudio();
            initialized = false;
        }
    }
};