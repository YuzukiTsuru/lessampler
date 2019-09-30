#include "Main.h"

int main(int argc, char *argv[]) {
    // Find the xml file for it
    if (!langpack.exists("config.json") || !langpack.exists("lang.json")) {
        int feedback = dialog.errorDialog("Missing configuration file,Do you want to create a file?\n\n缺失配置文件，是否新建文件？\n\n構成ファイルがありません，ファイルを作成しますか？");
        if (feedback == 1) {
            langpack.initFile();
        } else if (feedback == 2) {
            return -1;
        } else {
            return -1;
        }
    }

    if (argc < 3) {
        dialog.copyRight();
        int feedBack = dialog.errorDialog("The input parameter is wrong.please check if it is called correctly or the parameter is correct.");
        if (feedBack == 1) {
            dialog.infoDialog("Use Suggestions", "Please call in the UTAU program or call with relevant parameters");
        } else if (feedBack == -1) {
            dialog.infoDialog("Whats Wrong???", "Whats Wrong??? Should not be like this......");
        }
    }
    std::cout << langpack.fetch("1");
    return 0;
}
