/**
 * @file emu.h
 * @brief Header file containing the emulation initialization, graphics setup, and various utility functions.
 */

#ifndef EMU_H
#define EMU_H

#define _CRT_SECURE_NO_DEPRECATE

#include <math.h>
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "gl/glext.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The title of the emulator window.
 */
extern const char *emuWindowTitle;

/**
 * @brief The debug mode flag for the emulator.
 * @details If set to 1, the emulator is in debug mode; otherwise, it's in release mode.
 */
extern const int emuDebugMode;

#ifdef _DEBUG
    /**
     * @brief Macro to set the emulator window title and enable debug mode.
     */
    #define PSP_MODULE_INFO(text, zero, one, one2) const char *emuWindowTitle = text; const int emuDebugMode = 1;
#else
    /**
     * @brief Macro to set the emulator window title and disable debug mode.
     */
    #define PSP_MODULE_INFO(text, zero, one, one2) const char *emuWindowTitle = text; const int emuDebugMode = 0;
#endif

/**
 * @brief Initializes the emulator.
 */
extern void emuInit();

/**
 * @brief Starts the drawing process for the emulator.
 */
extern void emuStartDrawing();

/**
 * @brief Initializes the graphics subsystem for the emulator.
 */
extern void emuInitGfx();

/**
 * @brief A macro defining the main thread entry point for the emulator.
 */
#define PSP_MAIN_THREAD_ATTR(value) int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) { emuInit(); }

/* OpenGL Constants */
#define GL_UNSIGNED_BYTE_3_3_2            0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_10_10_10_2        0x8036
#define GL_RESCALE_NORMAL                 0x803A
#define GL_UNSIGNED_BYTE_2_3_3_REV        0x8362
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV       0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV     0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367

typedef unsigned char u8;  /**< Unsigned 8-bit integer */
typedef unsigned short u16; /**< Unsigned 16-bit integer */
typedef unsigned long u32; /**< Unsigned 32-bit integer */
typedef signed char s8; /**< Signed 8-bit integer */
typedef signed short s16; /**< Signed 16-bit integer */
typedef signed long s32; /**< Signed 32-bit integer */
typedef signed __int64 s64; /**< Signed 64-bit integer */
typedef unsigned __int64 u64; /**< Unsigned 64-bit integer */

/**
 * @brief Pixel storage modes for graphics.
 */
enum {
    GU_PSM_5650 = 0, /**< 16-bit color with 5:6:5 format */
    GU_PSM_5551, /**< 16-bit color with 5:5:5:1 format */
    GU_PSM_4444, /**< 16-bit color with 4:4:4:4 format */
    GU_PSM_8888, /**< 32-bit color with 8:8:8:8 format */
    GU_PSM_T4, /**< 4-bit indexed color */
    GU_PSM_T8 /**< 8-bit indexed color */
};

/**
 * @brief Texture filtering modes.
 */
enum {
    GU_NEAREST = GL_NEAREST, /**< Nearest-neighbor filtering */
    GU_LINEAR = GL_LINEAR /**< Linear filtering */
};

/**
 * @brief Shading modes.
 */
enum {
    GU_FLAT = GL_FLAT, /**< Flat shading */
    GU_SMOOTH = GL_SMOOTH /**< Smooth shading */
};

/* State Definitions */
#define GU_ALPHA_TEST       GL_ALPHA_TEST
#define GU_DEPTH_TEST       GL_DEPTH_TEST
#define GU_SCISSOR_TEST     GL_SCISSOR_TEST
#define GU_STENCIL_TEST     GL_STENCIL_TEST
#define GU_BLEND            GL_BLEND
#define GU_CULL_FACE        GL_CULL_FACE
#define GU_DITHER           GL_DITHER
#define GU_FOG              GL_FOG
#define GU_CLIP_PLANES      GL_CLIP_PLANES
#define GU_TEXTURE_2D       GL_TEXTURE_2D
#define GU_LIGHTING         GL_LIGHTING
#define GU_LIGHT0           GL_LIGHT0
#define GU_LIGHT1           GL_LIGHT1
#define GU_LIGHT2           GL_LIGHT2
#define GU_LIGHT3           GL_LIGHT3
#define GU_UNKNOWN_15       (-15)
#define GU_UNKNOWN_16       (-16)
#define GU_COLOR_TEST       (-17)
#define GU_COLOR_LOGIC_OP   (-18)
#define GU_FACE_NORMAL_REVERSE  (-19)
#define GU_PATCH_FACE       (-20)
#define GU_FRAGMENT_2X      (-21)

/* Primitive Types */
#define GU_POINTS           (-1)
#define GU_LINES            GL_LINES
#define GU_LINE_STRIP       GL_LINE_STRIP
#define GU_TRIANGLES        GL_TRIANGLES
#define GU_TRIANGLE_STRIP   GL_TRIANGLE_STRIP
#define GU_TRIANGLE_FAN     GL_TRIANGLE_FAN
#define GU_SPRITES          (-6)

/* Test Functions */
#define GU_NEVER        GL_NEVER
#define GU_ALWAYS       GL_ALWAYS
#define GU_EQUAL        GL_EQUAL
#define GU_NOTEQUAL     GL_NOTEQUAL
#define GU_LESS         GL_LESS
#define GU_LEQUAL       GL_LEQUAL
#define GU_GREATER      GL_GREATER
#define GU_GEQUAL       GL_GEQUAL

/* Clear Buffer Mask */
#define GU_COLOR_BUFFER_BIT GL_COLOR_BUFFER_BIT
#define GU_STENCIL_BUFFER_BIT GL_STENCIL_BUFFER_BIT
#define GU_DEPTH_BUFFER_BIT GL_DEPTH_BUFFER_BIT

/* Texture Effect */
#define GU_TFX_MODULATE (0)
#define GU_TFX_DECAL    (1)
#define GU_TFX_BLEND    (2)
#define GU_TFX_REPLACE  (3)
#define GU_TFX_ADD      (4)

/* Texture Color Component */
#define GU_TCC_RGB  (0)
#define GU_TCC_RGBA (1)

/* Blending Operations */
#define GU_ADD          GL_FUNC_ADD
#define GU_SUBTRACT     GL_FUNC_SUBTRACT
#define GU_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT
#define GU_MIN          GL_MIN
#define GU_MAX          GL_MAX
#define GU_ABS          (-5)

/* Blending Factors */
#define GU_SRC_COLOR            GL_SRC_COLOR
#define GU_ONE_MINUS_SRC_COLOR  GL_ONE_MINUS_SRC_COLOR
#define GU_SRC_ALPHA            GL_SRC_ALPHA
#define GU_ONE_MINUS_SRC_ALPHA  GL_ONE_MINUS_SRC_ALPHA
#define GU_DST_COLOR            GL_DST_COLOR
#define GU_ONE_MINUS_DST_COLOR  GL_ONE_MINUS_DST_COLOR
#define GU_DST_ALPHA            GL_DST_ALPHA
#define GU_ONE_MINUS_DST_ALPHA  GL_ONE_MINUS_DST_ALPHA
#define GU_FIX                  GL_CONSTANT_COLOR

/* Vertex Declarations */
#define GU_TEXTURE_SHIFT(n) ((n)<<0)
#define GU_TEXTURE_8BIT     GU_TEXTURE_SHIFT(1)
#define GU_TEXTURE_16BIT    GU_TEXTURE_SHIFT(2)
#define GU_TEXTURE_32BITF   GU_TEXTURE_SHIFT(3)
#define GU_TEXTURE_BITS     GU_TEXTURE_SHIFT(3)

#define GU_COLOR_SHIFT(n)   ((n)<<2)
#define GU_COLOR_RES1       GU_COLOR_SHIFT(1)
#define GU_COLOR_RES2       GU_COLOR_SHIFT(2)
#define GU_COLOR_RES3       GU_COLOR_SHIFT(3)
#define GU_COLOR_5650       GU_COLOR_SHIFT(4)
#define GU_COLOR_5551       GU_COLOR_SHIFT(5)
#define GU_COLOR_4444       GU_COLOR_SHIFT(6)
#define GU_COLOR_8888       GU_COLOR_SHIFT(7)
#define GU_COLOR_BITS       GU_COLOR_SHIFT(7)

#define GU_NORMAL_SHIFT(n)  ((n)<<5)
#define GU_NORMAL_8BIT      GU_NORMAL_SHIFT(1)
#define GU_NORMAL_16BIT     GU_NORMAL_SHIFT(2)
#define GU_NORMAL_32BITF    GU_NORMAL_SHIFT(3)
#define GU_NORMAL_BITS      GU_NORMAL_SHIFT(3)

#define GU_VERTEX_SHIFT(n)  ((n)<<7)
#define GU_VERTEX_8BIT      GU_VERTEX_SHIFT(1)
#define GU_VERTEX_16BIT     GU_VERTEX_SHIFT(2)
#define GU_VERTEX_32BITF    GU_VERTEX_SHIFT(3)
#define GU_VERTEX_BITS      GU_VERTEX_SHIFT(3)

#define GU_WEIGHT_SHIFT(n)  ((n)<<9)
#define GU_WEIGHT_8BIT      GU_WEIGHT_SHIFT(1)
#define GU_WEIGHT_16BIT     GU_WEIGHT_SHIFT(2)
#define GU_WEIGHT_32BITF    GU_WEIGHT_SHIFT(3)
#define GU_WEIGHT_BITS      GU_WEIGHT_SHIFT(3)

#define GU_INDEX_SHIFT(n)   ((n)<<11)
#define GU_INDEX_8BIT       GU_INDEX_SHIFT(1)
#define GU_INDEX_16BIT      GU_INDEX_SHIFT(2)
#define GU_INDEX_BITS       GU_INDEX_SHIFT(3)

#define GU_WEIGHTS(n)       ((((n)-1)&7)<<14)
#define GU_WEIGHTS_BITS     GU_WEIGHTS(8)
#define GU_VERTICES(n)      ((((n)-1)&7)<<18)
#define GU_VERTICES_BITS    GU_VERTICES(8)

#define GU_TRANSFORM_SHIFT(n)   ((n)<<23)
#define GU_TRANSFORM_3D     GU_TRANSFORM_SHIFT(0)
#define GU_TRANSFORM_2D     GU_TRANSFORM_SHIFT(1)
#define GU_TRANSFORM_BITS   GU_TRANSFORM_SHIFT(1)

/* Context Definitions */
#define GU_DIRECT       (0)
#define GU_CALL         (1)
#define GU_SEND         (2)

/**
 * @brief Type definition for SceSize.
 */
typedef int SceSize;

/**
 * @brief Type definition for SceUID.
 */
typedef long SceUID;

/**
 * @brief Type definition for SceMode.
 */
typedef unsigned long SceMode;

/**
 * @brief Permission values for the sceIoAssign function.
 */
enum IoAssignPerms
{
    IOASSIGN_RDWR = 0, /**< Assign the device read/write */
    IOASSIGN_RDONLY = 1 /**< Assign the device read only */
};

/* PSP I/O Flags */
#define PSP_O_RDONLY    0x0001
#define PSP_O_WRONLY    0x0002
#define PSP_O_RDWR      (PSP_O_RDONLY | PSP_O_WRONLY)
#define PSP_O_NBLOCK    0x0004
#define PSP_O_DIROPEN   0x0008
#define PSP_O_APPEND    0x0100
#define PSP_O_CREAT     0x0200
#define PSP_O_TRUNC     0x0400
#define PSP_O_EXCL      0x0800
#define PSP_O_NOWAIT    0x8000

/* PSP Seek Modes */
#define PSP_SEEK_SET    0
#define PSP_SEEK_CUR    1
#define PSP_SEEK_END    2

/* Texture Wrapping Modes */
#define GU_REPEAT       (0)
#define GU_CLAMP        (1)

/**
 * @brief Structure representing controller data.
 */
typedef struct SceCtrlData {
    unsigned int TimeStamp; /**< The current read frame. */
    unsigned int Buttons; /**< Bitmask containing one or more of ::PspCtrlButtons. */
    unsigned char Lx; /**< Analog stick, X axis. */
    unsigned char Ly; /**< Analog stick, Y axis. */
    unsigned char Rsrv[6]; /**< Reserved. */
} SceCtrlData;

/**
 * @brief PSP controller button definitions.
 */
enum PspCtrlButtons
{
    PSP_CTRL_SELECT     = 0x000001, /**< Select button. */
    PSP_CTRL_START      = 0x000008, /**< Start button. */
    PSP_CTRL_UP         = 0x000010, /**< Up D-Pad button. */
    PSP_CTRL_RIGHT      = 0x000020, /**< Right D-Pad button. */
    PSP_CTRL_DOWN       = 0x000040, /**< Down D-Pad button. */
    PSP_CTRL_LEFT       = 0x000080, /**< Left D-Pad button. */
    PSP_CTRL_LTRIGGER   = 0x000100, /**< Left trigger. */
    PSP_CTRL_RTRIGGER   = 0x000200, /**< Right trigger. */
    PSP_CTRL_TRIANGLE   = 0x001000, /**< Triangle button. */
    PSP_CTRL_CIRCLE     = 0x002000, /**< Circle button. */
    PSP_CTRL_CROSS      = 0x004000, /**< Cross button. */
    PSP_CTRL_SQUARE     = 0x008000, /**< Square button. */
    PSP_CTRL_HOME       = 0x010000, /**< Home button. */
    PSP_CTRL_HOLD       = 0x020000, /**< Hold button. */
    PSP_CTRL_NOTE       = 0x800000, /**< Music Note button. */
};

/**
 * @brief Type definition for a kernel thread entry function.
 */
typedef int (*SceKernelThreadEntry)(SceSize args, void *argp);

/**
 * @brief Type definition for an unsigned integer.
 */
typedef unsigned int SceUInt;

/**
 * @brief Structure representing kernel thread options.
 */
typedef struct SceKernelThreadOptParam {
    SceSize size; /**< Size of the structure. */
    SceUID stackMpid; /**< Stack memory pool ID. */
} SceKernelThreadOptParam;

/**
 * @brief Initializes OpenGL for the emulator.
 */
void emuInitGL();

/**
 * @brief Initializes the emulation environment.
 */
void emuInitEmulation();

/**
 * @brief Exits the emulator.
 */
void emuExit();

/**
 * @brief Computes the sine of an angle using the VFPU.
 * @param f1 The angle in degrees.
 * @param f2 Additional parameter for calculations.
 * @return The sine of the angle.
 */
float vfpu_sini(int f1, int f2);

/**
 * @brief Computes the cosine of an angle using the VFPU.
 * @param f1 The angle in degrees.
 * @param f2 Additional parameter for calculations.
 * @return The cosine of the angle.
 */
float vfpu_cosi(int f1, int f2);

/**
 * @brief Invalidates and writes back a range of data cache.
 * @param p Pointer to the data.
 * @param n Size of the data.
 */
void sceKernelDcacheWritebackInvalidateRange(void *p, int n);

/**
 * @brief Writes back a range of data cache.
 * @param p Pointer to the data.
 * @param n Size of the data.
 */
void sceKernelDcacheWritebackRange(void *p, int n);

/**
 * @brief Invalidates the entire data cache.
 */
void sceKernelDcacheWritebackInvalidateAll();

/**
 * @brief Draws an array of vertices.
 * @param prim The type of primitive to render.
 * @param vtype The type of vertices to draw.
 * @param count The number of vertices.
 * @param indices Pointer to the indices.
 * @param vertices Pointer to the vertices.
 */
void sceGuDrawArray(int prim, int vtype, int count, const void* indices, const void* vertices);

/**
 * @brief Enables a specified state.
 * @param state The state to enable.
 */
void sceGuEnable(int state);

/**
 * @brief Disables a specified state.
 * @param state The state to disable.
 */
void sceGuDisable(int state);

/**
 * @brief Sets the alpha test function.
 * @param func The test function.
 * @param value The reference value.
 * @param mask The mask value.
 */
void sceGuAlphaFunc(int func, int value, int mask);

/**
 * @brief Sets the ambient color.
 * @param color The color to set.
 */
void sceGuAmbientColor(unsigned int color);

/**
 * @brief Sets the blend function.
 * @param op The blend operation.
 * @param src The source factor.
 * @param dest The destination factor.
 * @param srcfix The source fixed color.
 * @param destfix The destination fixed color.
 */
void sceGuBlendFunc(int op, int src, int dest, unsigned int srcfix, unsigned int destfix);

/**
 * @brief Sets the texture function.
 * @param tfx The texture function.
 * @param tcc The texture color component.
 */
void sceGuTexFunc(int tfx, int tcc);

/**
 * @brief Sets the color function.
 * @param func The color function.
 * @param color The reference color.
 * @param mask The mask value.
 */
void sceGuColorFunc(int func, unsigned int color, unsigned int mask);

/**
 * @brief Allocates memory from the graphics context.
 * @param size The size of memory to allocate.
 * @return A pointer to the allocated memory.
 */
void* sceGuGetMemory(int size);

/**
 * @brief Specifies a two-dimensional texture image.
 * @param mipmap The level of detail number.
 * @param width The width of the texture.
 * @param height The height of the texture.
 * @param tbw The texture buffer width.
 * @param tbp Pointer to the texture data.
 */
void sceGuTexImage(int mipmap, int width, int height, int tbw, const void* tbp);

/**
 * @brief Sets the texture mode.
 * @param tpsm The texture pixel storage mode.
 * @param maxmips The maximum number of mipmaps.
 * @param a2 Unused parameter.
 * @param swizzle Whether to swizzle the texture.
 */
void sceGuTexMode(int tpsm, int maxmips, int a2, int swizzle);

/**
 * @brief Loads a block of color lookup table data.
 * @param num_blocks The number of blocks to load.
 * @param cbp Pointer to the color lookup table data.
 */
void sceGuClutLoad(int num_blocks, const void* cbp);

/**
 * @brief Sets the color lookup table mode.
 * @param cpsm The color pixel storage mode.
 * @param a1 First mode parameter.
 * @param a2 Second mode parameter.
 * @param a3 Third mode parameter.
 */
void sceGuClutMode(unsigned int cpsm, unsigned int a1, unsigned int a2, unsigned int a3);

/**
 * @brief Starts a display list.
 * @param cid The context ID.
 * @param list Pointer to the display list.
 */
void sceGuStart(int cid, void* list);

/**
 * @brief Waits for rendering to complete.
 * @param mode The synchronization mode.
 * @param a1 Unused parameter.
 * @return The result of the synchronization.
 */
int sceGuSync(int mode, int a1);

/**
 * @brief Finishes the current display list.
 * @return The result of the finish operation.
 */
int sceGuFinish(void);

/**
 * @brief Sets the display buffer.
 * @param width The width of the display buffer.
 * @param height The height of the display buffer.
 * @param dispbp Pointer to the display buffer.
 * @param dispbw The display buffer width.
 */
void sceGuDispBuffer(int width, int height, void* dispbp, int dispbw);

/**
 * @brief Waits for the vertical blanking interval to start.
 */
void sceDisplayWaitVblankStart();

/**
 * @brief Sets the display state.
 * @param state The state to set.
 * @return The result of the display operation.
 */
int sceGuDisplay(int state);

/**
 * @brief Sets the shading model.
 * @param mode The shading model to set.
 */
void sceGuShadeModel(int mode);

/**
 * @brief Sets the texture filtering mode.
 * @param min The minification filter.
 * @param mag The magnification filter.
 */
void sceGuTexFilter(int min, int mag);

/**
 * @brief Defines the scissor box.
 * @param x The x-coordinate of the scissor box.
 * @param y The y-coordinate of the scissor box.
 * @param w The width of the scissor box.
 * @param h The height of the scissor box.
 */
void sceGuScissor(int x, int y, int w, int h);

/**
 * @brief Sets the drawing buffer.
 * @param psm The pixel storage mode.
 * @param fbp Pointer to the frame buffer.
 * @param fbw The frame buffer width.
 */
void sceGuDrawBuffer(int psm, void* fbp, int fbw);

/**
 * @brief Initializes the GU library.
 */
void sceGuInit(void);

/**
 * @brief Initializes the debug screen.
 */
void pspDebugScreenInit();

/**
 * @brief Swaps the front and back buffers.
 * @return A pointer to the new back buffer.
 */
void* sceGuSwapBuffers(void);

/**
 * @brief Sets the clear color.
 * @param color The color to clear with.
 */
void sceGuClearColor(unsigned int color);

/**
 * @brief Clears buffers to preset values.
 * @param flags The buffers to clear.
 */
void sceGuClear(int flags);

/**
 * @brief Terminates the GU library.
 */
void sceGuTerm();

/**
 * @brief Exits the game.
 */
void sceKernelExitGame();

/**
 * @brief Opens a file.
 * @param file The name of the file to open.
 * @param flags The flags for opening the file.
 * @param mode The mode in which to open the file.
 * @return The file descriptor.
 */
SceUID sceIoOpen(const char *file, int flags, SceMode mode);

/**
 * @brief Closes a file.
 * @param fd The file descriptor to close.
 * @return The result of the close operation.
 */
int sceIoClose(SceUID fd);

/**
 * @brief Reads data from a file.
 * @param fd The file descriptor.
 * @param data Pointer to the buffer to read into.
 * @param size The size of the buffer.
 * @return The number of bytes read.
 */
int sceIoRead(SceUID fd, void *data, SceSize size);

/**
 * @brief Writes data to a file.
 * @param fd The file descriptor.
 * @param data Pointer to the buffer to write from.
 * @param size The number of bytes to write.
 * @return The number of bytes written.
 */
int sceIoWrite(SceUID fd, const void *data, SceSize size);

/**
 * @brief Sets the sampling cycle for the controller.
 * @param cycle The sampling cycle.
 * @return The result of the operation.
 */
int sceCtrlSetSamplingCycle(int cycle);

/**
 * @brief Sets the sampling mode for the controller.
 * @param mode The sampling mode.
 * @return The result of the operation.
 */
int sceCtrlSetSamplingMode(int mode);

/**
 * @brief Peeks the controller buffer for positive values.
 * @param pad_data Pointer to the controller data buffer.
 * @param count The number of elements to peek.
 * @return The result of the operation.
 */
int sceCtrlPeekBufferPositive(SceCtrlData *pad_data, int count);

/**
 * @brief Reads pixels from the framebuffer.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param format The pixel format.
 * @param type The data type of the pixel data.
 * @param pixels Pointer to the pixel data buffer.
 */
void emuGlReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);

/**
 * @brief Gets the current time of day.
 * @param tv Pointer to the timeval structure.
 * @param unused Unused parameter.
 */
void gettimeofday(struct timeval *tv, int unused);

/**
 * @brief Configures the 2D transfer for emulation.
 * @param enable Enable or disable the 2D transfer.
 */
void emuConfigure2DTransfer(int enable);

/**
 * @brief Seeks to a position in a file.
 * @param fd The file descriptor.
 * @param offset The offset to seek to.
 * @param whence The seek mode.
 * @return The new position in the file.
 */
int sceIoLseek32(SceUID fd, int offset, int whence);

/**
 * @brief Reserves an audio channel.
 * @param channel The channel to reserve.
 * @param samplecount The number of samples.
 * @param format The format of the audio data.
 * @return The result of the operation.
 */
int sceAudioChReserve(int channel, int samplecount, int format);

/**
 * @brief Releases an audio channel.
 * @param channel The channel to release.
 * @return The result of the operation.
 */
int sceAudioChRelease(int channel);

/**
 * @brief Outputs audio in stereo with panning.
 * @param channel The audio channel.
 * @param leftvol The left volume.
 * @param rightvol The right volume.
 * @param buffer Pointer to the audio buffer.
 * @return The result of the operation.
 */
int sceAudioOutputPannedBlocking(int channel, int leftvol, int rightvol, void *buffer);

/**
 * @brief Changes the channel configuration for audio.
 * @param channel The audio channel.
 * @param format The new audio format.
 * @return The result of the operation.
 */
int sceAudioChangeChannelConfig(int channel, int format);

/**
 * @brief Creates a new thread.
 * @param name The name of the thread.
 * @param entry The thread entry function.
 * @param initPriority The initial priority of the thread.
 * @param stackSize The size of the stack.
 * @param attr The thread attributes.
 * @param option Pointer to the thread options structure.
 * @return The thread ID.
 */
SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, SceKernelThreadOptParam *option);

/**
 * @brief Starts a thread.
 * @param thid The thread ID.
 * @param arglen The length of the arguments.
 * @param argp Pointer to the arguments.
 * @return The result of the start operation.
 */
int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp);

/**
 * @brief Waits for a thread to end.
 * @param thid The thread ID.
 * @param timeout Pointer to the timeout value.
 * @return The result of the wait operation.
 */
int sceKernelWaitThreadEnd(SceUID thid, SceUInt *timeout);

/**
 * @brief Deletes a thread.
 * @param thid The thread ID.
 * @return The result of the delete operation.
 */
int sceKernelDeleteThread(SceUID thid);

/**
 * @brief Exits and deletes the current thread.
 * @param status The exit status.
 * @return The result of the operation.
 */
int sceKernelExitDeleteThread(int status);

/**
 * @brief Sets the texture wrapping mode.
 * @param u The wrapping mode for the U axis.
 * @param v The wrapping mode for the V axis.
 */
void sceGuTexWrap(int u, int v);

#define EMU_NB_TEXTURES 1 /**< Number of textures */
extern GLuint emuGlTexture[EMU_NB_TEXTURES]; /**< Array of texture IDs */
extern u32 emuCurrentTexSizeX; /**< Current texture width */
extern u32 emuCurrentTexSizeY; /**< Current texture height */
extern u32 emuCurrentTextureState; /**< Current texture state */
extern u32 emuCurrentAmbientColor; /**< Current ambient color */
extern const int emu_pixelPhysFormats[]; /**< Array of pixel format values */
extern int emuReuseTextures; /**< Flag to indicate whether to reuse textures */
extern int emuUsePixelShaders; /**< Flag to indicate whether to use pixel shaders */
extern int emuFrameSkipMode; /**< Frame skip mode */
extern int emuPauseGame; /**< Pause game flag */

#ifdef __cplusplus
}
#endif

#endif // EMU_H
