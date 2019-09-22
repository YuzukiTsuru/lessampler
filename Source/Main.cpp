#include "../JuceLibraryCode/JuceHeader.h"

#include "Dialogs.h"

#include "Main.h"

int main(int argc, char *argv[]) {
    if(argc < 3){
        errorDialog();
    }
    return 0;
}
