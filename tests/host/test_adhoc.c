#include <pspkernel.h>
#include <pspsdk.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <pspnet_adhoc.h>
#include <pspnet_adhocctl.h>
#include <pspnet_adhocmatching.h>
#include <psputility_sysparam.h>
#include <string.h>

#include "pspadhoc.h"
#include "test.h"

// Fake network stack: every call succeeds and the control link connects at once
static char sentProduct[sizeof(((struct productStruct *)0)->product)];

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
	return 0;
}

int sceNetTerm(void) {
	return 0;
}

int sceNetAdhocInit(void) {
	return 0;
}

int sceNetAdhocTerm(void) {
	return 0;
}

int sceNetAdhocPdpCreate(unsigned char *mac, unsigned short port, unsigned int bufsize, int unk1) {
	(void)mac; (void)port; (void)bufsize; (void)unk1;
	return 1;
}

int sceNetAdhocPdpDelete(int id, int unk1) {
	(void)id; (void)unk1;
	return 0;
}

int sceNetAdhocPdpSend(int id, unsigned char *destMacAddr, unsigned short port, void *data, unsigned int len, unsigned int timeout, int nonblock) {
	(void)id; (void)destMacAddr; (void)port; (void)data; (void)len; (void)timeout; (void)nonblock;
	return 0;
}

int sceNetAdhocPdpRecv(int id, unsigned char *srcMacAddr, unsigned short *port, void *data, void *dataLength, unsigned int timeout, int nonblock) {
	(void)id; (void)srcMacAddr; (void)port; (void)data; (void)dataLength; (void)timeout; (void)nonblock;
	return 0;
}

int sceNetAdhocGetPdpStat(int *size, pdpStatStruct *stat) {
	(void)size; (void)stat;
	return 0;
}

int sceNetAdhocctlInit(int stacksize, int priority, struct productStruct *product) {
	(void)stacksize; (void)priority;
	memcpy(sentProduct, product->product, sizeof(sentProduct));
	return 0;
}

int sceNetAdhocctlTerm(void) {
	return 0;
}

int sceNetAdhocctlConnect(const char *name) {
	(void)name;
	return 0;
}

int sceNetAdhocctlDisconnect(void) {
	return 0;
}

int sceNetAdhocctlGetState(int *event) {
	*event = 1;
	return 0;
}

int sceNetAdhocMatchingInit(int memsize) {
	(void)memsize;
	return 0;
}

int sceNetAdhocMatchingTerm(void) {
	return 0;
}

int sceNetAdhocMatchingCreate(int mode, int maxpeers, unsigned short port, int bufsize, unsigned int hellodelay, unsigned int pingdelay, int initcount, unsigned int msgdelay, pspAdhocMatchingCallback callback) {
	(void)mode; (void)maxpeers; (void)port; (void)bufsize; (void)hellodelay; (void)pingdelay; (void)initcount; (void)msgdelay; (void)callback;
	return 2;
}

int sceNetAdhocMatchingDelete(int matchingid) {
	(void)matchingid;
	return 0;
}

int sceNetAdhocMatchingStart(int matchingid, int evthpri, int evthstack, int inthpri, int inthstack, int optlen, void *optdata) {
	(void)matchingid; (void)evthpri; (void)evthstack; (void)inthpri; (void)inthstack; (void)optlen; (void)optdata;
	return 0;
}

int sceNetAdhocMatchingStop(int matchingid) {
	(void)matchingid;
	return 0;
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

int main(void) {
	test_product_id_keeps_all_nine_characters();
	return TEST_RESULT();
}
