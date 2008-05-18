#ifndef BROWSER_H
#define BROWSER_H

/** @defgroup Browser

	Functions to display Sony's internet browser
	@{
*/

#define OSL_BROSER_ERROR_MEMORY		-1
#define OSL_BROSER_ERROR_INIT		-2


int oslBrowserInit(char *url, char *downloadDir);

void oslDrawBrowser();

int oslGetBrowserStatus();

void oslEndBrowser();

int oslBrowserIsActive();


/** @} */ // end of Browser
#endif
