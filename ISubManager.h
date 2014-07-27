#pragma once
#include "IManager.h"
#include <QtSql/QSqlDatabase>

class DatabaseManager;

/// Interface for a class that manages a sub-part of the program.
/// Used for BookmarkManager, FileManager, TagManager, etc.
/// An ISubManager MUST NOT begin or end transactions in the database.
class ISubManager : public IManager
{
protected:
    QSqlDatabase db;

protected:
    ISubManager(QWidget* dialogParent, Config* conf)
        : IManager(dialogParent, conf)
    {

    }

    virtual void CreateTables() = 0;
    virtual void PopulateModels() = 0;

    void setSqlDatabase(QSqlDatabase& db_)
    {
        db = db_;
    }
};
