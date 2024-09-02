#include "oslib.h"

//We're beginning at the VRAM base
u32 osl_vramBase = 0x40000000;
//2 MBytes
int osl_vramSize = 2 << 20;
//Use it or not?
int osl_useVramManager = 1;
u32 osl_currentVramPtr;

#define DEFAULT_TABLE_SIZE 1024

int osl_vramBlocksMax = 0, osl_vramBlocksNb = 0;

typedef struct		{
	u32 offset, size;
} OSL_VRAMBLOCK;

#define isBlockFree(i)				(osl_vramBlocks[i].size & 0x80000000)
#define getBlockSize(i)				(osl_vramBlocks[i].size & 0x7fffffff)
#define getBlockOffset(i)			(osl_vramBlocks[i].offset)

#define setBlockFree(i, free)		(osl_vramBlocks[i].size = (osl_vramBlocks[i].size & ((free)? 0xffffffff : 0x7fffffff)) | ((free)? 0x80000000 : 0))
#define setBlockSize(i, nsize)		(osl_vramBlocks[i].size = (osl_vramBlocks[i].size & ~0x7fffffff) | (nsize))
#define setBlockOffset(i, noffset)	(osl_vramBlocks[i].offset = noffset)

OSL_VRAMBLOCK *osl_vramBlocks;

void oslVramMgrInit() {
	// If we don't use it OR it has already been initialized
	if (!osl_useVramManager || osl_vramBlocksMax > 0)
		return;

	osl_vramBlocksMax = DEFAULT_TABLE_SIZE;
	osl_vramBlocksNb = 1;
	osl_vramBlocks = (OSL_VRAMBLOCK*)malloc(osl_vramBlocksMax * sizeof(OSL_VRAMBLOCK));
	if (!osl_vramBlocks) {
		osl_useVramManager = 0;
		osl_vramBlocksMax = 0;
		osl_vramBlocksNb = 0;
		return;
	}

	// Initialize the first block's size to 0
	osl_vramBlocks[0].size = 0;

	// First block: free, total size of VRAM, address 0
	setBlockOffset(0, 0);
	// The block size must be divided by 16 because we don't use bytes; otherwise, it would be impossible to encode all of VRAM in 16 bits
	setBlockSize(0, osl_vramSize);
	setBlockFree(0, 1);
}

void *oslVramMgrAllocBlock(int blockSize) {
	int i;

	osl_skip = osl_vramBlocks[0].size;
	// The block cannot be of zero or negative size
	if (blockSize <= 0)
		return NULL;

	// The size is always a multiple of 16 - round up to the next block
	if (blockSize & 15)
		blockSize += 16;

	// Without the manager, it's simpler...
	if (!osl_useVramManager) {
		int ptr = osl_currentVramPtr;
		// Memory overflow?
		if (osl_currentVramPtr + blockSize >= osl_vramBase + osl_vramSize)
			return NULL;
		osl_currentVramPtr += blockSize;
		return (void*)ptr;
	}

	for (i = 0; i < osl_vramBlocksNb; i++) {
		// Is this block sufficient?
		if (isBlockFree(i) && getBlockSize(i) >= blockSize)
			break;
	}

	// No free block
	if (i >= osl_vramBlocksNb)
		return NULL;

	// Exactly the needed memory? - not handled, the last block must always be marked as free (even if 0 bytes are left) for ulSetTexVramParameters
	if (getBlockSize(i) == blockSize && i != osl_vramBlocksNb - 1) {
		// It is no longer free
		setBlockFree(i, 0);
	} else {
		// We will add a new block
		osl_vramBlocksNb++;

		// No more memory for the array? Let's expand it
		if (osl_vramBlocksNb >= osl_vramBlocksMax) {
			OSL_VRAMBLOCK *oldBlock = osl_vramBlocks;
			osl_vramBlocksMax += DEFAULT_TABLE_SIZE;
			osl_vramBlocks = (OSL_VRAMBLOCK*)realloc(osl_vramBlocks, osl_vramBlocksMax);

			// Check that memory allocation was successful
			if (!osl_vramBlocks) {
				osl_vramBlocks = oldBlock;
				osl_vramBlocksMax -= DEFAULT_TABLE_SIZE;
				// Not enough memory
				return NULL;
			}
		}

		// Shift to insert our new element
		memmove(osl_vramBlocks + i + 1, osl_vramBlocks + i, sizeof(OSL_VRAMBLOCK) * (osl_vramBlocksNb - i - 1));

		// Fill our new block
		setBlockSize(i, blockSize);
		// It has the address of the block that was there before
		setBlockOffset(i, getBlockOffset(i + 1));
		// It is not free
		setBlockFree(i, 0);

		// For the next one, its size decreases
		setBlockSize(i + 1, getBlockSize(i + 1) - blockSize);
		// WARNING: offset calculation
		setBlockOffset(i + 1, getBlockOffset(i + 1) + blockSize);
	}

	// Note: the offset must be translated into a real address
	return (void*)(getBlockOffset(i) + osl_vramBase);
}

// Note: we need to translate a real address into an offset
int oslVramMgrFreeBlock(void *blockAddress, int blockSize) {
	int i, j, updateNeeded;
	int blockOffset = (u32)blockAddress - (u32)osl_vramBase;

	// Without the manager, it's simpler...
	if (!osl_useVramManager) {
		osl_currentVramPtr -= blockSize;
		// Not really useful, just here to ensure we never exceed the allocated space
		if (osl_currentVramPtr < osl_vramBase)
			osl_currentVramPtr = osl_vramBase;
		return 1;
	}

	// Let's find the correct block
	for (i = 0; i < osl_vramBlocksNb; i++) {
		if (getBlockOffset(i) == blockOffset)
			break;
	}

	// Unable to find the block
	if (i >= osl_vramBlocksNb)
		return 0;

	// The block is now free ^^
	setBlockFree(i, 1);

	// Now let's "merge" adjacent free blocks
	do {
		updateNeeded = 0;
		for (j = 0; j < osl_vramBlocksNb - 1; j++) {
			// Let's look for two adjacent blocks
			if ((isBlockFree(j) && isBlockFree(j + 1))
				|| (isBlockFree(j) && getBlockSize(j) == 0)) {
				// Merge these blocks now
				int newSize = getBlockSize(j) + getBlockSize(j + 1), newAdd = getBlockOffset(j);
				memmove(osl_vramBlocks + j, osl_vramBlocks + j + 1, sizeof(OSL_VRAMBLOCK) * (osl_vramBlocksNb - j - 1));
				setBlockOffset(j, newAdd);
				setBlockSize(j, newSize);
				// The block in between is removed
				osl_vramBlocksNb--;
				// Note: We will need another pass to check if new blocks were created
				updateNeeded = 1;
			}
		}

	} while (updateNeeded);

	return 1;
}

int oslVramMgrSetParameters(void *baseAddr, int size) {
   int curVramSize = osl_vramSize;
   int blockNum = osl_vramBlocksNb - 1;
   int sizeDiff;

	if (!osl_useVramManager)
		return 0;
	// The size is always a multiple of 16 - round up to the next block
	if (size & 15)
		size += 16;

	// Size difference (negative for reduction, positive for increase)
   sizeDiff = size - curVramSize;

	// The last block is ALWAYS free, even if there are 0 bytes left. See the workaround in ulTexVramAlloc
	if (isBlockFree(blockNum) && getBlockSize(blockNum) + sizeDiff >= 0) {
		setBlockSize(blockNum, getBlockSize(blockNum) + sizeDiff);
		osl_vramBase = (u32)baseAddr;
		osl_vramSize = size;
		// For those who do not want to use the manager...
		osl_currentVramPtr = osl_vramBase;
	}
	else
		return 0;
	return 1;
}
