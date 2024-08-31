#ifndef DIALOG_H
#define DIALOG_H

/** @defgroup Dialogs Dialogs
 *  @{
 *  @brief Functions to display Sony's dialogs.
 *
 *  This module provides various functions and definitions for handling Sony's PSP dialogs,
 *  such as message dialogs, error dialogs, and network configuration dialogs.
 */

/** @brief Dialog result indicating the user canceled the dialog. */
#define OSL_DIALOG_CANCEL 1

/** @brief Dialog result indicating the user confirmed the dialog. */
#define OSL_DIALOG_OK 0

/** @brief No dialog is currently active. */
#define OSL_DIALOG_NONE 0

/** @brief Message dialog type. */
#define OSL_DIALOG_MESSAGE 1

/** @brief Error dialog type. */
#define OSL_DIALOG_ERROR 2

/** @brief Network configuration dialog type. */
#define OSL_DIALOG_NETCONF 3

/** @brief On-screen keyboard dialog type. */
#define OSL_DIALOG_OSK 4

/** @brief Save/Load dialog type. */
#define OSL_DIALOG_SAVELOAD 5

/** @brief Browser dialog type. */
#define OSL_DIALOG_BROWSER 6

/**
 * @brief Universal routine which draws the current dialog (of the given dialog type)
 *        and waits for the user to finish interacting with it.
 * 
 * @param dialogType The type of dialog to be drawn.
 * @return int Returns 0 on success, or an error code (<0) on failure.
 */
int oslDialogDrawAndWait(int dialogType);

/**
 * @brief Tests whether any of the ERROR, MESSAGE, or NETCONF dialogs is currently active.
 * 
 * @return int Returns 1 if a dialog is active, or 0 if no dialog is active.
 */
int oslDialogIsActive();

/** @brief No dialog is currently active. */
#define OSL_DIALOG_STATUS_NONE PSP_UTILITY_DIALOG_NONE

/** @brief The dialog is currently being initialized. */
#define OSL_DIALOG_STATUS_INIT PSP_UTILITY_DIALOG_INIT

/** @brief The dialog is visible and ready for use. */
#define OSL_DIALOG_STATUS_VISIBLE PSP_UTILITY_DIALOG_VISIBLE

/** @brief The dialog has been canceled and should be shut down. */
#define OSL_DIALOG_STATUS_QUIT PSP_UTILITY_DIALOG_QUIT

/** @brief The dialog has successfully shut down. */
#define OSL_DIALOG_STATUS_FINISHED PSP_UTILITY_DIALOG_FINISHED

/**
 * @brief Initializes a Message Dialog.
 * 
 * @param message Text shown in the message dialog.
 * @param enableYesno If set to 1, the dialog will have Yes/No options.
 * @return int Returns 0 on success, or an error code (<0) on failure.
 */
int oslInitMessageDialog(const char *message, int enableYesno);

/**
 * @brief Initializes an Error Dialog.
 * 
 * @param error Error code to be displayed in the dialog.
 * @return int Returns 0 on success, or an error code (<0) on failure.
 */
int oslInitErrorDialog(const unsigned int error);

/**
 * @brief Draws the current dialog to the screen.
 * 
 * After drawing, check if the user has closed it by calling oslGetDialogStatus.
 * If the dialog is closed, remember to call oslEndDialog.
 * 
 * @code
 * oslDrawDialog();
 * if (oslGetDialogStatus() == PSP_UTILITY_DIALOG_NONE){
 *     // The user closed the dialog
 *     oslEndDialog();
 * }
 * @endcode
 */
void oslDrawDialog();

/**
 * @brief Returns the current dialog type.
 * 
 * @return int The type of the current dialog.
 */
int oslGetDialogType();

/**
 * @brief Returns the current dialog status.
 * 
 * @return int The status of the current dialog.
 */
int oslGetDialogStatus();

/**
 * @brief Returns the button pressed in the dialog (only for message dialogs with enableYesno == 1).
 * 
 * @return int The button pressed (e.g., OSL_DIALOG_OK or OSL_DIALOG_CANCEL).
 */
int oslGetDialogButtonPressed();

/**
 * @brief Initializes the network configuration dialog.
 * 
 * @return int Returns 0 on success, or an error code (<0) on failure.
 */
int oslInitNetDialog();

/**
 * @brief Gets the dialog result (either OSL_DIALOG_CANCEL or OSL_DIALOG_OK).
 * 
 * @return int The result of the dialog.
 */
int oslDialogGetResult();

/**
 * @brief Ends the current dialog.
 */
void oslEndDialog();

/** @} */ // end of Dialogs

#endif // DIALOG_H
