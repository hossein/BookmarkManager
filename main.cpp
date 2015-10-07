#include "BMApplication.h"
#include "MainWindow.h"

#include "CtLogger.h"
#include "Util.h"

int main(int argc, char *argv[])
{
    initializeMessageOutputFileAndInstallHandler();

    Util::SeedRandomWithTime();

    BMApplication app("uniqueid.BookmarkManager", argc, argv);

    //Collect all Qt plugins in a 'plugins/' subdir.
    app.addLibraryPath(app.applicationDirPath() + "/plugins");

    //QtSingleApplication
    //This app needs to be single instance both because of database usage, and because we delete
    //  SandBox folder contents on startup, and also we use only one log file.
    if (app.isRunning())
        return !app.sendMessage("Activate");

    MainWindow w;

    //CtLogger
    setMessageOutputFileParent(&w);
    //QtSingleApplication
    app.setActivationWindow(&w, true);

    if (w.shouldExit())
        return -1; //Don't even run the app if constructor failed.

    w.show();
    
    int appresult = app.exec();

    removeMessageHandler();

    return appresult;
}
