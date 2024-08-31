#ifndef OSK_H
#define OSK_H

/** @defgroup OSK On-Screen Keyboard
    @{
    Functions to display Sony's On Screen Keyboard.
*/

/** 
 * @brief Structure representing the on-screen keyboard (OSK) parameters.
 */
typedef struct {
    SceUtilityOskParams oskParams; /**< Parameters for the on-screen keyboard. */
} OSL_KEYBOARD;

/** @brief Global instance of the on-screen keyboard. */
extern OSL_KEYBOARD* osl_osk;

/** 
 * @brief Initializes the on-screen keyboard with specified parameters.
 * 
 * @param nData Number of data entries to initialize.
 * @param language Language of the on-screen keyboard.
 * 
 * @return Pointer to the initialized OSL_KEYBOARD structure.
 */
extern OSL_KEYBOARD* oslInitOskEx(int nData, int language);

/** 
 * @brief Initializes specific OSK data.
 * 
 * @param kbd Pointer to the OSL_KEYBOARD structure.
 * @param idx Index of the data entry.
 * @param desc Description text (UTF-16).
 * @param intext Initial text input (UTF-16).
 * @param textLimit Maximum number of characters.
 * @param linesNumber Number of lines for input.
 * 
 * @return Status code of the initialization process.
 */
extern int oslInitOskDataEx(OSL_KEYBOARD *kbd, int idx, unsigned short *desc,
                            unsigned short *intext, int textLimit, int linesNumber);

/** 
 * @brief Activates the on-screen keyboard.
 * 
 * @param kbd Pointer to the OSL_KEYBOARD structure.
 * @param waitcycle Time to wait before the OSK is fully activated.
 * 
 * @return Status code of the activation process.
 */
extern int oslActivateOskEx(OSL_KEYBOARD *kbd, int waitcycle);

/** 
 * @brief Checks if the on-screen keyboard is active.
 * 
 * @param kbd Pointer to the OSL_KEYBOARD structure.
 * 
 * @return Non-zero if the OSK is active, zero otherwise.
 */
extern int oslOskIsActiveEx(OSL_KEYBOARD *kbd);

/** 
 * @brief Deactivates the on-screen keyboard.
 * 
 * @param kbd Pointer to the OSL_KEYBOARD structure.
 */
extern void oslDeActivateOskEx(OSL_KEYBOARD *kbd);

/** 
 * @brief Gets the result of the on-screen keyboard interaction.
 * 
 * @param kbd Pointer to the OSL_KEYBOARD structure.
 * @param idx Index of the data entry.
 * 
 * @return Result code indicating whether the text was changed, unchanged, or canceled.
 */
extern int oslOskGetResultEx(OSL_KEYBOARD *kbd, int idx);

/** 
 * @brief Retrieves the text output from the on-screen keyboard.
 * 
 * @param kbd Pointer to the OSL_KEYBOARD structure.
 * @param idx Index of the data entry.
 * 
 * @return Pointer to the text output (UTF-16).
 */
extern unsigned short* oslOskOutTextEx(OSL_KEYBOARD *kbd, int idx);

/** 
 * @brief Ends the on-screen keyboard session and releases resources.
 * 
 * @param kbd Pointer to the OSL_KEYBOARD structure.
 */
extern void oslEndOskEx(OSL_KEYBOARD *kbd);

/**
 * @def OSL_OSK_RESULT_UNCHANGED
 * @brief Indicates that the OSK result is unchanged.
 * 
 * This macro maps to `PSP_UTILITY_OSK_RESULT_UNCHANGED`, which is used to indicate that the user did not modify the text in the On-Screen Keyboard.
 */
#define OSL_OSK_RESULT_UNCHANGED    PSP_UTILITY_OSK_RESULT_UNCHANGED

/**
 * @def OSL_OSK_RESULT_CANCELLED
 * @brief Indicates that the OSK operation was cancelled.
 * 
 * This macro maps to `PSP_UTILITY_OSK_RESULT_CANCELLED`, which is used to indicate that the user cancelled the On-Screen Keyboard without making changes.
 */
#define OSL_OSK_RESULT_CANCELLED    PSP_UTILITY_OSK_RESULT_CANCELLED

/**
 * @def OSL_OSK_RESULT_CHANGED
 * @brief Indicates that the OSK result has changed.
 * 
 * This macro maps to `PSP_UTILITY_OSK_RESULT_CHANGED`, which is used to indicate that the user modified the text in the On-Screen Keyboard.
 */
#define OSL_OSK_RESULT_CHANGED      PSP_UTILITY_OSK_RESULT_CHANGED

/** 
 * @brief OSK cancel result.
 * 
 * @deprecated Use PSP_UTILITY_OSK_RESULT_CANCELLED instead.
 */
#define OSL_OSK_CANCEL    PSP_UTILITY_OSK_RESULT_CANCELLED

/** 
 * @brief OSK changed result.
 */
#define OSL_OSK_CHANGED    PSP_UTILITY_OSK_RESULT_CHANGED

/** 
 * @brief OSK unchanged result.
 */
#define OSL_OSK_UNCHANGED    PSP_UTILITY_OSK_RESULT_UNCHANGED

/** 
 * @brief Initializes the OSK with basic parameters.
 * 
 * @param descStr Text shown as a description (bottom right corner).
 * @param initialStr Initial text in the OSK.
 * @param textLimit Maximum number of characters.
 * @param linesNumber Number of lines.
 * @param language Language for the OSK.
 *                - 0: JAPANESE
 *                - 1: ENGLISH
 *                - 2: FRENCH
 *                - 3: SPANISH
 *                - 4: GERMAN
 *                - 5: ITALIAN
 *                - 6: DUTCH
 *                - 7: PORTUGUESE
 *                - 8: RUSSIAN
 *                - 9: KOREAN
 *                - 10: CHINESE_TRADITIONAL
 *                - 11: CHINESE_SIMPLIFIED
 *                - -1: Use the language set in the firmware.
 */
void oslInitOsk(char *descStr, char *initialStr, int textLimit, int linesNumber, int language);

/** 
 * @brief Draws the OSK on the screen.
 * 
 * After drawing it, you should check if the user has closed it. 
 * Remember to call oslEndOsk to properly terminate the OSK.
 * 
 * @code
 * if (oslOskIsActive()){
 *     oslDrawOsk();
 *     if (oslGetOskStatus() == PSP_UTILITY_DIALOG_NONE){
 *         //The user closed the OSK
 *         oslEndOsk();
 *     }
 * }
 * @endcode
 */
void oslDrawOsk();

/** 
 * @brief Checks if the OSK is currently active.
 * 
 * @return Non-zero if the OSK is active, zero otherwise.
 */
int oslOskIsActive();

/** 
 * @brief Returns the current OSK status.
 * 
 * @return Current status of the OSK.
 */
int oslGetOskStatus();

/** 
 * @brief Returns the result of the OSK interaction.
 * 
 * @return Result code (OSL_OSK_CHANGED, OSL_OSK_UNCHANGED, or OSL_OSK_CANCEL).
 */
int oslOskGetResult();

/** 
 * @brief Retrieves the text inserted by the user in the OSK.
 * 
 * @param text Pointer to the buffer where the text will be stored (UTF-8).
 */
void oslOskGetText(char *text);

/** 
 * @brief Retrieves the text inserted by the user in the OSK (UCS2).
 * 
 * @param text Pointer to the buffer where the text will be stored (UTF-16).
 */
void oslOskGetTextUCS2(unsigned short *text);

/** 
 * @brief Ends the OSK session.
 * 
 * This function should be called after the OSK is closed to clean up resources.
 */
void oslEndOsk();

/** @} */ // end of OSK group

#endif
