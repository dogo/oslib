/* Host-side stand-in for the SSL calls used by net.c; the test defines them. */
#ifndef OSL_HOST_PSPSSL_H
#define OSL_HOST_PSPSSL_H

int sceSslInit(int unknown1);
int sceSslEnd(void);

#endif
