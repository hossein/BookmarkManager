#pragma once
#include <QString>
#include <QtGui/QMessageBox>
#include <QtSql/QSqlError>

class Config;
class QWidget;

/// Interface for a class that manages something in the program.
/// Used only for Database Manager, our main manager.
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
