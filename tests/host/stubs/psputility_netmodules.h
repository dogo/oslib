/* Host-side stand-in: the network module ids net.c loads. */
#ifndef OSL_HOST_PSPUTILITY_NETMODULES_H
#define OSL_HOST_PSPUTILITY_NETMODULES_H

#define PSP_NET_MODULE_COMMON 1
#define PSP_NET_MODULE_ADHOC 2
#define PSP_NET_MODULE_INET 3
#define PSP_NET_MODULE_PARSEURI 4
#define PSP_NET_MODULE_PARSEHTTP 5
#define PSP_NET_MODULE_HTTP 6
#define PSP_NET_MODULE_SSL 7

int sceUtilityLoadNetModule(int module);
int sceUtilityUnloadNetModule(int module);

#endif
