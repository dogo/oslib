#ifndef _OSL_KEYS_H_
#define _OSL_KEYS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup keys Controller
 * @{
 * Controller functions in OSLib.
 */

/** 
 * @union OSL_KEYLIST
 * @brief List of keys.
 *
 * This union represents the state of the buttons on a controller.
 */
typedef union {
    struct {
        int select:1;           //!< Select button.
        int reserved1:2;        //!< Reserved for padding, do not use.
        int start:1;            //!< Start button.
        int up:1;               //!< Up on the D-pad.
        int right:1;            //!< Right on the D-pad.
        int down:1;             //!< Down on the D-pad.
        int left:1;             //!< Left on the D-pad.
        int L:1;                //!< L shoulder button.
        int R:1;                //!< R shoulder button.
        int reserved2:2;        //!< Reserved for padding, do not use.
        int triangle:1;         //!< Triangle button.
        int circle:1;           //!< Circle button.
        int cross:1;            //!< Cross button.
        int square:1;           //!< Square button.
        int home:1;             //!< Home button (may not work).
        int hold:1;             //!< Hold button (power switch in the opposite direction).
        int reserved3:5;        //!< Reserved for padding, do not use.
        int note:1;             //!< Note button (may not work).
    };
    unsigned int value;         //!< 32-bit value containing all keys.
} OSL_KEYLIST;

/**
 * @struct OSL_CONTROLLER
 * @brief Controller type.
 *
 * This structure represents the state of the controller, including held, pressed, and released keys.
 */
typedef struct {
    OSL_KEYLIST held;                           //!< Keys currently held down.
    OSL_KEYLIST pressed;                        //!< Keys pressed (reported once when the user presses it).
    OSL_KEYLIST released;                       //!< Keys released (reported once when the user releases it).
    OSL_KEYLIST lastHeld;                       //!< Last held state, for handling auto-repeat without interference.

    short autoRepeatInit;                       //!< Time for the initialization of the auto-repeat feature.
    short autoRepeatInterval;                   //!< Interval before the auto-repeat feature activates.
    int autoRepeatMask;                         //!< Keys affected by the auto-repeat feature.
    short autoRepeatCounter;                    //!< Internal counter for auto-repeat.

    signed char analogToDPadSensivity;          //!< Minimum sensitivity for analog to D-pad conversion. 0 disables this feature, 127 is maximum sensitivity.

    signed char analogX;                        //!< Horizontal position of the analog stick (-128: left, +127: right).
    signed char analogY;                        //!< Vertical position of the analog stick (-128: top, +127: bottom).

    int holdAffectsAnalog;                      //!< Determines if HOLD affects the analog stick.
} OSL_CONTROLLER;

/**
 * @enum OSL_KEY_BITS
 * @brief Bit number for each key in the 'value' field.
 *
 * This enumeration defines the bit positions for each key in the `value` field of `OSL_KEYLIST`.
 */
enum OSL_KEY_BITS {
    OSL_KEY_SELECT = 1,            //!< Select key.
    OSL_KEY_START = 4,             //!< Start key.
    OSL_KEY_UP = 5,                //!< Up D-pad key.
    OSL_KEY_RIGHT = 6,             //!< Right D-pad key.
    OSL_KEY_DOWN = 7,              //!< Down D-pad key.
    OSL_KEY_LEFT = 8,              //!< Left D-pad key.
    OSL_KEY_L = 9,                 //!< L shoulder key.
    OSL_KEY_R = 10,                //!< R shoulder key.
    OSL_KEY_TRIANGLE = 13,         //!< Triangle key.
    OSL_KEY_CIRCLE = 14,           //!< Circle key.
    OSL_KEY_CROSS = 15,            //!< Cross key.
    OSL_KEY_SQUARE = 16,           //!< Square key.
    OSL_KEY_HOME = 17,             //!< Home key (may not work in normal operation).
    OSL_KEY_HOLD = 18,             //!< Hold key (power switch in the opposite direction).
    OSL_KEY_NOTE = 24              //!< Note key (may not work).
};

/**
 * @enum OSL_KEY_MASKS
 * @brief Mask for each key in the 'value' field.
 *
 * This enumeration defines the bitmasks for each key in the `value` field of `OSL_KEYLIST`.
 */
enum OSL_KEY_MASKS {
    OSL_KEYMASK_SELECT = 0x1,
    OSL_KEYMASK_START = 0x8,
    OSL_KEYMASK_UP = 0x10,
    OSL_KEYMASK_RIGHT = 0x20,
    OSL_KEYMASK_DOWN = 0x40,
    OSL_KEYMASK_LEFT = 0x80,
    OSL_KEYMASK_L = 0x100,
    OSL_KEYMASK_R = 0x200,
    OSL_KEYMASK_TRIANGLE = 0x1000,
    OSL_KEYMASK_CIRCLE = 0x2000,
    OSL_KEYMASK_CROSS = 0x4000,
    OSL_KEYMASK_SQUARE = 0x8000,
    OSL_KEYMASK_HOME = 0x10000,
    OSL_KEYMASK_HOLD = 0x20000,
    OSL_KEYMASK_NOTE = 0x800000
};

/**
 * @brief Sets the auto-repeat parameters for keys.
 * @param keys The keys affected by the auto-repeat feature.
 * @param init Time (in number of calls) before the auto-repeat feature turns on.
 * @param interval Time interval between each key repeat when the auto-repeat has been turned on.
 */
#define oslSetKeyAutorepeat(keys, init, interval) \
    (osl_keys->autoRepeatMask = keys, osl_keys->autoRepeatInit = init, osl_keys->autoRepeatInterval = interval)

/**
 * @brief Sets the key auto-repeat mask.
 * @param mask The key auto-repeat mask.
 */
#define oslSetKeyAutorepeatMask(mask) \
    (osl_keys->autoRepeatMask = mask)

/**
 * @brief Sets the key auto-repeat initialization value.
 * @param value The initialization value for auto-repeat.
 */
#define oslSetKeyAutorepeatInit(value) \
    (osl_keys->autoRepeatInit = value)

/**
 * @brief Sets the key auto-repeat interval value.
 * @param value The interval value for auto-repeat.
 */
#define oslSetKeyAutorepeatInterval(value) \
    (osl_keys->autoRepeatInterval = value)

/**
 * @brief Enables or disables automatic redirection from the analog stick to D-pad buttons.
 * @param sensivity Sensitivity for the analog press. A typical value is 80.
 */
#define oslSetKeyAnalogToDPad(sensivity) \
    (osl_keys->analogToDPadSensivity = sensivity)

/** 
 * @var osl_keys
 * @brief Current keys. Only here for compatibility, use osl_pad now.
 */
extern OSL_CONTROLLER *osl_keys;

/** 
 * @var osl_pad
 * @brief Current keys.
 *
 * This structure holds the current state of the controller.
 */
extern OSL_CONTROLLER osl_pad;

/**
 * @brief Reads the current controller state and stores the result in the osl_pad structure.
 * @return A pointer to the actual key structure.
 */
extern OSL_CONTROLLER *oslReadKeys();

/**
 * @brief Sets an external function to read keys.
 * @param sceCtrlReadBufferPositive The external function to read keys.
 * @return 0 on success, non-zero on failure.
 */
extern int oslSetReadKeysFunction(int (*sceCtrlReadBufferPositive)(SceCtrlData *pad_data, int count));

/**
 * @brief Unsets the function set with oslSetReadKeysFunction.
 * @return 0 on success, non-zero on failure.
 */
extern int oslUnsetReadKeysFunction();

/**
 * @brief Decides if HOLD will affect the analog stick.
 * @param holdForAnalog 1 to enable, 0 to disable.
 * @return 0 on success, non-zero on failure.
 */
extern int oslSetHoldForAnalog(int holdForAnalog);

/**
 * @brief Waits for a key and returns its code.
 * @return The key code.
 */
extern int oslWaitKey();

/**
 * @brief Determines whether a key is currently buffered and returns its code.
 * @return The key code if a key is buffered, 0 otherwise.
 */
extern int oslKbhit();

/**
 * @brief Flushes the key buffer, removing the pending key.
 */
extern void oslFlushKey();

/**
 * @union OSL_REMOTEKEYLIST
 * @brief List of remote keys.
 *
 * This union represents the state of the buttons on a remote controller.
 */
typedef union {
    struct {
        int rmplaypause:1;       //!< Play/Pause button.
        int reserved1:1;         //!< Reserved for padding, do not use.
        int rmforward:1;         //!< Forward button.
        int rmback:1;            //!< Back button.
        int rmvolup:1;           //!< Volume Up button.
        int rmvoldown:1;         //!< Volume Down button.
        int rmhold:1;            //!< Hold button.
        int reserved2:1;         //!< Reserved for padding, do not use.
    };
    u32 value;                    //!< 32-bit value containing all keys.
} OSL_REMOTEKEYLIST;

/**
 * @struct OSL_REMOTECONTROLLER
 * @brief Remote Controller type.
 *
 * This structure represents the state of the remote controller, including held, pressed, and released keys.
 */
typedef struct {
    OSL_REMOTEKEYLIST held;                           //!< Keys currently held down.
    OSL_REMOTEKEYLIST pressed;                        //!< Keys pressed (reported once when the user presses it).
    OSL_REMOTEKEYLIST released;                       //!< Keys released (reported once when the user releases it).
    OSL_REMOTEKEYLIST lastHeld;                       //!< Last held state, for handling auto-repeat without interference.

    short autoRepeatInit;                             //!< Time for the initialization of the auto-repeat feature.
    short autoRepeatInterval;                         //!< Interval before the auto-repeat feature activates.
    int autoRepeatMask;                               //!< Keys affected by the auto-repeat feature.
    short autoRepeatCounter;                          //!< Internal counter for auto-repeat.
} OSL_REMOTECONTROLLER;

/** 
 * @var osl_remotekeys
 * @brief Current remote keys. 
 *
 * This structure holds the current state of the remote controller.
 */
extern OSL_REMOTECONTROLLER *osl_remotekeys;

/**
 * @var osl_remote
 * @brief Current remote keys.
 *
 * This structure holds the current state of the remote controller.
 */
extern OSL_REMOTECONTROLLER osl_remote;

/**
 * @brief Reads the current remote controller state and stores the result in the osl_remote structure.
 * @return A pointer to the actual key structure.
 */
extern OSL_REMOTECONTROLLER *oslReadRemoteKeys();

/**
 * @brief Flushes the remote key buffer, removing the pending key.
 */
extern void oslFlushRemoteKey();

/**
 * @brief Sets the auto-repeat parameters for remote keys.
 * @param keys The keys affected by the auto-repeat feature.
 * @param init Time (in number of calls) before the auto-repeat feature turns on.
 * @param interval Time interval between each key repeat when the auto-repeat has been turned on.
 */
#define oslSetRemoteKeyAutorepeat(keys, init, interval) \
    (osl_remotekeys->autoRepeatMask = keys, osl_remotekeys->autoRepeatInit = init, osl_remotekeys->autoRepeatInterval = interval)

/**
 * @brief Sets the remote key auto-repeat mask.
 * @param mask The key auto-repeat mask.
 */
#define oslSetRemoteKeyAutorepeatMask(mask) \
    (osl_remotekeys->autoRepeatMask = mask)

/**
 * @brief Sets the remote key auto-repeat initialization value.
 * @param value The initialization value for auto-repeat.
 */
#define oslSetRemoteKeyAutorepeatInit(value) \
    (osl_remotekeys->autoRepeatInit = value)

/**
 * @brief Sets the remote key auto-repeat interval value.
 * @param value The interval value for auto-repeat.
 */
#define oslSetRemoteKeyAutorepeatInterval(value) \
    (osl_remotekeys->autoRepeatInterval = value)

/**
 * @brief Determines whether the remote is plugged in.
 * @return 1 if the remote is plugged in, 0 otherwise.
 */
int oslIsRemoteExist();

/** @} */ // end of keys

#ifdef __cplusplus
}
#endif

#endif
