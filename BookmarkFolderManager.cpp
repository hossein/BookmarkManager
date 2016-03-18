#include "BookmarkFolderManager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

#include "Util.h"

BookmarkFolderManager::BookmarkFolderManager(QWidget* dialogParent, Config* conf)
    : ISubManager(dialogParent, conf)
{
}

bool BookmarkFolderManager::RetrieveBookmarkFolder(long long FOID, BookmarkFolderManager::BookmarkFolderData& fodata)
{
    if (!bookmarkFolders.contains(FOID))
        return Error("Could not get folder information: The selected folder was not found.");
    fodata = bookmarkFolders[FOID];
    return true;
}

bool BookmarkFolderManager::AddOrEditBookmarkFolder(long long& FOID, BookmarkFolderData& fodata)
{
    QString updateError = (FOID == -1
                           ? "Could not add folder information to database."
                           : "Could not edit folder information.");

    QString querystr;
    if (FOID == -1)
    {
        querystr = "INSERT INTO BookmarkFolder(ParentFOID, Name, Desc, DefFileArchive) "
                   "VALUES (?, ?, ?, ?);";
    }
    else
    {
        querystr =
                "UPDATE BookmarkFolder "
                "SET ParentFOID = ?, Name = ?, Desc = ?, DefFileArchive = ? "
                "WHERE FOID = ?";
    }

    QSqlQuery query(db);
    query.prepare(querystr);

    query.addBindValue(fodata.ParentFOID);
    query.addBindValue(fodata.Name);
    query.addBindValue(fodata.Desc);
    query.addBindValue(fodata.DefFileArchive);

    if (FOID != -1) //Edit
        query.addBindValue(FOID);

    if (!query.exec())
        return Error(updateError, query.lastError());

    if (FOID == -1)
    {
        long long addedFOID = query.lastInsertId().toLongLong();
        FOID = addedFOID;
        fodata.FOID = addedFOID;
    }

    //We are [RESPONSIBLE] for updating the internal tables.
    fodata.Ex_AbsolutePath = bookmarkFolders[fodata.ParentFOID].Ex_AbsolutePath + fodata.Name + '/';
    bookmarkFolders[FOID] = fodata;

    return true;
}

bool BookmarkFolderManager::RemoveBookmarkFolder(long long FOID)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM BookmarkFolder WHERE FOID = ?");
    query.addBindValue(FOID);

    if (!query.exec())
        return Error("Could not remove folder.", query.lastError());

    //We are [RESPONSIBLE] for updating the internal tables.
    bookmarkFolders.remove(FOID);

    return true;
}

bool BookmarkFolderManager::GetFileArchiveForBookmarkFolder(long long FOID, QString& fileArchiveName)
{
    if (!bookmarkFolders.contains(FOID))
        return Error("Could not get folder file archive information: The selected folder was not found.");

    fileArchiveName = bookmarkFolders[FOID].DefFileArchive;
    while (fileArchiveName.isEmpty()) //Inherited from parent
    {
        //Important: The '0, Unsorted' folder must always have a fixed file archive set, or this
        //  parent-finding will surpass that folder as well and cause errors.
        FOID = bookmarkFolders[FOID].ParentFOID;
        fileArchiveName = bookmarkFolders[FOID].DefFileArchive;
    }

    return true;
}

QList<long long> BookmarkFolderManager::GetChildrenIDs(long long FOID)
{
    QList<long long> childrenIDs;
    foreach (BookmarkFolderData fodata, bookmarkFolders)
        if (fodata.ParentFOID == FOID)
            childrenIDs.append(fodata.FOID);
    return childrenIDs;
}

QStringList BookmarkFolderManager::GetChildrenNames(long long FOID)
{
    QStringList childrenNames;
    foreach (BookmarkFolderData fodata, bookmarkFolders)
        if (fodata.ParentFOID == FOID)
            childrenNames.append(fodata.Name);
    return childrenNames;
}

void BookmarkFolderManager::CreateTables()
{
    QSqlQuery query(db);

    //Empty DefFileArchive for a BookmarkFolder means it inherits its parent's DefFileArchive.
    //Don't allow removing a folder that has sub-folders: ON DELETE RESTRICT.
    query.exec("CREATE TABLE BookmarkFolder "
               "( FOID INTEGER PRIMARY KEY AUTOINCREMENT, ParentFOID INTEGER, "
               "  Name TEXT, Desc TEXT, DefFileArchive TEXT, "
               "  FOREIGN KEY(ParentFOID) REFERENCES BookmarkFolder(FOID) ON DELETE RESTRICT )");

    //Insert the first Folder.
    query.prepare("INSERT INTO BookmarkFolder(FOID, ParentFOID, Name, Desc, DefFileArchive) VALUES (?, ?, ?, ?, ?);");
    query.addBindValue(0); //Force first PK to be 0.
    query.addBindValue(-1);
    query.addBindValue("Unsorted Bookmarks");
    query.addBindValue("Bookmarks that still aren't in a folder.");
    query.addBindValue(":arch0:");
    query.exec();
}

void BookmarkFolderManager::PopulateModelsAndInternalTables()
{
    QSqlQuery query(db);
    bool success = query.exec("SELECT * FROM BookmarkFolder");

    if (!success)
    {
        Error("Error while reading bookmark folders.", query.lastError());
        return;
    }

    //Set indexes
    //Indexes of empty query.record() from empty table are correct.
    const QSqlRecord record = query.record();
    foidx.FOID           = record.indexOf("FOID"          );
    foidx.ParentFOID     = record.indexOf("ParentFOID"    );
    foidx.Name           = record.indexOf("Name"          );
    foidx.Desc           = record.indexOf("Desc"          );
    foidx.DefFileArchive = record.indexOf("DefFileArchive");

    //Populate internal tables
    bookmarkFolders.clear();
    BookmarkFolderData fodata;
    while (query.next())
    {
        const QSqlRecord record = query.record();
        fodata.FOID           = record.value("FOID"          ).toLongLong();
        fodata.ParentFOID     = record.value("ParentFOID"    ).toLongLong();
        fodata.Name           = record.value("Name"          ).toString();
        fodata.Desc           = record.value("Desc"          ).toString();
        fodata.DefFileArchive = record.value("DefFileArchive").toString();
        bookmarkFolders.insert(fodata.FOID, fodata);
    }

    //We do this in a separate loop because some parent-ids might come later than child ids.
    //We can't assume all parents come before their children.
    foreach (long long FOID, bookmarkFolders.keys())
    {
        if (FOID == 0)
        {
             //Don't care about the first Unsorted folder.
            bookmarkFolders[FOID].Ex_AbsolutePath = "";
        }
        else
        {
            bookmarkFolders[FOID].Ex_AbsolutePath = bookmarkFolders[FOID].Name + '/';
            long long ParentFOID = bookmarkFolders[FOID].ParentFOID;
            while (ParentFOID != 0)
            {
                bookmarkFolders[FOID].Ex_AbsolutePath.insert(0, bookmarkFolders[ParentFOID].Name + '/');
                ParentFOID = bookmarkFolders[ParentFOID].ParentFOID;
            }
        }
    }
}
