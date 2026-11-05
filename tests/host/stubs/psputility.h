/* Host-side stand-in for the utility calls used by net.c; the test defines them. */
#ifndef OSL_HOST_PSPUTILITY_H
#define OSL_HOST_PSPUTILITY_H

#include <stdint.h>

#define PSP_NETPARAM_NAME 0
#define PSP_NETPARAM_IP 5

typedef union {
	uint32_t asUint;
	char asString[128];
} netData;

int sceUtilityCheckNetParam(int id);
int sceUtilityGetNetParam(int conf, int param, netData *data);

#endif
