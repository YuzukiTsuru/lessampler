#include "Dialogs.h"
#include "../JuceLibraryCode/JuceHeader.h"

#include "portable-file-dialogs.h"

int errorDialog() {
    // Set verbosity to true
    pfd::settings::verbose(true);

    // Message box
    auto m = pfd::message("Personal Message",
                          "You are an amazing person, don’t let anyone make you think otherwise.",
                          pfd::choice::yes_no_cancel,
                          pfd::icon::warning);

    // Do something according to the selected button
    switch (m.result()) {
        case pfd::button::yes:
            std::cout << "User agreed.\n";
            break;
        case pfd::button::no:
            std::cout << "User disagreed.\n";
            break;
        case pfd::button::cancel:
            std::cout << "User freaked out.\n";
            break;
        default:
            break; // Should not happen
    }
    return 0;
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
