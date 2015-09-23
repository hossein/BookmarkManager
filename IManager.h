#pragma once
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QtSql/QSqlError>

class Config;
class QWidget;

/// Interface for a class that manages something in the program.
/// Used only for Database Manager, our main manager, and for IArchiveManager that has nothing
///     to do with databases and doesn't have models, tables, etc.
/// An IManager MUST NOT begin or end transactions in the database (as for IArchiveManager,
///     it must not begin or end TransactionalFileOperations transactions neither).
class IManager
{
protected:
    QWidget* dialogParent;
    Config* conf;

protected:
    IManager(QWidget* dialogParent, Config* conf)
        : dialogParent(dialogParent), conf(conf)
    {
    }

    virtual ~IManager()
    {
    }

    bool Error(const QString& errorText)
    {
        qDebug() << "IManager::Error: " << errorText;
        QMessageBox::critical(dialogParent, "Error", errorText);
        return false;
    }

    bool Error(const QString& errorText, const QSqlError& sqlError)
    {
        qDebug() << "IManager::Error: " << errorText << "\n"
                 << "  - SQL native error: " << sqlError.nativeErrorCode() << "\n"
                 << "  - Error number/text: " << sqlError.number() << " " << sqlError.text() << "\n"
                 << "  - DB/Driver text: " << sqlError.databaseText() << "/" << sqlError.driverText();
        QMessageBox::critical(dialogParent, "Error", errorText
                              + "\n\nSQLite Error:\n" + sqlError.text());
        return false;
    }
};
