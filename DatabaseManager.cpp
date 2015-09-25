#include "DatabaseManager.h"

#include "Config.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

DatabaseManager::DatabaseManager(QWidget* dialogParent, Config* conf)
    : IManager(dialogParent, conf), bms(dialogParent, conf), files(dialogParent, conf)
    , fview(dialogParent, conf), sets(dialogParent, conf), tags(dialogParent, conf)
{
}

DatabaseManager::~DatabaseManager()
{
    Close();
}

bool DatabaseManager::BackupOpenOrCreate(const QString& fileName)
{
    bool success;
    if (QFileInfo(fileName).exists())
        success = BackupOpenDatabase(fileName);
    else
        success = CreateDatabase(fileName);

    bms.setSqlDatabase(db);
    files.setSqlDatabase(db);
    fview.setSqlDatabase(db);
    sets.setSqlDatabase(db);
    tags.setSqlDatabase(db);

    return success;
}

void DatabaseManager::Close()
{
    if (db.isOpen())
        db.close();
}

void DatabaseManager::PopulateModelsAndInternalTables()
{
    bms.PopulateModelsAndInternalTables();
    files.PopulateModelsAndInternalTables();
    fview.PopulateModelsAndInternalTables();
    sets.PopulateModelsAndInternalTables();
    tags.PopulateModelsAndInternalTables();
}

bool DatabaseManager::BackupOpenDatabase(const QString& fileName)
{
    if (!BackupDatabase(fileName))
        return false;

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(fileName);
    if (!db.open())
        return Error("Could not open the database file:\n" + fileName, db.lastError());

    //Need to enable this manually.
    if (!EnableForeignKeysSupport())
        return false;

    if (!CheckVersion())
        return false;

    return true;
}

bool DatabaseManager::BackupDatabase(const QString& fileName)
{
    QString backupsDirName = "Backups";
    QFileInfo fileInfo(fileName);
    QDir fileDir = fileInfo.dir();
    QFileInfo backupDirInfo(fileDir.absolutePath() + "/" + backupsDirName);

    if (!backupDirInfo.exists())
    {
        if (!fileDir.mkpath(backupsDirName))
            return Error("Backups directory could not be created!");
    }
    else if (!backupDirInfo.isDir())
    {
        return Error("Backups is not a directory!");
    }

    QString newFileName = fileInfo.baseName()
            + "-" + QLocale("en-us").toString(QDateTime::currentDateTime(), "yyyy-MM-dd.hh.mm.ss")
            + "." + fileInfo.completeSuffix() + ".bak";
    QString newFilePath = backupDirInfo.absoluteFilePath() + "/" + newFileName;

    if (!QFile::copy(fileName, newFilePath))
        return Error("Backup file could not be created:\n" + newFilePath);

    return true;
}

bool DatabaseManager::CreateDatabase(const QString& fileName)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(fileName);
    if (!db.open())
        return Error("Could not create the database file:\n" + fileName, db.lastError());

    //Before creating the tables, try to query and enable foreign key support. This is useful to
    //  warn the user about non-existent foreign-keys support before parsing the FOREIGN key statements
    //  in tables fail.
    if (!EnableForeignKeysSupport())
        return false;

    QSqlQuery query(db);

    query.exec("CREATE TABLE Info( Version INTEGER )");
    query.exec("INSERT INTO Info(Version) VALUES(" + QString::number(conf->programDatabaseVersion) + ")");

    bms.CreateTables();
    files.CreateTables();
    fview.CreateTables();
    sets.CreateTables();
    tags.CreateTables();

    return true;
}

bool DatabaseManager::EnableForeignKeysSupport()
{
    //http://www.sqlite.org/foreignkeys.html
    //Foreign key support in SQLite 3 is either disabled using a directive, or anyway is disabled
    //  by default.
    //We should query the state using 'PRAGMA foreign_keys;': if it returns no data, foreign keys
    //  aren't supported. If it returns 0, we enable them with another PRAGMA. If it returns 1, it
    //  is already enabled (maybe at a future SQLite version).

    //This is just an assertion.
    if (!db.open())
        return Error("To enable foreign key support in SQLite first open the database connection.");

    QSqlQuery query(db);
    if (!query.exec("PRAGMA foreign_keys;"))
        return Error("Error while querying foreign key support on SQLite database.", query.lastError());

    if (!query.first())
        return Error("Foreign keys are not supported! SQLite version used is too old.");

    int foreignKeyEnabled = query.value(0).toInt();
    if (foreignKeyEnabled == 0)
    {
        //Enable it
        if (!query.exec("PRAGMA foreign_keys = ON;"))
            return Error("Could not enable foreign key support in SQLite!", query.lastError());

        //Check again to make sure.
        QString checkError = "Error while checking the enabling of foreign key support in SQLite";
        if (!query.exec("PRAGMA foreign_keys;"))
            return Error(checkError + ".", query.lastError());
        if (!query.first())
            return Error(checkError + ":\n\nNo results were returned.");
        if (query.value(0).toInt() != 1)
            return Error(checkError + ":\n\nForeign key support was not enabled as expected. "
                         "Returned result was: " + query.value(0).toString() + ".");
        return true;
    }
    else if (foreignKeyEnabled == 1)
    {
        //Already enabled
        return true;
    }
    else
    {
        //Unknown value
        return Error("Unknown result of foreign key support query in SQLite: " +
                     QString::number(foreignKeyEnabled) + ".");
    }
}

bool DatabaseManager::CheckVersion()
{
    QString versionError = "Could not get version information from database file.";
    QSqlQuery query("SELECT Version FROM Info LIMIT 1", db);

    if (!query.exec())
        return Error(versionError, query.lastError());

    if (!query.first())
        return Error(versionError, query.lastError());

    bool okay = false;
    int version = query.record().value("Version").toInt(&okay);

    if (!okay)
        return Error(versionError, query.lastError());

    if (version < conf->programDatabaseVersion)
        return UpgradeDatabase();
    else if (version > conf->programDatabaseVersion)
        return Error("Database file format is newer than current application!\n"
                     "Please update the application.");

    return true;
}

bool DatabaseManager::UpgradeDatabase()
{
    return true;
}
