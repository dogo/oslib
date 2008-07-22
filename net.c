#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <psputility.h>
#include <psputility_netmodules.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <psphttp.h>
#include <pspssl.h>
#include <arpa/inet.h>

#include "oslib.h"
#include "net.h"

int oslLoadNetModules()
{
	sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
	sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
	sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEURI);
	sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEHTTP);
	sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP);
	sceUtilityLoadNetModule(PSP_NET_MODULE_SSL);
	return 0;
}


int oslUnloadNetModules()
{
	sceUtilityUnloadNetModule(PSP_NET_MODULE_SSL);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_HTTP);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_PARSEHTTP);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_PARSEURI);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Public API
///////////////////////////////////////////////////////////////////////////////////////////////////
int oslIsWlanPowerOn(){
    return sceWlanDevIsPowerOn();
}


int oslIsWlanConnected(){
	char ipaddr[32];

	if(sceNetApctlGetInfo(8, ipaddr) == 0)
		return 1;
	else
		return 0;
}

int oslGetNetConfigs(struct oslNetConfig *result){
    int index = 0;
    netData name, ip;

    for (index=1; index<OSL_MAX_NET_CONFIGS; index++){
       if (sceUtilityCheckNetParam(index))
          break;

       sceUtilityGetNetParam(index, PSP_NETPARAM_NAME, &name);
       sceUtilityGetNetParam(index, PSP_NETPARAM_IP, &ip);

       strcpy(result[index - 1].name, name.asString);
       strcpy(result[index - 1].IP, ip.asString);
    }
    return index - 1;
}


int oslNetInit()
{
	int res;

	oslLoadNetModules();
	//res = sceNetInit(0x20000, 0x2A, 0, 0x2A, 0);
	res = sceNetInit(0x20000, 0x20, 0x1000, 0x20, 0x1000);
	if (res < 0)
		return OSL_NET_ERROR_NET;

	res = sceNetInetInit();
	if (res < 0) {
		oslNetTerm();
		return OSL_NET_ERROR_INET;
	}

	res = sceNetResolverInit();
	if (res < 0)
	{
		oslNetTerm();
		return OSL_NET_ERROR_RESOLVER;
	}

	//res = sceNetApctlInit(0x1800, 0x30);
	res = sceNetApctlInit(0x1400, 0x42);
	if (res < 0)
	{
		oslNetTerm();
		return OSL_NET_ERROR_APCTL;
	}

	res = sceSslInit(0x28000);
	if (res < 0)
	{
		oslNetTerm();
		return OSL_NET_ERROR_SSL;
	}

	res = sceHttpInit(0x25800);
	if (res < 0)
	{
		oslNetTerm();
		return OSL_NET_ERROR_HTTP;
	}

	res = sceHttpsInit(0, 0, 0, 0);
	if (res < 0)
	{
		oslNetTerm();
		return OSL_NET_ERROR_HTTPS;
	}

	res = sceHttpsLoadDefaultCert(0, 0);
	if (res < 0)
	{
		oslNetTerm();
		return OSL_NET_ERROR_CERT;
	}

	res = sceHttpLoadSystemCookie();
	if (res < 0)
	{
		oslNetTerm();
		return OSL_NET_ERROR_COOKIE;
	}
	return 0;
}


int oslNetTerm()
{
	sceHttpSaveSystemCookie();
	sceHttpsEnd();
	sceHttpEnd();
	sceSslEnd();
	sceNetApctlTerm();
	sceNetInetTerm();
	sceNetTerm();

	oslUnloadNetModules();
	return 0;
}

int oslGetIPaddress(char *IPaddress){
    strcpy(IPaddress, "");
    if (sceNetApctlGetInfo(8, IPaddress))
        return OSL_ERR_APCTL_GETINFO;
    return 0;
}


int oslConnectToAP(int config, int timeout,
                   int (*apctlCallback)(int state)){
	int err = 0;
	int stateLast = -1;

    if (!oslIsWlanPowerOn())
        return OSL_ERR_WLAN_OFF;

	err = sceNetApctlConnect(config);
	if (err)
		return OSL_ERR_APCTL_CONNECT;

    time_t startTime;
    time_t currTime;
    time(&startTime);
	while (1){
        //Check timeout:
        time(&currTime);
        if (currTime - startTime >= timeout){
            if (apctlCallback != NULL)
                (*apctlCallback)(OSL_ERR_APCTL_TIMEOUT);
            oslDisconnectFromAP();
            err = OSL_ERR_APCTL_TIMEOUT;
            break;
        }
		int state;
		err = sceNetApctlGetState(&state);
		if (err){
            if (apctlCallback != NULL)
                (*apctlCallback)(OSL_ERR_APCTL_GETSTATE);
            oslDisconnectFromAP();
            err = OSL_ERR_APCTL_GETSTATE;
            break;
        }
		if (state > stateLast){
			stateLast = state;
            if (apctlCallback != NULL){
                if ((*apctlCallback)(state)){
                    err = OSL_USER_ABORTED;
                    break;
                }
            }
        }
		if (state == 4)
			break;  // connected with static IP
		sceKernelDelayThread(50*1000);
	}

	return err;
}

int oslDisconnectFromAP(){
    sceNetApctlDisconnect();
    return 0;
}

int oslGetAPState(){
    int state;
    int err = sceNetApctlGetState(&state);
    if (err)
        return OSL_ERR_APCTL_GETSTATE;
    return state;
}


int oslResolveAddress(char *address, char *resolvedIP){
    int rid = -1;
    char buf[1024];
    struct in_addr addr = { 0 };
	static struct in_addr saddr = { 0 };
	int err, i;

    for (i=0; i<OSL_RESOLVER_RETRY; i++){
        strcpy(resolvedIP, "");
        if(sceNetInetInetAton(address, &addr) == 0){
            if(sceNetResolverCreate(&rid, buf, sizeof(buf)) < 0)
                return OSL_ERR_RESOLVER_CREATE;

            err = sceNetResolverStartNtoA(rid, address, &saddr, 2, 3);
            sceNetResolverStop(rid);
            sceNetResolverDelete(rid);
            if(err < 0)
                return OSL_ERR_RESOLVER_RESOLVING;
            strcpy(resolvedIP, inet_ntoa(addr));
        }
    }


    return 0;
}
