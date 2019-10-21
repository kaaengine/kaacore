#pragma once

#include <SDL.h>
#include <SDL_mixer.h>

#include "kaacore/resources.h"

namespace kaacore {

extern const uint32_t event_music_finished;

struct SoundData {
    Mix_Chunk* _raw_sound;

    SoundData(Mix_Chunk* raw_sound);
    ~SoundData();

    static Resource<SoundData> load(const char* path);
};

struct Sound {
    Resource<SoundData> _sound_data;
    double volume;

    Sound();
    Sound(Resource<SoundData> sound_data, double volume = 1.);
    static Sound load(const char* path, double volume = 1.);

    operator bool() const;
    void play(double volume_factor = 1.);
};

enum struct MusicState {
    stopped = 1,
    paused = 2,
    playing = 3,
};

struct MusicData {
    Mix_Music* _raw_music;

    MusicData(Mix_Music* raw_music);
    ~MusicData();

    static Resource<MusicData> load(const char* path);
};

struct Music {
    Resource<MusicData> _music_data;
    double volume;

    Music();
    Music(Resource<MusicData> effect_data, double volume = 1.);
    static Music load(const char* path, double volume = 1.);
    static Music get_current();

    operator bool() const;
    bool is_playing() const;
    void play(double volume_factor = 1.);
};

struct AudioManager {
    double master_sound_volume;
    double master_music_volume;

    Music current_music;

    AudioManager();
    ~AudioManager();

    Mix_Chunk* load_raw_sound(const char* path);
    Mix_Music* load_raw_music(const char* path);

    void play_sound(const Sound& sound, const double volume_factor = 1.);
    void play_music(const Music& music, const double volume_factor = 1.);
    MusicState music_state();

    uint16_t mixing_channels() const;
    void mixing_channels(const uint16_t channels);
};

} // namespace kaacore
