/*
 * Host-side stand-in for src/oslib.h: just enough for the pure-logic sources
 * under test to compile without the PSP SDK. Real headers are reused where
 * they are SDK-free.
 */
#ifndef _OSLIB_H_
#define _OSLIB_H_

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define oslMin(x, y) (((x) < (y)) ? (x) : (y))
#define oslMax(x, y) (((x) > (y)) ? (x) : (y))

#include "VirtualFile.h"
#include "vram_mgr.h"

#endif
