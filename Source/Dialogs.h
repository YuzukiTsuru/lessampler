#pragma once

#if _WIN32
#define DEFAULT_PATH "C:\\"
#else
#define DEFAULT_PATH "/tmp"
#endif

#define ERROR_MSG "ERROR - LESSAMPLER"
#define INFO_MSG "INFO - LESSAMPLER"
#define WARNING_MSG "WARNING - LESSAMPLER"

class Dialogs {
  public:
    /*
     * Error Dialog Display
     *
     * IN: Error Messages
     *
     * OUT: int :: 1 -> Agree | 2 -> Disagree | -1 -> Error
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
    
    /*
     * Warning Dialog Display
     *
     * IN: ERROR MEssage
     *
     * OUT: int :: 1 -> Agree | 2 -> Disagree | -1 -> Error
     */
    int warnDialog(const char *warn);
    
    /*
     * Dialog Display
     *
     * IN: Message
     */
    void dialog(const char *message);

    // CopyRight
    void copyRight();
};

