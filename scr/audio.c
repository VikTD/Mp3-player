#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include "playlist.h"
#include <stdio.h>
#include <windows.h> // Sleep

static ma_engine engine;
static ma_sound sound;
static int sound_playing = 0;
static int sound_initialized = 0;
static int sound_paused = 0;
static ma_uint64 paused_cursor = 0;
static char current_file[MAX_PATH] = {0};

int audio_init(AudioEngine *audio) {
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        printf("Failed to initialize audio engine.\n");
        return -1;
    }
    return 0;
}

int audio_is_playing(AudioEngine *audio) {
    return sound_initialized && !sound_paused && ma_sound_is_playing(&sound);
}

int audio_is_paused(AudioEngine *audio) {
    return sound_initialized && sound_paused;
}

static void audio_reset_sound_state(void) {
    sound_playing = 0;
    sound_initialized = 0;
    sound_paused = 0;
    paused_cursor = 0;
    current_file[0] = '\0';
}

void audio_play(AudioEngine *audio, const char *filename) {
    if (sound_initialized) {
        ma_sound_stop(&sound);
        ma_sound_uninit(&sound);
        audio_reset_sound_state();
    }

    if (ma_sound_init_from_file(&engine, filename, MA_SOUND_FLAG_STREAM, NULL, NULL, &sound) != MA_SUCCESS) {
        printf("Failed to play %s\n", filename);
        return;
    }

    strncpy(current_file, filename, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = '\0';
    paused_cursor = 0;
    sound_paused = 0;
    ma_sound_set_volume(&sound, audio->volume / 100.0f);
    ma_sound_start(&sound);
    sound_initialized = 1;
    sound_playing = 1;
}

void audio_stop(AudioEngine *audio) {
    if (sound_initialized) {
        ma_sound_stop(&sound);
        ma_sound_uninit(&sound);
        audio_reset_sound_state();
    }
}

void audio_uninit(AudioEngine *audio) {
    audio_stop(audio);
    ma_engine_uninit(&engine);
}

void audio_set_volume(AudioEngine *audio, int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    audio->volume = volume;
    if (sound_initialized) {
        ma_sound_set_volume(&sound, volume / 100.0f);
    }
}

int audio_get_duration(AudioEngine *audio) {
    if (!sound_initialized) return 0;
    ma_uint64 length;
    ma_sound_get_length_in_pcm_frames(&sound, &length);
    ma_uint32 sampleRate = engine.sampleRate;
    return (int)(length / sampleRate);
}

int audio_get_position(AudioEngine *audio) {
    if (!sound_initialized) return 0;
    if (sound_paused) {
        ma_uint32 sampleRate = engine.sampleRate;
        return (int)(paused_cursor / sampleRate);
    }
    ma_uint64 cursor;
    ma_sound_get_cursor_in_pcm_frames(&sound, &cursor);
    ma_uint32 sampleRate = engine.sampleRate;
    return (int)(cursor / sampleRate);
}

int audio_poll(AudioEngine *audio) {
    (void)audio;
    if (!sound_initialized || sound_paused) return 0;

    ma_uint64 cursor = 0;
    ma_uint64 length = 0;
    ma_sound_get_cursor_in_pcm_frames(&sound, &cursor);
    ma_sound_get_length_in_pcm_frames(&sound, &length);

    /* Robust end detection:
       - Some backends keep ma_sound_is_playing() true at the end.
       - If we have a valid length and the cursor reached it, treat as ended. */
    if ((length > 0 && cursor >= length) || !ma_sound_is_playing(&sound)) {
        ma_sound_stop(&sound);
        ma_sound_uninit(&sound);
        audio_reset_sound_state();
        return 1;
    }
    return 0;
}

void audio_pause(AudioEngine *audio) {
    if (!sound_initialized || sound_paused) return;
    ma_sound_get_cursor_in_pcm_frames(&sound, &paused_cursor);
    ma_sound_stop(&sound);
    ma_sound_seek_to_pcm_frame(&sound, paused_cursor);
    sound_paused = 1;
    sound_playing = 0;
}

void audio_resume(AudioEngine *audio) {
    if (!sound_initialized || !sound_paused) return;
    ma_sound_seek_to_pcm_frame(&sound, paused_cursor);
    ma_sound_set_volume(&sound, audio->volume / 100.0f);
    ma_sound_start(&sound);
    sound_paused = 0;
    sound_playing = 1;
}

void audio_seek_relative(AudioEngine *audio, int delta_seconds) {
    if (!sound_initialized) return;
    ma_uint64 cursor_frames = 0;
    ma_uint64 length_frames = 0;
    ma_sound_get_length_in_pcm_frames(&sound, &length_frames);
    if (sound_paused) {
        cursor_frames = paused_cursor;
    } else {
        ma_sound_get_cursor_in_pcm_frames(&sound, &cursor_frames);
    }
    ma_uint32 sampleRate = engine.sampleRate;
    long long target = (long long)cursor_frames + (long long)delta_seconds * sampleRate;
    if (target < 0) target = 0;
    if (target > (long long)length_frames) target = (long long)length_frames;
    ma_sound_seek_to_pcm_frame(&sound, (ma_uint64)target);
    if (sound_paused) {
        paused_cursor = (ma_uint64)target;
    } else {
        sound_playing = 1;
        ma_sound_start(&sound);
    }
}
