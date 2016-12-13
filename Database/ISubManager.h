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

    virtual ~ISubManager()
    {
    }

    /// Called only upon creating the database.
    virtual void CreateTables() = 0;

    /// Execute queries on models and populate them. Called multiple times when refreshing by program.
    /// Subclasses must NEVER assign to their models, as the models are assigned upon program loading
    ///   to proxy models and UI elements and must change after that.
    virtual void PopulateModelsAndInternalTables() = 0;

    void setSqlDatabase(QSqlDatabase& db_)
    {
        db = db_;
    }
};
