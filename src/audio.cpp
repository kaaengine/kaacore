#include <string>

#include "SDL_mixer.h"

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/input.h"
#include "kaacore/log.h"

#include "kaacore/audio.h"

namespace kaacore {

const uint16_t default_mixing_channels_count = 32;
ResourcesRegistry<std::string, SoundData> _sound_registry;
ResourcesRegistry<std::string, MusicData> _music_registry;

void
initialize_audio_resources()
{
    _sound_registry.initialze();
    _music_registry.initialze();
}

void
uninitialize_audio_resources()
{
    _sound_registry.uninitialze();
    _music_registry.uninitialze();
}

SoundData::SoundData(const std::string& path) : path(path)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

SoundData::~SoundData()
{
    _sound_registry.unregister_resource(this->path);
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<SoundData>
SoundData::load(const std::string& path)
{
    std::shared_ptr<SoundData> sound_data;
    if ((sound_data = _sound_registry.get_resource(path))) {
        return sound_data;
    }

    sound_data = std::shared_ptr<SoundData>(new SoundData(path));
    _sound_registry.register_resource(path, sound_data);
    return sound_data;
}

void
SoundData::_initialize()
{
    this->_raw_sound =
        get_engine()->audio_manager->load_raw_sound(this->path.c_str());
    this->is_initialized = true;
}

void
SoundData::_uninitialize()
{
    if (this->_raw_sound) {
        Mix_FreeChunk(this->_raw_sound);
        this->_raw_sound = nullptr;
    }
    this->is_initialized = false;
}

Sound::Sound() : _volume(1.) {}

Sound::Sound(ResourceReference<SoundData> sound_data, double volume)
    : _sound_data(sound_data), _volume(volume)
{}

Sound
Sound::load(const char* path, double volume)
{
    return Sound(SoundData::load(path), volume);
}

double
Sound::volume() const
{
    return this->_volume;
}

Sound::operator bool() const
{
    return bool(this->_sound_data);
}

bool
Sound::operator==(const Sound& other) const
{
    return this->_sound_data == other._sound_data;
}

void
Sound::play(double volume_factor)
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    get_engine()->audio_manager->play_sound(
        *this, this->_volume * volume_factor);
}

SoundPlayback::SoundPlayback(const Sound& sound, const double volume)
    : _sound(sound), _volume(volume), _playback_uid(0)
{}

Sound
SoundPlayback::sound() const
{
    return this->_sound;
}

double
SoundPlayback::volume() const
{
    return this->_volume;
}

void
SoundPlayback::volume(const double vol)
{
    this->_volume = vol;
    if (this->status() != AudioStatus::stopped) {
        get_engine()->audio_manager->_update_channel_volume(
            this->_channel_id, this->_volume * this->_sound.volume());
    }
}

AudioStatus
SoundPlayback::status() const
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    if (this->_playback_uid > 0) {
        return get_engine()->audio_manager->_check_playback(
            this->_channel_id, this->_playback_uid);
    }
    return AudioStatus::stopped;
}

bool
SoundPlayback::is_playing() const
{
    return this->status() == AudioStatus::playing;
}

void
SoundPlayback::play(const int loops)
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    if (this->status() != AudioStatus::stopped) {
        this->stop();
    }
    auto [channel_id, playback_uid] = get_engine()->audio_manager->play_sound(
        this->_sound, this->_volume * this->_sound.volume(), loops);
    this->_channel_id = channel_id;
    this->_playback_uid = playback_uid;
}

bool
SoundPlayback::is_paused() const
{
    return this->status() == AudioStatus::paused;
}

bool
SoundPlayback::pause()
{
    if (this->status() == AudioStatus::playing) {
        get_engine()->audio_manager->_pause_channel(this->_channel_id);
        return true;
    }
    return false;
}

bool
SoundPlayback::resume()
{
    if (this->status() == AudioStatus::paused) {
        get_engine()->audio_manager->_resume_channel(this->_channel_id);
        return true;
    }
    return false;
}

bool
SoundPlayback::stop()
{
    if (this->status() != AudioStatus::stopped) {
        get_engine()->audio_manager->_stop_channel(this->_channel_id);
        return true;
    }
    return false;
}

MusicData::MusicData(const std::string& path) : path(path)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

MusicData::~MusicData()
{
    _music_registry.unregister_resource(this->path);
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<MusicData>
MusicData::load(const std::string& path)
{
    std::shared_ptr<MusicData> music_data;
    if ((music_data = _music_registry.get_resource(path))) {
        return music_data;
    }

    music_data = std::shared_ptr<MusicData>(new MusicData(path));
    _music_registry.register_resource(path, music_data);
    return music_data;
}

void
MusicData::_initialize()
{
    this->_raw_music =
        get_engine()->audio_manager->load_raw_music(this->path.c_str());
    this->is_initialized = true;
}

void
MusicData::_uninitialize()
{
    if (this->_raw_music) {
        Mix_FreeMusic(this->_raw_music);
        this->_raw_music = nullptr;
    }
    this->is_initialized = false;
}

Music::Music() : _volume(1.) {}

Music::Music(ResourceReference<MusicData> music_data, double volume)
    : _music_data(music_data), _volume(volume)
{}

Music
Music::load(const char* path, double volume)
{
    return Music(MusicData::load(path), volume);
}

Music
Music::get_current()
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    return get_engine()->audio_manager->_music_state.current_music;
}

double
Music::volume() const
{
    return this->_volume;
}

Music::operator bool() const
{
    return bool(this->_music_data);
}

bool
Music::operator==(const Music& other) const
{
    return this->_music_data == other._music_data;
}

AudioStatus
Music::status() const
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    return get_engine()->audio_manager->music_state();
}

bool
Music::is_playing() const
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    return *this == this->get_current() and
           this->get_current().status() == AudioStatus::playing;
}

void
Music::play(double volume_factor)
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    get_engine()->audio_manager->play_music(
        *this, this->_volume * volume_factor);
}

bool
Music::is_paused() const
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    if (this->get_current() == *this and
        this->get_current().status() == AudioStatus::paused) {
        return true;
    }
    return false;
}

bool
Music::pause()
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    if (this->get_current() == *this and
        this->get_current().status() == AudioStatus::playing) {
        get_engine()->audio_manager->_pause_music();
        return true;
    }
    return false;
}

bool
Music::resume()
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    if (this->get_current() == *this and
        this->get_current().status() == AudioStatus::paused) {
        get_engine()->audio_manager->_resume_music();
        return true;
    }
    return false;
}

bool
Music::stop()
{
    KAACORE_ASSERT(get_engine()->audio_manager);
    auto status = this->get_current().status();
    if (this->get_current() == *this and
        (status == AudioStatus::paused or status == AudioStatus::playing)) {
        get_engine()->audio_manager->_stop_music();
        return true;
    }
    return false;
}

void
_ChannelState::reset()
{
    this->current_sound = Sound();
    this->playback_uid = 0;
    this->paused = false;
}

void
_music_finished_hook()
{
    SDL_Event event;
    event.type = static_cast<uint32_t>(EventType::music_finished);
    SDL_PushEvent(&event);
}

void
_channel_finished_hook(int channel)
{
    SDL_Event event;
    event.type = static_cast<uint32_t>(EventType::channel_finished);
    event.user.code = channel;
    SDL_PushEvent(&event);
}

AudioManager::AudioManager()
    : _master_volume(1.), _master_sound_volume(1.), _master_music_volume(1.)
{
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    Mix_Init(MIX_INIT_OGG);
    auto err_code =
        Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048);
    if (err_code == -1) {
        log<LogLevel::error>("Failed to open audio (%s)", Mix_GetError());
        return;
    }
    Mix_HookMusicFinished(_music_finished_hook);
    Mix_ChannelFinished(_channel_finished_hook);
    this->_channels_state.resize(MIX_CHANNELS);
    this->mixing_channels(default_mixing_channels_count);
}

AudioManager::~AudioManager()
{
    Mix_CloseAudio();
    Mix_Quit();
    Mix_HookMusicFinished(nullptr);
    Mix_ChannelFinished(nullptr);
}

Mix_Chunk*
AudioManager::load_raw_sound(const char* path)
{
    auto raw_sound = Mix_LoadWAV(path);
    if (not raw_sound) {
        log<LogLevel::error>(
            "Failed to load sound from path %s (%s)", path, Mix_GetError());
    }
    return raw_sound;
}

Mix_Music*
AudioManager::load_raw_music(const char* path)
{
    auto raw_music = Mix_LoadMUS(path);
    if (not raw_music) {
        log<LogLevel::error>(
            "Failed to load music from path %s (%s)", path, Mix_GetError());
    }
    return raw_music;
}

std::pair<ChannelId, PlaybackUid>
AudioManager::play_sound(
    const Sound& sound, const double volume_factor, const int loops)
{
    KAACORE_ASSERT(bool(sound));
    if (sound._sound_data->_raw_sound) {
        // SDL_mixer loops meaning are different, -1 is infinite, 0 is once, 1
        // is twice, ...
        auto mixer_loops = loops - 1;
        auto channel =
            Mix_PlayChannel(-1, sound._sound_data->_raw_sound, mixer_loops);
        if (channel >= 0) {
            KAACORE_ASSERT(channel < this->_channels_state.size());
            this->_channels_state[channel].current_sound = sound;
            this->_channels_state[channel].requested_volume = volume_factor;
            auto playback_uid = random_uid<PlaybackUid>();
            this->_channels_state[channel].playback_uid = playback_uid;
            this->_recalc_channel_volume(channel);
            log<LogLevel::debug, LogCategory::audio>(
                "Playing sound at channel %u, uid: %llx", channel,
                playback_uid);
            return {channel, playback_uid};
        } else {
            log<LogLevel::error>("Failed to play sound (%s)", Mix_GetError());
        }
    } else {
        log<LogLevel::error>("Failed to played incorrectly loaded sound");
    }
    return {-1, 0};
}

void
AudioManager::play_music(const Music& music, const double volume_factor)
{
    KAACORE_ASSERT(bool(music));
    if (music._music_data->_raw_music) {
        auto err_code = Mix_PlayMusic(music._music_data->_raw_music, 1);
        if (err_code == -1) {
            log<LogLevel::error>("Failed to play music (%s)", Mix_GetError());
            return;
        }
        this->_music_state.current_music = music;
        this->_music_state.requested_volume = volume_factor;
        this->_recalc_music_volume();
    } else {
        log<LogLevel::error>("Failed to played incorrectly loaded music");
    }
}

AudioStatus
AudioManager::music_state()
{
    if (Mix_PlayingMusic()) {
        if (Mix_PausedMusic()) {
            return AudioStatus::paused;
        } else {
            return AudioStatus::playing;
        }
    } else {
        return AudioStatus::stopped;
    }
}

uint16_t
AudioManager::mixing_channels() const
{
    return Mix_AllocateChannels(-1);
}

void
AudioManager::mixing_channels(const uint16_t channels)
{
    Mix_AllocateChannels(channels);
    this->_channels_state.resize(channels);
}

double
AudioManager::master_volume() const
{
    return this->_master_volume;
}

void
AudioManager::master_volume(const double vol)
{
    this->_master_volume = vol;
    this->_recalc_music_volume();
    this->_recalc_channels_volume();
}

double
AudioManager::master_sound_volume() const
{
    return this->_master_sound_volume;
}

void
AudioManager::master_sound_volume(const double vol)
{
    this->_master_sound_volume = vol;
    this->_recalc_channels_volume();
}

double
AudioManager::master_music_volume() const
{
    return this->_master_music_volume;
}

void
AudioManager::master_music_volume(const double vol)
{
    this->_master_music_volume = vol;
    this->_recalc_music_volume();
}

AudioStatus
AudioManager::_check_playback(
    const ChannelId& channel_id, const PlaybackUid& playback_uid)
{
    if (channel_id < this->_channels_state.size()) {
        const auto& channel_state = this->_channels_state[channel_id];
        if (channel_state.playback_uid == playback_uid) {
            if (channel_state.paused) {
                return AudioStatus::paused;
            }
            return AudioStatus::playing;
        }
    }
    return AudioStatus::stopped;
}

void
AudioManager::_pause_channel(const ChannelId& channel_id)
{
    KAACORE_ASSERT(this->_channels_state.size() > channel_id);
    auto& channel_state = this->_channels_state[channel_id];
    if (channel_state.current_sound) {
        channel_state.paused = true;
        Mix_Pause(channel_id);
    }
}

void
AudioManager::_resume_channel(const ChannelId& channel_id)
{
    KAACORE_ASSERT(this->_channels_state.size() > channel_id);
    auto& channel_state = this->_channels_state[channel_id];
    if (channel_state.current_sound) {
        channel_state.paused = false;
        Mix_Resume(channel_id);
    }
}

void
AudioManager::_stop_channel(const ChannelId& channel_id)
{
    KAACORE_ASSERT(this->_channels_state.size() > channel_id);
    auto& channel_state = this->_channels_state[channel_id];
    if (channel_state.current_sound) {
        Mix_HaltChannel(channel_id);
        channel_state.reset();
        channel_state._manually_stopped = true;
    }
}

void
AudioManager::_update_channel_volume(
    const ChannelId& channel_id, const double volume)
{
    KAACORE_ASSERT(this->_channels_state.size() > channel_id);
    auto& channel_state = this->_channels_state[channel_id];
    if (channel_state.current_sound) {
        channel_state.requested_volume = volume;
        this->_recalc_channel_volume(channel_id);
    }
}

void
AudioManager::_pause_music()
{
    Mix_PauseMusic();
}

void
AudioManager::_resume_music()
{
    Mix_ResumeMusic();
}

void
AudioManager::_stop_music()
{
    Mix_HaltMusic();
}

void
AudioManager::_recalc_music_volume()
{
    Mix_VolumeMusic(
        this->_master_volume * this->_master_music_volume *
        this->_music_state.requested_volume * MIX_MAX_VOLUME);
}

void
AudioManager::_recalc_channels_volume()
{
    size_t channels_count = this->_channels_state.size();
    for (size_t i = 0; i < channels_count; i++) {
        if (this->_channels_state[i].current_sound) {
            this->_recalc_channel_volume(i);
        }
    }
}

void
AudioManager::_recalc_channel_volume(ChannelId channel_id)
{
    KAACORE_ASSERT(channel_id < this->_channels_state.size());
    KAACORE_ASSERT(this->_channels_state[channel_id].current_sound);
    Mix_Volume(
        channel_id, this->_master_volume * this->_master_sound_volume *
                        this->_channels_state[channel_id].requested_volume *
                        MIX_MAX_VOLUME);
}

void
AudioManager::_handle_music_finished()
{
    log<LogLevel::debug>("Music channel finished playback");
    this->_music_state.current_music = Music(); // empty Music
}

void
AudioManager::_handle_channel_finished(ChannelId channel_id)
{
    log<LogLevel::debug>("Sound channel #%u finished playback", channel_id);
    if (channel_id < this->_channels_state.size()) {
        auto& channel_state = this->_channels_state[channel_id];
        if (not channel_state._manually_stopped) {
            channel_state.reset();
        } else {
            channel_state._manually_stopped = false;
        }
    }
}

} // namespace kaacore
