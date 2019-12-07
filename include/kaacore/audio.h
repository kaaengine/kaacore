#pragma once

#include <vector>

#include <SDL.h>
#include <SDL_mixer.h>

#include "kaacore/resources.h"

namespace kaacore {

struct SoundData {
    Mix_Chunk* _raw_sound;

    SoundData(Mix_Chunk* raw_sound);
    ~SoundData();

    static Resource<SoundData> load(const char* path);
};

class Sound {
    friend class AudioManager;

    Resource<SoundData> _sound_data;
    double _volume;

    Sound(Resource<SoundData> sound_data, double volume = 1.);

  public:
    Sound();
    static Sound load(const char* path, double volume = 1.);

    operator bool() const;
    bool operator==(const Sound& other) const;

    double volume() const;
    void volume(const double vol);

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

class Music {
    friend class AudioManager;

    Resource<MusicData> _music_data;
    double _volume;

    Music(Resource<MusicData> effect_data, double volume = 1.);

  public:
    Music();
    static Music load(const char* path, double volume = 1.);
    static Music get_current();
    static MusicState get_state();

    operator bool() const;
    bool operator==(const Music& other) const;

    double volume() const;
    void volume(const double vol);

    bool is_playing() const;
    void play(double volume_factor = 1.);

    bool is_paused() const;
    bool pause();
    bool resume();
    bool stop();
};

struct _MusicState {
    double requested_volume;
    Music current_music;
};

struct _ChannelState {
    double requested_volume;
    Sound current_sound;
};

class AudioManager {
    friend class Engine;
    friend class Sound;
    friend struct SoundData;
    friend class Music;
    friend struct MusicData;

    double _master_volume;
    double _master_sound_volume;
    double _master_music_volume;

    _MusicState _music_state;
    std::vector<_ChannelState> _channels_state;

    Mix_Chunk* load_raw_sound(const char* path);
    Mix_Music* load_raw_music(const char* path);

    void play_sound(const Sound& sound, const double volume_factor = 1.);
    void play_music(const Music& music, const double volume_factor = 1.);
    MusicState music_state();

    void _pause_music();
    void _resume_music();
    void _stop_music();

    void _recalc_music_volume();
    void _recalc_channels_volume();
    void _recalc_channel_volume(uint16_t channel_id);

    void _handle_music_finished();
    void _handle_channel_finished(uint16_t channel_id);

  public:
    AudioManager();
    ~AudioManager();

    double master_volume() const;
    void master_volume(const double vol);

    double master_sound_volume() const;
    void master_sound_volume(const double vol);

    double master_music_volume() const;
    void master_music_volume(const double vol);

    uint16_t mixing_channels() const;
    void mixing_channels(const uint16_t channels);
};

} // namespace kaacore
