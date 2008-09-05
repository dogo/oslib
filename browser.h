#ifndef BROWSER_H
#define BROWSER_H

/** @defgroup Browser

	Functions to display Sony's internet browser
	@{
*/

/**Memory error initializing the browser*/
#define OSL_BROSER_ERROR_MEMORY		-1
/**Error initializing the browser*/
#define OSL_BROSER_ERROR_INIT		-2


int oslBrowserInit(char *url, char *downloadDir, int browserMemory, unsigned int displaymode, unsigned int options, unsigned int interfacemode, unsigned int connectmode);

void oslDrawBrowser();

int oslGetBrowserStatus();

void oslEndBrowser();

int oslBrowserIsActive();


/** @} */ // end of Browser
#endif
