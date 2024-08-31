/**
 * @file messagebox.h
 * @brief Message box functionality for OSLib.
 *
 * This file contains definitions and functions for creating and handling message boxes 
 * in the OSLib environment. These message boxes can display various messages, such as debug 
 * information, warnings, and fatal errors, with options for user interaction through different buttons.
 */

#ifndef _OSL_MESSAGEBOX_H_
#define _OSL_MESSAGEBOX_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup msgbox Message Boxes
 * @brief Functions and definitions related to message boxes in OSLib.
 * @{
 */


/**
 * @brief Message box button actions.
 *
 * These enumerations represent the different actions that can be associated with buttons in a message box.
 */
enum OSL_MB_ACTIONS {
    OSL_MB_OK = 1,     /*!< Ok button */
    OSL_MB_CANCEL,     /*!< Cancel button */
    OSL_MB_YES,        /*!< Yes button */
    OSL_MB_NO,         /*!< No button */
    OSL_MB_QUIT        /*!< Quit button (exits immediately with an oslQuit call!) */
};

#ifdef PSP
/**
 * @brief Displays a debug message, with the same format as printf.
 *
 * The title displays the file, the line number, and the function from where it has been called.
 * 
 * Example usage:
 * @code
 * int var = 2;
 * oslDebug("var = %i", var);
 * @endcode
 */
#define oslDebug(format...) ({ char __str2[1000], __str[1000]; sprintf(__str2, "Debug (%s:%i,%s)",__FUNCTION__,__LINE__,__FILE__); sprintf(__str , ##format); oslMessageBox(__str, __str2, oslMake3Buttons(OSL_KEY_CROSS,OSL_MB_OK,OSL_KEY_TRIANGLE,OSL_MB_QUIT,0,0)); })

/**
 * @brief Creates a 32-bit integer to be used for the 'flags' parameter when calling oslMessageBox.
 *
 * There are 6 arguments, 2 per button. The first of each pair represents the button assigned (one of the
 * #OSL_KEY_BITS values) and the second one represents the action (one of #OSL_MB_ACTIONS values).
 * 
 * Example usage:
 * @code
 * oslMessageBox("Message text", "Message title", oslMake3Buttons(OSL_KEY_CROSS, OSL_MB_OK, OSL_KEY_CIRCLE, OSL_MB_CANCEL, 0, 0));
 * @endcode
 */
#define oslMake3Buttons(b1,a1,b2,a2,b3,a3) ((b1)|((a1)<<5)|((b2)<<9)|((a2)<<14)|((b3)<<18)|((a3)<<23))

/**
 * @brief Displays a warning message with the same format as oslDebug.
 */
#define oslWarning(format...) ({ char __str[1000]; sprintf(__str , ##format); oslMessageBox(__str, "Warning", oslMake3Buttons(OSL_KEY_CROSS,OSL_MB_OK,OSL_KEY_TRIANGLE,OSL_MB_QUIT,0,0)); })

/**
 * @brief Displays a fatal error message. This function will terminate execution after displaying the message.
 *
 * The only available choice is 'Quit', ensuring that this function is a dead end and no one will ever return from it.
 */
#define oslFatalError(format...) ({ char __str[1000]; sprintf(__str , ##format); oslMessageBox(__str, "Fatal error", oslMake3Buttons(OSL_KEY_CROSS,OSL_MB_QUIT,0,0,0,0)); })
#else
#define oslDebug(...) ({ char __str2[1000], __str[1000]; sprintf(__str2, "Debug (%s:%i,%s)",__FUNCTION__,__LINE__,__FILE__); sprintf(__str , __VA_ARGS__); oslMessageBox(__str, __str2, oslMake3Buttons(OSL_KEY_CROSS,OSL_MB_OK,OSL_KEY_TRIANGLE,OSL_MB_QUIT,0,0)); })
#define oslMake3Buttons(b1,a1,b2,a2,b3,a3) ((b1)|((a1)<<5)|((b2)<<9)|((a2)<<14)|((b3)<<18)|((a3)<<23))
#define oslWarning(...) ({ char __str[1000]; sprintf(__str , __VA_ARGS__); oslMessageBox(__str, "Warning", oslMake3Buttons(OSL_KEY_CROSS,OSL_MB_OK,OSL_KEY_TRIANGLE,OSL_MB_QUIT,0,0)); })
#define oslFatalError(...) ({ char __str[1000]; sprintf(__str , __VA_ARGS__); oslMessageBox(__str, "Fatal error", oslMake3Buttons(OSL_KEY_CROSS,OSL_MB_QUIT,0,0,0,0)); })
#endif

/**
 * @brief Displays a fatal error message if the condition is not satisfied.
 *
 * This macro allows you to assert that a condition is true. If it is not, a fatal error message is displayed, and the program is terminated.
 * 
 * Example usage:
 * @code
 * OSL_IMAGE *img = oslLoadImageFilePNG("test.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_5551);
 * oslAssert(img != NULL);
 * @endcode
 */
#define oslAssert(cond) ({ if (!(cond)) { char __str[1000]; sprintf(__str , "This program encountered a fatal error and must be terminated.\n\nFile : %s:%i\nError: %s",__FILE__,__LINE__,""#cond); oslMessageBox(__str, "Fatal error", oslMake3Buttons(OSL_KEY_CROSS,OSL_MB_QUIT,0,0,0,0)); } })

/**
 * @brief Displays a message box.
 *
 * @param text The text displayed inside of the message box.
 * @param title The title of the message box.
 * @param flags Flags representing the buttons and their actions, created using #oslMake3Buttons.
 * @return Returns a 32-bit unsigned integer representing the message box result.
 *
 * Example usage:
 * @code
 * oslMessageBox("This is a message", "Message Title", oslMake3Buttons(OSL_KEY_CROSS, OSL_MB_OK, OSL_KEY_CIRCLE, OSL_MB_CANCEL, 0, 0));
 * @endcode
 */
extern unsigned int oslMessageBox(const char *text, const char *title, unsigned int flags);

/** @} */ // end of msgbox

#ifdef __cplusplus
}
#endif

#endif // _OSL_MESSAGEBOX_H_
