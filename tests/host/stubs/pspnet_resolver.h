/* Host-side stand-in for the resolver calls used by net.c; the test defines them. */
#ifndef OSL_HOST_PSPNET_RESOLVER_H
#define OSL_HOST_PSPNET_RESOLVER_H

int sceNetResolverInit(void);
int sceNetResolverTerm(void);

#endif
