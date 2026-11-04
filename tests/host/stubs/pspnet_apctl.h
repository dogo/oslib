/* Host-side stand-in for the access point calls used by net.c; the test defines them. */
#ifndef OSL_HOST_PSPNET_APCTL_H
#define OSL_HOST_PSPNET_APCTL_H

#define PSP_NET_APCTL_STATE_DISCONNECTED 0
#define PSP_NET_APCTL_STATE_SCANNING 1
#define PSP_NET_APCTL_STATE_JOINING 2
#define PSP_NET_APCTL_STATE_GETTING_IP 3
#define PSP_NET_APCTL_STATE_GOT_IP 4
#define PSP_NET_APCTL_STATE_EAP_AUTH 5
#define PSP_NET_APCTL_STATE_KEY_EXCHANGE 6
#define PSP_NET_APCTL_INFO_IP 8

union SceNetApctlInfo {
	char name[64];
	char ip[16];
};

int sceNetApctlInit(int stackSize, int initPriority);
int sceNetApctlTerm(void);
int sceNetApctlConnect(int connIndex);
int sceNetApctlDisconnect(void);
int sceNetApctlGetState(int *pState);
int sceNetApctlGetInfo(int code, union SceNetApctlInfo *pInfo);

#endif
