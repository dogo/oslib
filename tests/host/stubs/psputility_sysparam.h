#ifndef OSL_HOST_PSPUTILITY_SYSPARAM_H
#define OSL_HOST_PSPUTILITY_SYSPARAM_H

#define PSP_SYSTEMPARAM_ID_STRING_NICKNAME 1

int sceUtilityGetSystemParamString(int id, char *str, int len);

#endif
