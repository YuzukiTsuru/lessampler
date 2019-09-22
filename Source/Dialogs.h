#pragma once

#if _WIN32
#define DEFAULT_PATH "C:\\"
#else
#define DEFAULT_PATH "/tmp"
#endif

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
