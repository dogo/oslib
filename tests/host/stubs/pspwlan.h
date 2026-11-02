#ifndef OSL_HOST_PSPWLAN_H
#define OSL_HOST_PSPWLAN_H

#include <stdint.h>

int sceWlanDevIsPowerOn(void);
int sceWlanGetEtherAddr(uint8_t *etherAddr);

#endif
