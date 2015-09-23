#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Util::SeedRandomWithTime();

    MainWindow w;

    if (w.shouldExit())
        return -1; //Don't even run the app if constructor failed.

    w.show();
    
    return a.exec();
}
