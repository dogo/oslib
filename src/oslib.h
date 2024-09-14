/**
 * @file oslib.h
 * @brief Main Oldschool Library header file.
 *
 * This header defines the main structures, macros, and functions used in the
 * OldSchool Library (OSLib) framework. OSLib provides a comprehensive set of tools
 * for developing applications on the PSP platform, including graphics, input handling,
 * memory management, and more. This file includes both general utility functions
 * and platform-specific implementations for the PSP.
 */

#ifndef _OSLIB_H_
#define _OSLIB_H_

#ifdef PSP
    #include <pspkernel.h>
    #include <pspdisplay.h>
    #include <pspdebug.h>
#else
    #define _CRT_SECURE_NO_DEPRECATE
    #include "emu.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef PSP
    #include <pspctrl.h>
    #include <psputility.h>
    #include <pspgu.h>
    #include <pspgum.h>
    #include <psppower.h>
    #include <pspiofilemgr.h>
    #include <psphprm.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup main Main
 * @brief Main OSLib section.
 *
 * This section contains the core functionalities of OSLib, including initialization,
 * termination, and synchronization routines. It also includes various utility functions
 * and definitions that are commonly used throughout the library.
 * @{
 */

/**
 * @defgroup main_general General
 * @brief General routines to initialize and terminate the library.
 *
 * These functions are responsible for setting up and tearing down the OSLib environment.
 * Initialization typically involves configuring system settings and allocating necessary
 * resources, while termination handles cleanup and resource deallocation.
 * @{
 */

/** @brief OSLib version. */
#define OSL_VERSION "1.3.0"

/** @brief Initializes the library.
 *
 * This function initializes the OSLib environment. The flags parameter is used
 * to control specific initialization options, which are primarily intended for
 * advanced users.
 *
 * @param flags Can be formed of one or more of the values from the
 * \ref OSL_INITFLAGS enumeration (ORed together).
 */
extern void oslInit(int flags);

/** @brief Flags for oslInit function.
 *
 * These flags control specific aspects of the initialization process.
 */
enum OSL_INITFLAGS {
	OSL_IF_USEOWNCALLBACKS = 1, /*!< Do not set up the standard callbacks. You'll have to provide them by yourself as shown in almost all PSPSDK samples. */
	OSL_IF_NOVBLANKIRQ = 2 /*!< Do not reserve the VBLANK IRQ. You should set up your own VBL handler, which should call oslVblankNextFrame() if osl_vblankCounterActive is TRUE, else oslSyncFrame(Ex) will not work properly. */
};

/** @brief Exits the application immediately.
 *
 * This function terminates the application and returns to the XMB (PSP main menu) or
 * to the calling application (e.g., IRShell).
 */
extern void oslQuit();

/** @brief Sets the "no fail" mode.
 *
 * In this mode, if OSLib cannot load a file (for example, an image), it will display a
 * message indicating the name of the missing file and then quit immediately.
 *
 * @param enabled Enables or disables the no-fail feature.
 */
#define oslSetQuitOnLoadFailure(enabled) (osl_noFail = (enabled))

/** @brief Sets the function to call in case of failure to load something.
 *
 * The default behavior is to display a message box. This function allows you to
 * override that behavior with your custom function.
 *
 * @param function Function pointer to be called on load failure.
 */
#define oslSetQuitOnLoadFailureCallback(function) (osl_noFailCallback = function)

/** @brief Sets an exit callback.
 *
 * This function is executed when the user chooses "Exit" from the HOME menu.
 * By default, this callback sets osl_quit to true, indicating that the application
 * should quit. You can provide your custom callback function to handle the exit process.
 *
 * @param function Function pointer to be executed as the exit callback.
 */
#define oslSetExitCallback(function) (osl_exitCallback = function)

/**
 * @brief Indicates if the internal initialization process of OSLib is complete.
 *
 * This variable is used internally by OSLib to track the state of the library's initialization.
 * It is set to 1 when the initialization is complete and 0 otherwise.
 */
extern int osl_intraInit;

/**
 * @brief Controls the "no fail" mode in OSLib.
 *
 * This variable is used to enable or disable the "no fail" mode, which automatically
 * quits the application with an error message if a file cannot be loaded.
 *
 * - 0: "No fail" mode is disabled.
 * - 1: "No fail" mode is enabled.
 */
extern int osl_noFail;

/**
 * @brief Pointer to the callback function used when a file load operation fails in "no fail" mode.
 *
 * This function is called whenever a file fails to load while "no fail" mode is enabled.
 * The callback function receives the name of the file that failed to load and a reserved
 * parameter that can be used for additional context.
 *
 * @param filename The name of the file that failed to load.
 * @param reserved Reserved parameter for additional context (usually not used).
 */
extern void (*osl_noFailCallback)(const char *filename, u32 reserved);

/**
 * @brief Handles errors when a file fails to load in "no fail" mode.
 *
 * This function is called internally by OSLib when a file fails to load and "no fail" mode
 * is enabled. It invokes the callback function specified by `osl_noFailCallback` if one is set,
 * or displays a default error message and quits the application.
 *
 * @param filename The name of the file that failed to load.
 */
extern void oslHandleLoadNoFailError(const char *filename);

/** @} */ // end of main_general

/**
 * @defgroup main_sync Synchronization
 * @brief Synchronization routines for your game.
 *
 * These functions are used to synchronize the game's frame rate and ensure smooth
 * rendering and input handling. They include functions for managing frame skipping
 * and VSync.
 * @{
 */

/** @brief Sets the current fixed frameskipping value.
 *
 * @param val 1 enables auto frameskip, 2 or more enables fixed skipping
 * (val-1 frames out of val are skipped).
 */
#define oslSetFrameskip(val) (osl_frameskip = val)

/** @brief Sets the maximum frameskip value.
 *
 * @param val Maximum frameskip value.
 */
#define oslSetMaxFrameskip(val) (osl_maxFrameskip = val)

/** @brief Sets the VSync parameter.
 *
 * @param val VSync parameter value.
 */
#define oslSetVSync(val) (osl_vsyncEnabled = val)

/** @brief Synchronization function to be called at the end of a frame.
 *
 * This function synchronizes the frame rate based on the current frameskip and VSync settings.
 *
 * @return A boolean value indicating if the game is running late.
 */
#define oslSyncFrame() oslSyncFrameEx(osl_frameskip, osl_maxFrameskip, osl_vsyncEnabled)

/** @brief Call this function when a frame has ended.
 *
 * This function should be called just before calling oslSyncFrame or oslWaitVSync.
 * It handles audio synchronization and other end-of-frame tasks.
 */
extern void oslEndFrame();

/**
 * @brief Indicates whether the application should exit.
 *
 * This variable is set to 1 when the user chooses "Exit" from the HOME menu,
 * signaling that the application should clean up resources and quit.
 * Game loops should monitor this variable and exit gracefully when it is set to 1.
 */
extern int osl_quit;

/**
 * @brief Controls whether standby mode is permitted.
 *
 * This variable can be set to 1 to prevent the PSP from entering standby mode.
 * If set to 0, standby mode is allowed.
 */
extern int osl_standByUnpermitted;

/**
 * @brief Pointer to the power callback function.
 *
 * This function is called when a power event occurs, such as low battery,
 * power switch action, or other power-related events. The callback receives
 * three parameters: two integers and a void pointer for user-defined data.
 *
 * @param arg1 First argument, typically indicating the type of power event.
 * @param arg2 Second argument, providing additional context or flags related to the event.
 * @param common A pointer to user-defined data, typically passed when setting up the callback.
 *
 * @return Typically returns 0, but the return value may be used to influence power event handling.
 */
extern int (*osl_powerCallback)(int arg1, int arg2, void* common);

/**
 * @brief Pointer to the exit callback function.
 *
 * This function is executed when the user chooses "Exit" from the HOME menu. By default,
 * it sets `osl_quit` to 1. You can override this function to handle exit events in a custom way.
 *
 * @param arg1 First argument, usually indicating the reason for the exit request.
 * @param arg2 Second argument, providing additional context or flags related to the exit request.
 * @param common A pointer to user-defined data, typically passed when setting up the callback.
 *
 * @return Typically returns 0, but the return value may be used to influence exit event handling.
 */
extern int (*osl_exitCallback)(int arg1, int arg2, void* common);

/**
 * @brief The interrupt number for the VBLANK interrupt.
 *
 * This variable stores the number of the VBLANK interrupt used by OSLib.
 * It is generally set during initialization and used to manage frame synchronization.
 */
extern int osl_vblInterruptNumber;

/**
 * @brief Maximum allowed frameskip value.
 *
 * This variable determines the maximum number of frames that can be skipped
 * to maintain a stable frame rate. A higher value may result in a less smooth
 * visual experience but can help the game maintain its target frame rate.
 */
extern int osl_maxFrameskip;

/**
 * @brief Indicates whether VSync is enabled.
 *
 * This variable controls the VSync setting for the application. When VSync is enabled,
 * the rendering process is synchronized with the screen refresh rate to prevent screen tearing.
 */
extern int osl_vsyncEnabled;

/**
 * @brief Current frameskip setting.
 *
 * This variable determines how many frames should be skipped between rendering
 * frames. A value of 1 disables frameskipping, while higher values increase the number
 * of frames skipped, potentially improving performance at the cost of smoothness.
 */
extern int osl_frameskip;

/**
 * @brief Count of VBLANK interrupts.
 *
 * This volatile variable tracks the number of VBLANK interrupts that have occurred.
 * It is used internally to synchronize rendering with the display's refresh rate.
 */
extern volatile int osl_vblCount;

/**
 * @brief Count of VBLANK interrupt calls.
 *
 * This volatile variable tracks the number of times the VBLANK interrupt has been handled.
 * It is used to ensure that the application correctly responds to each interrupt.
 */
extern volatile int osl_vblCallCount;

/**
 * @brief Indicates if the VBLANK counter is active.
 *
 * This volatile variable is set to 1 when the VBLANK counter is actively tracking
 * VBLANK interrupts. It is used internally by OSLib for frame synchronization.
 */
extern volatile int osl_vblankCounterActive;

/**
 * @brief Indicates whether the current frame should be skipped.
 *
 * This volatile variable is used internally to determine if rendering for the current
 * frame should be skipped based on the frameskip settings and current performance.
 */
extern volatile int osl_skip;

/**
 * @brief Current framerate setting.
 *
 * This volatile variable stores the current target framerate, typically set to 60 for a
 * standard PSP display. It can be modified to simulate different framerates, such as 50 for PAL games.
 */
extern volatile int osl_currentFrameRate;

/** @brief Synchronization function with explicit parameters.
 *
 * This function is similar to oslSyncFrame but allows you to pass all synchronization
 * parameters directly in one call.
 *
 * @param frameskip Frameskip value.
 * @param max_frameskip Maximum frameskip value.
 * @param vsync VSync parameter.
 *
 * @return A boolean value indicating if the game is running late.
 */
extern int oslSyncFrameEx(int frameskip, int max_frameskip, int vsync);

/** @brief Sets the framerate for oslSyncFrame(Ex).
 *
 * This can be any value from 1 to 60. For example, use 50 to simulate a PAL (European) game.
 *
 * @param framerate Desired framerate.
 */
static inline void oslSetFramerate(int framerate) {
	if (framerate <= 60)
		osl_currentFrameRate = framerate;
}

/** @brief Waits for the VSync, synchronizing your game.
 *
 * This function is a simpler and less efficient method than oslSyncFrame.
 */
extern void oslWaitVSync();

/** @} */ // end of main_sync

/**
 * @defgroup main_memory Memory
 * @brief Memory management routines.
 *
 * This section includes functions and macros for managing memory in OSLib, including
 * cache management, memory copying, and alignment.
 * @{
 */

/** @brief Copies data using the internal DMAC.
 *
 * This function is generally faster than memcpy but requires that the data to be copied
 * is no longer in the cache.
 *
 * @param dest Destination address.
 * @param source Source address.
 * @param size Size of the data to be copied.
 *
 * @return Result of the DMA copy operation.
 */
extern int sceDmacMemcpy(void *dest, const void *source, unsigned int size);

/** @brief Same as sceDmacMemcpy.
 *
 * This function is identical to sceDmacMemcpy but may be less reliable.
 *
 * @param dest Destination address.
 * @param source Source address.
 * @param size Size of the data to be copied.
 *
 * @return Result of the DMA copy operation.
 */
extern int sceDmacTryMemcpy(void *dest, const void *source, unsigned int size);

/** @brief Does a memset using the 64-bit capabilities of the CPU.
 *
 * This function is faster than memcpy for certain operations.
 *
 * @param dst Destination address.
 * @param src Source address.
 * @param length Length of the data to be set.
 */
extern void oslFasterMemset(u64 *dst, u64 *src, u32 length);

/** @brief Ensures that the data is no longer in the CPU cache.
 *
 * This function flushes the cache, writing the data to the actual memory.
 *
 * @param data Address of the memory area to flush.
 * @param size Size (in bytes) of the area.
 */
#define oslUncacheData(data, size) sceKernelDcacheWritebackInvalidateRange(data, size);

/** @brief Returns a pointer to an uncached address.
 *
 * This function allows you to bypass the cache, but it is generally less efficient
 * than using cached addresses.
 *
 * @param adr Cached address.
 *
 * @return Uncached pointer.
 */
#define oslGetUncachedPtr(adr) ((void*)((int)(adr) | 0x40000000))

/** @brief Returns a pointer to cached data.
 *
 * This function allows you to revert to using cached addresses.
 *
 * @param adr Uncached address.
 *
 * @return Cached pointer.
 */
#define oslGetCachedPtr(adr) ((void*)((int)(adr) & (~0x40000000)))

#ifdef PSP
/** @brief Flushes the whole cache.
 *
 * This function is slow and should be avoided if possible.
 * Use oslUncacheData instead when appropriate.
 */
extern void oslFlushDataCache();
#else
/** @brief Flushes the whole cache.
 *
 * This function is slow and should be avoided if possible.
 * Use oslUncacheData instead when appropriate.
 */
extern void oslFlushDataCache();
#endif

/** @brief Allocates a memory block, ensuring it is aligned.
 *
 * This function allocates memory with a specified alignment.
 *
 * @param alignment Alignment in bytes.
 * @param size Size of the block.
 *
 * @return Pointer to the allocated memory block.
 */
void *memalign(size_t alignment, size_t size);

/** @brief Structure for the return value of oslGetRamStatus.
 *
 * This structure contains information about the available memory.
 */
typedef struct {
	int maxAvailable; /*!< Maximum available memory */
	int maxBlockSize; /*!< Maximum linear memory available */
} OSL_MEMSTATUS;

/** @brief Gets info about currently available memory in main RAM.
 *
 * This function returns an OSL_MEMSTATUS structure with information about
 * the available memory.
 *
 * @return OSL_MEMSTATUS structure containing available memory info.
 */
OSL_MEMSTATUS oslGetRamStatus();

/** @} */ // end of main_memory

/**
 * @defgroup main_maths Maths
 * @brief Mathematical functions.
 *
 * This section provides mathematical utilities, such as trigonometric functions,
 * commonly used in game development.
 * @{
 */

/** @brief Returns the absolute value of a number.
 *
 * This function returns the positive part of the number.
 *
 * @param x Input value.
 *
 * @return Absolute value of the input.
 */
#define oslAbs(x) (((x) < 0) ? (-(x)) : (x))

/** @brief Returns the smallest value between the two.
 *
 * This function returns the smaller of the two provided values.
 *
 * @param x First value.
 * @param y Second value.
 *
 * @return Smallest value.
 */
#define oslMin(x, y) (((x) < (y)) ? (x) : (y))

/** @brief Returns the greatest value between the two.
 *
 * This function returns the larger of the two provided values.
 *
 * @param x First value.
 * @param y Second value.
 *
 * @return Greatest value.
 */
#define oslMax(x, y) (((x) > (y)) ? (x) : (y))

/** @brief Returns a value clamped between a minimum and maximum.
 *
 * This function ensures that the provided value is within the specified range.
 *
 * @param x Input value.
 * @param min Minimum value.
 * @param max Maximum value.
 *
 * @return Clamped value.
 */
#define oslMinMax(x, min, max) ((x) < (max) ? ((x) > (min) ? (x) : (min)) : (max))

/** @brief Returns the number of objects in an array.
 *
 * This function calculates the number of elements in an array.
 *
 * @param n Array.
 *
 * @return Number of elements in the array.
 */
#define oslNumberof(n) (sizeof(n) / sizeof(*(n)))

/** @brief Calculates the sine of an angle in degrees multiplied by a radius.
 *
 * This function returns the sine of the specified angle, multiplied by the provided radius.
 *
 * @param angle Angle in degrees.
 * @param dist Radius of the circle.
 *
 * @return Sine of the angle multiplied by the radius.
 */
extern float oslSin(float angle, float dist);

/** @brief Calculates the cosine of an angle in degrees multiplied by a radius.
 *
 * This function returns the cosine of the specified angle, multiplied by the provided radius.
 *
 * @param angle Angle in degrees.
 * @param dist Radius of the circle.
 *
 * @return Cosine of the angle multiplied by the radius.
 */
extern float oslCos(float angle, float dist);

/** @brief Returns the sine of an angle in degrees (0 to 360) multiplied by an integer radius.
 *
 * This function uses a lookup table for faster computation.
 *
 * @param angle Angle in degrees.
 * @param dist Radius of the circle.
 *
 * @return Sine of the angle multiplied by the radius.
 */
extern int oslSini(int angle, int dist);

/** @brief Returns the cosine of an angle in degrees (0 to 360) multiplied by an integer radius.
 *
 * This function uses a lookup table for faster computation.
 *
 * @param angle Angle in degrees.
 * @param dist Radius of the circle.
 *
 * @return Cosine of the angle multiplied by the radius.
 */
extern int oslCosi(int angle, int dist);

/** @brief Returns the next (upper) power of two of a number.
 *
 * This function calculates the next power of two that is greater than or equal to the input value.
 *
 * @param val Input value.
 *
 * @return Next power of two.
 */
extern int oslGetNextPower2(int val);

/**
 * @brief Precomputed sine values for angles from 0 to 360 degrees.
 *
 * This array contains the precomputed sine values for each degree from 0 to 360.
 * The values are scaled to a fixed-point format for faster computation in integer-based
 * operations. This array is used by functions like `oslSini` for efficient trigonometric
 * calculations without needing to compute sine values at runtime.
 *
 * @note The array index corresponds directly to the angle in degrees.
 */
extern int osl_isinus[361];

/**
 * @brief Precomputed cosine values for angles from 0 to 360 degrees.
 *
 * This array contains the precomputed cosine values for each degree from 0 to 360.
 * Like `osl_isinus`, the values are scaled to a fixed-point format for efficient integer-based
 * operations. This array is used by functions like `oslCosi` to quickly obtain cosine values
 * without runtime computation.
 *
 * @note The array index corresponds directly to the angle in degrees.
 */
extern int osl_icosinus[361];

/**
 * @brief Sets up the floating-point trigonometric tables.
 *
 * This function initializes internal trigonometric tables that are used for floating-point
 * trigonometric calculations. It must be called during the setup process of OSLib before
 * any floating-point trigonometric functions are used.
 *
 * @note This setup is necessary for functions like `oslSin` and `oslCos` to work properly.
 */
extern void oslSetupFTrigo();

/** @} */ // end of main_maths

/**
 * @defgroup main_debug Debug console
 * @brief Debugging features.
 *
 * This section includes functions and macros for debugging, such as printing
 * formatted text to the screen and controlling the debug console.
 * @{
 */
#ifdef PSP
/** @brief Prints formatted text to the current position of the cursor.
 *
 * This macro uses a format string similar to printf.
 *
 * @param format Formatted text.
 */
    #define oslPrintf(format ...) ({ char __str[1000]; sprintf(__str, ## format); oslConsolePrint(__str); })

/** @brief Prints formatted text at a specific position on the screen.
 *
 * This macro prints formatted text at the specified screen coordinates.
 *
 * @param x X-coordinate (in characters).
 * @param y Y-coordinate (in characters).
 * @param str Formatted string.
 * @param format Additional formatted text.
 */
    #define oslPrintf_xy(x, y, str, format ...) ({ osl_consolePosX = x; osl_consolePosY = y; oslPrintf(str, ## format); })
#else
    #define oslPrintf(...) { char __str[1000]; sprintf(__str, __VA_ARGS__); oslConsolePrint(__str); }
    #define oslPrintf_xy(x, y, ...) { osl_consolePosX = x; osl_consolePosY = y; oslPrintf(__VA_ARGS__); }
#endif

/** @brief Clears the screen (to black) and resets the cursor to the top-left corner of the screen.
 *
 * This macro clears the screen and moves the cursor to the origin.
 */
#define oslCls() (oslClearScreen(0), oslMoveTo(0, 0))

/** @brief Moves the cursor to a place on the screen.
 *
 * This macro moves the cursor to the specified screen coordinates, using character units.
 *
 * @param x X-coordinate (in characters).
 * @param y Y-coordinate (in characters).
 */
#define oslMoveTo(x, y) (osl_consolePosX = x * osl_sceFont->charWidths['0'], osl_consolePosY = y * osl_sceFont->charHeight)

/** @} */ // end of main_debug

/**
 * @defgroup main_misc Miscellaneous
 * @brief Miscellaneous utilities.
 *
 * This section includes various utility functions and definitions that do not fall
 * under other categories.
 * @{
 */

/** @brief Does a benchmark action.
 *
 * This function performs a benchmarking action, such as starting or stopping a benchmark,
 * or retrieving the elapsed time.
 *
 * @param startend Benchmark action type (e.g., OSL_BENCH_START, OSL_BENCH_END).
 * @param slot Benchmark slot (0-3 for user, 4-7 for system).
 *
 * @return Time elapsed between start and end in microseconds.
 */
extern int oslBenchmarkTestEx(int startend, int slot);

/** @brief Same as oslBenchmarkTestEx but does a mean of 20 samples before returning a value.
 *
 * This function performs a benchmarking action and returns the mean of 20 samples.
 *
 * @param startend Benchmark action type.
 * @param slot Benchmark slot.
 *
 * @return Mean benchmark value.
 */
extern int oslMeanBenchmarkTestEx(int startend, int slot);

/** @brief Does a benchmark in slot 0.
 *
 * This function performs a benchmarking action in slot 0.
 *
 * @param startend Benchmark action type.
 *
 * @return Benchmark result for slot 0.
 */
static inline int oslBenchmarkTest(int startend) {
	return oslBenchmarkTestEx(startend, 0);
}

/** @brief Displays the system benchmark results on the top-left corner of the screen.
 *
 * This function displays the benchmark results, which can be useful for debugging performance issues.
 */
extern void oslSysBenchmarkDisplay();

/**
 * @brief Enumeration for benchmark actions in OSLib.
 *
 * This enumeration defines the various actions that can be performed during a benchmark test
 * in OSLib. These actions are used with functions like `oslBenchmarkTestEx` to control the
 * start, end, and retrieval of benchmark data.
 */
enum {
	OSL_BENCH_INIT = 0, /*!< Initializes the benchmark. This is typically done at the beginning of the benchmarking process. */
	OSL_BENCH_START,   /*!< Starts the benchmark timer. This marks the beginning of the period you want to measure. */
	OSL_BENCH_END,     /*!< Ends the benchmark timer. This marks the end of the period you want to measure. */
	OSL_BENCH_GET,     /*!< Retrieves the time elapsed between the start and end of the benchmark in microseconds. */
	OSL_BENCH_GET_LAST, /*!< Retrieves the last benchmark result without starting or ending a new benchmark. */
	OSL_BENCH_DISPLAY  /*!< Displays the benchmark results. This action is primarily used for debugging and performance analysis. */
};

/**
 * @brief Number of available benchmark slots.
 *
 * This macro defines the number of benchmark slots that can be used concurrently.
 * Each slot can hold independent benchmark data, allowing multiple benchmarks
 * to be performed and compared simultaneously.
 */
#define OSL_BENCH_SLOTS 8

/**
 * @brief Number of samples used for mean benchmarking.
 *
 * This macro defines the number of samples to be averaged when performing
 * a mean benchmark. This helps in obtaining a more accurate and stable
 * benchmark result by averaging multiple measurements.
 */
#define OSL_BENCH_SAMPLES 20

#ifndef PSP
/**
 * @brief Debugging function for non-PSP platforms.
 *
 * This function is used to print formatted debug messages to the console or other
 * logging facilities on platforms other than the PSP. It behaves similarly to `printf`,
 * allowing you to format and output debug information.
 *
 * @param format A format string that specifies how subsequent arguments are converted
 *               for output, following the standard C printf conventions.
 * @param ... Additional arguments to be formatted and output according to the format string.
 */
extern void Debug(const char *format, ...);
#endif

/**
 * @brief Handles the VBLANK interrupt.
 *
 * This function is called during the VBLANK interrupt to perform tasks that need to be
 * synchronized with the vertical blanking period of the display. The function can be
 * used to update frame counters, synchronize rendering, or perform other time-sensitive
 * operations that should occur during the VBLANK period.
 *
 * @param sub A sub-parameter used to provide additional context or configuration for the interrupt handling.
 * @param parg A pointer to user-defined data or structure that may be used within the interrupt handler.
 */
extern void oslVblankInterrupt(int sub, void *parg);

/**
 * @brief Displays the first predefined splash screen.
 *
 * This function shows the first splash screen provided by the OldSchool Library, typically
 * the OSLib logo. It can be used at the start of your application as an introduction.
 *
 * @return Returns 1 on success, 0 if the splash screen could not be displayed.
 */
extern int oslShowSplashScreen1();

/**
 * @brief Displays the second predefined splash screen.
 *
 * This function shows the second splash screen provided by the OldSchool Library, typically
 * associated with the Neoflash logo. It can be used at the start of your application as an introduction.
 *
 * @return Returns 1 on success, 0 if the splash screen could not be displayed.
 */
extern int oslShowSplashScreen2();

/** @brief Shows one of the predefined splash screens in OldSchool Library.
 *
 * This function displays a predefined splash screen, such as the OSLib or Neoflash logo.
 *
 * @param splashType 1 shows the OSLib logo, 2 shows the Neoflash logo.
 *
 * @return 1 in case of success, 0 otherwise.
 */
static inline int oslShowSplashScreen(int splashType) {
	if (splashType == 1)
		return oslShowSplashScreen1();
	else if (splashType == 2)
		return oslShowSplashScreen2();
	return 0;
}

/** @brief Shows the Neoflash logo splash screen.
 *
 * This function displays the Neoflash logo splash screen.
 *
 * @return 1 in case of success, 0 otherwise.
 */
static inline int oslShowNeoflashLogo() {
	return oslShowSplashScreen(2);
}

#ifdef PSP
/**
 * @brief Base address of the UVRAM (Uncached Video RAM) on the PSP.
 *
 * This macro defines the base address of the uncached video RAM on the PSP.
 * This memory region is used for operations that require direct access to video
 * memory without the benefits of CPU caching. It is typically used for graphics
 * operations where cache coherence with the GPU is critical.
 */
#define OSL_UVRAM_BASE ((u8*)0x04000000)
#else
/**
 * @brief Pointer to the base address of the UVRAM (Uncached Video RAM) on non-PSP platforms.
 *
 * On non-PSP platforms, this variable represents the base address of the uncached video RAM.
 * This is useful for emulation or testing environments where direct memory access to video
 * RAM is simulated.
 */
extern u8 *OSL_UVRAM_BASE;

/**
 * @brief Macro for stack memory allocation on non-PSP platforms.
 *
 * This macro is used to allocate memory on the stack with a specified size. On non-PSP platforms,
 * it is typically mapped to `_alloca`, which performs stack allocation.
 *
 * @param size The size of the memory block to allocate.
 */
#define alloca _alloca
#endif

/**
 * @brief Size of the UVRAM (Uncached Video RAM) in bytes.
 *
 * This macro defines the size of the uncached video RAM region. The value represents
 * 2 megabytes (2 << 20), which is typically the size allocated for video memory operations
 * that require uncached access on the PSP.
 */
#define OSL_UVRAM_SIZE (2 << 20)

/**
 * @brief End address of the UVRAM (Uncached Video RAM) region.
 *
 * This macro calculates the end address of the uncached video RAM region by adding the size
 * of the UVRAM to its base address. It is useful for determining the bounds of the video
 * memory area when performing operations that involve the entire UVRAM.
 */
#define OSL_UVRAM_END ((u8*)((u32)OSL_UVRAM_BASE + OSL_UVRAM_SIZE))

/** @} */ // end of main_misc

/** @} */ // end of main

#include "oslmath.h"
#include "vram_mgr.h"
#include "VirtualFile.h"
#include "drawing.h"
#include "map.h"
#include "text.h"
#include "messagebox.h"
#include "keys.h"
#include "audio.h"
#include "bgm.h"
#include "usb.h"
#include "dialog.h"
#include "osk.h"
#include "saveload.h"
#include "net.h"
#include "browser.h"
#include "adhoc/pspadhoc.h"
#include "ccc.h"
#include "sfont.h"

#ifdef __cplusplus
}
#endif

#endif /* _OSLIB_H_ */
