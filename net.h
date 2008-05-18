#ifndef NET_H
#define NET_H

/** @defgroup Network

	Network functions
	@{
*/

#define OSL_NET_ERROR_NET		-1
#define OSL_NET_ERROR_INET		-2
#define OSL_NET_ERROR_RESOLVER	-3
#define OSL_NET_ERROR_APCTL		-4
#define OSL_NET_ERROR_SSL		-5
#define OSL_NET_ERROR_HTTP		-6
#define OSL_NET_ERROR_HTTPS		-7
#define OSL_NET_ERROR_CERT		-8
#define OSL_NET_ERROR_COOKIE	-9

#define OSL_ERR_APCTL_GETINFO   -10
#define OSL_ERR_APCTL_CONNECT	-11
#define OSL_ERR_APCTL_TIMEOUT	-12
#define OSL_ERR_APCTL_GETSTATE	-13

#define OSL_ERR_RESOLVER_CREATE		-14
#define OSL_ERR_RESOLVER_RESOLVING	-15


#define OSL_MAX_NET_CONFIGS		20
struct oslNetConfig{
    char name[128];
    char IP[128];
};

#define OSL_RESOLVER_RETRY		4

/**Checks if the WLAN switch is powered on (returns 1, else 0).
*/
int oslIsWlanPowerOn();

/**Gets the network configs.
\code
struct oslNetConfig configs[OSL_MAX_NET_CONFIGS];
int numconfigs = oslGetNetConfigs(configs);
\endcode
*/
int oslGetNetConfigs(struct oslNetConfig *result);

/**Inits network
*/
int oslNetInit();

/**Terminates network
*/
int oslNetTerm();

/**Gets the current IP address (when connected to an Access Point)
*/
int oslGetIPaddress(char *IPaddress);

/**Tries to connect to an Access Point using the given configuration
	\param int config
        Index of the configuration to use
	\param int timeout
        Timeout (in seconds) for the connection
	\param int (*apctlCallback)(int state)
        Pointer to a callback function. It will be called with the current state
*/
int oslConnectToAP(int config, int timeout, int (*apctlCallback)(int state));

/**Disconnectes from the Access Point
*/
int oslDisconnectFromAP();

/**Gets the current Access Point state
*/
int oslGetAPState();

/**Resolves an address to its IP address
*/
int oslResolveAddress(char *address, char *resolvedIP);

/** @} */ // end of net
#endif
