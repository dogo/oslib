#include "oslib.h"

static u32 __ramAvailableMaxLinear(void) {
    // Blocks of 1 MB
    u32 size = 0;
    u32 sizeBlock = 1024 * 1024;
    u8 *ram;

    while (sizeBlock) {
		// Increment size
        size += sizeBlock;

		// Allocate ram
        ram = (u8*)malloc(size);

        if (!ram) {
            size -= sizeBlock; // Restore old size
            sizeBlock >>= 1; // Reduce the block size by half
        } else {
            free(ram);
        }
    }

    return size;
}

static u32 __ramAvailable(void) {
    u8 **ram = NULL;
    u32 size = 0;
    u32 count = 0;

    while (1) {
        // Allocate more entries if needed
        if (count % 10 == 0) {
            u8 **temp = (u8**)realloc(ram, sizeof(u8*) * (count + 10));
            if (!temp) break;

            ram = temp;
            size += sizeof(u8*) * 10;
        }

        // Find max linear size available
        u32 maxLinearSize = __ramAvailableMaxLinear();
        if (!maxLinearSize) break;

        // Allocate ram
        ram[count] = (u8*)malloc(maxLinearSize);
        if (!ram[count]) break;

        size += maxLinearSize;
        count++;
    }

    // Free allocated memory
    for (u32 i = 0; i < count; i++) {
        free(ram[i]);
    }
    free(ram);

    return size;
}

OSL_MEMSTATUS oslGetRamStatus(void) {
    OSL_MEMSTATUS ram;
    ram.maxAvailable = __ramAvailable();
    ram.maxBlockSize = __ramAvailableMaxLinear();
    return ram;
}
