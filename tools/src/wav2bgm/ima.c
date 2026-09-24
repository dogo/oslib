/* myima.c
   encode ADPCM

Copyright (C) 2003  Damian Yerrick

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to 
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA  02111-1307, USA.
GNU licenses can be viewed online at http://www.gnu.org/copyleft/

In addition, as a special exception, Damian Yerrick gives
permission to link the code of this program with import libraries
distributed as part of the plug-in development tools published by
the vendor of any audio manipulation program, and distribute
linked combinations including the two.  You must obey the GNU
General Public License in all respects for all of the code used
other than the plug-in interface.  If you modify this file, you
may extend this exception to your version of the file, but you
are not obligated to do so.  If you do not wish to do so, delete
this exception statement from your version.

Visit http://www.pineight.com/ for more information.

*/

/* Explanation

This is an ADPCM encoder and decoder.  It writes out an ADPCM
bitstream (which, with the appropriate settings, can be IMA
compliant) and also writes out the result of coding and decoding.
It allows for interchangeable quantizers and predictors; one is the
common IMA ADPCM setup, and the other has been tweaked for better
attack characteristics by Damian Yerrick.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readwav.h"


/* WAV OUTPUT STUFF ************************************************/

typedef struct WAVOUT_SPECS
{
  unsigned long len;  /* in samples per channel */
  unsigned long sample_rate;  /* in Hz */
  unsigned char sample_width;  /* in bytes per sample, usually 1 or 2 */
  unsigned char channels;
} WAVOUT_SPECS;

const unsigned char canonical_wav_header[44] =
{
  'R','I','F','F',  0,  0,  0,  0,'W','A','V','E','f','m','t',' ',
   16,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,'d','a','t','a',  0,  0,  0,  0
};

void fill_32(unsigned char *dest, unsigned long src)
{
  int i;

  for(i = 0; i < 4; i++)
    {
      *dest++ = src;
      src >>= 8;
    }
}

void wavout_make_header(unsigned char *header, const WAVOUT_SPECS *data)
{
  memcpy(header, canonical_wav_header, 44);

  header[22] = data->channels;
  header[32] = data->sample_width * header[22];
  fill_32(header + 24, data->sample_rate);
  fill_32(header + 28, header[32] * data->sample_rate);
  header[34] = 8 * data->sample_width;
  fill_32(header + 4, data->len * header[32] + 36);
  fill_32(header + 40, data->len * header[32]);
}




/* IMA ENCODER STUFF ***********************************************/

typedef struct IMA_STATE
{
  int last_sample;
  int last_index;
  int min_index;
  unsigned int (*quantize)(int, int);
  int (*rescale)(int, unsigned int);
  const signed char *step_indices;
} IMA_STATE;

static const signed char ima_step_indices[16] =
{
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

static const signed char ima9_step_indices[16] =
{
  -1, -1, -1, -1, 2, 4, 7, 12,
  -1, -1, -1, -1, 2, 4, 7, 12
};

static const unsigned short ima_step_table[89] =
{
      7,    8,    9,   10,   11,   12,   13,   14,   16,   17,
     19,   21,   23,   25,   28,   31,   34,   37,   41,   45,
     50,   55,   60,   66,   73,   80,   88,   97,  107,  118,
    130,  143,  157,  173,  190,  209,  230,  253,  279,  307,
    337,  371,  408,  449,  494,  544,  598,  658,  724,  796,
    876,  963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
   2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
   5894, 6484, 7132, 7845, 8630, 9493,10442,11487,12635,13899,
  15289,16818,18500,20350,22385,24623,27086,29794,32767
};


static unsigned int ima_quantize(int step, int diff)
{
  unsigned int sign = (diff < 0) ? 8 : 0;
  unsigned int code;

  if(sign)
    diff = -diff;
  code = 4 * diff / step;
  if(code > 7)
    code = 7;
  return code | sign;
}


static int ima_rescale(int step, unsigned int code)
{
  int diff = step >> 3;
  if(code & 1)
    diff += step >> 2;
  if(code & 2)
    diff += step >> 1;
  if(code & 4)
    diff += step;
  if(code & 8)
    diff = -diff;
  return diff;
}


static unsigned int ima9_quantize(int step, int diff)
{
  unsigned int sign = (diff < 0) ? 8 : 0;
  unsigned int code;

  if(sign)
    diff = -diff;
  code = 4 * diff / step;
  if(code == 7)
    code = 6;
  if(code > 7)
    code = 7;
  return code | sign;
}


static int ima9_rescale(int step, unsigned int code)
{
  /* 0,1,2,3,4,5,6,9 */
  int diff = step >> 3;
  if(code & 1)
    diff += step >> 2;
  if(code & 2)
    diff += step >> 1;
  if(code & 4)
    diff += step;
  if((code & 7) == 7)
    diff += step >> 1;
  if(code & 8)
    diff = -diff;
  return diff;
}


static unsigned int ima35_quantize(int step, int diff)
{
  unsigned int sign = (diff < 0) ? 8 : 0;
  unsigned int deci, code;

  if(sign)
    diff = -diff;

  deci = 16 * diff / step;

  if(deci < 25)
  {
    if(deci < 9)
      code = 0 | (deci >= 4);
    else
      code = 2 | (deci >= 16);
  }
  else
  {
    if(deci < 49)
      code = 4 | (deci >= 36);
    else
      code = 6 | (deci >= 64);
  }

  return code | sign;
}


static int ima35_rescale(int step, unsigned int code)
{
  const int recon_levels[8] = {1, 3, 6, 10, 15, 21, 28, 36};
  int diff = step * recon_levels[code & 0x07] >> 3;

  if(code & 8)
    diff = -diff;
  return diff;
}


void init_encode_ima(IMA_STATE *encstate, int min_index)
{
  encstate->last_sample = 0;
  encstate->last_index = min_index;
  encstate->min_index = min_index;
  encstate->quantize = ima_quantize;
  encstate->rescale = ima_rescale;
  encstate->step_indices = ima_step_indices;
}


void init_encode_ima9(IMA_STATE *encstate, int min_index)
{
  encstate->last_sample = 0;
  encstate->last_index = min_index;
  encstate->min_index = min_index;
  encstate->quantize = ima9_quantize;
  encstate->rescale = ima9_rescale;
  encstate->step_indices = ima9_step_indices;
}


void init_encode_ima35(IMA_STATE *encstate, int min_index)
{
  encstate->last_sample = 0;
  encstate->last_index = min_index;
  encstate->min_index = min_index;
  encstate->quantize = ima35_quantize;
  encstate->rescale = ima35_rescale;
  encstate->step_indices = ima9_step_indices;
}


void encode_ima(IMA_STATE *encstate,
                unsigned char *dst, signed short *src, size_t len)
{
  int last_sample = encstate->last_sample;
  int index = encstate->last_index;
  int min_index = encstate->min_index;
  unsigned char cur_byte = 0;

  while(len > 0)
  {
    int step, diff, code;

    if(index < min_index)
      index = min_index;
    if(index > 88)
      index = 88;
    step = ima_step_table[index];

    diff = *src++ - last_sample;

    code = encstate->quantize(step, diff);
    diff = encstate->rescale(step, code);
    index += encstate->step_indices[code & 0x07];

    last_sample += diff;
    if(last_sample < -32768)
      last_sample = -32768;
    if(last_sample > 32767)
      last_sample = 32767;

    if(len & 1)  // if we're encoding an odd-numbered sample
      *dst++ = (code << 4) | cur_byte;
    else
      cur_byte = code;

    len--;
  }

  encstate->last_index = index;
  encstate->last_sample = last_sample;
}


void decode_ima(IMA_STATE *encstate,
                signed short *dst, unsigned char *src, size_t len)
{
  int last_sample = encstate->last_sample;
  int index = encstate->last_index;
  int min_index = encstate->min_index;

  while(len > 0)
  {
    int step, diff, code;

    if(index < min_index)
      index = min_index;
    if(index > 88)
      index = 88;
    step = ima_step_table[index];

    if(len & 1)
      code = *src++ >> 4;
    else
      code = *src & 0x0f;

    diff = encstate->rescale(step, code);
    index += encstate->step_indices[code & 0x07];

    last_sample += diff;
    if(last_sample < -32768)
      last_sample = -32768;
    if(last_sample > 32767)
      last_sample = 32767;
    *dst++ = last_sample;

    len--;
  }

  encstate->last_index = index;
  encstate->last_sample = last_sample;
}


/* STATISTICAL STUFF */

unsigned long histo_bins[16];
unsigned long histo_total;

void clear_bins(void)
{
  unsigned int i;

  for(i = 0; i < 16; i++)
    histo_bins[i] = 0;
  histo_total = 0;
}

void add_nibbles_to_bins(const unsigned char *src, size_t n_samples)
{
  histo_total += n_samples;
  for(; n_samples >= 2; n_samples -= 2)
  {
    unsigned char s = *src++;

    histo_bins[s >> 4]++;
    histo_bins[s & 0x0f]++;
  }
}

void write_bins(void)
{
  unsigned int i;

  for(i = 0; i < 16; i++)
  {
    unsigned long pct = (unsigned __int64)histo_bins[i] * 10000 / histo_total;
    unsigned int pctwhole = pct / 100;
    unsigned int pctfrac = pct % 100;

    printf("%1x:%9lu (%2u.%02u%%)\n", i, histo_bins[i], pctwhole, pctfrac);
  }
}

void DisplayUsage()		{
	printf("=============== WAV2BGM ===============\n");
	printf("Converts a mono WAV file to BGM format.\n\n======\n");
	printf("Usage:\n======\nwav2bgm \"yourwavfile.wav\" \"yourbgmfile.bgm\"\n");
}

/* TEST DRIVER *****************************************************/
typedef struct			{
	char strVersion[11];				// "OSLBGM v01"
	int format;							// Toujours 1
	int sampleRate;						// Taux d'échantillonnage
	unsigned char nbChannels;			// Mono ou stéréo
	unsigned char reserved[32];			// Réservé
} BGM_FORMAT_HEADER;

int main(int argc, char **argv)
{
  WAVE_SRC wav;
  IMA_STATE enc, dec;
  FILE *outfp, *codefp;
  BGM_FORMAT_HEADER bfh;

  if(argc < 3)
  {
	  DisplayUsage();
    return EXIT_FAILURE;
  }

  if(open_wave_src(&wav, argv[1]) < 0)
  {
    fputs("Can't open WAV file.\n", stderr);
    return EXIT_FAILURE;
  }

  if(wav.fmt.channels != 1)
  {
    fputs("WAV file must be MONO.\n", stderr);
    close_wave_src(&wav);
    return EXIT_FAILURE;
  }

  codefp = fopen(argv[2], "wb");
  if(!codefp)
  {
    fputs("Can't open BGM file for writing\n", stderr);
    perror(argv[2]);
    close_wave_src(&wav);
    return EXIT_FAILURE;
  }

  //Ecrit l'en-tête
  bfh.format = 1;
  bfh.nbChannels = 1;
  memset(bfh.reserved, 0, sizeof(bfh.reserved));
  bfh.sampleRate = wav.fmt.sample_rate;
  strcpy(bfh.strVersion, "OSLBGM v01");
  fwrite(&bfh, sizeof(bfh), 1, codefp);

/*  outfp = fopen("decomp.wav", "wb");
  if(!outfp)
  {
    fputs("Can't write to output WAV file\n", stderr);
    close_wave_src(&wav);
    fclose(codefp);
    return EXIT_FAILURE;
  }*/

  {
    WAVOUT_SPECS specs;
    unsigned char header[44];

    specs.len = wav.chunk_left / wav.fmt.frame_size;
    specs.sample_rate = wav.fmt.sample_rate;
    specs.sample_width = 2;
    specs.channels = 1;

    wavout_make_header(header, &specs);
//    fwrite(header, 1, sizeof(header), outfp);
  }

  init_encode_ima9(&enc, 0);
  init_encode_ima9(&dec, enc.min_index);
  clear_bins();

  while(wav.chunk_left > 0)
  {
    int i;
    signed short samples[64];
    unsigned char ima[32];
    signed short samples_out[64];

    /* read samples */
    for(i = 0; i < 64; i++)
      samples[i] = get_next_wav_sample(&wav);

    /* compress and decompress */
    encode_ima(&enc, ima, samples, 64);
    fwrite(ima, 1, sizeof(ima), codefp);
    add_nibbles_to_bins(ima, 64);
    decode_ima(&dec, samples_out, ima, 64);

    /* write samples */
    for(i = 0; i < 32; i++)
    {
      int yn = samples_out[i];

//      fputc(yn & 0xff, outfp);
//      fputc(yn >> 8, outfp);
    }
  }
//  fclose(outfp);
  fclose(codefp);
  close_wave_src(&wav);

//  fputs("code occurrence frequencies\n", stdout);
//  write_bins();

  return 0;
}

