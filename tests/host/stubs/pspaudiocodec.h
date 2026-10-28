/* Host-side stand-in for <pspaudiocodec.h>; the test defines these. */
#ifndef OSL_HOST_PSPAUDIOCODEC_H
#define OSL_HOST_PSPAUDIOCODEC_H

int sceAudiocodecCheckNeedMem(unsigned long *Buffer, int Type);
int sceAudiocodecInit(unsigned long *Buffer, int Type);
int sceAudiocodecDecode(unsigned long *Buffer, int Type);
int sceAudiocodecGetEDRAM(unsigned long *Buffer, int Type);
int sceAudiocodecReleaseEDRAM(unsigned long *Buffer);

#endif
