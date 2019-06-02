#include <memory>

#include "SDL_mixer.h"

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/audio.h"


namespace kaacore {

SoundData::SoundData(Mix_Chunk* raw_sound)
    : _raw_sound(raw_sound)
{}

SoundData::~SoundData()
{
    Mix_FreeChunk(this->_raw_sound);
}

Resource<SoundData> SoundData::load(const char* path)
{
    auto raw_sound = get_engine()->audio_manager->load_raw_sound(path);
    return std::make_shared<SoundData>(raw_sound);
}

Sound::Sound()
    : volume(1.)
{
}

Sound::Sound(Resource<SoundData> sound_data, double volume)
    : _sound_data(sound_data), volume(volume)
{
}

Sound Sound::load(const char* path, double volume)
{
    return Sound(SoundData::load(path), volume);
}

Sound::operator bool() const
{
    return bool(this->_sound_data);
}

void Sound::play(double volume_factor)
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    get_engine()->audio_manager->play_sound(
        *this, this->volume * volume_factor
    );
}


MusicData::MusicData(Mix_Music* raw_music)
    : _raw_music(raw_music)
{}

MusicData::~MusicData()
{
    Mix_FreeMusic(this->_raw_music);
}

Resource<MusicData> MusicData::load(const char* path)
{
    auto raw_music = get_engine()->audio_manager->load_raw_music(path);
    return std::make_shared<MusicData>(raw_music);
}

Music::Music()
    : volume(1.)
{
}

Music::Music(Resource<MusicData> music_data, double volume)
    : _music_data(music_data), volume(volume)
{
}

Music Music::load(const char* path, double volume)
{
    return Music(MusicData::load(path), volume);
}

Music Music::get_current()
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    return get_engine()->audio_manager->current_music;
}

Music::operator bool() const
{
    return bool(this->_music_data);
}

void Music::play()
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    get_engine()->audio_manager->play_music(
        *this, this->volume
    );
}


AudioManager::AudioManager()
    : master_sound_volume(1.), master_music_volume(1.)
{
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    Mix_Init(0); // no libraries, just WAV support
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048);
}

AudioManager::~AudioManager()
{
    Mix_CloseAudio();
    Mix_Quit();
}

Mix_Chunk* AudioManager::load_raw_sound(const char* path)
{
    auto raw_sound = Mix_LoadWAV(path);
    if (not raw_sound) {
        log<LogLevel::error>("Failed to load sound from path %s (%s)",
                             path, Mix_GetError());
    }
    KAACORE_CHECK(raw_sound != nullptr);
    return raw_sound;
}

Mix_Music* AudioManager::load_raw_music(const char* path)
{
    auto raw_music = Mix_LoadMUS(path);
    if (not raw_music) {
        log<LogLevel::error>("Failed to load music from path %s (%s)",
                             path, Mix_GetError());
    }
    KAACORE_CHECK(raw_music != nullptr);
    return raw_music;
}

void AudioManager::play_sound(const Sound& sound, const double volume_factor)
{
    KAACORE_ASSERT(bool(sound));
    Mix_VolumeChunk(sound._sound_data->_raw_sound,
                    this->master_sound_volume * volume_factor * MIX_MAX_VOLUME);
    Mix_PlayChannel(-1, sound._sound_data->_raw_sound, 0);
}

void AudioManager::play_music(const Music& music, const double volume_factor)
{
    KAACORE_ASSERT(bool(music));
    Mix_VolumeMusic(this->master_music_volume * volume_factor * MIX_MAX_VOLUME);
    Mix_PlayMusic(music._music_data->_raw_music, 1);
    this->current_music = music;
}

MusicState AudioManager::music_state()
{
    if (Mix_PlayingMusic()) {
        return MusicState::playing;
    } else if (Mix_PausedMusic()) {
        return MusicState::paused;
    } else {
        return MusicState::stopped;
    }
}

} // namespace kaacore
