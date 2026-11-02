// PSP adhoc functions.
// based on pspZorba's sample
// www.pspZorba.com
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_adhoc.h>
#include <pspnet_adhocctl.h>
#include <pspnet_adhocmatching.h>
#include <psputility_sysparam.h>
#include <string.h>

#include "pspadhoc.h"

static int state = ADHOC_UNINIT;
static u8 myMacAddress[8];
static struct productStruct product;
static int matchingHD = 0;
static int pdpHD = 0;
static int port = 0;

// Entries never move, so a remotePsp pointer keeps naming the same PSP; one that
// left stays in its slot as OSL_ADHOC_DISCONNECTED
static struct remotePsp allRemotePSP[MAX_REMOTEPSP];
static int allRemotePSPCount; // PSPs present
static int usedRemotePSPSlots; // slots filled since init, present or not

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions:
////////////////////////////////////////////////////////////////////////////////////////////////////////
int _getMacAddress() {
	return sceWlanGetEtherAddr(myMacAddress);
}

int _loadModule(const char *prx, int mode) {
	return pspSdkLoadStartModule(prx, mode);
}

int _loadModules() {
	int ret;
	ret = _loadModule("flash0:/kd/ifhandle.prx", PSP_MEMORY_PARTITION_KERNEL);
	if (ret < 0) return -1;
	ret = _loadModule("flash0:/kd/memab.prx", PSP_MEMORY_PARTITION_KERNEL);
	if (ret < 0) return -1;
	ret = _loadModule("flash0:/kd/pspnet_adhoc_auth.prx", PSP_MEMORY_PARTITION_KERNEL);
	if (ret < 0) return -1;
	ret = _loadModule("flash0:/kd/pspnet.prx", PSP_MEMORY_PARTITION_USER);
	if (ret < 0) return -1;
	ret = _loadModule("flash0:/kd/pspnet_adhoc.prx", PSP_MEMORY_PARTITION_USER);
	if (ret < 0) return -1;
	ret = _loadModule("flash0:/kd/pspnet_adhocctl.prx", PSP_MEMORY_PARTITION_USER);
	if (ret < 0) return -1;
	ret = _loadModule("flash0:/kd/pspnet_adhoc_matching.prx", PSP_MEMORY_PARTITION_USER);
	if (ret < 0) return -1;
	return 0;
}

int _libNetInit() {
	return sceNetInit(0x20000, 0x20, 0x1000, 0x20, 0x1000);
	// return sceNetInit(128*1024, 42, 4*1024, 42, 4*1024);
}

int _libAdhocInit() {
	return sceNetAdhocInit();
}

int _libAdhocctlInit() {
	return sceNetAdhocctlInit(0x2000, 0x20, &product);
}

int _adhocctlConnect() {
	return sceNetAdhocctlConnect(NULL);
}

void _adhocctlState() {
	int encore = 1, s;
	while (encore) {
		sceNetAdhocctlGetState(&s);
		if (s == 1) {
			encore = 0; /* connected */
		} else {
			sceKernelDelayThread(50 * 1000); // 50ms
		}
	}
}

int _pdpCreate() {
	pdpHD = sceNetAdhocPdpCreate(myMacAddress, port, 0x400, 0);
	return pdpHD;
}

int _matchingInit() {
	return sceNetAdhocMatchingInit(0x20000);
}

void _matchingCB(int unk1, int event, unsigned char *macSource, int size, void *data);
int _createMatching() {
	matchingHD = sceNetAdhocMatchingCreate(3, 0xa, 0x22b, 0x800, 0x2dc6c0, 0x5b8d80, 3, 0x7a120, _matchingCB);
	return matchingHD;
}

int _startMatching() {
	char name[50] = "";
	sceUtilityGetSystemParamString(PSP_SYSTEMPARAM_ID_STRING_NICKNAME, name, 50);

	return sceNetAdhocMatchingStart(matchingHD, 0x10, 0x2000, 0x10, 0x2000, strlen(name), name);
}

struct remotePsp *_findSlotByMacAddress(const u8 aMacAddress[6]) {
	int i = 0;
	for ( i = 0; i < usedRemotePSPSlots; i++) {
		if (memcmp(aMacAddress, allRemotePSP[i].macAddress, 6 * sizeof(u8)) == 0)
			return &allRemotePSP[i];
	}
	return NULL;
}

struct remotePsp *_findByMacAddress(const u8 aMacAddress[6]) {
	struct remotePsp *pPsp = _findSlotByMacAddress(aMacAddress);
	return (pPsp != NULL && pPsp->connectionState != OSL_ADHOC_DISCONNECTED) ? pPsp : NULL;
}

struct remotePsp *_findByState(int remotePspState) {
	int i = 0;
	for ( i = 0; i < usedRemotePSPSlots; i++) {
		if (allRemotePSP[i].connectionState == remotePspState)
			return &allRemotePSP[i];
	}
	return NULL;
}

struct remotePsp *_newSlot(const u8 aMacAddress[6]) {
	int i = 0;
	struct remotePsp *pPsp = _findSlotByMacAddress(aMacAddress); // a PSP coming back gets its old slot
	if (pPsp != NULL) return pPsp;
	if (usedRemotePSPSlots < MAX_REMOTEPSP) return &allRemotePSP[usedRemotePSPSlots++];
	// shortcut: once every slot was filled a newcomer reuses a departed PSP's slot, so a pointer kept to that PSP names the newcomer; add a generation counter if that matters
	for ( i = 0; i < usedRemotePSPSlots; i++) {
		if (allRemotePSP[i].connectionState == OSL_ADHOC_DISCONNECTED)
			return &allRemotePSP[i];
	}
	return NULL;
}

void _matchingCB(int unk1, int event, unsigned char *macSource, int size, void *data) {
	struct remotePsp *pPsp = _findByMacAddress( (u8 *)macSource);

	switch (event) {
	case MATCHING_JOINED: // A psp has joined
		if (pPsp == NULL && (pPsp = _newSlot(macSource)) != NULL) { // it's a new PSP, we add it to the list
			memset(pPsp, 0, sizeof(*pPsp));
			memcpy(pPsp->macAddress, macSource, 6 * sizeof(u8));
			// the nickname is size bytes, not NUL-terminated
			if (data != NULL && size > 0)
				memcpy(pPsp->name, data, size < (int)sizeof(pPsp->name) ? size : (int)sizeof(pPsp->name) - 1);
			pPsp->connectionState = OSL_ADHOC_JOINED;
			allRemotePSPCount++;
		}
		break;
	case MATCHING_DISCONNECT: // A psp has given up
		if (pPsp != NULL) {
			pPsp->connectionState = OSL_ADHOC_DISCONNECTED;
			allRemotePSPCount--;
		}
		break;
	case MATCHING_SELECTED: // A PSP has selected me
		if (pPsp != NULL) {
			pPsp->connectionState = OSL_ADHOC_SELECTED;
		}
		break;
	case MATCHING_REJECTED:
		if (pPsp != NULL) {
			pPsp->connectionState = OSL_ADHOC_REJECTED;
		}
		break;
	case MATCHING_CANCELED:
		if (pPsp != NULL) {
			pPsp->connectionState = OSL_ADHOC_CANCELED;
		}
		break;
	case MATCHING_ESTABLISHED:
		if (pPsp != NULL) {
			pPsp->connectionState = OSL_ADHOC_ESTABLISHED;
		}
		break;
	case MATCHING_ACCEPTED:  // connection accepted
		if (pPsp != NULL) {
			pPsp->connectionState = OSL_ADHOC_ACCEPTED;
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions:
////////////////////////////////////////////////////////////////////////////////////////////////////////
int oslAdhocInit(char *productID) {
	if (state != ADHOC_UNINIT) return 0;

	memset(myMacAddress, 0, 8 * sizeof(u8));

	// product is a fixed 9-byte field (e.g. "ULUS99999"), not a NUL-terminated string
	memset(product.product, 0, sizeof(product.product));
	memcpy(product.product, productID, strnlen(productID, sizeof(product.product)));

	product.unknown = 0;
	matchingHD = -1;
	pdpHD = -1;
	port = 0x309;
	allRemotePSPCount = 0;
	usedRemotePSPSlots = 0;

	if (sceWlanDevIsPowerOn() != 1)
		return ADHOC_ERROR_WLAN;

	if (_getMacAddress() < 0)
		return ADHOC_ERROR_MAC;

	_loadModules();

	if (_libNetInit() < 0)
		return ADHOC_ERROR_NET_INIT;

	// oslAdhocTerm() only runs once fully initialized, so undo a partial init here
	int ret;
	if (_libAdhocInit() < 0) {
		ret = ADHOC_ERROR_INIT;
		goto net_term;
	}

	if (_libAdhocctlInit() < 0) {
		ret = ADHOC_ERROR_CTL_INIT;
		goto adhoc_term;
	}

	if (_adhocctlConnect()) {
		ret = ADHOC_ERROR_CTL_CONNECT;
		goto ctl_term;
	}

	_adhocctlState();

	if (_pdpCreate() < 0) {
		ret = ADHOC_ERROR_PDP_CREATE;
		goto ctl_disconnect;
	}

	if (_matchingInit() < 0) {
		ret = ADHOC_ERROR_MATCHING_INIT;
		goto pdp_delete;
	}

	if (_createMatching() < 0) {
		ret = ADHOC_ERROR_MATCHING_CREATE;
		goto matching_term;
	}

	if (_startMatching() < 0) {
		ret = ADHOC_ERROR_MATCHING_START;
		goto matching_delete;
	}

	state = ADHOC_INIT;
	return 0;

matching_delete:
	sceNetAdhocMatchingDelete(matchingHD);
matching_term:
	sceNetAdhocMatchingTerm();
pdp_delete:
	sceNetAdhocPdpDelete(pdpHD, 0);
ctl_disconnect:
	sceNetAdhocctlDisconnect();
ctl_term:
	sceNetAdhocctlTerm();
adhoc_term:
	sceNetAdhocTerm();
net_term:
	sceNetTerm();
	matchingHD = -1;
	pdpHD = -1;
	return ret;
}

u8 *oslAdhocGetMacAddress() {
	return myMacAddress;
}

int oslAdhocGetRemotePspCount() {
	return allRemotePSPCount;
}

struct remotePsp *oslAdhocGetPspByMacAddress(const u8 aMacAddress[6]) {
	return _findByMacAddress(aMacAddress);
}

struct remotePsp *oslAdhocFindRemotePspByMacAddress(const u8 aMacAddress[6]) {
	return _findByMacAddress(aMacAddress);
}

struct remotePsp *oslAdhocGetPspByIndex(int index) {
	int i = 0;
	for ( i = 0; i < usedRemotePSPSlots; i++) {
		if (allRemotePSP[i].connectionState != OSL_ADHOC_DISCONNECTED && index-- == 0)
			return &allRemotePSP[i];
	}
	return NULL;
}

int oslAdhocRequestConnection(struct remotePsp *aPsp, int timeOut, int (*requestConnectionCB)(int aPspState)) {
	// oslAdhocGetPspByIndex() returns NULL for an unknown index, and the loop
	// below already guards against it, so reject it before the first use too.
	if (aPsp == NULL) return -1;

	int ret = sceNetAdhocMatchingSelectTarget(matchingHD, (unsigned char *)aPsp->macAddress, 0, 0);
	if (ret < 0) return ret;

	int quit = 0;
	time_t startTime;
	time_t currentTime;
	sceKernelLibcTime(&startTime);

	while (!quit) {
		if (aPsp->connectionState == OSL_ADHOC_DISCONNECTED)
			return -1;

		if (requestConnectionCB != NULL)
			quit = requestConnectionCB(aPsp->connectionState);

		// connection accepted
		if ((aPsp->connectionState == OSL_ADHOC_ACCEPTED) || (aPsp->connectionState == OSL_ADHOC_ESTABLISHED)) return aPsp->connectionState;

		// connection not accepted
		if (aPsp->connectionState == OSL_ADHOC_REJECTED) {
			aPsp->connectionState = OSL_ADHOC_JOINED; // reset the state flag
			return aPsp->connectionState;
		}
		sceKernelLibcTime(&currentTime);
		if (timeOut > 0 && currentTime - startTime >= timeOut)
			break;
		sceKernelDelayThread(1000 * 1000); // 1 sec
	}
	return -1;
}

int oslAdhocSendData(struct remotePsp *pPsp, void *data, int lenData) {
	return sceNetAdhocPdpSend(pdpHD, pPsp->macAddress, port, data, lenData, 0, 0);
}

int oslAdhocReceiveData(struct remotePsp *pPsp, void *data, int maxLen) {
	pdpStatStruct aStat;
	aStat.next = NULL;
	int sizeStat = sizeof(pdpStatStruct);
	unsigned int sizeData = maxLen;
	u8 sourceMac[8];

	int ret = sceNetAdhocGetPdpStat(&sizeStat, &aStat);
	if (ret < 0) return ret;

	// rcvdData is everything queued; one recv returns one datagram, from any PSP
	if (aStat.rcvdData == 0) return 0;
	ret = sceNetAdhocPdpRecv(pdpHD, sourceMac, &(aStat.port), data, &sizeData, 0, 1);
	if (ret < 0)
		return ret;
	if (memcmp(sourceMac, pPsp->macAddress, 6 * sizeof(u8)) != 0)
		return 0; // sent by another PSP: dropped
	return sizeData;
}

struct remotePsp *oslAdhocGetConnectionRequest() {
	return _findByState(OSL_ADHOC_SELECTED);
}

void oslAdhocRejectConnection(struct remotePsp *aPsp) {
	sceNetAdhocMatchingCancelTarget(matchingHD, (unsigned char *)aPsp->macAddress);
	aPsp->connectionState = OSL_ADHOC_JOINED;
}

void oslAdhocAcceptConnection(struct remotePsp *aPsp) {
	sceNetAdhocMatchingSelectTarget(matchingHD, (unsigned char *)aPsp->macAddress, 0, 0);
}

void oslAdhocTerm() {
	if (state != ADHOC_INIT) return;

	sceNetAdhocctlDisconnect();
	sceNetAdhocPdpDelete(pdpHD, 0);
	sceNetAdhocMatchingStop(matchingHD);
	sceNetAdhocMatchingDelete(matchingHD);
	sceNetAdhocMatchingTerm();
	sceNetAdhocctlTerm();
	sceNetAdhocTerm();
	sceNetTerm();

	memset(myMacAddress, 0, 8 * sizeof(u8));
	matchingHD = 0;
	pdpHD = 0;
	port = 0;
	state = ADHOC_UNINIT;
}

int oslAdhocGetState() {
	return state;
}
