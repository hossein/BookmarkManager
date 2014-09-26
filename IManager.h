#pragma once
#include <QString>
#include <QtGui/QMessageBox>
#include <QtSql/QSqlError>

class Config;
class QWidget;

/// Interface for a class that manages something in the program.
/// Used only for Database Manager, our main manager, and for FileArchiveManager that has nothing
///     to do with databases and doesn't have models, tables, etc.
/// An IManager MUST NOT begin or end transactions in the database (as for FileArchiveManager,
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
        QMessageBox::critical(dialogParent, "Error", errorText);
        return false;
    }

    bool Error(const QString& errorText, const QSqlError& sqlError)
    {
        QMessageBox::critical(dialogParent, "Error", errorText
                              + "\n\nSQLite Error:\n" + sqlError.text());
        return false;
    }
};
