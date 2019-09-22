#pragma once

#if _WIN32
#define DEFAULT_PATH "C:\\"
#else
#define DEFAULT_PATH "/tmp"
#endif

#define ERROR_MSG   "ERROR - LESSAMPLER"
#define INFO_MSG    "INFO - LESSAMPLER"
#define WARNING_MSG "WARNING - LESSAMPLER"

/*
 * Error Dialog Display
 *
 * IN: Error Messages
 *
 * OUT: int
 */
int errorDialog(const char *error);

/*
 * Info Dialog Display
 *
 * IN: Title Info
 *
 * OUT: void
 */
void infoDialog(const char *title, const char *msg);

// CopyRight
void copyRight();
