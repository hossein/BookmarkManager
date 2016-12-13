#include "CtLogger.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <cstdio>
#include <cstdlib>

static QFile* myOutputFile;

void initializeMessageOutputFileAndInstallHandler()
{
    QString logFileName = "bookmarkmanager";
    QString logFileExt = ".log";
    QString logFilePathName = QDir::tempPath() + QDir::separator() + logFileName + logFileExt;

    //We rename the file if it's more than a certain size. This works since we only allow a single
    //  instance of the application.
    //Favor small file sizes so the os can efficiently cache it or whatever.
    const qint64 logFileSizeThreshold = 200 * 1024;

    bool logRenameSuccess = true;
    if (QFileInfo(logFilePathName).size() >= logFileSizeThreshold)
    {
        logRenameSuccess = QFile::rename(logFilePathName,
                      QDir::tempPath() + QDir::separator() + logFileName + '-' +
                      QLocale("en-us").toString(QDateTime::currentDateTime(), "yyyyMMddThh.mm.ss") +
                      logFileExt);
    }

    myOutputFile = new QFile(logFilePathName);
    if (myOutputFile->open(QIODevice::WriteOnly | QIODevice::Append))
        myOutputFile->write(QString("\r\n\r\nBookmark Manager Program Log @ " +
                            QLocale("en-us").toString(QDateTime::currentDateTime()) + "\r\n").toUtf8());
    else
        qDebug() << "Opening log file failed.";

    if (!logRenameSuccess)
        myOutputFile->write("Error renaming the big old log file.\r\n");

    qInstallMessageHandler(myMessageHandler);
}

void removeMessageHandler()
{
    qInstallMessageHandler(0);
}

void setMessageOutputFileParent(QObject* parent)
{
    myOutputFile->setParent(parent);
}

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

    //Evil! QLocale("en-us").toString(QDateTime::currentDateTime(), "yyyy-MM-dd hh:mm:ss.zzz").toUtf8().data();
    QByteArray dtstring = QLocale("en-us").toString(QDateTime::currentDateTime(), "yyyy-MM-dd hh:mm:ss.zzz").toUtf8();
    //Doesn't work!
    //dtstring += QString(": [%1] %2: Line %4 at function [%3] (V%5): ")
    //        .arg(context.category).arg(context.file).arg(context.function)
    //        .arg(context.line).arg(context.version).toUtf8();
    dtstring += QByteArray(": ");
    char* dtstamp = dtstring.data();

    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "%s%s\n", dtstamp, msg.toUtf8().constData());
        fflush(stderr);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s%s\n", dtstamp, msg.toUtf8().constData());
        fflush(stderr);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s%s\n", dtstamp, msg.toUtf8().constData());
        fflush(stderr);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s%s\n", dtstamp, msg.toUtf8().constData());
        fflush(stderr);
        abort();
    }

    if (myOutputFile->isOpen())
    {
        myOutputFile->write(dtstring);
        myOutputFile->write(msg.toUtf8());
        myOutputFile->write("\r\n");
        myOutputFile->flush(); //Good if the app crashes.
    }
}
