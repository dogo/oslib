/**
 * @file vram_mgr.h
 * @brief VRAM Manager for OSLib
 *
 * This header file defines the functions and macros for managing VRAM (Video RAM) in OSLib.
 * It includes functions for initializing the VRAM manager, allocating and freeing blocks of VRAM,
 * and setting parameters for VRAM management.
 */

#ifndef _OSL_VRAM_MGR_H_
#define _OSL_VRAM_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def OSL_VRAM_SIZE
 * @brief Defines the size of the VRAM (2 MB).
 */
#define OSL_VRAM_SIZE   (2 << 20)

/**
 * @def OSL_VRAM_BASE
 * @brief Defines the base address of the VRAM.
 */
#define OSL_VRAM_BASE   (0x40000000)

/**
 * @def OSL_VRAM_END
 * @brief Defines the end address of the VRAM.
 */
#define OSL_VRAM_END    (OSL_VRAM_BASE + OSL_VRAM_SIZE)

/**
 * @brief Global flag indicating whether the VRAM manager is in use.
 *
 * This flag is set to non-zero if the VRAM manager is initialized and in use.
 */
extern int osl_useVramManager;

/**
 * @brief Pointer to the current VRAM allocation pointer.
 *
 * This pointer keeps track of the current position in VRAM where the next allocation can be made.
 */
extern u32 osl_currentVramPtr;

/**
 * @brief Initializes the VRAM manager.
 *
 * This function initializes the VRAM manager, setting up the necessary internal structures and
 * pointers for VRAM management.
 */
extern void oslVramMgrInit();

/**
 * @brief Allocates a block of VRAM.
 *
 * This function allocates a block of VRAM of the specified size.
 *
 * @param blockSize The size of the block to allocate, in bytes.
 * @return A pointer to the allocated block of VRAM, or NULL if the allocation fails.
 */
extern void *oslVramMgrAllocBlock(int blockSize);

/**
 * @brief Frees a previously allocated block of VRAM.
 *
 * This function frees a block of VRAM that was previously allocated using oslVramMgrAllocBlock().
 *
 * @param blockAddress The address of the block to free.
 * @param blockSize The size of the block to free, in bytes.
 * @return 0 if the block was successfully freed, or a negative value if an error occurred.
 */
extern int oslVramMgrFreeBlock(void *blockAddress, int blockSize);

/**
 * @brief Sets the parameters for the VRAM manager.
 *
 * This function sets the start address and size for VRAM management. It can be used to
 * reconfigure the VRAM manager if necessary.
 *
 * @param adrStart The start address for VRAM management.
 * @param size The size of the VRAM area to manage, in bytes.
 * @return 0 if the parameters were successfully set, or a negative value if an error occurred.
 */
extern int oslVramMgrSetParameters(void *adrStart, int size);

#ifdef __cplusplus
}
#endif

#endif /* _OSL_VRAM_MGR_H_ */
