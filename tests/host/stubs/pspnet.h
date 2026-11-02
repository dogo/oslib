#ifndef OSL_HOST_PSPNET_H
#define OSL_HOST_PSPNET_H

int sceNetInit(int poolsize, int calloutprio, int calloutstack, int netintrprio, int netintrstack);
int sceNetTerm(void);

#endif
