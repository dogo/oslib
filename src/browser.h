#ifndef BROWSER_H
#define BROWSER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Browser Browser
    @brief Functions to display Sony's internet browser.
    @{
*/

/** @brief Memory error initializing the browser.
 */
#define OSL_BROWSER_ERROR_MEMORY -1

/** @brief Error initializing the browser.
 */
#define OSL_BROWSER_ERROR_INIT -2

/** 
 * @brief Initializes the internet browser.
 * 
 * This function initializes Sony's internet browser with the specified URL and options.
 * 
 * @param url The URL to open.
 * @param downloadDir The default download directory.
 * @param browserMemory The amount of memory to allocate for the browser.
 * @param displaymode One of the `pspUtilityHtmlViewerDisplayModes`.
 * @param options One of the `pspUtilityHtmlViewerOptions`.
 * @param interfacemode One of the `pspUtilityHtmlViewerInterfaceModes`.
 * @param connectmode One of the `pspUtilityHtmlViewerConnectModes`.
 * 
 * @return Returns 0 on success, or a negative error code on failure.
 */
int oslBrowserInit(char *url, char *downloadDir, int browserMemory, unsigned int displaymode, unsigned int options, unsigned int interfacemode, unsigned int connectmode);

/**
 * @brief Renders the browser display.
 * 
 * This function handles the drawing/rendering of the browser's display.
 */
void oslDrawBrowser();

/**
 * @brief Gets the current status of the browser.
 * 
 * This function returns the current status of the browser, which can be used to determine
 * if the browser is still active or if there are any errors.
 * 
 * @return The current status of the browser.
 */
int oslGetBrowserStatus();

/**
 * @brief Ends the browser session.
 * 
 * This function closes the internet browser and frees any resources allocated during initialization.
 */
void oslEndBrowser();

/**
 * @brief Checks if the browser is active.
 * 
 * This function checks whether the browser is currently active.
 * 
 * @return Returns 1 if the browser is active, 0 otherwise.
 */
int oslBrowserIsActive();

/** @} */ // end of Browser

#ifdef __cplusplus
}
#endif

#endif
