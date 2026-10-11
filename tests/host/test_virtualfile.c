#include "oslib.h"
#include "test.h"

// vfsFile.c needs the PSP IO API; the memory source is what's under test.
int VF_FILE = -1;
int oslInitVfsFile(void) {
	return -1;
}

static VIRTUAL_FILE *openMem(void *data, int size, int mode) {
	return VirtualFileOpen(data, size, VF_MEMORY, mode);
}

static void readClampsAtEnd(void) {
	char data[] = "abcdef", buf[16] = { 0 };
	VIRTUAL_FILE *f = openMem(data, 6, VF_O_READ);
	CHECK_EQ(VirtualFileRead(buf, 1, 4, f), 4);
	CHECK_EQ(VirtualFileRead(buf, 1, 4, f), 2);
	CHECK(VirtualFileEof(f));
	CHECK_EQ(VirtualFileRead(buf, 1, 4, f), 0);
	VirtualFileClose(f);
}

static void readReturnsBytesNotItems(void) {
	char data[12] = { 0 }, buf[12];
	VIRTUAL_FILE *f = openMem(data, 12, VF_O_READ);
	CHECK_EQ(VirtualFileRead(buf, 4, 3, f), 12);
	VirtualFileClose(f);
}

static void seekClampsToBounds(void) {
	char data[10];
	VIRTUAL_FILE *f = openMem(data, 10, VF_O_READ);
	VirtualFileSeek(f, 4, SEEK_SET);
	CHECK_EQ(VirtualFileTell(f), 4);
	VirtualFileSeek(f, -2, SEEK_CUR);
	CHECK_EQ(VirtualFileTell(f), 2);
	VirtualFileSeek(f, -3, SEEK_END);
	CHECK_EQ(VirtualFileTell(f), 7);
	VirtualFileSeek(f, 100, SEEK_SET);
	CHECK_EQ(VirtualFileTell(f), 10);
	VirtualFileSeek(f, -100, SEEK_CUR);
	CHECK_EQ(VirtualFileTell(f), 0);
	VirtualFileClose(f);
}

static void writeRespectsModeAndSize(void) {
	char data[4] = { 0 };
	VIRTUAL_FILE *ro = openMem(data, 4, VF_O_READ);
	CHECK_EQ(VirtualFileWrite("xy", 1, 2, ro), 0);
	VirtualFileClose(ro);

	VIRTUAL_FILE *rw = openMem(data, 4, VF_O_READWRITE);
	CHECK_EQ(VirtualFileWrite("123456", 1, 6, rw), 4);
	CHECK(memcmp(data, "1234", 4) == 0);
	VirtualFileClose(rw);
}

static void getsSplitsLinesAndEndings(void) {
	char data[] = "first\nsecond\r\nthird";
	char line[32];
	VIRTUAL_FILE *f = openMem(data, sizeof(data) - 1, VF_O_READ);
	CHECK(strcmp(VirtualFileGets(line, sizeof(line), f), "first") == 0);
	CHECK(strcmp(VirtualFileGets(line, sizeof(line), f), "second") == 0);
	CHECK(strcmp(VirtualFileGets(line, sizeof(line), f), "third") == 0);
	VirtualFileClose(f);
}

static void getsLineLongerThanInternalBlock(void) {
	char data[] = "0123456789abcdefghijklmnop\nnext";
	char line[64];
	VIRTUAL_FILE *f = openMem(data, sizeof(data) - 1, VF_O_READ);
	CHECK(strcmp(VirtualFileGets(line, sizeof(line), f), "0123456789abcdefghijklmnop") == 0);
	CHECK(strcmp(VirtualFileGets(line, sizeof(line), f), "next") == 0);
	VirtualFileClose(f);
}

static void virtualFilenameListResolvesByName(void) {
	static char blob[] = "hello";
	OSL_VIRTUALFILENAME list[] = { { "ram:/hello.txt", blob, 5, &VF_MEMORY } };
	char buf[8] = { 0 };
	CHECK(oslAddVirtualFileList(list, 1));
	VIRTUAL_FILE *f = VirtualFileOpen((void *)"ram:/hello.txt", 0, VF_AUTO, VF_O_READ);
	CHECK(f != NULL);
	if (f) {
		CHECK_EQ(VirtualFileRead(buf, 1, 8, f), 5);
		CHECK(strcmp(buf, "hello") == 0);
		VirtualFileClose(f);
	}
	oslRemoveVirtualFileList(list, 1);
}

int main(void) {
	VirtualFileInit();
	readClampsAtEnd();
	readReturnsBytesNotItems();
	seekClampsToBounds();
	writeRespectsModeAndSize();
	getsSplitsLinesAndEndings();
	getsLineLongerThanInternalBlock();
	virtualFilenameListResolvesByName();
	return TEST_RESULT();
}
