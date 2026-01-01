#ifndef AUDIO_H
#define AUDIO_H

#include "miniaudio.h"
#include "playlist.h" // Include Playlist definition

typedef struct {
    ma_engine engine;
    int volume;
    int auto_next;
    int shuffle;
    int loop;
    Playlist *playlist; // Add reference to Playlist
} AudioEngine;

int audio_init(AudioEngine *audio);
void audio_play(AudioEngine *audio, const char *filename);
void audio_stop(AudioEngine *audio);
void audio_uninit(AudioEngine *audio);
void audio_set_volume(AudioEngine *audio, int volume); // Declare `audio_set_volume`
int audio_get_duration(AudioEngine *audio); // Declare this function
int audio_get_position(AudioEngine *audio); // Declare this function
int audio_is_playing(AudioEngine *audio); // Declare function to check if audio is playing
int audio_poll(AudioEngine *audio); // Polls playback; returns 1 if track ended
void audio_pause(AudioEngine *audio);
void audio_resume(AudioEngine *audio);
int audio_is_paused(AudioEngine *audio);
void audio_seek_relative(AudioEngine *audio, int delta_seconds);

#endif
