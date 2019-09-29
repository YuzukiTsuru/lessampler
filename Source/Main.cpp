#include "Main.h"

int main(int argc, char *argv[]) {
    //
    if(!langpack.exists("config.xml") || !langpack.exists("language.xml" )){
        dialog.errorDialog("Missing configuration file\n缺失配置文件\n構成ファイルがありません");
        return -1;
    }
    
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
