/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifdef MUSIC_GME

/* This file supports Game Music Emulators music streams */

#include "SDL_mixer_ext.h"

/* First parameter of most gme_ functions is a pointer to the Music_Emu */
typedef struct Music_Emu Music_Emu;

#include "music_gme.h"

#include <gme.h>
#include <stdio.h>

/* This file supports Game Music Emulator music streams */
typedef struct
{
    int play_count;
    Music_Emu* game_emu;
    int volume;
    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;

    char *mus_title;
    char *mus_artist;
    char *mus_album;
    char *mus_copyright;
} GME_Music;

static void GME_delete(void *context);

/* Set the volume for a MOD stream */
void GME_setvolume(void *music_p, int volume)
{
    GME_Music *music = (GME_Music*)music_p;
    if(music)
        music->volume = (int)round(128.0 * sqrt(((double)volume) * (1.0 / 128.0)));
}

GME_Music *GME_LoadSongRW(SDL_RWops *src, int trackNum)
{
    if(src != NULL)
    {
        void *bytes = 0;
        long spcsize, bytes_l;
        unsigned char byte[1];
        Music_Emu *game_emu;
        gme_info_t *musInfo;
        GME_Music *music;
        const char *err;

        Sint64 length = 0;

        music = (GME_Music *)SDL_calloc(1, sizeof(GME_Music));

        music->stream = SDL_NewAudioStream(AUDIO_S16, 2, music_spec.freq,
                                           music_spec.format, music_spec.channels, music_spec.freq);
        if (!music->stream) {
            GME_delete(music);
            return NULL;
        }

        music->buffer_size = music_spec.samples * sizeof(Sint16) * 2/*channels*/ * music_spec.channels;
        music->buffer = SDL_malloc(music->buffer_size);
        if (!music->buffer) {
            GME_delete(music);
            return NULL;
        }

        length = SDL_RWseek(src, 0, RW_SEEK_END);
        if(length < 0)
        {
            GME_delete(music);
            Mix_SetError("GAME-EMU: wrong file\n");
            return NULL;
        }

        SDL_RWseek(src, 0, RW_SEEK_SET);
        bytes = SDL_malloc((size_t)length);

        spcsize = 0;
        while((bytes_l = (long)SDL_RWread(src, &byte, sizeof(unsigned char), 1)) != 0)
        {
            ((unsigned char *)bytes)[spcsize] = byte[0];
            spcsize++;
        }

        if(spcsize == 0)
        {
            GME_delete(music);
            Mix_SetError("GAME-EMU: wrong file\n");
            return NULL;
        }

        err = gme_open_data(bytes, spcsize, &game_emu, music_spec.freq);
        /* spc_load_spc( snes_spc, bytes, spcsize ); */
        SDL_free(bytes);
        if(err != 0)
        {
            GME_delete(music);
            Mix_SetError("GAME-EMU: %s", err);
            return NULL;
        }

        if((trackNum < 0) || (trackNum >= gme_track_count(game_emu)))
            trackNum = gme_track_count(game_emu) - 1;

        err = gme_start_track(game_emu, trackNum);
        if(err != 0)
        {
            GME_delete(music);
            Mix_SetError("GAME-EMU: %s", err);
            return NULL;
        }

        music->game_emu = game_emu;
        music->volume = MIX_MAX_VOLUME;
        music->mus_title = NULL;
        music->mus_artist = NULL;
        music->mus_album = NULL;
        music->mus_copyright = NULL;

        err = gme_track_info(music->game_emu, &musInfo, trackNum);
        if(err != 0)
        {
            GME_delete(music);
            Mix_SetError("GAME-EMU: %s", err);
            return NULL;
        }

        /*
         * TODO:
         * Implement loop and length management to catch loop times and be able
         * to limit song to play specified loops count
         */
        music->mus_title = (char *)SDL_malloc(sizeof(char) * strlen(musInfo->song) + 1);
        strcpy(music->mus_title, musInfo->song);
        music->mus_artist = (char *)SDL_malloc(sizeof(char) * strlen(musInfo->author) + 1);
        strcpy(music->mus_artist, musInfo->author);
        music->mus_album = (char *)SDL_malloc(sizeof(char) * strlen(musInfo->game) + 1);
        strcpy(music->mus_album, musInfo->game);
        music->mus_copyright = (char *)SDL_malloc(sizeof(char) * strlen(musInfo->copyright) + 1);
        strcpy(music->mus_copyright, musInfo->copyright);
        gme_free_info(musInfo);

        return music;
    }
    return NULL;
}

/* Load a Game Music Emulators stream from an SDL_RWops object */
static void *GME_new_RWEx(struct SDL_RWops *src, int freesrc, const char *extraSettings)
{
    GME_Music *gmeMusic;
    int trackNumber = extraSettings ? atoi(extraSettings) : 0;
    gmeMusic = GME_LoadSongRW(src, trackNumber);
    if(!gmeMusic)
    {
        Mix_SetError("GAME-EMU: Can't load file");
        return NULL;
    }
    if(freesrc)
        SDL_RWclose(src);
    return gmeMusic;
}

static void *GME_new_RW(struct SDL_RWops *src, int freesrc)
{
    return GME_new_RWEx(src, freesrc, "0");
}

/* Start playback of a given Game Music Emulators stream */
static int GME_play(void *music_p, int play_count)
{
    GME_Music *music = (GME_Music*)music_p;
    if(music)
    {
        music->play_count = play_count;
        gme_seek(music->game_emu, 0);
    }
    return 0;
}

static int GME_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    GME_Music *music = (GME_Music*)context;
    int filled;
    const char *err = NULL;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    /* Align bytes length to correctly capture a stereo input */
    if ((bytes % 4) != 0) {
        bytes += (4 - (bytes % 4));
    }

    err = gme_play(music->game_emu, (bytes / 2), (short*)music->buffer);
    if(err != NULL)
    {
        Mix_SetError("GAME-EMU: %s", err);
        return 0;
    }

    if (SDL_AudioStreamPut(music->stream, music->buffer, bytes) < 0) {
        return -1;
    }
    return 0;
}

/* Play some of a stream previously started with GME_play() */
static int GME_playAudio(void *music_p, void *data, int bytes)
{
    GME_Music *music = (GME_Music*)music_p;
    return music_pcm_getaudio(music_p, data, bytes, music->volume, GME_GetSome);
}

/* Stop playback of a stream previously started with GME_play() */
/*
static void GME_stop(void *music_p)
{
    GME_Music *music = (GME_Music*)music_p;
    if(music)
        music->playing = -1;
}*/

/* Close the given Game Music Emulators stream */
static void GME_delete(void *context)
{
    GME_Music *music = (GME_Music*)context;
    if(music)
    {
        /* META-TAGS */
        if(music->mus_title)
            SDL_free(music->mus_title);
        if(music->mus_artist)
            SDL_free(music->mus_artist);
        if(music->mus_album)
            SDL_free(music->mus_album);
        if(music->mus_copyright)
            SDL_free(music->mus_copyright);
        /* META-TAGS */
        if(music->game_emu)
        {
            gme_delete(music->game_emu);
            music->game_emu = NULL;
        }
        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        SDL_free(music);
    }
}

static const char *GME_metaTitle(void *music_p)
{
    GME_Music *music = (GME_Music *)music_p;
    return music->mus_title ? music->mus_title : "";
}

static const char *GME_metaArtist(void *music_p)
{
    GME_Music *music = (GME_Music *)music_p;
    return music->mus_artist ? music->mus_artist : "";
}

static const char *GME_metaAlbum(void *music_p)
{
    GME_Music *music = (GME_Music *)music_p;
    return music->mus_album ? music->mus_album : "";
}

static const char *GME_metaCopyright(void *music_p)
{
    GME_Music *music = (GME_Music *)music_p;
    return music->mus_copyright ? music->mus_copyright : "";
}

/* Jump (seek) to a given position (time is in seconds) */
static int GME_jump_to_time(void *music_p, double time)
{
    GME_Music *music = (GME_Music*)music_p;
    gme_seek(music->game_emu, (int)round(time * 1000.0));
    return 0;
}

static double GME_get_cur_time(void *music_p)
{
    GME_Music *music = (GME_Music*)music_p;
    return (double)gme_tell(music->game_emu) / 1000.0;
}

Mix_MusicInterface Mix_MusicInterface_GME =
{
    "GME",
    MIX_MUSIC_GME,
    MUS_GME,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    GME_new_RW,
    NULL,   /* CreateFromFile */
    GME_setvolume,
    GME_play,
    NULL,   /* IsPlaying */
    GME_playAudio,
    GME_jump_to_time,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    GME_delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};

#endif
