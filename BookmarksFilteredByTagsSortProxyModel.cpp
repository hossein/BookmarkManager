#include "BookmarksFilteredByTagsSortProxyModel.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

BookmarksFilteredByTagsSortProxyModel::BookmarksFilteredByTagsSortProxyModel
    (DatabaseManager* dbm, QWidget* dialogParent, Config* conf, QObject* parent)
    : QSortFilterProxyModel(parent), IManager(dialogParent, conf), dbm(dbm), allowAllBookmarks(true)
{

}

void BookmarksFilteredByTagsSortProxyModel::ClearFilters()
{
    allowAllBookmarks = true;
    filteredBookmarkIDs.clear();
    invalidateFilter(); //Read function header docs
}

bool BookmarksFilteredByTagsSortProxyModel::FilterSpecificBookmarkIDs(const QList<long long>& BIDs)
{
    allowAllBookmarks = false;
    filteredBookmarkIDs.clear();
    foreach (long long BID, BIDs)
        filteredBookmarkIDs.insert(BID);
    invalidateFilter(); //Read function header docs
    return true;
}

bool BookmarksFilteredByTagsSortProxyModel::FilterSpecificTagIDs(const QSet<long long>& tagIDs)
{
    allowAllBookmarks = false;
    bool populateSuccess = populateValidBookmarkIDs(tagIDs);
    invalidateFilter(); //Read function header docs
    return populateSuccess;
}

bool BookmarksFilteredByTagsSortProxyModel::populateValidBookmarkIDs(const QSet<long long>& tagIDs)
{
    //NOTE: We should show a "loading" cursor to user when this class is initialized.

    QString commaSeparatedTIDs;
    foreach (long long TID, tagIDs)
        commaSeparatedTIDs += QString::number(TID) + ",";
    //Remove the last comma.
    if (commaSeparatedTIDs.length() > 0) //Empty-check
        commaSeparatedTIDs.chop(1);

    QString retrieveError = "Could not get tag information for bookmarks from database.";
    QSqlQuery query(dbm->db);
    //Note: Ordering by BID or anything here is useless. We use this info to filter the data later.
    //  Also empty commaSeparatedTIDs string is fine.
    query.prepare("SELECT DISTINCT BID FROM BookmarkTag WHERE TID IN (" + commaSeparatedTIDs + ")");

    if (!query.exec())
        return Error(retrieveError, query.lastError());

    //Might have remained from previous filter; called without calling `ClearFilters` first.
    filteredBookmarkIDs.clear();

    //`query.record()` works even if zero bookmarks were returned (related: query.isActive|isValid).
    //  Moreover, the indexes are also correct in this case, just the record is empty.
    //if (!query.first()) //Simply no results where returned.
    //  return true;
    int indexOfBID = query.record().indexOf("BID");
    while (query.next())
        filteredBookmarkIDs.insert(query.value(indexOfBID).toLongLong());

    return true;
}

bool BookmarksFilteredByTagsSortProxyModel::filterAcceptsRow
    (int source_row, const QModelIndex& source_parent) const
{
    Q_UNUSED(source_parent);

    if (allowAllBookmarks)
        return true;

    if (filteredBookmarkIDs.contains(
        dbm->bms.model.index(source_row, dbm->bms.bidx.BID).data().toLongLong()))
        return true;

    return false;
}
