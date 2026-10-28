/*
 * Host-side stand-in for the PSP SDK pieces used by the audio and file sources.
 * sceIo* map onto POSIX so sources read real files; the kernel/utility/codec
 * calls are declared here and defined by the test that needs them.
 */
#ifndef OSL_HOST_PSPSDK_H
#define OSL_HOST_PSPSDK_H

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

typedef int32_t s32;
typedef int SceUID;
typedef unsigned int SceSize;

#define PSP_O_RDONLY O_RDONLY
#define PSP_O_WRONLY O_WRONLY
#define PSP_O_RDWR O_RDWR
#define PSP_O_CREAT O_CREAT
#define PSP_O_TRUNC O_TRUNC
#define PSP_SEEK_SET SEEK_SET
#define PSP_SEEK_CUR SEEK_CUR
#define PSP_SEEK_END SEEK_END

static inline SceUID sceIoOpen(const char *file, int flags, int mode) {
	return open(file, flags, mode);
}

static inline int sceIoClose(SceUID fd) {
	return close(fd);
}

static inline int sceIoRead(SceUID fd, void *data, SceSize size) {
	return (int)read(fd, data, size);
}

static inline int sceIoWrite(SceUID fd, const void *data, SceSize size) {
	return (int)write(fd, data, size);
}

static inline int sceIoLseek32(SceUID fd, int offset, int whence) {
	return (int)lseek(fd, offset, whence);
}

// glibc only declares memalign in <malloc.h>, macOS not at all
static inline void *osl_host_memalign(size_t align, size_t size) {
	void *p = NULL;
	return posix_memalign(&p, align, size) ? NULL : p;
}

#define memalign osl_host_memalign

#define PSP_MEMORY_PARTITION_KERNEL 1
#define PSP_AV_MODULE_AVCODEC 0

typedef struct SceKernelLMOption {
	SceSize size;
	SceUID mpidtext;
	SceUID mpiddata;
	unsigned int flags;
	char position;
	char access;
	char creserved[2];
} SceKernelLMOption;

int sceKernelDevkitVersion(void);
SceUID sceKernelLoadModule(const char *path, int flags, SceKernelLMOption *option);
int sceKernelStartModule(SceUID modid, SceSize argsize, void *argp, int *status, void *option);
int sceUtilityLoadAvModule(int module);

#endif
