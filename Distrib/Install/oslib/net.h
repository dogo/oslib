#ifndef NET_H
#define NET_H

#include <sys/select.h>
#include <pspnet_apctl.h>

/**
 * @defgroup Network Network
 * @{
 *
 * @brief Network functions for managing WLAN, AP connection, and socket operations on the PSP.
 */

/**
 * @name APCTL States
 * @{
 * @brief APCTL states returned by oslGetAPState().
 */
#define OSL_APCTL_STATE_DISCONNECTED  PSP_NET_APCTL_STATE_DISCONNECTED ///< Disconnected state
#define OSL_APCTL_STATE_SCANNING      PSP_NET_APCTL_STATE_SCANNING ///< Scanning for access points
#define OSL_APCTL_STATE_JOINING       PSP_NET_APCTL_STATE_JOINING ///< Joining access point
#define OSL_APCTL_STATE_GETTING_IP    PSP_NET_APCTL_STATE_GETTING_IP ///< Getting IP address
#define OSL_APCTL_STATE_GOT_IP        PSP_NET_APCTL_STATE_GOT_IP ///< Successfully got IP address
#define OSL_APCTL_STATE_EAP_AUTH      PSP_NET_APCTL_STATE_EAP_AUTH ///< EAP authentication in progress
#define OSL_APCTL_STATE_KEY_EXCHANGE  PSP_NET_APCTL_STATE_KEY_EXCHANGE ///< Key exchange in progress
/** @} */ // end of APCTL States

/**
 * @name Error Codes
 * @{
 * @brief Error codes returned by network functions.
 */
#define OSL_NET_ERROR_NET               -1 ///< Generic network error
#define OSL_NET_ERROR_INET              -2 ///< Internet error
#define OSL_NET_ERROR_RESOLVER  -3 ///< Resolver error
#define OSL_NET_ERROR_APCTL             -4 ///< APCTL error
#define OSL_NET_ERROR_SSL               -5 ///< SSL error
#define OSL_NET_ERROR_HTTP              -6 ///< HTTP error
#define OSL_NET_ERROR_HTTPS             -7 ///< HTTPS error
#define OSL_NET_ERROR_CERT              -8 ///< Certificate error
#define OSL_NET_ERROR_COOKIE    -9 ///< Cookie error

#define OSL_ERR_APCTL_GETINFO   -10 ///< Error getting APCTL info
#define OSL_ERR_APCTL_CONNECT   -11 ///< Error connecting to APCTL
#define OSL_ERR_APCTL_TIMEOUT   -12 ///< APCTL connection timeout
#define OSL_ERR_APCTL_GETSTATE  -13 ///< Error getting APCTL state

#define OSL_ERR_RESOLVER_CREATE         -14 ///< Error creating resolver
#define OSL_ERR_RESOLVER_RESOLVING      -15 ///< Error resolving address

#define OSL_ERR_WLAN_OFF                -16 ///< WLAN is turned off
#define OSL_USER_ABORTED            -17 ///< Operation was aborted by the user

#define OSL_ERR_HTTP_INIT           -18 ///< Error initializing HTTP
#define OSL_ERR_HTTP_TEMPLATE       -19 ///< Error with HTTP template
#define OSL_ERR_HTTP_TIMEOUT        -20 ///< HTTP connection timeout
#define OSL_ERR_HTTP_CONNECT        -21 ///< Error connecting via HTTP
#define OSL_ERR_HTTP_REQUEST        -22 ///< Error making HTTP request
#define OSL_ERR_HTTP_GENERIC        -23 ///< Generic HTTP error
/** @} */ // end of Error Codes

#define OSL_MAX_NET_CONFIGS             20 ///< Maximum number of network configurations

/** @brief Structure for network configuration. */
struct oslNetConfig {
	char name[128]; ///< Name of the network configuration
	char IP[128]; ///< IP address associated with the network configuration
};

#define OSL_RESOLVER_RETRY              4 ///< Number of retry attempts for resolving an address

/**
 * @brief Checks if the WLAN switch is powered on.
 *
 * @return 1 if the WLAN is powered on, else 0.
 */
int oslIsWlanPowerOn();

/**
 * @brief Checks if the WLAN is connected to an Access Point.
 *
 * @return 1 if connected to an AP, else 0.
 */
int oslIsWlanConnected();

/**
 * @brief Gets the network configurations.
 *
 * @param result Pointer to an array of oslNetConfig structures to be filled with the network configurations.
 * @return The number of network configurations found.
 */
int oslGetNetConfigs(struct oslNetConfig *result);

/**
 * @brief Returns whether the network is initialized.
 *
 * @return 1 if network is active, else 0.
 */
int oslIsNetActive();

/**
 * @brief Initializes the network.
 *
 * @return 0 on success, otherwise an error code.
 */
int oslNetInit();

/**
 * @brief Terminates the network.
 *
 * @return 0 on success, otherwise an error code.
 */
int oslNetTerm();

/**
 * @brief Gets the current IP address when connected to an Access Point.
 *
 * @param IPaddress Pointer to a char array where the IP address will be stored.
 * @return 0 on success, otherwise an error code.
 */
int oslGetIPaddress(char *IPaddress);

/**
 * @brief Tries to connect to an Access Point using the given configuration.
 *
 * @param config Index of the configuration to use.
 * @param timeout Timeout (in seconds) for the connection.
 * @param apctlCallback Pointer to a callback function that will be called with the current state.
 * @return 0 on success, otherwise an error code.
 */
int oslConnectToAP(int config, int timeout, int (*apctlCallback)(int state));

/**
 * @brief Disconnects from the Access Point.
 *
 * @return 0 on success, otherwise an error code.
 */
int oslDisconnectFromAP();

/**
 * @brief Gets the current Access Point state.
 *
 * @return The current AP state.
 */
int oslGetAPState();

/**
 * @brief Resolves an address to its IP address.
 *
 * @param address The address to resolve.
 * @param resolvedIP Pointer to a char array where the resolved IP address will be stored.
 * @return 0 on success, otherwise an error code.
 */
int oslResolveAddress(char *address, char *resolvedIP);

/**
 * @brief Gets a file from the web and saves it to the specified filepath.
 *
 * @param url The URL of the file to download.
 * @param filepath The path where the file should be saved.
 * @return 0 on success, otherwise an error code.
 */
int oslNetGetFile(const char *url, const char *filepath);

/**
 * @brief Posts a web form.
 *
 * @param url The URL to post the form to.
 * @param data The form data to be posted.
 * @param response Pointer to a char array where the server response will be stored.
 * @param responsesize The size of the response buffer.
 * @return 0 on success, otherwise an error code.
 */
int oslNetPostForm(const char *url, char *data, char *response, unsigned int responsesize);

/**
 * @brief Creates a network socket.
 *
 * @return The socket descriptor on success, otherwise an error code.
 */
int oslNetSocketCreate(void);

/**
 * @brief Accepts a connection on a socket.
 *
 * @param socket The socket descriptor.
 * @return The descriptor of the accepted socket, otherwise an error code.
 */
int oslNetSocketAccept(int socket);

/**
 * @brief Binds a socket to a port.
 *
 * @param socket The socket descriptor.
 * @param port The port number to bind the socket to.
 * @return 0 on success, otherwise an error code.
 */
int oslNetSocketBind(int socket, unsigned short port);

/**
 * @brief Listens for incoming connections on a socket.
 *
 * @param socket The socket descriptor.
 * @param maxconn The maximum number of connections to listen for.
 * @return 0 on success, otherwise an error code.
 */
int oslNetSocketListen(int socket, unsigned int maxconn);

/**
 * @brief Connects a socket to a remote host.
 *
 * @param socket The socket descriptor.
 * @param ip The IP address of the remote host.
 * @param port The port number of the remote host.
 * @return 0 on success, otherwise an error code.
 */
int oslNetSocketConnect(int socket, char *ip, unsigned short port);

/**
 * @brief Sends data over a socket.
 *
 * @param socket The socket descriptor.
 * @param data The data to be sent.
 * @param length The length of the data to be sent.
 * @return The number of bytes sent, otherwise an error code.
 */
int oslNetSocketSend(int socket, const void *data, int length);

/**
 * @brief Receives data from a socket.
 *
 * @param socket The socket descriptor.
 * @param data The buffer to store the received data.
 * @param length The length of the buffer.
 * @return The number of bytes received, otherwise an error code.
 */
int oslNetSocketReceive(int socket, void *data, int length);

/**
 * @brief Closes a socket.
 *
 * @param socket The socket descriptor.
 */
void oslNetSocketClose(int socket);

/**
 * @brief Clears a socket set.
 *
 * @param set The socket set to be cleared.
 */
void oslNetSocketSetClear(fd_set *set);

/**
 * @brief Adds a socket to a socket set.
 *
 * @param socket The socket descriptor.
 * @param set The socket set to add the socket to.
 */
void oslNetSocketSetAdd(int socket, fd_set *set);

/**
 * @brief Removes a socket from a socket set.
 *
 * @param socket The socket descriptor.
 * @param set The socket set to remove the socket from.
 */
void oslNetSocketSetRemove(int socket, fd_set *set);

/**
 * @brief Checks if a socket is a member of a socket set.
 *
 * @param socket The socket descriptor.
 * @param set The socket set to check.
 * @return 1 if the socket is a member, else 0.
 */
int oslNetSocketSetIsMember(int socket, fd_set *set);

/**
 * @brief Selects the ready sockets from a set of sockets.
 *
 * @param maxsockets The maximum number of sockets.
 * @param set The socket set to select from.
 * @return The number of ready sockets, otherwise an error code.
 */
int oslNetSocketSetSelect(unsigned int maxsockets, fd_set *set);

/** @} */ // end of Network

#endif // NET_H
