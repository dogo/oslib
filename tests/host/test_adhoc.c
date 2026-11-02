#include <pspkernel.h>
#include <pspsdk.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <pspnet_adhoc.h>
#include <pspnet_adhocctl.h>
#include <pspnet_adhocmatching.h>
#include <psputility_sysparam.h>
#include <stdlib.h>
#include <string.h>

#include "pspadhoc.h"
#include "test.h"

// Fake network stack: the control link connects at once and every setup call
// succeeds, except the failStage-th one; opened counts what is left to tear down
static char sentProduct[sizeof(((struct productStruct *)0)->product)];
static int opened, setupCalls, failStage;

static int setup(int handle) {
	if (++setupCalls == failStage) return -1;
	opened++;
	return handle;
}

static int teardown(void) {
	opened--;
	return 0;
}

int sceKernelDelayThread(unsigned int delay) {
	(void)delay;
	return 0;
}

time_t sceKernelLibcTime(time_t *t) {
	if (t) *t = 0;
	return 0;
}

SceUID pspSdkLoadStartModule(const char *filename, int mpid) {
	(void)filename; (void)mpid;
	return 0;
}

int sceWlanDevIsPowerOn(void) {
	return 1;
}

int sceWlanGetEtherAddr(uint8_t *etherAddr) {
	memset(etherAddr, 0x11, 6);
	return 0;
}

int sceNetInit(int poolsize, int calloutprio, int calloutstack, int netintrprio, int netintrstack) {
	(void)poolsize; (void)calloutprio; (void)calloutstack; (void)netintrprio; (void)netintrstack;
	return setup(0);
}

int sceNetTerm(void) {
	return teardown();
}

int sceNetAdhocInit(void) {
	return setup(0);
}

int sceNetAdhocTerm(void) {
	return teardown();
}

int sceNetAdhocPdpCreate(unsigned char *mac, unsigned short port, unsigned int bufsize, int unk1) {
	(void)mac; (void)port; (void)bufsize; (void)unk1;
	return setup(1);
}

int sceNetAdhocPdpDelete(int id, int unk1) {
	(void)id; (void)unk1;
	return teardown();
}

int sceNetAdhocPdpSend(int id, unsigned char *destMacAddr, unsigned short port, void *data, unsigned int len, unsigned int timeout, int nonblock) {
	(void)id; (void)destMacAddr; (void)port; (void)data; (void)len; (void)timeout; (void)nonblock;
	return 0;
}

// Received datagrams, oldest first
#define MAX_QUEUED 4
static struct { u8 mac[6]; char data[16]; unsigned int len; } queued[MAX_QUEUED];
static int queuedCount;

static void queue(u8 macByte, const char *data) {
	memset(queued[queuedCount].mac, macByte, 6);
	queued[queuedCount].len = strlen(data);
	memcpy(queued[queuedCount].data, data, queued[queuedCount].len);
	queuedCount++;
}

int sceNetAdhocPdpRecv(int id, unsigned char *srcMacAddr, unsigned short *port, void *data, void *dataLength, unsigned int timeout, int nonblock) {
	unsigned int *len = dataLength;
	(void)id; (void)timeout; (void)nonblock;
	if (queuedCount == 0 || *len < queued[0].len) return -1;
	memcpy(srcMacAddr, queued[0].mac, 6);
	*port = 0x309;
	memcpy(data, queued[0].data, queued[0].len);
	*len = queued[0].len;
	memmove(&queued[0], &queued[1], --queuedCount * sizeof(queued[0]));
	return *len;
}

int sceNetAdhocGetPdpStat(int *size, pdpStatStruct *stat) {
	int i;
	(void)size;
	stat->rcvdData = 0;
	for (i = 0; i < queuedCount; i++)
		stat->rcvdData += queued[i].len;
	return 0;
}

int sceNetAdhocctlInit(int stacksize, int priority, struct productStruct *product) {
	(void)stacksize; (void)priority;
	memcpy(sentProduct, product->product, sizeof(sentProduct));
	return setup(0);
}

int sceNetAdhocctlTerm(void) {
	return teardown();
}

int sceNetAdhocctlConnect(const char *name) {
	(void)name;
	return setup(0);
}

int sceNetAdhocctlDisconnect(void) {
	return teardown();
}

int sceNetAdhocctlGetState(int *event) {
	*event = 1;
	return 0;
}

int sceNetAdhocMatchingInit(int memsize) {
	(void)memsize;
	return setup(0);
}

int sceNetAdhocMatchingTerm(void) {
	return teardown();
}

int sceNetAdhocMatchingCreate(int mode, int maxpeers, unsigned short port, int bufsize, unsigned int hellodelay, unsigned int pingdelay, int initcount, unsigned int msgdelay, pspAdhocMatchingCallback callback) {
	(void)mode; (void)maxpeers; (void)port; (void)bufsize; (void)hellodelay; (void)pingdelay; (void)initcount; (void)msgdelay; (void)callback;
	return setup(2);
}

int sceNetAdhocMatchingDelete(int matchingid) {
	(void)matchingid;
	return teardown();
}

int sceNetAdhocMatchingStart(int matchingid, int evthpri, int evthstack, int inthpri, int inthstack, int optlen, void *optdata) {
	(void)matchingid; (void)evthpri; (void)evthstack; (void)inthpri; (void)inthstack; (void)optlen; (void)optdata;
	return setup(0);
}

int sceNetAdhocMatchingStop(int matchingid) {
	(void)matchingid;
	return teardown();
}

int sceNetAdhocMatchingSelectTarget(int matchingid, unsigned char *mac, int optlen, void *optdata) {
	(void)matchingid; (void)mac; (void)optlen; (void)optdata;
	return 0;
}

int sceNetAdhocMatchingCancelTarget(int matchingid, unsigned char *mac) {
	(void)matchingid; (void)mac;
	return 0;
}

int sceUtilityGetSystemParamString(int id, char *str, int len) {
	(void)id;
	snprintf(str, len, "me");
	return 0;
}

// The product code is a fixed 9-byte field, not a C string: all 9 characters must reach the SDK
static void test_product_id_keeps_all_nine_characters(void) {
	CHECK_EQ(oslAdhocInit("ULUS99999"), 0);
	CHECK(memcmp(sentProduct, "ULUS99999", 9) == 0);
	oslAdhocTerm();
}

void _matchingCB(int unk1, int event, unsigned char *macSource, int size, void *data);

// Heap copies sized exactly like the SDK's, so ASan flags any read past them
static void join(u8 macByte, const char *nick, int len) {
	unsigned char *mac = malloc(6);
	char *data = len ? malloc(len) : NULL;
	memset(mac, macByte, 6);
	if (len) memcpy(data, nick, len);
	_matchingCB(0, MATCHING_JOINED, mac, len, data);
	free(data);
	free(mac);
}

// The nickname arrives as optlen bytes without a NUL; the MAC is 6 bytes, not 8
static void test_join_reads_only_the_given_bytes(void) {
	CHECK_EQ(oslAdhocInit("ULUS99999"), 0);
	join(0xA1, "Alice", 5);
	join(0xB2, NULL, 0);
	CHECK_EQ(oslAdhocGetRemotePspCount(), 2);
	CHECK(strcmp(oslAdhocGetPspByIndex(0)->name, "Alice") == 0);
	CHECK(strcmp(oslAdhocGetPspByIndex(1)->name, "") == 0);
	CHECK_EQ(oslAdhocGetPspByIndex(0)->macAddress[5], 0xA1);
	CHECK_EQ(oslAdhocGetPspByIndex(0)->connectionState, OSL_ADHOC_JOINED);
	oslAdhocTerm();
}

static void leave(u8 macByte) {
	unsigned char mac[6];
	memset(mac, macByte, 6);
	_matchingCB(0, MATCHING_DISCONNECT, mac, 0, NULL);
}

// A full list must neither take one PSP more nor read past its end when compacting
static void test_peer_list_stays_in_bounds(void) {
	int i;
	CHECK_EQ(oslAdhocInit("ULUS99999"), 0);
	for (i = 0; i <= MAX_REMOTEPSP; i++)
		join((u8)i, "x", 1);
	CHECK_EQ(oslAdhocGetRemotePspCount(), MAX_REMOTEPSP);

	leave(MAX_REMOTEPSP - 1);
	leave(0);
	CHECK_EQ(oslAdhocGetRemotePspCount(), MAX_REMOTEPSP - 2);
	CHECK_EQ(oslAdhocGetPspByIndex(0)->macAddress[0], 1);
	CHECK_EQ(oslAdhocGetPspByIndex(MAX_REMOTEPSP - 3)->macAddress[0], MAX_REMOTEPSP - 2);
	CHECK(oslAdhocGetPspByIndex(MAX_REMOTEPSP - 2) == NULL);
	oslAdhocTerm();
}

// A kept pointer names the same PSP while others come and go; one that left reads as disconnected
static void test_peer_pointers_stay_with_their_psp(void) {
	int i;
	CHECK_EQ(oslAdhocInit("ULUS99999"), 0);
	join(0xA1, "Alice", 5);
	join(0xB2, "Bob", 3);
	struct remotePsp *alice = oslAdhocGetPspByIndex(0);
	struct remotePsp *bob = oslAdhocGetPspByIndex(1);

	leave(0xA1);
	CHECK(strcmp(bob->name, "Bob") == 0);
	CHECK_EQ(alice->connectionState, OSL_ADHOC_DISCONNECTED);
	CHECK_EQ(oslAdhocGetRemotePspCount(), 1);
	CHECK(oslAdhocGetPspByIndex(0) == bob);
	CHECK(oslAdhocGetPspByIndex(-1) == NULL);
	CHECK(oslAdhocGetPspByMacAddress(alice->macAddress) == NULL);

	join(0xC3, "Carol", 5);
	CHECK(strcmp(alice->name, "Alice") == 0);
	join(0xA1, "Alice", 5);
	CHECK(oslAdhocGetPspByMacAddress(alice->macAddress) == alice);
	CHECK_EQ(alice->connectionState, OSL_ADHOC_JOINED);
	CHECK_EQ(oslAdhocGetRemotePspCount(), 3);

	// shortcut: with every slot taken, a newcomer reuses a departed PSP's slot
	for (i = 3; i < MAX_REMOTEPSP; i++)
		join((u8)i, "x", 1);
	leave(0xB2);
	join(0xD4, "Dave", 4);
	CHECK_EQ(oslAdhocGetRemotePspCount(), MAX_REMOTEPSP);
	CHECK(oslAdhocGetPspByMacAddress(bob->macAddress) == bob);
	CHECK(strcmp(bob->name, "Dave") == 0);
	oslAdhocTerm();
}

// One call returns one datagram from the given PSP, and never rewrites that PSP's MAC
static void test_receive_returns_one_datagram_from_the_peer(void) {
	char buffer[16] = "";
	CHECK_EQ(oslAdhocInit("ULUS99999"), 0);
	join(0xA1, "Alice", 5);
	struct remotePsp *alice = oslAdhocGetPspByIndex(0);

	queue(0xA1, "hi");
	queue(0xB2, "stranger");
	queue(0xA1, "again");
	CHECK_EQ(oslAdhocReceiveData(alice, buffer, sizeof(buffer)), 2);
	CHECK(memcmp(buffer, "hi", 2) == 0);
	CHECK_EQ(oslAdhocReceiveData(alice, buffer, sizeof(buffer)), 0);
	CHECK_EQ(alice->macAddress[0], 0xA1);
	CHECK(oslAdhocGetPspByMacAddress(alice->macAddress) == alice);
	CHECK_EQ(oslAdhocReceiveData(alice, buffer, sizeof(buffer)), 5);
	CHECK(memcmp(buffer, "again", 5) == 0);
	CHECK_EQ(oslAdhocReceiveData(alice, buffer, sizeof(buffer)), 0);
	oslAdhocTerm();
}

// Whichever setup step fails, init undoes the earlier ones so it can be retried
static void test_failed_init_is_torn_down(void) {
	int stage;
	for (stage = 1; stage <= 8; stage++) {
		setupCalls = 0;
		failStage = stage;
		CHECK(oslAdhocInit("ULUS99999") < 0);
		CHECK_EQ(oslAdhocGetState(), ADHOC_UNINIT);
		CHECK_EQ(opened, 0);
	}
	failStage = 0;
	CHECK_EQ(oslAdhocInit("ULUS99999"), 0);
	CHECK_EQ(opened, 8);
	oslAdhocTerm();
	CHECK_EQ(opened, 0);
}

int main(void) {
	test_product_id_keeps_all_nine_characters();
	test_join_reads_only_the_given_bytes();
	test_peer_list_stays_in_bounds();
	test_peer_pointers_stay_with_their_psp();
	test_receive_returns_one_datagram_from_the_peer();
	test_failed_init_is_torn_down();
	return TEST_RESULT();
}
