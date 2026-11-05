/* Host-side stand-in: the adhoc source includes it for nothing; net.c inits and terms inet. */
#ifndef OSL_HOST_PSPNET_INET_H
#define OSL_HOST_PSPNET_INET_H

int sceNetInetInit(void);
int sceNetInetTerm(void);

#endif
