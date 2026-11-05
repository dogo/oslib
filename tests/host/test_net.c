#include "oslib.h"
#include "test.h"

#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <psphttp.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <pspssl.h>
#include <psputility.h>
#include <psputility_netmodules.h>
#include <pspwlan.h>
#include <unistd.h>

#include "net.h"

int osl_quit;

void oslHandleLoadNoFailError(const char *filename) {
	(void)filename;
}

// Network bring-up: never reached by the HTTP calls under test
int sceNetInit(int poolsize, int calloutprio, int calloutstack, int netintrprio, int netintrstack) {
	(void)poolsize; (void)calloutprio; (void)calloutstack; (void)netintrprio; (void)netintrstack;
	return 0;
}

int sceNetTerm(void) {
	return 0;
}

int sceNetInetInit(void) {
	return 0;
}

int sceNetInetTerm(void) {
	return 0;
}

int sceNetResolverInit(void) {
	return 0;
}

int sceNetResolverTerm(void) {
	return 0;
}

int sceSslInit(int unknown1) {
	(void)unknown1; return 0;
}

int sceSslEnd(void) {
	return 0;
}

int sceNetApctlInit(int stackSize, int initPriority) {
	(void)stackSize; (void)initPriority; return 0;
}

int sceNetApctlTerm(void) {
	return 0;
}

int sceNetApctlConnect(int connIndex) {
	(void)connIndex; return 0;
}

int sceNetApctlDisconnect(void) {
	return 0;
}

int sceNetApctlGetState(int *pState) {
	*pState = PSP_NET_APCTL_STATE_GOT_IP; return 0;
}

int sceNetApctlGetInfo(int code, union SceNetApctlInfo *pInfo) {
	(void)code; pInfo->ip[0] = 0; return 0;
}

int sceUtilityCheckNetParam(int id) {
	(void)id; return -1;
}

int sceUtilityGetNetParam(int conf, int param, netData *data) {
	(void)conf; (void)param; data->asUint = 0; return -1;
}

int sceUtilityLoadNetModule(int module) {
	(void)module; return 0;
}

int sceUtilityUnloadNetModule(int module) {
	(void)module; return 0;
}

int sceWlanDevIsPowerOn(void) {
	return 1;
}

int sceWlanGetEtherAddr(uint8_t *etherAddr) {
	(void)etherAddr; return 0;
}

int sceKernelDelayThread(unsigned int delay) {
	return usleep(delay);
}

// Fake HTTP: one scripted response; counts objects so leaks show; sceHttpEnd ends HTTP for good
static struct {
	int status;
	int length;  // Content-Length, -1 if not sent
	int size;    // bytes the "server" delivers
	int failAt;  // read error once this many bytes were read, -1 for none
	int chunk;   // most bytes one read returns, as a network packet would; 0 for no limit
} response;
static int delivered, live, ended, templatesCreated;

int sceHttpInit(unsigned int unknown1) {
	(void)unknown1; return 0;
}

int sceHttpEnd(void) {
	ended = 1; return 0;
}

int sceHttpsInit(int a, int b, int c, int d) {
	(void)a; (void)b; (void)c; (void)d; return 0;
}

int sceHttpsEnd(void) {
	return 0;
}

int sceHttpsLoadDefaultCert(int a, int b) {
	(void)a; (void)b; return 0;
}

int sceHttpLoadSystemCookie(void) {
	return 0;
}

int sceHttpSaveSystemCookie(void) {
	return 0;
}

int sceHttpCreateTemplate(char *agent, int a, int b) {
	(void)agent; (void)a; (void)b;
	templatesCreated++;
	return ended ? -1 : (live++, 1);
}

int sceHttpDeleteTemplate(int id) {
	(void)id; live--; return 0;
}

int sceHttpSetResolveTimeOut(int id, unsigned int t) {
	(void)id; (void)t; return 0;
}

int sceHttpSetRecvTimeOut(int id, unsigned int t) {
	(void)id; (void)t; return 0;
}

int sceHttpSetSendTimeOut(int id, unsigned int t) {
	(void)id; (void)t; return 0;
}

int sceHttpCreateConnectionWithURL(int id, const char *url, int a) {
	(void)id; (void)url; (void)a; live++; return 2;
}

int sceHttpDeleteConnection(int id) {
	(void)id; live--; return 0;
}

int sceHttpEnableKeepAlive(int id) {
	(void)id; return 0;
}

int sceHttpCreateRequestWithURL(int id, int method, char *url, SceULong64 length) {
	(void)id; (void)method; (void)url; (void)length;
	delivered = 0;
	live++;
	return 3;
}

int sceHttpDeleteRequest(int id) {
	(void)id; live--; return 0;
}

int sceHttpAddExtraHeader(int id, char *name, char *value, int a) {
	(void)id; (void)name; (void)value; (void)a; return 0;
}

int sceHttpSendRequest(int id, void *data, unsigned int size) {
	(void)id; (void)data; (void)size; return 0;
}

int sceHttpGetStatusCode(int id, int *status) {
	(void)id; *status = response.status; return 0;
}

int sceHttpGetContentLength(int id, SceULong64 *length) {
	(void)id;
	if (response.length < 0) return (int)0x80431085; // no Content-Length header
	*length = response.length;
	return 0;
}

int sceHttpReadData(int id, void *data, unsigned int size) {
	(void)id;
	if (response.failAt >= 0 && delivered >= response.failAt) return (int)0x80431081;
	int n = response.size - delivered;
	if ((unsigned int)n > size) n = size;
	if (response.chunk && n > response.chunk) n = response.chunk;
	if (response.failAt >= 0 && delivered + n > response.failAt) n = response.failAt - delivered;
	memset(data, 'n', n);
	delivered += n;
	return n;
}

// oslNetPostForm's reply is a string: read in full however it arrives, cut to the buffer, empty
// on failure. The buffer starts as 'x's, so a missing terminator shows.
static void post(int status, int size, int failAt, int bufferSize, int wantResult, int wantLength) {
	char reply[64];
	memset(reply, 'x', sizeof(reply));
	response.status = status;
	response.length = size;
	response.size = size;
	response.failAt = failAt;
	response.chunk = 8;
	int r = oslNetPostForm("http://host/form", "a=1", reply, bufferSize);
	response.chunk = 0;
	CHECK(wantResult > 0 ? r == wantResult : r < 0);
	CHECK(memchr(reply, 0, bufferSize) != NULL);
	if (memchr(reply, 0, bufferSize)) {
		CHECK_EQ(strlen(reply), wantLength);
		CHECK_EQ(strspn(reply, "n"), wantLength);
	}
	CHECK_EQ(live, 0);
}

static void postReply(void) {
	post(200, 30, -1, 64, 1, 30);  // four reads
	post(200, 100, -1, 16, 1, 15); // longer than the buffer: cut, still terminated
	post(200, 0, -1, 16, 1, 0);    // empty reply
	post(404, 10, -1, 16, -1, 0);  // failure: empty
	post(200, 30, 12, 64, -1, 0);  // read error after 12 bytes: empty
}

#define FILE_ "test_net.bin"

static void writeOld(void) {
	FILE *f = fopen(FILE_, "wb");
	fputs("old", f);
	fclose(f);
}

static long fileSize(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return -1;
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fclose(f);
	return size;
}

// The lowest free descriptor: grows if a call leaves a file open
static int nextFd(void) {
	int fd = open("/dev/null", O_RDONLY);
	close(fd);
	return fd;
}

// Runs one download over an existing "old" file and checks what is left around it
static void get(int status, int length, int size, int failAt, int wantOk) {
	response.status = status;
	response.length = length;
	response.size = size;
	response.failAt = failAt;
	writeOld();
	int fd = nextFd();
	int r = oslNetGetFile("http://host/x", FILE_);
	CHECK_EQ(r == 0, wantOk);
	CHECK_EQ(fileSize(FILE_), wantOk ? size : 3);
	CHECK_EQ(fileSize(FILE_ ".oslpart"), -1);
	CHECK_EQ(fileSize(FILE_ ".oslbak"), -1);
	CHECK_EQ(nextFd(), fd);
	CHECK_EQ(live, 0);
	remove(FILE_);
}

// A failure before anything was written leaves files of that name alone
static void notOursIsKept(void) {
	FILE *f = fopen(FILE_ ".oslpart", "wb");
	fputs("mine", f);
	fclose(f);
	response.status = 404;
	CHECK(oslNetGetFile("http://host/x", FILE_) < 0);
	CHECK_EQ(fileSize(FILE_ ".oslpart"), 4);
	remove(FILE_ ".oslpart");
}

static void put(const char *path, const char *text) {
	FILE *f = fopen(path, "wb");
	CHECK(f != NULL);
	if (!f) return;
	fputs(text, f);
	fclose(f);
}

static void respond(int status, int size) {
	response.status = status;
	response.length = size;
	response.size = size;
	response.failAt = -1;
}

// Power-off after the old file was moved aside: the next call, even a failing one, puts it back
static void backupWithoutFileIsRestored(void) {
	remove(FILE_);
	put(FILE_ ".oslbak", "old");
	respond(404, 10);
	CHECK(oslNetGetFile("http://host/x", FILE_) < 0);
	CHECK_EQ(fileSize(FILE_), 3);
	CHECK_EQ(fileSize(FILE_ ".oslbak"), -1);
	remove(FILE_);
}

// Power-off after the new file went in (or a backup that couldn't be removed): downloads go on
static void leftoverBackupIsRemoved(void) {
	put(FILE_, "new");
	put(FILE_ ".oslbak", "old");
	for (int i = 0; i < 2; i++) {
		respond(200, 100 + i);
		CHECK_EQ(oslNetGetFile("http://host/x", FILE_), 0);
		CHECK_EQ(fileSize(FILE_), 100 + i);
		CHECK_EQ(fileSize(FILE_ ".oslbak"), -1);
	}
	remove(FILE_);
}

// A directory is rejected before recovery can remove the only old copy
static void directoryIsRejected(void) {
	for (int withBackup = 0; withBackup < 2; withBackup++) {
		CHECK_EQ(mkdir(FILE_, 0777), 0);
		put(FILE_ "/inner", "keep");
		if (withBackup) put(FILE_ ".oslbak", "old");
		respond(200, 100);
		int before = templatesCreated;
		CHECK(oslNetGetFile("http://host/x", FILE_) < 0);
		CHECK_EQ(templatesCreated, before);
		CHECK_EQ(fileSize(FILE_ "/inner"), 4);
		CHECK_EQ(fileSize(FILE_ ".oslbak"), withBackup ? 3 : -1);
		CHECK_EQ(fileSize(FILE_ ".oslpart"), -1);
		remove(FILE_ ".oslbak");
		remove(FILE_ "/inner");
		CHECK_EQ(rmdir(FILE_), 0);
	}
}

// A backup directory cannot be an interrupted file swap
static void backupDirectoryIsRejected(void) {
	for (int withFile = 0; withFile < 2; withFile++) {
		CHECK_EQ(mkdir(FILE_ ".oslbak", 0777), 0);
		put(FILE_ ".oslbak/inner", "keep");
		if (withFile) put(FILE_, "old");
		respond(200, 100);
		int before = templatesCreated;
		CHECK(oslNetGetFile("http://host/x", FILE_) < 0);
		CHECK_EQ(templatesCreated, before);
		CHECK_EQ(fileSize(FILE_ ".oslbak/inner"), 4);
		CHECK_EQ(fileSize(FILE_), withFile ? 3 : -1);
		CHECK_EQ(fileSize(FILE_ ".oslpart"), -1);
		remove(FILE_);
		remove(FILE_ ".oslbak/inner");
		CHECK_EQ(rmdir(FILE_ ".oslbak"), 0);
	}
}

// Use short components so the old 256-byte path buffer is the only former limit
static void longPath(int length) {
	char path[PATH_MAX], tmppath[PATH_MAX], bakpath[PATH_MAX];
	int prefix = 0;
	while (length - prefix > 200) {
		memset(path + prefix, 'd', 64);
		prefix += 64;
		path[prefix] = 0;
		CHECK_EQ(mkdir(path, 0777), 0);
		path[prefix++] = '/';
	}
	memset(path + prefix, 'f', length - prefix);
	path[length] = 0;
	snprintf(tmppath, sizeof(tmppath), "%s.oslpart", path);
	snprintf(bakpath, sizeof(bakpath), "%s.oslbak", path);
	put(path, "old");
	respond(200, 100);
	CHECK_EQ(oslNetGetFile("http://host/x", path), 0);
	CHECK_EQ(fileSize(path), 100);
	CHECK_EQ(fileSize(tmppath), -1);
	CHECK_EQ(fileSize(bakpath), -1);
	CHECK_EQ(live, 0);
	CHECK_EQ(remove(path), 0);
	while (prefix > 0) {
		path[prefix - 1] = 0;
		CHECK_EQ(rmdir(path), 0);
		char *slash = strrchr(path, '/');
		prefix = slash ? (int)(slash - path) + 1 : 0;
	}
}

static void derivedPathOverflowIsRejected(void) {
	char path[PATH_MAX];
	memset(path, 'p', sizeof(path) - 8);
	path[sizeof(path) - 8] = 0;
	respond(200, 100);
	int before = templatesCreated;
	CHECK(oslNetGetFile("http://host/x", path) < 0);
	CHECK_EQ(templatesCreated, before);
	CHECK_EQ(live, 0);
}

int main(void) {
	char workdir[] = "test_net.XXXXXX";
	if (!mkdtemp(workdir)) {
		perror("mkdtemp");
		return 1;
	}
	if (chdir(workdir) < 0) {
		perror("chdir");
		rmdir(workdir);
		return 1;
	}
	get(200, 5000, 5000, -1, 1);   // complete, replaces the old file
	get(200, -1, 3000, -1, 1);     // no Content-Length (chunked): the end of data ends it
	get(404, 10, 10, -1, 0);       // not the file
	get(200, 5000, 1000, -1, 0);   // the connection closed early
	get(200, 5000, 5000, 1000, 0); // read error: its file must be closed too
	for (int i = 0; i < 3; i++) get(200, 5000, 5000, 1000, 0);

	// A failed post must not end HTTP for the next call
	char reply[16];
	response.status = 404;
	CHECK(oslNetPostForm("http://host/form", "a=1", reply, sizeof(reply)) < 0);
	CHECK_EQ(live, 0);
	postReply();
	get(200, 100, 100, -1, 1);
	notOursIsKept();
	backupWithoutFileIsRestored();
	leftoverBackupIsRemoved();
	directoryIsRejected();
	backupDirectoryIsRejected();
	longPath(248);
	longPath(511);
	derivedPathOverflowIsRejected();
	CHECK_EQ(chdir(".."), 0);
	CHECK_EQ(rmdir(workdir), 0);
	return TEST_RESULT();
}
