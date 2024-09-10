#include "oslib.h"

void osl_noFailDefaultCallback(const char *filename, u32 reserved);
void (*osl_noFailCallback)(const char *filename, u32 reserved) = osl_noFailDefaultCallback;

void osl_noFailDefaultCallback(const char *filename, u32 reserved) {
    oslFatalError("Unable to locate the following file: %s\nPlease reinstall the application and read the instructions carefully.", filename);
}

void oslHandleLoadNoFailError(const char *filename) {
    // Check if error trapping is enabled (bitwise AND with 1)
    if (osl_noFail & 1) {
        osl_noFailCallback(filename, 0);

        // Check if the application should quit after displaying the error
        // (bitwise AND with 2, if not set, proceed to quit)
        if (!(osl_noFail & 2)) {
            oslQuit();
        }
    }
}
