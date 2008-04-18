#ifndef DIALOG_H
#define DIALOG_H

/** @defgroup Dialogs

	Functions to display Sony's dialogs
	@{
*/

#define OSL_DIALOG_CANCEL 1
#define OSL_DIALOG_OK 0

/**No dialog*/
#define OSL_DIALOG_NONE 0
/**Message dialog*/
#define OSL_DIALOG_MESSAGE 1
/**Error dialog*/
#define OSL_DIALOG_ERROR 2
/**Net conf dialog*/
#define OSL_DIALOG_NETCONF 3

/**Initializes a Message Dialog
	\param *message
        Text shown in the message dialog
    \param enableYesno
        if 1 the dialog will have Yes/No option
*/
void oslInitMessageDialog(const char *message, int enableYesno);

/**Initializes an Error Dialog
	\param error
        Error code
*/
void oslInitErrorDialog(const unsigned int error);

/**Draws the current dialog to screen.
After drawing it you should check if the user closed it. Remember to call oslEndDialog.
\code
oslDrawDialog();
if (oslGetDialogStatus() == PSP_UTILITY_DIALOG_NONE){
    //The user closed the dialog
    oslEndDialog();
}
\endcode
*/
void oslDrawDialog();

/**Returns the current dialog type
*/
int oslGetDialogType();

/**Returns the current dialog status
*/
int oslGetDialogStatus();

/**Returns the button pressed in the dialog (only for message dialog with enableYesno == 1)
*/
int oslGetDialogButtonPressed();

/**Initializes the net conf dialog
*/
void oslInitNetDialog();

/**Gets the dialog result (OSL_DIALOG_CANCEL or OSL_DIALOG_OK)
*/
int oslDialogGetResult();

/**Ends the current dialog
*/
void oslEndDialog();

/** @} */ // end of dialog
#endif
