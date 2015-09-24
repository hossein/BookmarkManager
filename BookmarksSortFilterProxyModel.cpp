#include "BookmarksSortFilterProxyModel.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

BookmarksSortFilterProxyModel::BookmarksSortFilterProxyModel
    (DatabaseManager* dbm, QWidget* dialogParent, Config* conf, QObject* parent)
    : QSortFilterProxyModel(parent), IManager(dialogParent, conf), dbm(dbm), allowAllBookmarks(true)
{

}

bool BookmarksSortFilterProxyModel::SetFilter(const BookmarkFilter& filter)
{
    if (!m_filter.FilterEquals(filter))
    {
        m_filter = filter;
        bool success = populateFilteredBookmarkIDs();
        invalidateFilter(); //Read function header docs
        return success;
    }
    return true;
}

bool BookmarksSortFilterProxyModel::populateFilteredBookmarkIDs()
{
    bool first = true;
    bool success = true;
    allowAllBookmarks = true;
    filteredBookmarkIDs.clear();

    //We will add items of the first filter to the master filter set, then we will intersect it with
    //  the future filters. Hence the unite/intersect function choices below. (Well `intersect` for
    //  the first filter is unused.) Can't use `if (filteredBookmarkIDs.empty())` because it may
    //  have become empty in the previous filters, So we use a `first` variable.

    if (!m_filter.filterBIDs.empty())
    {
        allowAllBookmarks = false;
        if (first)
            filteredBookmarkIDs.unite(m_filter.filterBIDs);
        else
            filteredBookmarkIDs.intersect(m_filter.filterBIDs);
        first = false;
    }

    if (!m_filter.filterTIDs.empty())
    {
        allowAllBookmarks = false;
        QSet<long long> bookmarkIDsForTags;
        if (!getBookmarkIDsForTags(m_filter.filterTIDs, bookmarkIDsForTags))
            success = false;
        if (first)
            filteredBookmarkIDs.unite(bookmarkIDsForTags);
        else
            filteredBookmarkIDs.intersect(bookmarkIDsForTags);
        first = false;
    }

    return success;
}

bool BookmarksSortFilterProxyModel::getBookmarkIDsForTags(const QSet<long long>& tagIDs, QSet<long long>& bookmarkIDs)
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

    //Do it for caller.
    bookmarkIDs.clear();

    //`query.record()` works even if zero bookmarks were returned (related: query.isActive|isValid).
    //  Moreover, the indexes are also correct in this case, just the record is empty.
    //if (!query.first()) //Simply no results where returned.
    //  return true;
    int indexOfBID = query.record().indexOf("BID");
    while (query.next())
        bookmarkIDs.insert(query.value(indexOfBID).toLongLong());

    return true;
}

bool BookmarksSortFilterProxyModel::filterAcceptsRow
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
