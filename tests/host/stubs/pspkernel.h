/* Host-side stand-in for the PSP kernel pieces used by the adhoc source. */
#ifndef OSL_HOST_PSPKERNEL_H
#define OSL_HOST_PSPKERNEL_H

#include <stdint.h>
#include <time.h>

typedef uint8_t u8;

int sceKernelDelayThread(unsigned int delay);
time_t sceKernelLibcTime(time_t *t);

#endif
