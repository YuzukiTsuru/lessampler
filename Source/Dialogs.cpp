#include "Dialogs.h"
#include "../JuceLibraryCode/JuceHeader.h"

#include "portable-file-dialogs.h"

int Dialogs::errorDialog(const char *error) {
    // Set verbosity to true
    pfd::settings::verbose(true);

    // Message box
    auto m = pfd::message(ERROR_MSG, error, pfd::choice::yes_no, pfd::icon::error);

    // Do something according to the selected button
    switch (m.result()) {
        case pfd::button::yes:
            Logger::writeToLog("User agreed.");
            return 1;
            break;
        case pfd::button::no:
            Logger::writeToLog("User disagreed.");
            return 2;
            break;
        default:
            return -1;
            break; // Should not happen
    }
    return -1;
}

int Dialogs::warnDialog(const char *warn) {
    // Set verbosity to true
    pfd::settings::verbose(true);

    // Message box
    auto m = pfd::message(WARNING_MSG, warn, pfd::choice::yes_no, pfd::icon::warning);

    // Do something according to the selected button
    switch (m.result()) {
        case pfd::button::yes:
            Logger::writeToLog("User agreed.");
            return 1;
            break;
        case pfd::button::no:
            Logger::writeToLog("User disagreed.");
            return 2;
            break;
        default:
            return -1;
            break; // Should not happen
    }
    return -1;
}

void Dialogs::dialog(const char *message) {
    // Set verbosity to true
    pfd::settings::verbose(true);
    pfd::message(ProjectInfo::projectName, message, pfd::choice::ok, pfd::icon::info);
}

void Dialogs::infoDialog(const char *title, const char *msg) {
    pfd::settings::verbose(true);
    pfd::message(title, msg, pfd::choice::ok, pfd::icon::info);
}

void Dialogs::copyRight() {
    std::cout << ProjectInfo::companyName
              << "Auther:\tGloomyGhost\n\n"
              << "Project:\t"
              << ProjectInfo::projectName
              << "\nVersion:\t"
              << ProjectInfo::versionString
              << "\n\n"
              << std::endl;
}
