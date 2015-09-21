#pragma once
#include "IManager.h"
#include "BookmarkManager.h"
#include "FileManager.h"
#include "FileViewManager.h"
#include "TagManager.h"
#include <QtSql/QSqlDatabase>

//TODO: It's never necessary to pass both dbm and conf together to a class. Fix it everywhere.
class DatabaseManager : public IManager
{
public:
    QSqlDatabase db;
    BookmarkManager bms;
    FileManager files;
    FileViewManager fview;
    TagManager tags;

    DatabaseManager(QWidget* dialogParent, Config* conf);
    ~DatabaseManager();

public:
    /// If a database exists, opens it and creates a backup, or creates a new database file.
    /// Returns true on success.
    bool BackupOpenOrCreate(const QString& fileName);
    void Close();

    void PopulateModelsAndInternalTables(); //This is NOT from ISubManager.

private:
    bool BackupOpenDatabase(const QString& fileName);
    bool BackupDatabase(const QString& fileName);
    bool CreateDatabase(const QString& fileName);

    bool EnableForeignKeysSupport();

    bool CheckVersion();
    bool UpgradeDatabase();

};
