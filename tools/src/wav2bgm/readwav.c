/* readwav.c
   RIFF Wave file parser

Copyright 2003 Damian Yerrick

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

*/
#include "readwav.h"
#include <string.h>


unsigned int fgetu16(FILE *fp)
{
  unsigned char a = fgetc(fp);
  unsigned char b = fgetc(fp);

  return a | (b << 8);
}

unsigned long fgetu32(FILE *fp)
{
  unsigned char a = fgetc(fp);
  unsigned char b = fgetc(fp);
  unsigned char c = fgetc(fp);
  unsigned char d = fgetc(fp);

  return a | (b << 8) | (c << 16) | (d << 24);
}


/* get_fmt() ***************************
   Reads a format chunk from a wav file.
   Returns 0 for success or negative for failure.
*/
int get_fmt(WAVE_FMT *format, FILE *fp)
{
  unsigned int fmt_len = fgetu32(fp);

  if(fmt_len < 16)
    return -3;

  format->format = fgetu16(fp);
  format->channels = fgetu16(fp);
  format->sample_rate = fgetu32(fp);
  format->bytes_sec = fgetu32(fp);
  format->frame_size = fgetu16(fp);
  format->bits_sample = fgetu16(fp);

  fseek(fp, fmt_len - 16, SEEK_CUR);
  return 0;
}


void close_wave_src(WAVE_SRC *wav)
{
  fclose(wav->fp);
  wav->fp = 0;
}

/* open_wave_src() *********************
   Opens a RIFF WAVE (.wav) file for reading through
   get_next_wav_sample().  Returns the following error codes:
     -1  could not open; details are in errno
     -2  bad signature
     -3  bad format metadata or no format metadata before sample data
     -4  no sample data
*/
int open_wave_src(WAVE_SRC *wav, const char *filename)
{
  char buf[256];
  int got_fmt = 0;

  /* open the file */
  wav->fp = fopen(filename, "rb");
  if(!wav->fp)
    return -1;

  /* read the header */
  if(fread(buf, 1, 12, wav->fp) < 12)
  {
    close_wave_src(wav);
    return -2;
  }

  /* check for RIFF/WAVE signature */
  if(memcmp("RIFF", buf, 4) || memcmp("WAVE", buf + 8, 4))
  {
    close_wave_src(wav);
    return -2;
  }

  /* parse chunks */
  while(fread(buf, 4, 1, wav->fp))
  {
    if(!memcmp("fmt ", buf, 4))
    {
      int errc = get_fmt(&(wav->fmt), wav->fp);
      if(errc < 0)
      {
        close_wave_src(wav);
        return -3;
      }
      got_fmt = 1;
    }
    else if(!memcmp("data", buf, 4))
    {
      if(!got_fmt)
      {
        close_wave_src(wav);
        return -3;
      }

      wav->chunk_left = fgetu32(wav->fp);
      if(wav->chunk_left == 0)
      {
        close_wave_src(wav);
        return -4;
      }

      /* at this point, we have success */
      wav->cur_chn = 0;
      return 0;
    }
    else /* skip unrecognized chunk type */
    {
      unsigned long chunk_size = fgetu32(wav->fp);

      fseek(wav->fp, chunk_size, SEEK_CUR);
    }
  }
  /* we've come to the end of all the chunks and found no data */
  close_wave_src(wav);
  return -4;
}


/* get_next_wav_sample() ***************
   Get the next sample from a wav file.
*/
int get_next_wav_sample(WAVE_SRC *wav)
{
  int cur_sample = 0;
  int i;

  if(wav->chunk_left == 0)
    return 0;

  for(i = 0; i < wav->fmt.bits_sample && wav->chunk_left > 0; i += 8)
  {
    int c = fgetc(wav->fp);

    cur_sample >>= 8;
    cur_sample |= (c & 0xff) << 8;
    wav->chunk_left--;
  }

  if(wav->fmt.bits_sample <= 8) /* handle unsigned samples */
    cur_sample -= 32768;
  cur_sample = (signed short)cur_sample; /* sign-extend */

  if(++wav->cur_chn >= wav->fmt.channels)
    wav->cur_chn = 0;

  return cur_sample;
}



