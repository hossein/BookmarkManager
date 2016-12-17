#include "BookmarkFolderManager.h"

#include <QQueue>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

#include "Util/Util.h"

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
    bool isAdd = (FOID == -1);
    QString updateError = (isAdd
                           ? "Could not add folder information to database."
                           : "Could not edit folder information.");

    QString querystr;
    if (isAdd)
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

    if (!isAdd) //Edit
        query.addBindValue(FOID);

    if (!query.exec())
        return Error(updateError, query.lastError());

    if (isAdd)
    {
        long long addedFOID = query.lastInsertId().toLongLong();
        FOID = addedFOID;
        fodata.FOID = addedFOID;
    }

    //We are [RESPONSIBLE] for updating the internal tables.
    if (isAdd)
    {
        fodata.Ex_AbsolutePath = bookmarkFolders[fodata.ParentFOID].Ex_AbsolutePath + fodata.Name + '/';
        bookmarkFolders[FOID] = fodata;
    }
    else
    {
        //Note: We can do additional checks and actions, but by our standards doing that would need
        //  transactions and complete rolling back in case of return Errors and stuff; we won't do
        //  them here!

        QString originalName = bookmarkFolders[FOID].Name;
        QString originalDefFileArchive = bookmarkFolders[FOID].DefFileArchive;
        long long originalParentFOID = bookmarkFolders[FOID].ParentFOID;

        if (originalName != fodata.Name)
        {
            //If name changed, we need to recursively fix the absolute paths of all children.
            CalculateAbsolutePaths(); //We don't return it this fails (it just checks integrity).

            //Note: We can also rename the folder on file system, but that would need transactions.
        }
        if (originalDefFileArchive != fodata.DefFileArchive)
        {
            //Note: We can also move files to new archive (taking into account the changed
            //  fodata.Name of course), but that would need transactions.
        }
        if (originalParentFOID != fodata.ParentFOID)
        {
            //(Enabling this would need transactions.)
            //If we allow all three together, implementation would become difficult. (But maybe not also!)
            //if (originalName != fodata.Name || originalDefFileArchive != fodata.DefFileArchive)
            //    return Error("Can't change parent folder while changing folder name or file archive.");

            //Note: We can also move folders inside archive, but that would need transactions.
        }
    }

    //And finally, don't forget to:
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

bool BookmarkFolderManager::GetFileArchiveAndFolderHint(long long FOID, QString& fileArchiveName,
                                                        QString& folderHint)
{
    if (!bookmarkFolders.contains(FOID))
        return Error("Could not get folder file archive information: The selected folder was not found.");

    folderHint = "";
    fileArchiveName = bookmarkFolders[FOID].DefFileArchive;
    while (fileArchiveName.isEmpty()) //Inherited from parent
    {
        folderHint = bookmarkFolders[FOID].Name + "/" + folderHint;

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

QString BookmarkFolderManager::GetPathOrName(long long FOID)
{
    QString folderName = bookmarkFolders[FOID].Ex_AbsolutePath;
    //For the '0, Unsorted' and '-1, All Bookmarks' folders the path is empty.
    if (folderName.isEmpty())
        folderName =  bookmarkFolders[FOID].Name;
    return folderName;
}

bool BookmarkFolderManager::CalculateAbsolutePaths()
{
    //We can't assume all parents come before their children; either do it recursively or this way.
    int count = 0;
    QQueue<long long> foldersQueue;
    foldersQueue.enqueue(-1); //The fake '-1, All Bookmarks' folder. It's also the parent of the
                              //'0, Unsorted', which is itself the parent of all others.
    while (!foldersQueue.isEmpty())
    {
        //Get a folder from queue
        long long FOID = foldersQueue.dequeue();
        count += 1;

        //Calculate absolute path of this folder
        if (FOID <= 0)
        {
             //Don't care about the '0, Unsorted' or '-1, All Bookmarks' folders.
            bookmarkFolders[FOID].Ex_AbsolutePath = "";
        }
        else
        {
            long long ParentFOID = bookmarkFolders[FOID].ParentFOID;
            bookmarkFolders[FOID].Ex_AbsolutePath =
                    bookmarkFolders[ParentFOID].Ex_AbsolutePath +
                    bookmarkFolders[FOID].Name + '/';
        }

        //Put children on queue
        foreach (const BookmarkFolderData& fodata, bookmarkFolders)
            if (fodata.ParentFOID == FOID && fodata.FOID != -1) //Parent of '-1, All Bookmarks' folder is also -1.
                foldersQueue.enqueue(fodata.FOID);
    }

    if (count != bookmarkFolders.count()) //We have an orphaned BookmarkFolder, error!
        return Error("Error in bookmark folders' structure.");
    return true;
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
    query.addBindValue(QVariant()); //Not -1, it will cause a foreign key violation error
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

    //Add a fake '-1, All Bookmarks' item.
    fodata.FOID = -1;
    fodata.ParentFOID = -1;
    fodata.Name = "All Bookmarks";
    fodata.Desc = "Show all bookmarks in every folder (read-only).";
    fodata.DefFileArchive = -1; //Invalid value
    bookmarkFolders.insert(fodata.FOID, fodata);

    //Special care with ParentFOID because parent of '0, Unsorted' is NULL.
    while (query.next())
    {
        const QSqlRecord record = query.record();
        fodata.FOID           = record.value("FOID"          ).toLongLong();
        fodata.ParentFOID     =(record.value("ParentFOID"    ).isNull() ? -1 : record.value("ParentFOID").toLongLong());
        fodata.Name           = record.value("Name"          ).toString();
        fodata.Desc           = record.value("Desc"          ).toString();
        fodata.DefFileArchive = record.value("DefFileArchive").toString();
        bookmarkFolders.insert(fodata.FOID, fodata);
    }

    //We do this separate from the above loop loop because some parent-ids might come later than
    //  child ids, e.g if/when we implement moving bookmark folders around and altering their tree.
    CalculateAbsolutePaths();
}
