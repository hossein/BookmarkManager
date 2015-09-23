#include <QApplication>
#include "MainWindow.h"

#include "CtLogger.h"
#include "Util.h"

int main(int argc, char *argv[])
{
    initializeMessageOutputFileAndInstallHandler();

    Util::SeedRandomWithTime();

    QApplication app(argc, argv);
    MainWindow w;

    //CtLogger
    setMessageOutputFileParent(&w);

    if (w.shouldExit())
        return -1; //Don't even run the app if constructor failed.

    w.show();
    
    int appresult = app.exec();

    removeMessageHandler();

    return appresult;
}
