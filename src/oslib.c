#include "oslib.h"

/*
	GRAPHICS CONSTANTS
*/
#define OSL_MAX_ANGLE 360
const int OSL_TRIGO_FACTOR = 16384;

/*
	Global Variables
*/
int osl_intraInit = 0;
int osl_quit = 0;
int osl_vblInterruptNumber = 0;
int (*osl_powerCallback)(int, int, void*) = NULL;
int (*osl_exitCallback)(int, int, void*) = NULL;
OSL_CONTROLLER *osl_keys;
OSL_REMOTECONTROLLER *osl_remotekeys;

int osl_noFail = 0;
int osl_isinus[OSL_MAX_ANGLE + 1];
int osl_icosinus[OSL_MAX_ANGLE + 1];

void oslSetupFTrigo() {
	// Precompute integer sine and cosine tables
	for (int i = 0; i <= OSL_MAX_ANGLE; i++) {
		osl_isinus[i] = (int)(oslSin(i, OSL_TRIGO_FACTOR));
		osl_icosinus[i] = (int)(oslCos(i, OSL_TRIGO_FACTOR));
	}
}

int oslCosi(int angle, int dist) {
	angle = (angle % OSL_MAX_ANGLE + OSL_MAX_ANGLE) % OSL_MAX_ANGLE;
	return (osl_icosinus[angle] * dist) >> 14;
}

int oslSini(int angle, int dist) {
	angle = (angle % OSL_MAX_ANGLE + OSL_MAX_ANGLE) % OSL_MAX_ANGLE;
	return (osl_isinus[angle] * dist) >> 14;
}

int oslGetNextPower2(int val) {
	int power = 1;
	while (power < val) {
		power <<= 1;
	}
	return power;
}

// Align data to the nearest multiple of align
unsigned int oslAlignData(unsigned int data, int align) {
	return (data + align - 1) & ~(align - 1);
}

// Fast memset using hardware optimized operations (e.g., DMA)
void oslFasterMemset(u64 *dst, u64 *src, u32 length) {
	u32 chunkCount = length >> 6;  // Process in 64-byte chunks (8 x 8 bytes)
	while (chunkCount--) {
		// Manually unrolled loop to copy 8 elements per iteration
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
		*dst++ = *src;
	}
	u32 remaining = (length & 63) >> 3;  // Handle remaining 8-byte chunks
	while (remaining--) {
		*dst++ = *src;
	}

	// Handle any remaining bytes (less than 8)
	u8 *dst2 = (u8*)dst;
	const u8 *src2 = (const u8*)src;
	remaining = length & 7;
	while (remaining--) {
		*dst2++ = *src2++;
	}
}

void oslWaitVSync() {
	sceDisplayWaitVblankStart();
}

inline void oslFlushDataCache() {
	sceKernelDcacheWritebackInvalidateAll();
}

int oslMeanBenchmarkTestEx(int startend, int slot) {
    static int val[OSL_BENCH_SLOTS] = {0};
    static int curr_ms[OSL_BENCH_SLOTS] = {0};
    static struct timeval start[OSL_BENCH_SLOTS], end;
    static int time[OSL_BENCH_SLOTS] = {0};

    if (startend == OSL_BENCH_INIT) {
        val[slot] = 0;
        time[slot] = 0;
        curr_ms[slot] = 0;
        gettimeofday(&start[slot], NULL);
    } else if (startend == OSL_BENCH_START) {
        gettimeofday(&start[slot], NULL);
    } else if (startend == OSL_BENCH_END) {
        gettimeofday(&end, NULL);
        time[slot] += (end.tv_sec - start[slot].tv_sec) * 1000000 + (end.tv_usec - start[slot].tv_usec);
        val[slot]++;
        if (val[slot] >= OSL_BENCH_SAMPLES) {
            curr_ms[slot] = time[slot] / OSL_BENCH_SAMPLES;
            val[slot] = 0;
            time[slot] = 0;
        }
    } else if (startend == OSL_BENCH_GET_LAST) { // Returns the last measure
		if (val[slot] != 0) {
			return time[slot] / val[slot];
		}
	}
    return curr_ms[slot];
}

int oslBenchmarkTestEx(int startend, int slot) {
	static struct timeval start[OSL_BENCH_SLOTS];
	static int time[OSL_BENCH_SLOTS] = {0};

	if (startend == OSL_BENCH_START) {
		gettimeofday(&start[slot], NULL);
	} else if (startend == OSL_BENCH_END) {
		struct timeval end;
		gettimeofday(&end, NULL);
		time[slot] = (end.tv_sec - start[slot].tv_sec) * 1000000;
		time[slot] += end.tv_usec - start[slot].tv_usec;
	}
	return time[slot];
}

static int osl_exitCbId = 0; // Stores the OSL's standard exit callback ID

void oslQuit() {
    // Set the quit flag to indicate that the application should terminate
    osl_quit = 1;

    // Allow other threads some time to handle the quit marker
    sceKernelDelayThread(500000); // Delay by 500 milliseconds (500000 microseconds)

    // Check if an exit callback is registered
    if (osl_exitCbId == 0) {
        // No callback set, proceed to exit the game immediately
        sceKernelExitGame();
    } else {
        // Notify the registered callback that an exit event has occurred
        sceKernelNotifyCallback(osl_exitCbId, 0);

        // Put the current thread to sleep indefinitely until it is explicitly woken up
        // This prevents further execution and gives control to the exit callback
        sceKernelSleepThreadCB();
    }
}

#ifdef PSP
	/* Exit callback */
	int oslStandardExitCallback(int arg1, int arg2, void *common) {
		osl_quit = 1;
		if (osl_exitCallback) {
			osl_exitCallback(arg1, arg2, common);
		}
		sceKernelExitGame();
		return 0;
	}

	int oslStandardPowerCallback(int unknown, int pwrflags, void *common) {
		if (osl_powerCallback) {
			return osl_powerCallback(unknown, pwrflags, common);
		} else {
			return 0;
		}
	}

	/* Callback thread */
	int oslCallbackThread(SceSize args, void *argp) {
		int cbid;
		cbid = sceKernelCreateCallback("exitCallback", oslStandardExitCallback, NULL);
		sceKernelRegisterExitCallback(cbid);
		osl_exitCbId = cbid;

		cbid = sceKernelCreateCallback("powerCallback", oslStandardPowerCallback, NULL);
		scePowerRegisterCallback(0, cbid);

		sceKernelSleepThreadCB();
		return 0;
	}

	/* Sets up the callback thread and returns its thread id */
	int oslSetupCallbacks() {
		int thid = sceKernelCreateThread("update_thread", oslCallbackThread, 0x11, 0xFA0, 0, 0);
		if (thid >= 0) {
			sceKernelStartThread(thid, 0, 0);
		}
		return thid;
	}
#endif

/*
    VSync and Frame Synchronization
*/
volatile int osl_vblCount = 0, osl_vblCountMultiple = 0, osl_currentFrameRate = 60, osl_vblCallCount = 0, osl_skip = 0, osl_nbSkippedFrames = 0;
volatile int osl_vblankCounterActive = 1, osl_vblShouldSwap = 0;

void oslVblankNextFrame() {
    osl_vblCountMultiple += osl_currentFrameRate;
    if (osl_vblCountMultiple >= 60) {
        osl_vblCountMultiple -= 60;
        osl_vblCount++;
    }
}

void oslVblankInterrupt(int sub, void *parg) {
    if (osl_vblankCounterActive) {
        oslVblankNextFrame();
    }

    if (osl_vblShouldSwap) {
        oslSwapBuffers();
        osl_vblShouldSwap = 0;
    }
}

int osl_maxFrameskip = 0, osl_vsyncEnabled = 0, osl_frameskip = 0;

void oslInit(int flags) {
	void *arg = 0;

    osl_keys = &osl_pad;
    osl_remotekeys = &osl_remote;

    osl_quit = 0;
    osl_vblCount = 0;
    osl_vblCallCount = 0;
    osl_skip = 0;
    osl_nbSkippedFrames = 0;
    osl_maxFrameskip = 5;
    osl_vsyncEnabled = 4;
    osl_frameskip = 0;
    osl_currentFrameRate = 60;

    oslSetKeyAutorepeat(OSL_KEYMASK_UP | OSL_KEYMASK_RIGHT | OSL_KEYMASK_DOWN | OSL_KEYMASK_LEFT | OSL_KEYMASK_R | OSL_KEYMASK_L, 0, 0);

#ifdef PSP
    if (!(flags & OSL_IF_USEOWNCALLBACKS)) {
        oslSetupCallbacks();
    }

    if (!(flags & OSL_IF_NOVBLANKIRQ)) {
        sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, osl_vblInterruptNumber, oslVblankInterrupt, arg);
        sceKernelEnableSubIntr(PSP_VBLANK_INT, osl_vblInterruptNumber);
    }
#endif

    for (int i = 0; i < OSL_BENCH_SLOTS; i++) {
        oslMeanBenchmarkTestEx(OSL_BENCH_INIT, i);
    }

    VirtualFileInit();

#ifndef PSP
    emuInitGfx();
    emuStartDrawing();
    emuInitGL();
#endif
}

#define OSL_SYSTEM_BENCHMARK_ENABLED

void oslSysBenchmarkDisplay() {
#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
    int ms4 = oslMeanBenchmarkTestEx(OSL_BENCH_GET_LAST, 4);
    int ms5 = oslMeanBenchmarkTestEx(OSL_BENCH_GET_LAST, 5);
    int ms6 = oslMeanBenchmarkTestEx(OSL_BENCH_GET_LAST, 6);

    oslSetTextColor(RGB(255, 255, 255));
    oslSetBkColor(RGBA(0, 0, 0, 0x80));
    oslPrintf_xy(0, 0, "%i.%03i + %i.%03i = %i.%03i", ms4 / 1000, ms4 % 1000, ms5 / 1000, ms5 % 1000, ms6 / 1000, ms6 % 1000);
#endif
}

/*
	Frameskip:
				0: No frameskip (normal)
				1: Normal frameskip
				>1: Depends on vsync, skips 1 frame out of X

	Max frameskip:
				>=1: Maximum allowed frameskip
	VSync:
				0: No VSync
				1: VSync enabled
				+4: If added with frameskip > 1, synchronizes at desired framerate (e.g., 2 -> 30 fps)
				+8: Maximal synchronization (similar to triple buffering) without vsync
				+16: No buffer swapping
	Examples:
		// 30 fps, no frameskip
		oslSyncFrameEx(2, 0, 0);
		// 30 fps, game runs at 60 fps with max frameskip of 2, meaning no more than one frame skipped for every one displayed
		oslSyncFrameEx(2, 2, 4);
		// Synchronize at 60 fps, no frameskip
		oslSyncFrameEx(0, 0, 0);
*/
int oslSyncFrameEx(int frameskip, int max_frameskip, int vsync) {
	int i = 0, wasDrawing = 0;

	// End drawing if it was started
	if (osl_isDrawingStarted) {
		oslEndDrawing();
		wasDrawing = 1;
	}

	if (frameskip == 0) {  // No frameskip case
		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		oslMeanBenchmarkTestEx(OSL_BENCH_END, 4);
		oslMeanBenchmarkTestEx(OSL_BENCH_END, 6);
		#endif

		osl_vblankCounterActive = 0;  // Disable vblank counting for accuracy
		if ((vsync & 5) || (osl_vblCallCount + 1 > osl_vblCount)) {
			do {
				oslWaitVSync();
				oslVblankNextFrame();
			} while (osl_vblCallCount + 1 > osl_vblCount);
		}
		osl_vblCallCount = osl_vblCount;
		osl_vblankCounterActive = 1;  // Re-enable vblank counting

		osl_skip = 0;
		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		oslMeanBenchmarkTestEx(OSL_BENCH_START, 4);
		oslMeanBenchmarkTestEx(OSL_BENCH_START, 6);
		#endif

		if (!(vsync & 16)) {
			oslSwapBuffers();  // Swap buffers if vsync does not disable it
		}
	} else {  // Frameskip is active
		osl_vblCallCount++;

		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		if (osl_skip) {
			oslMeanBenchmarkTestEx(OSL_BENCH_END, 5);
		} else {
			oslMeanBenchmarkTestEx(OSL_BENCH_END, 4);
		}
		#endif

		// Calculate reference for when vsync == 0
		i = ((vsync & 1) && !osl_skip && !(vsync & 8)) ? 1 : 0;

		// Check if we are lagging behind
		if ((osl_vblCount + i > osl_vblCallCount + frameskip - 1 || (vsync & 4 && osl_vblCallCount % frameskip)) &&
			osl_nbSkippedFrames < max_frameskip - 1) {

			// If we haven't skipped a frame yet, display it
			if (!osl_skip) {
				#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
				oslMeanBenchmarkTestEx(OSL_BENCH_END, 6);
				#endif

				if (vsync & 1) {  // Wait for VSync if enabled
					oslWaitVSync();
				}

				if (!(vsync & 16)) {
					oslSwapBuffers();
				}

				#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
				oslMeanBenchmarkTestEx(OSL_BENCH_START, 6);
				#endif
			}
			osl_nbSkippedFrames++;

			// Skip calculation based on frameskip value
			if (frameskip > 1 && osl_vblCallCount % frameskip == frameskip - 1) {
				osl_skip = 0;
			} else {
				osl_skip = 1;
			}
		} else {
			#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
			if (!osl_skip) {
				oslMeanBenchmarkTestEx(OSL_BENCH_END, 6);
			}
			#endif

			if (vsync & 1 && !osl_skip) {  // Wait for VSync if enabled
				oslWaitVSync();
			}

			if (!(vsync & 4)) {
				osl_vblCallCount += frameskip - 1;  // Skip frames properly
			}

			// Handle VBlank synchronization
			osl_vblankCounterActive = 0;
			while (osl_vblCount < osl_vblCallCount + ((vsync & 8) ? (1 - osl_skip) : 0)) {
				oslWaitVSync();
				oslVblankNextFrame();
			}
			osl_vblankCounterActive = 1;

			if (!osl_skip) {
				if (!(vsync & 16)) {
					oslSwapBuffers();
				}
			}

			#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
			if (!osl_skip) {
				oslMeanBenchmarkTestEx(OSL_BENCH_START, 6);
			}
			#endif

			osl_vblCallCount = osl_vblCount;  // Sync the call count with vblank count
			osl_skip = 0;

			// Adjust for fixed frameskip
			if (vsync & 4 && frameskip > 1 && osl_vblCallCount % frameskip == 0) {
				osl_skip = 1;
			}
		}

		#ifdef OSL_SYSTEM_BENCHMARK_ENABLED
		if (osl_skip) {
			oslMeanBenchmarkTestEx(OSL_BENCH_START, 5);
		} else {
			oslMeanBenchmarkTestEx(OSL_BENCH_START, 4);
		}
		#endif
	}

	if (!osl_skip) {
		osl_nbSkippedFrames = 0;  // Reset the number of skipped frames if no skipping occurs
	}

	if (wasDrawing) {
		oslStartDrawing();  // Restart drawing if it was previously in progress
	}

	return osl_skip;
}

void oslEndFrame() {
    oslAudioVSync();
}
