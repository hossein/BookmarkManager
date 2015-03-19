#pragma once
#include <QString>
#include "BookmarkManager.h"

class Config;
class DatabaseManager;

class BookmarksBusinessLogic
{
private:
    DatabaseManager* dbm;
    Config* conf;
    QWidget* dialogParent;

public:
    BookmarksBusinessLogic(DatabaseManager* dbm, Config* conf, QWidget* dialogParent);

    bool AddOrEditBookmark(long long& editBId, BookmarkManager::BookmarkData& bdata,
                           long long originalEditBId, BookmarkManager::BookmarkData& editOriginalBData,
                           const QList<long long>& editedLinkedBookmarks,
                           const QStringList& tagsList, QList<long long>& associatedTIDs,
                           const QList<FileManager::BookmarkFile>& editedFilesList,  int defaultFileIndex);

    bool DeleteBookmark(long long BID);

private:
    /// Rolls back transactions and shows error if they failed, also sets editBId to originalEditBId.
    bool DoRollBackAction();
    bool DoRollBackAction(long long& editBId, const long long originalEditBId);
};
