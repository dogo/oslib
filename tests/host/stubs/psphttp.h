/* Host-side stand-in for the PSP HTTP library used by net.c; the test defines the calls. */
#ifndef OSL_HOST_PSPHTTP_H
#define OSL_HOST_PSPHTTP_H

#include <stdint.h>

typedef uint64_t SceULong64;

#define PSP_HTTP_METHOD_GET 0
#define PSP_HTTP_METHOD_POST 1

int sceHttpInit(unsigned int unknown1);
int sceHttpEnd(void);
int sceHttpsInit(int unknown1, int unknown2, int unknown3, int unknown4);
int sceHttpsEnd(void);
int sceHttpsLoadDefaultCert(int unknown1, int unknown2);
int sceHttpLoadSystemCookie(void);
int sceHttpSaveSystemCookie(void);
int sceHttpCreateTemplate(char *agent, int unknown1, int unknown2);
int sceHttpDeleteTemplate(int templateid);
int sceHttpSetResolveTimeOut(int id, unsigned int timeout);
int sceHttpSetRecvTimeOut(int id, unsigned int timeout);
int sceHttpSetSendTimeOut(int id, unsigned int timeout);
int sceHttpCreateConnectionWithURL(int templateid, const char *url, int unknown1);
int sceHttpDeleteConnection(int connectionid);
int sceHttpEnableKeepAlive(int id);
int sceHttpCreateRequestWithURL(int connectionid, int method, char *url, SceULong64 contentlength);
int sceHttpDeleteRequest(int requestid);
int sceHttpAddExtraHeader(int id, char *name, char *value, int unknown1);
int sceHttpSendRequest(int requestid, void *data, unsigned int datasize);
int sceHttpGetStatusCode(int requestid, int *statuscode);
int sceHttpGetContentLength(int requestid, SceULong64 *contentlength);
int sceHttpReadData(int requestid, void *data, unsigned int datasize);

#endif
