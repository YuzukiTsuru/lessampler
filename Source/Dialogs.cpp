#include "Dialogs.h"
#include "../JuceLibraryCode/JuceHeader.h"

#include "portable-file-dialogs.h"

const char* getTitle(const char *msg) {
    String msgStr  = msg;
    String appName = ProjectInfo::projectName;
    String m       = msgStr + " - " + appName;
    return m.getCharPointer();
}

int errorDialog(const char *error) {
    // Set verbosity to true
    pfd::settings::verbose(true);

    // Message box
    auto m = pfd::message(getTitle("ERROR"), error, pfd::choice::yes_no, pfd::icon::error);

    // Do something according to the selected button
    switch (m.result()) {
        case pfd::button::yes:
            std::cout << "User agreed.\n";
            return 1;
            break;
        case pfd::button::no:
            std::cout << "User disagreed.\n";
            return 2;
            break;
        case pfd::button::cancel:
            std::cout << "User freaked out.\n";
            return 3;
            break;
        default:
            return -1;
            break; // Should not happen
    }
    return 0;
}

void infoDialog(const char *title, const char *msg) {
    pfd::settings::verbose(true);
    pfd::message(title, msg, pfd::choice::ok, pfd::icon::info);
}

void copyRight() {
    std::cout << "GSiSY 2019\n"
              << "Auther:\tGloomyGhost\n\n"
              << "Project:\t"
              << ProjectInfo::projectName
              << "\nVersion:\t"
              << ProjectInfo::versionString
              << std::endl;
}
