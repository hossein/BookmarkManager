#pragma once
#include <QString>
#include "BookmarkManager.h"

class DatabaseManager;

class BookmarksBusinessLogic
{
private:
    DatabaseManager* dbm;
    QWidget* dialogParent;

public:
    BookmarksBusinessLogic(DatabaseManager* dbm, QWidget* dialogParent);

    bool RetrieveBookmarkEx(long long BID, BookmarkManager::BookmarkData& bdata,
                            bool extraInfosModel, bool filesModel);

    void BeginActionTransaction();
    void CommitActionTransaction();
    bool RollBackActionTransaction();

    //Shortcut function that wraps AddOrEditBookmark in a transaction.
    bool AddOrEditBookmarkTrans(
            long long& editBId, BookmarkManager::BookmarkData& bdata,
            long long originalEditBId, BookmarkManager::BookmarkData& editOriginalBData,
            const QList<long long>& editedLinkedBookmarks,
            const QStringList& tagsList, QList<long long>& associatedTIDs,
            const QList<FileManager::BookmarkFile>& editedFilesList, int defaultFileIndex);

    //Needs transaction to have been started before calling.
    bool AddOrEditBookmark(
            long long& editBId, BookmarkManager::BookmarkData& bdata,
            long long originalEditBId, BookmarkManager::BookmarkData& editOriginalBData,
            const QList<long long>& editedLinkedBookmarks,
            const QStringList& tagsList, QList<long long>& associatedTIDs,
            const QList<FileManager::BookmarkFile>& editedFilesList, int defaultFileIndex);

    //The former is a shortcut function that wraps the latter which needs a transaction in a
    //  transaction.
    bool DeleteBookmarkTrans(long long BID);
    bool DeleteBookmark(long long BID);

    //Need transactions. These MOVE bookmarks and their files; not for adding new bookmarks.
    bool MoveBookmarksToFolderTrans(const QList<long long>& BIDs, long long FOID);
    bool MoveBookmarkToFolderTrans(long long BID, long long FOID);
    bool MoveBookmarkToFolder(long long BID, long long FOID);
};
