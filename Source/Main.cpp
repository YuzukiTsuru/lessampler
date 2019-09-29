#include "Main.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        dialog.copyRight();
        int feedBack = dialog.errorDialog("The input parameter is wrong.\nplease check if it is called correctly or the parameter is correct.");
        if (feedBack == 1) {
            dialog.infoDialog("Use Suggestions", "Please call in the UTAU program or call with relevant parameters");
        } else if (feedBack == -1) {
            dialog.infoDialog("Whats Wrong？？？", "Whats Wrong？？？\n Should not be like this......");
        }
    }
    return 0;
}
