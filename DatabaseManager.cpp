#include "DatabaseManager.h"

#include "Config.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtSql/QSQLiteDriver>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

DatabaseManager::DatabaseManager(QWidget* dialogParent, Config* conf)
    : IManager(dialogParent, conf),
      bms(dialogParent, conf), files(dialogParent, conf), tags(dialogParent, conf)
{
}

DatabaseManager::~DatabaseManager()
{
    Close();
}

bool DatabaseManager::BackupOpenOrCreate(const QString& fileName)
{
    if (QFileInfo(fileName).exists())
        return BackupOpenDatabase(fileName);
    else
        return CreateDatabase(fileName);

    bms.setSqlDatabase(db);
    files.setSqlDatabase(db);
    tags.setSqlDatabase(db);
}

void DatabaseManager::Close()
{
    if (db.isOpen())
        db.close();
}

void DatabaseManager::PopulateModels()
{
    bms.PopulateModels();
    files.PopulateModels();
    tags.PopulateModels();
}

bool DatabaseManager::BackupOpenDatabase(const QString& fileName)
{
    if (!BackupDatabase(fileName))
        return false;

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(fileName);
    if (!db.open())
        return Error("Could not open the database file:\n" + fileName, db.lastError());

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
            + "-" + QDateTime::currentDateTime().toString("yyyy-MM-dd.hh.mm.ss")
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

    QSqlQuery query(db);

    query.exec("CREATE TABLE Info( Version INTEGER )");
    query.exec("INSERT INTO Info(Version) VALUES(" + QString::number(conf->nominalDatabaseVersion) + ")");

    bms.CreateTables();
    files.CreateTables();
    tags.CreateTables();

    return true;
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

    if (version < conf->nominalDatabaseVersion)
        return UpgradeDatabase();
    else if (version > conf->nominalDatabaseVersion)
        return Error("Database file format is newer than current application!\n"
                     "Please update the application.");

    return true;
}

bool DatabaseManager::UpgradeDatabase()
{
    return true;
}
