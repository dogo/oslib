#include "oslib.h"

int (*readButtonsFunctions)(SceCtrlData *pad_data, int count) = NULL;

OSL_CONTROLLER osl_pad;
OSL_REMOTECONTROLLER osl_remote;

OSL_CONTROLLER *oslReadKeys() {
	SceCtrlData ctl = {0};
	SceCtrlData ctlHome = {0};

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);

	// Use custom function if set, otherwise use default PSP function
	if (readButtonsFunctions != NULL) {
		readButtonsFunctions(&ctl, 1);
	} else {
		sceCtrlPeekBufferPositive(&ctl, 1);
	}

	// Always check HOME button using user mode
	sceCtrlPeekBufferPositive(&ctlHome, 1);
	if (ctlHome.Buttons & PSP_CTRL_HOME) {
		memset(&osl_keys->held, 0, sizeof(OSL_KEYLIST));
		memset(&osl_keys->pressed, 0, sizeof(OSL_KEYLIST));
		memset(&osl_keys->released, 0, sizeof(OSL_KEYLIST));
		memset(&osl_keys->lastHeld, 0, sizeof(OSL_KEYLIST));
		return osl_keys;
	}

	// Handle analog stick and hold button
	if (osl_keys->holdAffectsAnalog && (ctl.Buttons & OSL_KEYMASK_HOLD)) {
		osl_keys->analogX = 0;
		osl_keys->analogY = 0;
	} else {
		osl_keys->analogX = (signed)ctl.Lx - 128;
		osl_keys->analogY = (signed)ctl.Ly - 128;
	}

	// Convert analog stick movement to D-pad button presses
	if (osl_keys->analogToDPadSensivity > 0) {
		if (osl_keys->analogY >= osl_keys->analogToDPadSensivity) {
			ctl.Buttons |= OSL_KEYMASK_DOWN;
			scePowerTick(0);
		}
		if (osl_keys->analogY <= -osl_keys->analogToDPadSensivity) {
			ctl.Buttons |= OSL_KEYMASK_UP;
			scePowerTick(0);
		}
		if (osl_keys->analogX >= osl_keys->analogToDPadSensivity) {
			ctl.Buttons |= OSL_KEYMASK_RIGHT;
			scePowerTick(0);
		}
		if (osl_keys->analogX <= -osl_keys->analogToDPadSensivity) {
			ctl.Buttons |= OSL_KEYMASK_LEFT;
			scePowerTick(0);
		}
	}

	// Auto-repeat logic
	if (osl_keys->autoRepeatInterval > 0) {
		// If button state changes -> reset counter to zero
		if (osl_keys->lastHeld.value != ctl.Buttons) {
			osl_keys->autoRepeatCounter = 0;
		} else {
			osl_keys->autoRepeatCounter++;
			// Auto-repeat triggered -> activate every autoRepeatInterval ticks
			if (osl_keys->autoRepeatCounter >= osl_keys->autoRepeatInit &&
			    (osl_keys->autoRepeatCounter - osl_keys->autoRepeatInit) % osl_keys->autoRepeatInterval == 0) {
				osl_keys->lastHeld.value &= ~osl_keys->autoRepeatMask;
			}
		}
	}

	// Update key states
	osl_keys->pressed.value = ~osl_keys->lastHeld.value & ctl.Buttons;
	osl_keys->released.value = osl_keys->lastHeld.value & ~ctl.Buttons;
	osl_keys->held.value = ctl.Buttons;
	osl_keys->lastHeld.value = ctl.Buttons;

	return osl_keys;
}

OSL_REMOTECONTROLLER *oslReadRemoteKeys() {
	u32 remote = 0;

	if (!sceHprmIsRemoteExist()) {
		memset(&osl_remotekeys->held, 0, sizeof(OSL_REMOTEKEYLIST));
		memset(&osl_remotekeys->pressed, 0, sizeof(OSL_REMOTEKEYLIST));
		memset(&osl_remotekeys->released, 0, sizeof(OSL_REMOTEKEYLIST));
		memset(&osl_remotekeys->lastHeld, 0, sizeof(OSL_REMOTEKEYLIST));
		return osl_remotekeys;
	}

	sceHprmPeekCurrentKey(&remote);

	// Auto-repeat logic for remote keys
	if (osl_remotekeys->autoRepeatInterval > 0) {
		if (osl_remotekeys->lastHeld.value != remote) {
			osl_remotekeys->autoRepeatCounter = 0;
		} else {
			osl_remotekeys->autoRepeatCounter++;
			if (osl_remotekeys->autoRepeatCounter >= osl_remotekeys->autoRepeatInit &&
			    (osl_remotekeys->autoRepeatCounter - osl_remotekeys->autoRepeatInit) % osl_remotekeys->autoRepeatInterval == 0) {
				osl_remotekeys->lastHeld.value &= ~osl_remotekeys->autoRepeatMask;
			}
		}
	}

	// Update remote key states
	osl_remotekeys->pressed.value = ~osl_remotekeys->lastHeld.value & remote;
	osl_remotekeys->released.value = osl_remotekeys->lastHeld.value & ~remote;
	osl_remotekeys->held.value = remote;
	osl_remotekeys->lastHeld.value = remote;

	return osl_remotekeys;
}

void oslFlushRemoteKey() {
	oslReadRemoteKeys();
}

int oslIsRemoteExist() {
	return sceHprmIsRemoteExist();
}

void oslFlushKey() {
	oslReadKeys();
}

int oslKbhit() {
	OSL_CONTROLLER *keys = oslReadKeys();
	int k = 0, j = 0;

	for (int i = 0; i < 24; i++) {
		if (keys->pressed.value & (1 << i)) {
			k |= (i + 1) << j;
			j += 8;
		}
	}
	return k;
}

int oslWaitKey() {
	int key = 0;
	while (!(key = oslKbhit()) && !osl_quit) {
		oslWaitVSync();
	}
	return key;
}

int oslSetReadKeysFunction(int (*sceCtrlReadBufferPositive)(SceCtrlData *pad_data, int count)) {
	readButtonsFunctions = sceCtrlReadBufferPositive;
	return 0;
}

int oslUnsetReadKeysFunction() {
	readButtonsFunctions = NULL;
	return 0;
}

int oslSetHoldForAnalog(int holdForAnalog) {
	osl_keys->holdAffectsAnalog = holdForAnalog;
	return 0;
}
