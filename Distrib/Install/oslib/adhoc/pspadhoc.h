/**
 * @file pspadhoc.h
 * @brief Header file for AdHoc functions and structures.
 *
 * This file contains the declarations and definitions for AdHoc functionalities, 
 * including error codes, connection states, and remote PSP management.
 */

#ifndef _ADHOC_H_
#define _ADHOC_H_

/**
 * @defgroup AdHoc AdHoc
 * @brief Functions and structures for managing PSP AdHoc connections.
 * @{
 */

/** Error codes for AdHoc operations. */
#define ADHOC_ERROR_WLAN                -1  ///< WLAN error.
#define ADHOC_ERROR_MAC                 -2  ///< MAC address error.
#define ADHOC_ERROR_MODULES             -3  ///< Module loading error.
#define ADHOC_ERROR_NET_INIT            -4  ///< Network initialization error.
#define ADHOC_ERROR_INIT                -5  ///< AdHoc initialization error.
#define ADHOC_ERROR_CTL_INIT            -6  ///< Control initialization error.
#define ADHOC_ERROR_CTL_CONNECT         -7  ///< Control connection error.
#define ADHOC_ERROR_PDP_CREATE          -8  ///< PDP creation error.
#define ADHOC_ERROR_MATCHING_INIT       -9  ///< Matching initialization error.
#define ADHOC_ERROR_MATCHING_CREATE     -10 ///< Matching creation error.
#define ADHOC_ERROR_MATCHING_START      -11 ///< Matching start error.

/** Matching states for AdHoc operations. */
#define MATCHING_JOINED                 0x1  ///< Another PSP has joined.
#define MATCHING_SELECTED               0x2  ///< Another PSP selected to match.
#define MATCHING_REJECTED               0x4  ///< The request has been rejected.
#define MATCHING_CANCELED               0x5  ///< The request has been cancelled.
#define MATCHING_ACCEPTED               0x6  ///< The request has been accepted.
#define MATCHING_ESTABLISHED            0x7  ///< Connection established between PSPs.
#define MATCHING_DISCONNECT             0xa  ///< A PSP has quit.

/** AdHoc initialization states. */
#define ADHOC_UNINIT                    -1  ///< AdHoc not initialized.
#define ADHOC_INIT                      0   ///< AdHoc initialized.

/** Maximum number of remote PSPs supported. */
#define MAX_REMOTEPSP                   100 ///< Maximum remote PSPs.

/**
 * @brief Connection states for remote PSPs.
 */
enum remotePspState {
    OSL_ADHOC_DISCONNECTED = 0, ///< PSP Disconnected.
    OSL_ADHOC_JOINED,            ///< PSP Joined.
    OSL_ADHOC_SELECTED,          ///< PSP selected.
    OSL_ADHOC_REJECTED,          ///< Connection rejected.
    OSL_ADHOC_CANCELED,          ///< Connection canceled.
    OSL_ADHOC_ACCEPTED,          ///< Connection accepted.
    OSL_ADHOC_ESTABLISHED        ///< Connection established.
};

/**
 * @brief Structure for holding information about a remote PSP.
 */
struct remotePsp
{
    u8 macAddress[8];            ///< MAC address of the PSP.
    char name[257];              ///< Nickname of the PSP.
    int connectionState;         ///< Connection state (see remotePspState).
};

/**
 * @brief Initializes the AdHoc connection.
 *
 * @param productID The product ID for the AdHoc session.
 * @return 0 on success, negative value on error (see error codes).
 */
int oslAdhocInit(char *productID);

/**
 * @brief Retrieves the current connection state.
 *
 * @return Current connection state (see remotePspState).
 */
int oslAdhocGetState();

/**
 * @brief Retrieves the current MAC address of the PSP.
 *
 * @return Pointer to the MAC address (array of 6 bytes).
 */
u8* oslAdhocGetMacAddress();

/**
 * @brief Retrieves the number of connected remote PSPs.
 *
 * @return Number of connected remote PSPs.
 */
int oslAdhocGetRemotePspCount();

/**
 * @brief Retrieves the remote PSP information by MAC address.
 *
 * @param aMacAddress MAC address to search for.
 * @return Pointer to the remotePsp structure if found, NULL otherwise.
 */
struct remotePsp *oslAdhocGetPspByMacAddress(const u8 aMacAddress[6]);

/**
 * @brief Retrieves the remote PSP information by index.
 *
 * @param index Index of the remote PSP.
 * @return Pointer to the remotePsp structure if found, NULL otherwise.
 */
struct remotePsp *oslAdhocGetPspByIndex(int index);

/**
 * @brief Requests a connection to a remote PSP.
 *
 * @param aPsp Pointer to the remotePsp structure.
 * @param timeOut Timeout for the connection request.
 * @param requestConnectionCB Callback function for connection status updates.
 * @return 0 on success, negative value on error.
 */
int oslAdhocRequestConnection(struct remotePsp *aPsp, int timeOut, int (*requestConnectionCB)(int aPspState));

/**
 * @brief Sends data to a remote PSP.
 *
 * @param pPsp Pointer to the remotePsp structure.
 * @param data Pointer to the data to send.
 * @param lenData Length of the data to send.
 * @return 0 on success, negative value on error.
 */
int oslAdhocSendData(struct remotePsp *pPsp, void *data, int lenData);

/**
 * @brief Receives data from a remote PSP.
 *
 * @param pPsp Pointer to the remotePsp structure.
 * @param data Pointer to the buffer to store received data.
 * @param maxLen Maximum length of the buffer.
 * @return Length of the received data, 0 if no data received.
 */
int oslAdhocReceiveData(struct remotePsp *pPsp, void *data, int maxLen);

/**
 * @brief Retrieves a remote PSP that is requesting a connection.
 *
 * @return Pointer to the remotePsp structure if found, NULL otherwise.
 */
struct remotePsp *oslAdhocGetConnectionRequest();

/**
 * @brief Rejects a connection request from a remote PSP.
 *
 * @param aPsp Pointer to the remotePsp structure.
 */
void oslAdhocRejectConnection(struct remotePsp *aPsp);

/**
 * @brief Accepts a connection request from a remote PSP.
 *
 * @param aPsp Pointer to the remotePsp structure.
 */
void oslAdhocAcceptConnection(struct remotePsp *aPsp);

/**
 * @brief Terminates the AdHoc connection.
 */
void oslAdhocTerm();

/** @} */ // end of AdHoc

#endif // _ADHOC_H_
