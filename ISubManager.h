#pragma once
#include "IManager.h"
#include <QtSql/QSqlDatabase>

class DatabaseManager;

/// Interface for a class that manages a sub-part of the program.
/// Used for BookmarkManager, FileManager, FileViewManager, TagManager, etc.
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

    //Must create the tables used by the manager.
    virtual void CreateTables() = 0;

    //In FileViewManager, this populates the internal lists instead of any models.
    virtual void PopulateModels() = 0;

    void setSqlDatabase(QSqlDatabase& db_)
    {
        db = db_;
    }
};
