#pragma once

#if _WIN32
#define DEFAULT_PATH "C:\\"
#else
#define DEFAULT_PATH "/tmp"
#endif

// Error Dialog Display
int errorDialog();

// CopyRight
void copyRight();
