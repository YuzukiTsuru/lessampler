#include "../JuceLibraryCode/JuceHeader.h"

#include "Dialogs.h"
#include "Main.h"

int main(int argc, char *argv[]) {
    if(argc < 3){
        copyRight();
        errorDialog();
    }
    return 0;
}
