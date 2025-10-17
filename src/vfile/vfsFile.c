#include "oslib.h"

/*
   SOURCE VFS: file
 */

#define FLAG_EOF 1
int VF_FILE = -1;

#define _file_ ((SceUID)f->ioPtr)

int vfsFileOpen(void *param1, int param2, int type, int mode, VIRTUAL_FILE* f) {
	int stdMode = PSP_O_RDONLY;

	switch (mode) {
	case VF_O_WRITE:
		stdMode = PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC;
		break;
	case VF_O_READWRITE:
		stdMode = PSP_O_RDWR;
		break;
	}

	f->ioPtr = (void*)sceIoOpen((char*)param1, stdMode, 0777);
	f->mode = mode; // Store the open mode
	return (s32)f->ioPtr >= 0; // Return true if file descriptor is valid
}

int vfsFileClose(VIRTUAL_FILE *f) {
	return sceIoClose(_file_) >= 0; // Return true if closing was successful
}

int vfsFileWrite(const void *ptr, size_t size, size_t n, VIRTUAL_FILE* f) {
	return sceIoWrite(_file_, ptr, size * n);
}

int vfsFileRead(void *ptr, size_t size, size_t n, VIRTUAL_FILE* f) {
	int readSize = sceIoRead(_file_, ptr, size * n);

	if (readSize < size * n) {
		f->userData |= FLAG_EOF; // Set EOF flag if less data was read
	}

	return readSize;
}

int vfsFileGetc(VIRTUAL_FILE *f) {
	// Fallback to memory-based getchar
	return vfsMemGetc(f);
}

int vfsFilePutc(int caractere, VIRTUAL_FILE *f) {
	// Fallback to memory-based putchar
	return vfsMemPutc(caractere, f);
}

char *vfsFileGets(char *str, int maxLen, VIRTUAL_FILE *f) {
	// Fallback to memory-based fgets
	return vfsMemGets(str, maxLen, f);
}

void vfsFilePuts(const char *s, VIRTUAL_FILE *f) {
	// Fallback to memory-based puts
	vfsMemPuts(s, f);
}

void vfsFileSeek(VIRTUAL_FILE *f, int offset, int whence) {
	sceIoLseek32(_file_, offset, whence);
	f->userData &= ~FLAG_EOF; // Reset EOF flag after seek
}

int vfsFileTell(VIRTUAL_FILE *f) {
	return sceIoLseek32(_file_, 0, SEEK_CUR); // Return current file position
}

int vfsFileEof(VIRTUAL_FILE *f) {
	return f->userData & FLAG_EOF; // Check EOF flag
}

VIRTUAL_FILE_SOURCE vfsFile = {
	vfsFileOpen,
	vfsFileClose,
	vfsFileRead,
	vfsFileWrite,
	vfsFileGetc,
	vfsFilePutc,
	vfsFileGets,
	vfsFilePuts,
	vfsFileSeek,
	vfsFileTell,
	vfsFileEof
};

int oslInitVfsFile() {
	VF_FILE = VirtualFileRegisterSource(&vfsFile);
	return VF_FILE;
}
