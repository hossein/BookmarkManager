#include "BookmarkRemovalManager.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>
#include <QtSql/QSqlTableModel>

BookmarkRemovalManager::BookmarkRemovalManager(QWidget* dialogParent, DatabaseManager* dbm, Config* conf)
    : ISubManager(dialogParent, conf), dbm(dbm)
{
}

bool BookmarkRemovalManager::MoveBookmarkToTrash(long long BID, TagManager)
{
    //Steps in deleting a bookmark:
    //  1. Convert its attached file id's to CSV string.
    //  2. Move its files to :trash: archive (file id's won't change).
    //  3. Convert its tag names to CSV string.
    //  4. Convert its extra info to one big chunk of text.
    //  5. Linked bookmarks will be forgotten.
    //  6. Move the information to BookmarkTrash table.

    bool success = true;
    BookmarkData bdata;

    success = RetrieveBookmark(BID, bdata);
    if (!success)
        return false;

    success = RetrieveLinkedBookmarks(BID, bdata.Ex_LinkedBookmarksList);
    if (!success)
        return false;

    success = RetrieveBookmarkExtraInfos(BID, bdata.Ex_ExtraInfosList);
    if (!success)
        return false;

    success = dbm.
    QSqlQuery query(db);
    query.prepare("DELETE FROM Bookmark WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error("Could not delete bookmark.", query.lastError());

    return true;
}

void BookmarkRemovalManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE BookmarkTrash"
               "( BID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, URL TEXT, "
               "  Desc TEXT, DefBFID INTEGER, Rating INTEGER, AddDate INTEGER )");
}

void BookmarkRemovalManager::PopulateModels()
{
    //No models to populate.
}
