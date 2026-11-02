#ifndef OSL_HOST_PSPNET_ADHOCCTL_H
#define OSL_HOST_PSPNET_ADHOCCTL_H

struct productStruct {
	int unknown;
	char product[9];
};

int sceNetAdhocctlInit(int stacksize, int priority, struct productStruct *product);
int sceNetAdhocctlTerm(void);
int sceNetAdhocctlConnect(const char *name);
int sceNetAdhocctlDisconnect(void);
int sceNetAdhocctlGetState(int *event);

#endif
