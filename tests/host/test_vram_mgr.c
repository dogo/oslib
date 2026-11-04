#include "oslib.h"
#include "test.h"

#define BASE OSL_VRAM_BASE
#define addr(p) ((u32)(uintptr_t)(p))

extern int osl_vramSize;

static void allocRoundsTo16AndIsContiguous(void) {
	void *a = oslVramMgrAllocBlock(100);
	void *b = oslVramMgrAllocBlock(16);
	CHECK_EQ(addr(a), BASE);
	CHECK_EQ(addr(b), BASE + 112);
	oslVramMgrFreeBlock(b, 16);
	oslVramMgrFreeBlock(a, 100);
}

static void rejectsInvalidSizes(void) {
	CHECK(oslVramMgrAllocBlock(0) == NULL);
	CHECK(oslVramMgrAllocBlock(-1) == NULL);
	CHECK(oslVramMgrAllocBlock(INT_MAX) == NULL);
	CHECK(oslVramMgrAllocBlock(OSL_VRAM_SIZE + 16) == NULL);
}

static void freedHoleIsReused(void) {
	void *a = oslVramMgrAllocBlock(64);
	void *b = oslVramMgrAllocBlock(64);
	void *c = oslVramMgrAllocBlock(64);
	CHECK(oslVramMgrFreeBlock(b, 64));
	CHECK_EQ(addr(oslVramMgrAllocBlock(64)), addr(b));
	oslVramMgrFreeBlock(a, 64);
	oslVramMgrFreeBlock(b, 64);
	oslVramMgrFreeBlock(c, 64);
}

static void freeingEverythingCoalesces(void) {
	void *blocks[8];
	for (int i = 0; i < 8; i++)
		blocks[i] = oslVramMgrAllocBlock(1024);
	// Free out of order so merging has to happen on both sides
	for (int i = 0; i < 8; i += 2)
		oslVramMgrFreeBlock(blocks[i], 1024);
	for (int i = 1; i < 8; i += 2)
		oslVramMgrFreeBlock(blocks[i], 1024);
	CHECK_EQ(addr(oslVramMgrAllocBlock(OSL_VRAM_SIZE)), BASE);
	oslVramMgrFreeBlock((void *)(uintptr_t)BASE, OSL_VRAM_SIZE);
}

static void unknownBlockIsRejected(void) {
	CHECK_EQ(oslVramMgrFreeBlock((void *)(uintptr_t)(BASE + 48), 16), 0);
}

static void growsBlockTablePastDefaultSize(void) {
	// 1024 entries is the initial table; 2000 live blocks forces a realloc
	enum { N = 2000 };
	for (int i = 0; i < N; i++)
		CHECK_EQ(addr(oslVramMgrAllocBlock(16)), BASE + i * 16);
	for (int i = 0; i < N; i++)
		CHECK(oslVramMgrFreeBlock((void *)(uintptr_t)(BASE + i * 16), 16));
	CHECK_EQ(addr(oslVramMgrAllocBlock(OSL_VRAM_SIZE)), BASE);
}

// Shrinking below what is allocated must fail and leave the region as it was
static void shrinkBelowAllocatedIsRejected(void) {
	void *a = oslVramMgrAllocBlock(1536 << 10);
	CHECK_EQ(addr(a), BASE);
	CHECK_EQ(oslVramMgrSetParameters((void *)(uintptr_t)BASE, 1 << 20), 0);
	CHECK_EQ(osl_vramSize, OSL_VRAM_SIZE);
	CHECK(oslVramMgrAllocBlock(2 << 20) == NULL);
	void *b = oslVramMgrAllocBlock(512 << 10); // the rest of the unchanged region
	CHECK_EQ(addr(b), BASE + (1536 << 10));
	oslVramMgrFreeBlock(b, 512 << 10);
	oslVramMgrFreeBlock(a, 1536 << 10);
}

int main(void) {
	oslVramMgrInit();
	allocRoundsTo16AndIsContiguous();
	rejectsInvalidSizes();
	freedHoleIsReused();
	freeingEverythingCoalesces();
	unknownBlockIsRejected();
	shrinkBelowAllocatedIsRejected();
	growsBlockTablePastDefaultSize();
	return TEST_RESULT();
}
