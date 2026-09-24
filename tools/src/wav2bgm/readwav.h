#ifndef READWAV_H
#define READWAV_H

#include <stdio.h>
#include <stdlib.h>

/* WAVE READING CODE ***********************************************/

typedef struct WAVE_FMT
{
  unsigned short int format;
  unsigned short int channels;
  unsigned int sample_rate;
  unsigned int bytes_sec;
  unsigned short int frame_size;
  unsigned short int bits_sample;
} WAVE_FMT;


typedef struct WAVE_SRC
{
  WAVE_FMT fmt;
  FILE *fp;
  size_t chunk_left;
  int cur_chn;
} WAVE_SRC;
 
int open_wave_src(WAVE_SRC *wav, const char *filename);
int get_next_wav_sample(WAVE_SRC *wav);
void close_wave_src(WAVE_SRC *wav);

#endif
