#pragma once

#include <QObject>

void initializeMessageOutputFileAndInstallHandler();
//If we don't remove this handler, things that produce output in QApplication's destructor
//  (e.g `Ct::DBApp::Settings::ClearAndSaveAllProgramSettings()` function) will crash the app!
void removeMessageHandler();
//IMPORTANT: If we delete `myOutputFile` manually before returning from main function, this may crash
//           Because the MainWindow IS DESTRUCTED AFTER THAT! So we set its parent like this to allow
//           automatic deletion when it's not needed.
void setMessageOutputFileParent(QObject* parent);

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
