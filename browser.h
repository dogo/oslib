#ifndef BROWSER_H
#define BROWSER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Browser

	Functions to display Sony's internet browser
	@{
*/

/**Memory error initializing the browser*/
#define OSL_BROWSER_ERROR_MEMORY		-1
/**Error initializing the browser*/
#define OSL_BROWSER_ERROR_INIT		-2

/** Initializes the internet browser.
	\param url
        The url to open
    \param downloadDir
        The default

*/
int oslBrowserInit(char *url, char *downloadDir, int browserMemory, unsigned int displaymode, unsigned int options, unsigned int interfacemode, unsigned int connectmode);

void oslDrawBrowser();

int oslGetBrowserStatus();

void oslEndBrowser();

int oslBrowserIsActive();


/** @} */ // end of Browser

#ifdef __cplusplus
}
#endif

#endif
