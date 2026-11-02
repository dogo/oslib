#ifndef OSL_HOST_PSPNET_ADHOCMATCHING_H
#define OSL_HOST_PSPNET_ADHOCMATCHING_H

typedef void (*pspAdhocMatchingCallback)(int matchingid, int event, unsigned char *mac, int optlen, void *optdata);

int sceNetAdhocMatchingInit(int memsize);
int sceNetAdhocMatchingTerm(void);
int sceNetAdhocMatchingCreate(int mode, int maxpeers, unsigned short port, int bufsize, unsigned int hellodelay, unsigned int pingdelay, int initcount, unsigned int msgdelay, pspAdhocMatchingCallback callback);
int sceNetAdhocMatchingDelete(int matchingid);
int sceNetAdhocMatchingStart(int matchingid, int evthpri, int evthstack, int inthpri, int inthstack, int optlen, void *optdata);
int sceNetAdhocMatchingStop(int matchingid);
int sceNetAdhocMatchingSelectTarget(int matchingid, unsigned char *mac, int optlen, void *optdata);
int sceNetAdhocMatchingCancelTarget(int matchingid, unsigned char *mac);

#endif
