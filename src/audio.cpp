#include "audio.hpp"

#include "logging.hpp"
#include "res.hpp"
#include "state.hpp"
#include "wav.hpp"

#include <AL/al.h>
#include <AL/alc.h>

struct {
    ALCdevice * device;
    ALCcontext * context;

    int click;
    int hurt;

} intern;

int source_from_wav( res_t wav )
{
    unsigned int source;

    alGenSources( 1, &source );
    alSourcef( source, AL_PITCH, 1 );
    alSourcef( source, AL_GAIN, 1 );
    alSource3f( source, AL_POSITION, 0, 0, 0 );
    alSource3f( source, AL_VELOCITY, 0, 0, 0 );
    alSourcei( source, AL_LOOPING, AL_FALSE );

    alSourcei( source, AL_BUFFER, load_wav( wav.data, wav.size ) );

    return (int) source;
}

int audio_init()
{
    intern.device = alcOpenDevice( nullptr );
    if ( !intern.device ) {
        ERROR_LOG( "failed to open audio device" );
        return 1;
    }

    intern.context = alcCreateContext( intern.device, nullptr );
    if ( !intern.context ) {
        ERROR_LOG( "failed to create audio context" );
        return 1;
    }

    ALCboolean is_current = alcMakeContextCurrent( intern.context );
    if ( is_current != ALC_TRUE ) {
        ERROR_LOG( "failed to make audio context current" );
        return 1;
    }

    INFO_LOG(
        "openal device: %s",
        alcGetString( intern.device, ALC_ALL_DEVICES_SPECIFIER )
    );
    INFO_LOG(
        "openal version: %s (%s) (%s)",
        alGetString( AL_VERSION ),
        alGetString( AL_VENDOR ),
        alGetString( AL_RENDERER )
    );

    float orientation[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };

    alListener3f( AL_POSITION, 0, 0, 1.0f );
    alListener3f( AL_VELOCITY, 0, 0, 0 );
    alListenerfv( AL_ORIENTATION, orientation );

    intern.click = source_from_wav( find_res( "click.wav" ) );
    intern.hurt = source_from_wav( find_res( "hurt.wav" ) );

    return 0;
}

void audio_tick()
{
}

void audio_destroy()
{
    alcMakeContextCurrent( nullptr );
    alcDestroyContext( intern.context );
    alcCloseDevice( intern.device );
}

void audio_play_jump()
{
    alSourcePlay( intern.click );
}

void audio_play_damage()
{
    alSourcePlay( intern.hurt );
}
