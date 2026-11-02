#ifndef OSL_HOST_PSPNET_ADHOC_H
#define OSL_HOST_PSPNET_ADHOC_H

typedef struct pdpStatStruct {
	struct pdpStatStruct *next;
	int pdpId;
	unsigned char mac[6];
	unsigned short port;
	unsigned int rcvdData;
} pdpStatStruct;

int sceNetAdhocInit(void);
int sceNetAdhocTerm(void);
int sceNetAdhocPdpCreate(unsigned char *mac, unsigned short port, unsigned int bufsize, int unk1);
int sceNetAdhocPdpDelete(int id, int unk1);
int sceNetAdhocPdpSend(int id, unsigned char *destMacAddr, unsigned short port, void *data, unsigned int len, unsigned int timeout, int nonblock);
int sceNetAdhocPdpRecv(int id, unsigned char *srcMacAddr, unsigned short *port, void *data, void *dataLength, unsigned int timeout, int nonblock);
int sceNetAdhocGetPdpStat(int *size, pdpStatStruct *stat);

#endif
