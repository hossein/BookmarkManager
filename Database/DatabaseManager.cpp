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
    : IManager(dialogParent, conf)
    , bms(dialogParent, conf), bfs(dialogParent, conf), files(dialogParent, conf)
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
    bfs.setSqlDatabase(db);
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
    bfs.PopulateModelsAndInternalTables();
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

    if (!CheckVersion())
        return false;

    //Need to enable this manually.
    //It is IMPORTANT to do this AFTER CheckVersion, which does database migration by dropping and
    //  recreating tables so disables this support in advance.
    if (!SetForeignKeysSupport(true))
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
    if (!SetForeignKeysSupport(true))
        return false;

    QSqlQuery query(db);

    query.exec("CREATE TABLE Info( Version INTEGER )");
    query.exec("INSERT INTO Info(Version) VALUES(" + QString::number(conf->programDatabaseVersion) + ")");

    bms.CreateTables();
    bfs.CreateTables();
    files.CreateTables();
    fview.CreateTables();
    sets.CreateTables();
    tags.CreateTables();

    return true;
}

bool DatabaseManager::SetForeignKeysSupport(bool enable)
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
    if (foreignKeyEnabled == 0 && enable)
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
    else if (foreignKeyEnabled == 1 && !enable)
    {
        //Disable it
        if (!query.exec("PRAGMA foreign_keys = OFF;"))
            return Error("Could not disable foreign key support in SQLite!", query.lastError());

        //Check again to make sure.
        QString checkError = "Error while checking the disabling of foreign key support in SQLite";
        if (!query.exec("PRAGMA foreign_keys;"))
            return Error(checkError + ".", query.lastError());
        if (!query.first())
            return Error(checkError + ":\n\nNo results were returned.");
        if (query.value(0).toInt() != 0)
            return Error(checkError + ":\n\nForeign key support was not disabled as expected. "
                         "Returned result was: " + query.value(0).toString() + ".");
        return true;
    }
    else if (foreignKeyEnabled != 0 && foreignKeyEnabled != 1)
    {
        //Unknown value
        return Error("Unknown result of foreign key support query in SQLite: " +
                     QString::number(foreignKeyEnabled) + ".");
    }
    else
    {
        //Okay, either already enabled or already disabled; no need to change.
        return true;
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
    {
        //Disable foreign keys to stop any triggers from running, if enabled.
        //IMPORTANT: Needs to be done before starting transactions.
        //  http://stackoverflow.com/questions/7359721/sqlite-are-pragma-statements-undone-by-rolling-back-transactions
        SetForeignKeysSupport(false);

        //IMPORTANT: ^^ If we had triggers, indices, etc we had to turn them off, e.g using:
        //  http://stackoverflow.com/questions/2250959/how-to-handle-a-missing-feature-of-sqlite-disable-triggers
        //Fortunately we don't need to delete and recreate them as well, just disabling them is enough.
        //Also sqlite_sequence table seems to remember the sequence and doesn't need modifications later.

        if (!db.transaction())
            return Error("Migration Error: Could not begin transaction.", db.lastError());

        bool upgradeSuccess = UpgradeDatabase(version);
        bool transSuccess
            = upgradeSuccess
            ? db.commit()
            : db.rollback();

        if (!transSuccess)
            return Error(QString("Migration Error: Could not %1 database changes.")
                         .arg(upgradeSuccess ? "commit" : "rollback"), db.lastError());

        return upgradeSuccess;
    }
    else if (version > conf->programDatabaseVersion)
    {
        return Error("Database file format is newer than current application!\n"
                     "Please update the application.");
    }

    return true;
}

bool DatabaseManager::UpgradeDatabase(int dbVersion)
{
    QSqlQuery query(db);

    if (dbVersion <= 1)
    {
        /// BookmarkLink had a syntax error (missing comma) in foreign key specifications.
        if (!query.exec("CREATE Table BookmarkLink2 "
                        "( BLID INTEGER PRIMARY KEY AUTOINCREMENT, BID1 INTEGER, BID2 INTEGER,"
                        "  FOREIGN KEY(BID1) REFERENCES Bookmark(BID) ON DELETE CASCADE, "
                        "  FOREIGN KEY(BID2) REFERENCES Bookmark(BID) ON DELETE CASCADE )"))
            return Error("Migration Error: v1, Creating BookmarkLink", query.lastError());

        if (!query.exec("INSERT INTO BookmarkLink2 (BLID, BID1, BID2) "
                        "SELECT BLID, BID1, BID2 FROM BookmarkLink"))
            return Error("Migration Error: v1, Copying BookmarkLink", query.lastError());

        if (!query.exec("DROP TABLE BookmarkLink"))
            return Error("Migration Error: v1, Dropping BookmarkLink", query.lastError());

        if (!query.exec("ALTER TABLE BookmarkLink2 RENAME TO BookmarkLink"))
            return Error("Migration Error: v1, Renaming BookmarkLink", query.lastError());


        /// BookmarkTag was missing an ON DELETE clause.
        if (!query.exec("CREATE TABLE BookmarkTag2 "
                        "( BTID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, TID INTEGER, "
                        "  FOREIGN KEY(BID) REFERENCES Bookmark(BID) ON DELETE CASCADE, "
                        "  FOREIGN KEY(TID) REFERENCES Tag(TID) ON DELETE CASCADE )"))
            return Error("Migration Error: v1, Creating BookmarkTag", query.lastError());

        if (!query.exec("INSERT INTO BookmarkTag2 (BTID, BID, TID) "
                        "SELECT BTID, BID, TID FROM BookmarkTag"))
            return Error("Migration Error: v1, Copying BookmarkTag", query.lastError());

        if (!query.exec("DROP TABLE BookmarkTag"))
            return Error("Migration Error: v1, Dropping BookmarkTag", query.lastError());

        if (!query.exec("ALTER TABLE BookmarkTag2 RENAME TO BookmarkTag"))
            return Error("Migration Error: v1, Renaming BookmarkTag", query.lastError());


        /// New BookmarkFolder table
        //Copied from BookmarkFolderManager::CreateTables
        if (!query.exec("CREATE TABLE BookmarkFolder "
                        "( FOID INTEGER PRIMARY KEY AUTOINCREMENT, ParentFOID INTEGER, "
                        "  Name TEXT, Desc TEXT, DefFileArchive TEXT, "
                        "  FOREIGN KEY(ParentFOID) REFERENCES BookmarkFolder(FOID) ON DELETE RESTRICT )"))
            return Error("Migration Error: v1, Creating BookmarkFolder", query.lastError());

        query.prepare("INSERT INTO BookmarkFolder(FOID, ParentFOID, Name, Desc, DefFileArchive) VALUES (?, ?, ?, ?, ?);");
        query.addBindValue(0); //Force first PK to be 0.
        query.addBindValue(QVariant()); //Not -1, it will cause a foreign key violation error
        query.addBindValue("Unsorted Bookmarks");
        query.addBindValue("Bookmarks that still aren't in a folder.");
        query.addBindValue(":arch0:");
        if (!query.exec())
            return Error("Migration Error: v1, Inserting into BookmarkFolder", query.lastError());


        /// Bookmark has a new FOID field
        if (!query.exec("CREATE TABLE Bookmark2 "
                        "( BID INTEGER PRIMARY KEY AUTOINCREMENT, FOID INTEGER, Name TEXT, URL TEXT, "
                        "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER, "
                        "  FOREIGN KEY(FOID) REFERENCES BookmarkFolder(FOID) ON DELETE RESTRICT )"))
            return Error("Migration Error: v1, Creating Bookmark", query.lastError());

        if (!query.exec("INSERT INTO Bookmark2 (BID, FOID, Name, URL, Desc, DefBFID, Rating, AddDate) "
                        "SELECT BID, 0, Name, URL, Desc, DefBFID, Rating, AddDate FROM Bookmark"))
            return Error("Migration Error: v1, Copying Bookmark", query.lastError());

        if (!query.exec("DROP TABLE Bookmark"))
            return Error("Migration Error: v1, Dropping Bookmark", query.lastError());

        if (!query.exec("ALTER TABLE Bookmark2 RENAME TO Bookmark"))
            return Error("Migration Error: v1, Renaming Bookmark", query.lastError());


        /// BookmarkTrash must get the new FOID field, too.
        if (!query.exec("CREATE TABLE BookmarkTrash2"
                        "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Folder TEXT, Name TEXT, URL TEXT, "
                        "  Desc TEXT, AttachedFIDs TEXT, DefFID INTEGER, Rating INTEGER, "
                        "  Tags TEXT, ExtraInfos TEXT, DeleteDate INTEGER, AddDate INTEGER )"))
            return Error("Migration Error: v1, Creating BookmarkTrash", query.lastError());

        if (!query.exec("INSERT INTO BookmarkTrash2 (BID, Folder, Name, URL, Desc, AttachedFIDs, "
                        "  DefFID, Rating, Tags, ExtraInfos, DeleteDate, AddDate) "
                        "SELECT BID, '', Name, URL, Desc, AttachedFIDs, "
                        "  DefFID, Rating, Tags, ExtraInfos, DeleteDate, AddDate FROM BookmarkTrash"))
            return Error("Migration Error: v1, Copying BookmarkTrash", query.lastError());

        if (!query.exec("DROP TABLE BookmarkTrash"))
            return Error("Migration Error: v1, Dropping BookmarkTrash", query.lastError());

        if (!query.exec("ALTER TABLE BookmarkTrash2 RENAME TO BookmarkTrash"))
            return Error("Migration Error: v1, Renaming BookmarkTrash", query.lastError());

        /// FileArchive has a new field UserAccess
        if (!query.exec("CREATE TABLE FileArchive2 "
                        "( FAID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, Type INTEGER, Path TEXT,"
                        "  FileLayout INTEGER, UserAccess INTEGER )"))
            return Error("Migration Error: v1, Creating FileArchive", query.lastError());

        if (!query.exec("INSERT INTO FileArchive2 (FAID, Name, Type, Path, FileLayout, UserAccess) "
                        "SELECT FAID, Name, Type, Path, FileLayout, 1 FROM FileArchive"))
            return Error("Migration Error: v1, Copying FileArchive", query.lastError());

        if (!query.exec("DROP TABLE FileArchive"))
            return Error("Migration Error: v1, Dropping FileArchive", query.lastError());

        if (!query.exec("ALTER TABLE FileArchive2 RENAME TO FileArchive"))
            return Error("Migration Error: v1, Renaming FileArchive", query.lastError());

        if (!query.exec("Update FileArchive Set UserAccess = 0 "
                        "WHERE Name = ':trash:' OR Name = ':sandbox:'"))
            return Error("Migration Error: v1, Updating FileArchive", query.lastError());

        /// Default FileLayout is now 2; FileLayout 1 that used file name prefixes is deprecated.
        if (!query.exec("Update FileArchive Set FileLayout = 2 WHERE FileLayout = 1"))
            return Error("Migration Error: v1, Updating FileLayout", query.lastError());
    }

    if (dbVersion <= 2)
    {
        /// Bookmark.URL has been renamed to Bookmark.URLs
        if (!query.exec("CREATE TABLE Bookmark2 "
                        "( BID INTEGER PRIMARY KEY AUTOINCREMENT, FOID INTEGER, Name TEXT, URLs TEXT, "
                        "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER, "
                        "  FOREIGN KEY(FOID) REFERENCES BookmarkFolder(FOID) ON DELETE RESTRICT )"))
            return Error("Migration Error: v2, Creating Bookmark", query.lastError());

        if (!query.exec("INSERT INTO Bookmark2 (BID, FOID, Name, URLs, Desc, DefBFID, Rating, AddDate) "
                        "SELECT BID, FOID, Name, URL, Desc, DefBFID, Rating, AddDate FROM Bookmark"))
            return Error("Migration Error: v2, Copying Bookmark", query.lastError());

        if (!query.exec("DROP TABLE Bookmark"))
            return Error("Migration Error: v2, Dropping Bookmark", query.lastError());

        if (!query.exec("ALTER TABLE Bookmark2 RENAME TO Bookmark"))
            return Error("Migration Error: v2, Renaming Bookmark", query.lastError());

        /// BookmarkTrash.URL should be renamed to BookmarkTrash.URLs, too.
        if (!query.exec("CREATE TABLE BookmarkTrash2"
                        "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Folder TEXT, Name TEXT, URLs TEXT, "
                        "  Desc TEXT, AttachedFIDs TEXT, DefFID INTEGER, Rating INTEGER, "
                        "  Tags TEXT, ExtraInfos TEXT, DeleteDate INTEGER, AddDate INTEGER )"))
            return Error("Migration Error: v2, Creating BookmarkTrash", query.lastError());

        if (!query.exec("INSERT INTO BookmarkTrash2 (BID, Folder, Name, URLs, Desc, AttachedFIDs, "
                        "  DefFID, Rating, Tags, ExtraInfos, DeleteDate, AddDate) "
                        "SELECT BID, Folder, Name, URL, Desc, AttachedFIDs, "
                        "  DefFID, Rating, Tags, ExtraInfos, DeleteDate, AddDate FROM BookmarkTrash"))
            return Error("Migration Error: v2, Copying BookmarkTrash", query.lastError());

        if (!query.exec("DROP TABLE BookmarkTrash"))
            return Error("Migration Error: v2, Dropping BookmarkTrash", query.lastError());

        if (!query.exec("ALTER TABLE BookmarkTrash2 RENAME TO BookmarkTrash"))
            return Error("Migration Error: v2, Renaming BookmarkTrash", query.lastError());
    }

    if (dbVersion <= 3)
    {
        /// BookmarkExtraInfo was missing FOREIGN KEY constraints for databases migrated from alpha versions.
        if (!query.exec("CREATE Table BookmarkExtraInfo2"
                        "( BEIID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, "
                        "  Name TEXT, Type TEXT, Value TEXT,"
                        "  FOREIGN KEY(BID) REFERENCES Bookmark(BID) ON DELETE CASCADE )"))
            return Error("Migration Error: v3, Creating BookmarkExtraInfo", query.lastError());

        if (!query.exec("INSERT INTO BookmarkExtraInfo2 (BEIID, BID, Name, Type, Value) "
                        "SELECT BEIID, BID, Name, Type, Value FROM BookmarkExtraInfo"))
            return Error("Migration Error: v3, Copying BookmarkExtraInfo", query.lastError());

        if (!query.exec("DROP TABLE BookmarkExtraInfo"))
            return Error("Migration Error: v3, Dropping BookmarkExtraInfo", query.lastError());

        if (!query.exec("ALTER TABLE BookmarkExtraInfo2 RENAME TO BookmarkExtraInfo"))
            return Error("Migration Error: v3, Renaming BookmarkExtraInfo", query.lastError());

        /// Remove BookmarkExtraInfo for bookmarks that don't exist
        if (!query.exec("DELETE FROM BookmarkExtraInfo WHERE BID NOT IN (SELECT BID FROM Bookmark)"))
            return Error("Migration Error: v3, Updating BookmarkExtraInfo", query.lastError());

        /// Migration v1 -> 2 fixed BookmarkLink's foreign key, but hasn't deleted wrong values.
        if (!query.exec("DELETE FROM BookmarkLink WHERE BID1 NOT IN (SELECT BID FROM Bookmark)"))
            return Error("Migration Error: v4, Updating BookmarkLink (step 1)", query.lastError());

        if (!query.exec("DELETE FROM BookmarkLink WHERE BID2 NOT IN (SELECT BID FROM Bookmark)"))
            return Error("Migration Error: v4, Updating BookmarkLink (step 2)", query.lastError());

        ///  Migration v1 -> 2 fixed BookmarkTag's foreign key, but hasn't deleted wrong values.
        if (!query.exec("DELETE FROM BookmarkTag WHERE BID NOT IN (SELECT BID FROM Bookmark)"))
            return Error("Migration Error: v4, Updating BookmarkTag", query.lastError());
    }

    if (!query.exec("UPDATE Info SET Version = " + QString::number(conf->programDatabaseVersion)))
        return Error("Migration Error: Updating database version", query.lastError());

    //We don't need to clear the query, it gets destroyed here and will not cause problems for
    //  committing the transaction.
    //  http://stackoverflow.com/questions/29532447/sqlite-transaction-in-qt (Read comments).

    return true;
}
