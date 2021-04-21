/* -*- C++ -*-
 *
 *  ONScripterLabel_sound.cpp - Methods for playing sound for ONScripter-EN
 *
 *  Copyright (c) 2001-2011 Ogapee. All rights reserved.
 *  (original ONScripter, of which this is a fork).
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  Copyright (c) 2008-2011 "Uncle" Mion Sonozaki
 *
 *  UncleMion@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>
 *  or write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Modified by Haeleth, Autumn 2006, to better support OS X/Linux packaging.

// Modified by Mion, April 2009, to update from
// Ogapee's 20090331 release source code.

// Modified by Mion, November 2009, to update from
// Ogapee's 20091115 release source code.

#include "ONScripterLabel.h"
#include <new>
#include <filesystem>

#ifdef LINUX
#include <signal.h>
#endif

#ifdef USE_AVIFILE
#include "AVIWrapper.h"
#endif

struct WAVE_HEADER{
    char chunk_riff[4];
    char riff_length[4];
    // format chunk
    char fmt_id[8];
    char fmt_size[4];
    char data_fmt[2];
    char channels[2];
    char frequency[4];
    char byte_size[4];
    char sample_byte_size[2];
    char sample_bit_size[2];
} header;
struct WAVE_DATA_HEADER{
    // data chunk
    char chunk_id[4];
    char data_length[4];
} data_header;
static void setupWaveHeader( unsigned char *buffer, int channels, int bits,
                             unsigned long rate, unsigned long data_length,
                             unsigned int extra_bytes=0, unsigned char *extra_ptr=NULL );

static inline void clearTimer(SDL_TimerID &timer_id)
{
    clearTimer( timer_id );
}

extern bool ext_music_play_once_flag;

extern "C"{
    extern void mp3callback( void *userdata, Uint8 *stream, int len );
    extern void oggcallback( void *userdata, Uint8 *stream, int len );
    extern Uint32 SDLCALL cdaudioCallback( Uint32 interval, void *param );
    extern Uint32 SDLCALL silentmovieCallback( Uint32 interval, void *param );
#if defined(MACOSX) //insani
    extern Uint32 SDLCALL seqmusicSDLCallback( Uint32 interval, void *param );
#endif
}
extern void seqmusicCallback( int sig );
extern void musicCallback( int sig );
extern SDL_TimerID timer_cdaudio_id;
extern SDL_TimerID timer_silentmovie_id;

#if defined(MACOSX) //insani
extern SDL_TimerID timer_seqmusic_id;
#endif

#define TMP_SEQMUSIC_FILE "tmp.mus"
#define TMP_MUSIC_FILE "tmp.mus"

#define SWAP_SHORT_BYTES(sptr){          \
            Uint8 *bptr = (Uint8 *)sptr; \
            Uint8 tmpb = *bptr;          \
            *bptr = *(bptr+1);           \
            *(bptr+1) = tmpb;            \
        }

//WMA header format
#define IS_ASF_HDR(buf)                           \
         ((buf[0] == 0x30) && (buf[1] == 0x26) && \
          (buf[2] == 0xb2) && (buf[3] == 0x75) && \
          (buf[4] == 0x8e) && (buf[5] == 0x66) && \
          (buf[6] == 0xcf) && (buf[7] == 0x11))

//AVI header format
#define IS_AVI_HDR(buf)                         \
         ((buf[0] == 'R') && (buf[1] == 'I') && \
          (buf[2] == 'F') && (buf[3] == 'F') && \
          (buf[8] == 'A') && (buf[9] == 'V') && \
          (buf[10] == 'I'))

//MIDI header format
#define IS_MIDI_HDR(buf)                         \
         ((buf[0] == 'M') && (buf[1] == 'T') && \
          (buf[2] == 'h') && (buf[3] == 'd') && \
          (buf[4] == 0)  && (buf[5] == 0) && \
          (buf[6] == 0)  && (buf[7] == 6))

extern long decodeOggVorbis(ONScripterLabel *scripterLabel, Uint8 *buf_dst, long len, bool do_rate_conversion)
{
    //SDL_LockMutex(scripterLabel->mMusicMutex);
    int current_section;
    long total_len = 0;

    OVInfo *ovi = scripterLabel->music_struct.ovi;
    char *buf = (char*)buf_dst;
    if (do_rate_conversion && ovi->cvt.needed){
        len = len * ovi->mult1 / ovi->mult2;
        if (ovi->cvt_len < len*ovi->cvt.len_mult){
            if (ovi->cvt.buf) delete[] ovi->cvt.buf;
            ovi->cvt.buf = new Uint8[len*ovi->cvt.len_mult];
            ovi->cvt_len = len*ovi->cvt.len_mult;
        }
        buf = (char*)ovi->cvt.buf;
    }

#ifdef USE_OGG_VORBIS
    while(1){
#ifdef INTEGER_OGG_VORBIS
        long src_len = ov_read( &ovi->ovf, buf, len, &current_section);
#else
        long src_len = ov_read( &ovi->ovf, buf, len, 0, 2, 1, &current_section);
#endif
        if (src_len <= 0) break;

        int vol = scripterLabel->music_struct.is_mute ? 0 : scripterLabel->music_struct.volume;
        if (scripterLabel->music_struct.voice_sample && *(scripterLabel->music_struct.voice_sample))
            vol /= 2;
        long dst_len = src_len;
        if (do_rate_conversion && ovi->cvt.needed){
            ovi->cvt.len = src_len;
            if (-1 == SDL_ConvertAudio(&ovi->cvt))
            {
              //SDL_UnlockMutex(scripterLabel->mMusicMutex);
              return 0;
            }
            memcpy(buf_dst, ovi->cvt.buf, ovi->cvt.len_cvt);
            dst_len = ovi->cvt.len_cvt;

            if (vol != DEFAULT_VOLUME){
                // volume change under SOUND_OGG_STREAMING
                for (int i=0 ; i<dst_len ; i+=2){
                    short a = *(short*)(buf_dst+i);
                    a = a*vol/100;
                    *(short*)(buf_dst+i) = a;
                }
            }
            buf_dst += ovi->cvt.len_cvt;
        }
        else{
            if (do_rate_conversion && vol != DEFAULT_VOLUME){ 
                // volume change under SOUND_OGG_STREAMING
                for (int i=0 ; i<dst_len ; i+=2){
                    if constexpr (SDL_BYTEORDER == SDL_BIG_ENDIAN){
                        SWAP_SHORT_BYTES( ((short*)(buf_dst+i)) )
                    }

                    short a = *(short*)(buf_dst+i);
                    a = a*vol/100;
                    *(short*)(buf_dst+i) = a;

                    if constexpr (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                        SWAP_SHORT_BYTES( ((short*)(buf_dst+i)) )
                    }
                }
            }
            buf += dst_len;
            buf_dst += dst_len;
        }

        total_len += dst_len;
        if (src_len == len) break;
        len -= src_len;
    }
#endif
    
    //SDL_UnlockMutex(scripterLabel->mMusicMutex);
    return total_len;
}

bool PlayOnSoundEngine(ONScripterLabel::SoundEngine& engine, const char *filename, bool loop, int channel, unsigned char* buffer = nullptr, size_t length = 0)
{
  std::filesystem::path path = filename;

  if (path.extension() == ".ogg")
    channel = MIX_BGM_CHANNEL; // probably need to do this in the other cases as well...


  if (buffer == nullptr)
  {
    if (false == std::filesystem::exists(path))
      path.replace_extension(".wav");

    if (false == std::filesystem::exists(path))
    {
      channel = MIX_BGM_CHANNEL; // probably need to do this in the other cases as well...
      path.replace_extension(".ogg");
    }

    if (false == std::filesystem::exists(path))
      path.replace_extension(".mp3");

    if (false == std::filesystem::exists(path))
      return false;
  }

  auto it = engine.mSamples.find(filename);
  if (it == engine.mSamples.end())
  {
    auto sampleIt = engine.mSamples.emplace(filename, SoLoud::Wav{});
    it = sampleIt.first;

    if (buffer == nullptr)
      it->second.load(path.u8string().c_str());
    else
      it->second.loadMem(buffer, length, true, false);
  }

  auto channelIt = engine.mHandles.find(channel);
  if (channelIt != engine.mHandles.end())
  {
    engine.mSoLoud.stop(channelIt->second);
    engine.mHandles.erase(channelIt);
  }
  
  engine.mHandles.emplace(channel, engine.mSoLoud.play(it->second));

  auto channelHandle = engine.mHandles[channel];
  engine.mSoLoud.setLooping(channelHandle, loop);

  printf("Playing filename: %s \t channel: %d\n", filename, channel);
  return true;
}

int ONScripterLabel::playSound(const char *filename, int format, bool loop_flag, int channel)
{
    if ( !audio_open_flag ) return SOUND_NONE;

    static bool gFirstTime = false;
    if (gFirstTime == false)
    {
      gFirstTime = true;
      mSoundEngine.mSoLoud.init();
    }

    if (PlayOnSoundEngine(mSoundEngine, filename, loop_flag, channel))
    {
      return SOUND_OTHER;
    }

    long length = script_h.cBR->getFileLength( filename );
    if (length == 0) return SOUND_NONE;
    
    //Mion: account for mode_wave_demo setting
    //(i.e. if not set, then don't play non-bgm wave/ogg during skip mode)
    if (!mode_wave_demo_flag &&
        ( (skip_mode & SKIP_NORMAL) || ctrl_pressed_status )) {
        if ((format & (SOUND_OGG | SOUND_WAVE)) &&
            ((channel < ONS_MIX_CHANNELS) || (channel == MIX_WAVE_CHANNEL) ||
             (channel == MIX_CLICKVOICE_CHANNEL)))
            return SOUND_NONE;
    }
    
    bool fileLoaded = false;
    unsigned char *buffer;
    
    if ((format & (SOUND_MP3 | SOUND_OGG_STREAMING)) && 
        (length == music_buffer_length) &&
        music_buffer ){
        buffer = music_buffer;
    }
    else{
        buffer = new(std::nothrow) unsigned char[length];
        if (buffer == NULL) {
            snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                     "failed to load sound file [%s] (%lu bytes)",
                     filename, length);
            errorAndCont( script_h.errbuf, "unable to allocate buffer", "Memory Issue" );
            return SOUND_NONE;
        }
        script_h.cBR->getFile( filename, buffer );
        fileLoaded = true;
    }
    
    if (fileLoaded)
    {
      if (format & SOUND_OGG_STREAMING) channel = MIX_BGM_CHANNEL;
      PlayOnSoundEngine(mSoundEngine, filename, loop_flag, channel, buffer, length);
    }
    return SOUND_OTHER;
}

void ONScripterLabel::playCDAudio()
{
    if (!audio_open_flag) return;

    if ( cdaudio_flag ){
#ifdef ONSCRIPTER_CDAUDIO
        if ( cdrom_info ){
            int length = cdrom_info->track[current_cd_track - 1].length / 75;
            SDL_CDPlayTracks( cdrom_info, current_cd_track - 1, 0, 1, 0 );
            timer_cdaudio_id = SDL_AddTimer( length * 1000, cdaudioCallback, NULL );
        }
#endif
    }
    else{
        //if CD audio is not available, search the "cd" subfolder
        //for a file named "track01.mp3" or similar, depending on the
        //track number; check for mp3, ogg and wav files
        //
        // Trying different extensions will actually occur in a sub function
        // so we don't have to deal with it here.
        char filename[256];
        sprintf( filename, "cd\\track%2.2d.ogg", current_cd_track );
        int ret = playSound( filename, SOUND_OGG_STREAMING, cd_play_loop_flag );
        if (ret == SOUND_OGG_STREAMING) return;
    }
}

int ONScripterLabel::playExternalMusic(bool loop_flag)
{
    // Yeah we are absolutely not doing this the way they originally did,
    // that's a _wild_ suggestion.
    int seqmusic_looping = loop_flag ? -1 : 0;
    char music_filename[256];
    sprintf(music_filename, "%s%s", script_h.save_path, TMP_MUSIC_FILE);

    if (!PlayOnSoundEngine(mSoundEngine, music_filename, seqmusic_looping, MIX_EXTERNAL_CHANNEL1)) {
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                 "error in sequenced music file %s", music_filename );
        puts(script_h.errbuf);
        errorAndCont(script_h.errbuf, Mix_GetError());
        return -1;
    }

    SetVolumeOnChannel(mSoundEngine, MIX_EXTERNAL_CHANNEL1, !volume_on_flag, music_volume);

    return 0;
}

int ONScripterLabel::playSequencedMusic(bool loop_flag)
{
    // Yeah we are absolutely not doing this the way they originally did,
    // that's a _wild_ suggestion.
    char seqmusic_filename[256];
    sprintf(seqmusic_filename, "%s%s", script_h.save_path, TMP_SEQMUSIC_FILE);
    
    int seqmusic_looping = loop_flag ? -1 : 0;

    if (!PlayOnSoundEngine(mSoundEngine, seqmusic_filename, seqmusic_looping, MIX_SEQUENCE_CHANNEL0)) {
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                 "error in sequenced music file %s", seqmusic_filename );
        puts(script_h.errbuf);
        errorAndCont(script_h.errbuf, Mix_GetError());
        return -1;
    }

    SetVolumeOnChannel(mSoundEngine, MIX_SEQUENCE_CHANNEL0, !volume_on_flag, music_volume);

    current_cd_track = -2;

    return 0;
}

int ONScripterLabel::playingMusic()
{
    if (audio_open_flag && 
        ( (Mix_GetMusicHookData() != NULL) ||
          (Mix_Playing(MIX_BGM_CHANNEL) == 1) ||
          (Mix_PlayingMusic() == 1) ))
        return 1;
    else
        return 0;
}

int ONScripterLabel::setCurMusicVolume( int volume )
{
    if (!audio_open_flag) return 0;
    
    SetVolumeOnChannel(mSoundEngine, MIX_BGM_CHANNEL, !volume_on_flag, volume);
    return 0;
}

int ONScripterLabel::setVolumeMute( bool do_mute )
{
    if (!audio_open_flag) return 0;
    
    // In theory, music_vol should be used here and perhaps halved, as below.
    for ( int i=1 ; i<ONS_MIX_CHANNELS ; i++ )
      SetVolumeOnChannel(mSoundEngine, i, do_mute, se_volume);
    
    SetVolumeOnChannel(mSoundEngine, MIX_BGM_CHANNEL, do_mute, music_volume);
    SetVolumeOnChannel(mSoundEngine, MIX_LOOPBGM_CHANNEL0, do_mute, music_volume);
    SetVolumeOnChannel(mSoundEngine, MIX_LOOPBGM_CHANNEL1, do_mute, music_volume);
    
    return 0;
}

void ONScripterLabel::SMpegDisplayCallback(void* data, SMPEG_Frame* frame)
{

}

int ONScripterLabel::playMPEG( const char *filename, bool async_flag, bool use_pos, int xpos, int ypos, int width, int height )
{
    int ret = 0;
#ifndef MP3_MAD
    bool different_spec = false;
    if (async_movie) stopMovie(async_movie);
    async_movie = NULL;
    if (movie_buffer) delete[] movie_buffer;
    movie_buffer = NULL;
    if (surround_rects) delete[] surround_rects;
    surround_rects = NULL;

    unsigned long length = script_h.cBR->getFileLength( filename );

    if (length == 0) {
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                 "couldn't load movie '%s'", filename);
        errorAndCont(script_h.errbuf);
        return 0;
    }

    movie_buffer = new unsigned char[length];
    script_h.cBR->getFile( filename, movie_buffer );

    /* check for AVI header format */
    if ( IS_AVI_HDR(movie_buffer) ){
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                 "movie file '%s' is in AVI format", filename);
        errorAndCont(script_h.errbuf);
        if (movie_buffer) delete[] movie_buffer;
        movie_buffer = NULL;
        return 0;
    }

    if (mSmpegMutex == nullptr)
      mSmpegMutex = SDL_CreateMutex();
    
    SMPEG_Info info;
    SMPEG *mpeg_sample = SMPEG_new_rwops( SDL_RWFromMem( movie_buffer, length ), &info, 0, 1);
    char *errstr = SMPEG_error( mpeg_sample );
    if (errstr){
        
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                 "SMPEG error on '%s'", filename);
        errorAndCont(script_h.errbuf, errstr);
        if (movie_buffer) delete[] movie_buffer;
        movie_buffer = NULL;
        return 0;
    }
    else {
        if (info.has_audio){
            stopBGM( false );
            SMPEG_enableaudio( mpeg_sample, 0 );

            if ( audio_open_flag ){
                //Mion - SMPEG doesn't handle different audio spec well, so
                // we might reset the SDL mixer for this video playback
                SDL_AudioSpec wanted;
                SMPEG_wantedSpec( mpeg_sample, &wanted );
                //printf("SMPEG wants audio: %d Hz %d bit %s\n", wanted.freq,
                //       (wanted.format&0xFF),
                //       (wanted.channels > 1) ? "stereo" : "mono");
                if ((wanted.format != audio_format.format) ||
                    (wanted.freq != audio_format.freq)) {
                    different_spec = true;
                    Mix_CloseAudio();
                    openAudio(wanted.freq, wanted.format, wanted.channels);
                    if (!audio_open_flag) {
                        // didn't work, use the old settings
                        openAudio();
                        different_spec = false;
                    }
                }
                SMPEG_actualSpec( mpeg_sample, &audio_format );
                SMPEG_enableaudio( mpeg_sample, 1 );
            }
        } else {
            different_spec = false;
        }
        SMPEG_enablevideo( mpeg_sample, 1 );
        SMPEG_setdisplay(mpeg_sample, &ONScripterLabel::SMpegDisplayCallback, this, mSmpegMutex);
        //SMPEG_setdisplay( mpeg_sample, screen_surface, NULL, NULL );
        //if (use_pos) {
        //    SMPEG_scaleXY( mpeg_sample, width, height );
        //    SMPEG_move( mpeg_sample, xpos, ypos );
        //}
        //else if (nomovieupscale_flag && (info.width < screen_width) &&
        //         (info.height < screen_height)) {
        //    //"no-movie-upscale" set, so use its native
        //    //width/height & center within the screen
        //    SMPEG_scaleXY( mpeg_sample, info.width, info.height );
        //    SMPEG_move( mpeg_sample, (screen_width - info.width) / 2,
        //               (screen_height - info.height) / 2 );
        //}
#ifdef RCA_SCALE
        //center the movie on the screen, using standard aspect ratio
        else if ( (scr_stretch_x > 1.0) || (scr_stretch_y > 1.0) ) {
            width = ExpandPos(script_width);
            height = ExpandPos(script_height);
            SMPEG_scaleXY( mpeg_sample, width, height );
            SMPEG_move( mpeg_sample, (screen_width - width) / 2,
                       (screen_height - height) / 2 );
        }
#endif

        if (info.has_audio){
            SMPEG_setvolume( mpeg_sample, !volume_on_flag? 0 : music_volume );
            Mix_HookMusic( mp3callback, mpeg_sample );
        }

        surround_rects = new SDL_Rect[4];
        for (int i=0; i<4; ++i) {
            surround_rects[i].x = surround_rects[i].y = 0;
            surround_rects[i].w = surround_rects[i].h = 0;
        }

        if (use_pos) {
            async_movie_rect.x = xpos;
            async_movie_rect.y = ypos;
            async_movie_rect.w = width;
            async_movie_rect.h = height;

            //sur_rect[0] = { 0, 0, screen_width, ypos };
            //sur_rect[1] = { 0, ypos, xpos, height };
            //sur_rect[2] = { xpos + width, ypos, screen_width - (xpos + width), height };
            //sur_rect[3] = { 0, ypos + height, screen_width, screen_height - (ypos + height) };
            surround_rects[0].w = surround_rects[3].w = screen_width;
            surround_rects[0].h = surround_rects[1].y = surround_rects[2].y = ypos;
            surround_rects[1].w = xpos;
            surround_rects[1].h = surround_rects[2].h = height;
            surround_rects[2].x = xpos + width;
            surround_rects[2].w = screen_width - (xpos + width);
            surround_rects[3].y = ypos + height;
            surround_rects[3].h = screen_height - (ypos + height);
        } else {
            async_movie_rect.x = 0;
            async_movie_rect.y = 0;
            async_movie_rect.w = screen_width;
            async_movie_rect.h = screen_height;
        }

        if (movie_loop_flag)
            SMPEG_loop( mpeg_sample, -1 );
        SMPEG_play( mpeg_sample );

        if (async_flag){
            async_movie = mpeg_sample;
            if (!info.has_audio && movie_loop_flag){
                timer_silentmovie_id = SDL_AddTimer(100, silentmovieCallback,
                                                    (void*)&async_movie);
            }
            return 0;
        }

        bool done_flag = false;
        while( !done_flag ){
            if (SMPEG_status(mpeg_sample) != SMPEG_PLAYING){
                if (movie_loop_flag)
                    SMPEG_play( mpeg_sample );
                else
                    break;
            }

            SDL_Event event;

            while( SDL_PollEvent( &event ) ){
                switch (event.type){
                  case SDL_KEYUP:
                    if ( ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_RETURN ||
                         ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_SPACE ||
                         ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_ESCAPE )
                        done_flag = movie_click_flag;
                    else if ( ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_f ){
                        if ( !ToggleFullscreen(mWindow) ){
                            SMPEG_pause( mpeg_sample );
                            SDL_FreeSurface(screen_surface);
                            if (fullscreen_mode)
                            {
                                screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, false );
                            }
                            else
                            {
                                screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, true );
                            }
                            
                            SMPEG_setdisplay(mpeg_sample, &ONScripterLabel::SMpegDisplayCallback, this, mSmpegMutex);
                            SMPEG_play( mpeg_sample );
                        }
                        fullscreen_mode = !fullscreen_mode;
                    }
                    else if ( ((SDL_KeyboardEvent *)&event)->keysym.sym == SDLK_m ){
                        volume_on_flag = !volume_on_flag;
                        SMPEG_setvolume( mpeg_sample, !volume_on_flag? 0 : music_volume );
                        printf("turned %s volume mute\n", !volume_on_flag?"on":"off");
                    }
                    break;
                  case SDL_QUIT:
                    ret = 1;
                    done_flag = true;
                    break;
                  case SDL_MOUSEBUTTONUP:
                    done_flag = movie_click_flag;
                    break;
                  default:
                    break;
                }
                SDL_Delay( 5 );
            }
        }
        ctrl_pressed_status = 0;

        stopMovie(mpeg_sample);

        if (different_spec) {
            //restart mixer with the old audio spec
            Mix_CloseAudio();
            openAudio();
        }
    }
#else
    errorAndCont( "mpeg video playback is disabled." );
#endif

    return ret;
}

int ONScripterLabel::playAVI( const char *filename, bool click_flag )
{
#ifdef USE_AVIFILE
    char *absolute_filename = new char[ strlen(archive_path) + strlen(filename) + 1 ];
    sprintf( absolute_filename, "%s%s", archive_path, filename );
    for ( unsigned int i=0 ; i<strlen( absolute_filename ) ; i++ )
        if ( absolute_filename[i] == '/' ||
             absolute_filename[i] == '\\' )
            absolute_filename[i] = DELIMITER;

    if ( audio_open_flag ) Mix_CloseAudio();

    AVIWrapper *avi = new AVIWrapper();
    if ( avi->init( absolute_filename, false ) == 0 &&
         avi->initAV( screen_surface, audio_open_flag ) == 0 ){
        if (avi->play( click_flag )) return 1;
    }
    delete avi;
    delete[] absolute_filename;

    if ( audio_open_flag ){
        Mix_CloseAudio();
        openAudio();
    }
#else
    errorAndCont( "avi: avi video playback is disabled." );
#endif

    return 0;
}

void ONScripterLabel::stopMovie(SMPEG *mpeg)
{
    if (mpeg) {
        SMPEG_Info info;
        SMPEG_getinfo(mpeg, &info);
        SMPEG_stop( mpeg );
        if (info.has_audio){
            Mix_HookMusic( NULL, NULL );
        }
        SMPEG_delete( mpeg );

        dirty_rect.add( async_movie_rect );
    }

    if (movie_buffer) delete[] movie_buffer;
    movie_buffer = NULL;
    if (surround_rects) delete[] surround_rects;
    surround_rects = NULL;
}

void ONScripterLabel::stopBGM( bool continue_flag )
{
    printf("Stopping channel: %d\n", MIX_BGM_CHANNEL);
    auto channelIt = mSoundEngine.mHandles.find(MIX_BGM_CHANNEL);
    if (channelIt != mSoundEngine.mHandles.end())
    {
      mSoundEngine.mSoLoud.stop(channelIt->second);
      mSoundEngine.mHandles.erase(channelIt);
    }

    if ( !continue_flag ) current_cd_track = -1;
}

void ONScripterLabel::stopDWAVE( int channel )
{
    if (!audio_open_flag) return;

    //avoid stopping dwave outside array
    if (channel < 0) channel = 0;
    else if (channel >= ONS_MIX_CHANNELS) channel = ONS_MIX_CHANNELS-1;

    if ( wave_sample[channel] ){
        Mix_Pause( channel );
        if ( !channel_preloaded[channel] || channel == 0 ){
            //don't free preloaded channels, _except_:
            //always free voice channel, for now - could be
            //messy for bgmdownmode and/or voice-waiting FIXME
            Mix_FreeChunk( wave_sample[channel] );
            wave_sample[channel] = NULL;
            channel_preloaded[channel] = false;
        }
    }
    if ((channel == 0) && bgmdownmode_flag)
        setCurMusicVolume( music_volume );
}

void ONScripterLabel::stopAllDWAVE()
{
    if (!audio_open_flag) return;

    for (int ch=0; ch<ONS_MIX_CHANNELS ; ch++) {
        if ( wave_sample[ch] ){
            Mix_Pause( ch );
            if ( !channel_preloaded[ch] || ch == 0 ){
                //always free voice channel sample, for now - could be
                //messy for bgmdownmode and/or voice-waiting FIXME
                Mix_FreeChunk( wave_sample[ch] );
                wave_sample[ch] = NULL;
            }
        }
    }
    // just in case the bgm was turned down for the voice channel,
    // set the bgm volume back to normal
    if (bgmdownmode_flag)
        setCurMusicVolume( music_volume );
}

void ONScripterLabel::playClickVoice()
{
    if      ( clickstr_state == CLICK_NEWPAGE ){
        if ( clickvoice_file_name[CLICKVOICE_NEWPAGE] )
            playSound(clickvoice_file_name[CLICKVOICE_NEWPAGE],
                      SOUND_WAVE|SOUND_OGG, false, MIX_CLICKVOICE_CHANNEL);
    }
    else if ( clickstr_state == CLICK_WAIT ){
        if ( clickvoice_file_name[CLICKVOICE_NORMAL] )
            playSound(clickvoice_file_name[CLICKVOICE_NORMAL],
                      SOUND_WAVE|SOUND_OGG, false, MIX_CLICKVOICE_CHANNEL);
    }
}

void setupWaveHeader( unsigned char *buffer, int channels, int bits,
                      unsigned long rate, unsigned long data_length,
                      unsigned int extra_bytes, unsigned char *extra_ptr )
{
    memcpy( header.chunk_riff, "RIFF", 4 );
    unsigned long riff_length = sizeof(WAVE_HEADER) + sizeof(WAVE_DATA_HEADER) +
                                data_length + extra_bytes - 8;
    header.riff_length[0] = riff_length & 0xff;
    header.riff_length[1] = (riff_length >> 8) & 0xff;
    header.riff_length[2] = (riff_length >> 16) & 0xff;
    header.riff_length[3] = (riff_length >> 24) & 0xff;
    memcpy( header.fmt_id, "WAVEfmt ", 8 );
    header.fmt_size[0] = 0x10 + extra_bytes;
    header.fmt_size[1] = header.fmt_size[2] = header.fmt_size[3] = 0;
    header.data_fmt[0] = 1; header.data_fmt[1] = 0; // PCM format
    header.channels[0] = channels; header.channels[1] = 0;
    header.frequency[0] = rate & 0xff;
    header.frequency[1] = (rate >> 8) & 0xff;
    header.frequency[2] = (rate >> 16) & 0xff;
    header.frequency[3] = (rate >> 24) & 0xff;

    int sample_byte_size = channels * bits / 8;
    unsigned long byte_size = sample_byte_size * rate;
    header.byte_size[0] = byte_size & 0xff;
    header.byte_size[1] = (byte_size >> 8) & 0xff;
    header.byte_size[2] = (byte_size >> 16) & 0xff;
    header.byte_size[3] = (byte_size >> 24) & 0xff;
    header.sample_byte_size[0] = sample_byte_size;
    header.sample_byte_size[1] = 0;
    header.sample_bit_size[0] = bits;
    header.sample_bit_size[1] = 0;

    memcpy( data_header.chunk_id, "data", 4 );
    data_header.data_length[0] = (char)(data_length & 0xff);
    data_header.data_length[1] = (char)((data_length >> 8) & 0xff);
    data_header.data_length[2] = (char)((data_length >> 16) & 0xff);
    data_header.data_length[3] = (char)((data_length >> 24) & 0xff);

    memcpy( buffer, &header, sizeof(header) );
    if (extra_bytes > 0) {
        if (extra_ptr != NULL)
            memcpy( buffer+sizeof(header), extra_ptr, extra_bytes );
        else
            memset( buffer+sizeof(header), 0, extra_bytes );
    }
    memcpy( buffer+sizeof(header)+extra_bytes, &data_header, sizeof(data_header) );
}
#ifdef USE_OGG_VORBIS
static size_t oc_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    OVInfo *ogg_vorbis_info = (OVInfo*)datasource;

    size_t len = size*nmemb;
    if ((size_t)ogg_vorbis_info->pos+len > (size_t)ogg_vorbis_info->length) 
        len = (size_t)(ogg_vorbis_info->length - ogg_vorbis_info->pos);
    memcpy(ptr, ogg_vorbis_info->buf+ogg_vorbis_info->pos, len);
    ogg_vorbis_info->pos += len;

    return len;
}

static int oc_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    OVInfo *ogg_vorbis_info = (OVInfo*)datasource;

    ogg_int64_t pos = 0;
    if (whence == 0)
        pos = offset;
    else if (whence == 1)
        pos = ogg_vorbis_info->pos + offset;
    else if (whence == 2)
        pos = ogg_vorbis_info->length + offset;

    if (pos < 0 || pos > ogg_vorbis_info->length) return -1;

    ogg_vorbis_info->pos = pos;

    return 0;
}

static int oc_close_func(void *datasource)
{
    return 0;
}

static long oc_tell_func(void *datasource)
{
    OVInfo *ogg_vorbis_info = (OVInfo*)datasource;

    return (long)ogg_vorbis_info->pos;
}
#endif
OVInfo *ONScripterLabel::openOggVorbis( unsigned char *buf, long len, int &channels, int &rate )
{
    OVInfo *ovi = NULL;

#ifdef USE_OGG_VORBIS
    ovi = new OVInfo();

    ovi->buf = buf;
    ovi->decoded_length = 0;
    ovi->length = len;
    ovi->pos = 0;

    ov_callbacks oc;
    oc.read_func  = oc_read_func;
    oc.seek_func  = oc_seek_func;
    oc.close_func = oc_close_func;
    oc.tell_func  = oc_tell_func;
    if (ov_open_callbacks(ovi, &ovi->ovf, NULL, 0, oc) < 0){
        delete ovi;
        return NULL;
    }

    vorbis_info *vi = ov_info( &ovi->ovf, -1 );
    if (vi == NULL){
        ov_clear(&ovi->ovf);
        delete ovi;
        return NULL;
    }

    channels = vi->channels;
    rate = vi->rate;

    ovi->cvt.buf = NULL;
    ovi->cvt_len = 0;
    SDL_BuildAudioCVT(&ovi->cvt,
                      AUDIO_S16, channels, rate,
                      audio_format.format, audio_format.channels, audio_format.freq);
    ovi->mult1 = 10;
    ovi->mult2 = (int)(ovi->cvt.len_ratio*10.0);

    ovi->decoded_length = (long)(ov_pcm_total(&ovi->ovf, -1) * channels * 2);
#endif

    return ovi;
}

int ONScripterLabel::closeOggVorbis(OVInfo *ovi)
{
    if (ovi->buf){
        ovi->buf = NULL;
#ifdef USE_OGG_VORBIS
        ovi->length = 0;
        ovi->pos = 0;
        ov_clear(&ovi->ovf);
#endif
    }
    if (ovi->cvt.buf){
        delete[] ovi->cvt.buf;
        ovi->cvt.buf = NULL;
        ovi->cvt_len = 0;
    }
    delete ovi;

    return 0;
}
