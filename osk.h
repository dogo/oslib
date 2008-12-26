#ifndef OSK_H
#define OSK_H

/** @defgroup OSK

	Functions to display Sony's On Screen Keyboard
	@{
*/

/**OSK cancel*/
#define OSL_OSK_CANCEL 1
/**OSK OK*/
#define OSL_OSK_OK 0

/** Initializes the OSK
	\param *descStr
        Text shown as a description (bottom right corner)
    \param initialStr
        Initial text in the OSK
    \param textLimit
        Maximum number of chars
    \param linesNumber
        Number of lines
    \param language
        Language
			JAPANESE			0
			ENGLISH				1
			FRENCH				2
			SPANISH				3
			GERMAN				4
			ITALIAN				5
			DUTCH				6
			PORTUGUESE			7
			RUSSIAN				8
			KOREAN				9
			CHINESE_TRADITIONAL	10
			CHINESE_SIMPLIFIED	11
		If you pass to the function -1 then the language set in the firmware is used
*/
void oslInitOsk(char *descStr, char *initialStr, int textLimit, int linesNumber, int language);

/** Draws the OSK
After drawing it you should check if the user closed it. Remember to call oslEndOsk.
\code
if (oslOskIsActive()){
    oslDrawOsk();
    if (oslGetOskStatus() == PSP_UTILITY_DIALOG_NONE){
        //The user closed the OSK
        oslEndOsk();
    }
}
\endcode
*/
void oslDrawOsk();

/** Checks if the OSK is active.
*/
int oslOskIsActive();

/**Returns the current OSK status
*/
int oslGetOskStatus();

/**Returns the OSK result (OSL_OSK_OK or OSL_OSK_CANCEL)
*/
int oslOskGetResult();

/**Get the text inserted in the OSK
*/
void oslOskGetText(char *text);
void oslOskGetTextUCS2(unsigned short *text);

/**Ends the OSK
*/
void oslEndOsk();


/** @} */ // end of osk
#endif
