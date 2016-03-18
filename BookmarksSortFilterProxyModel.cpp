#include "BookmarksSortFilterProxyModel.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

BookmarksSortFilterProxyModel::BookmarksSortFilterProxyModel
    (DatabaseManager* dbm, QWidget* dialogParent, QObject* parent)
    : QSortFilterProxyModel(parent), IManager(dialogParent, dbm->conf), dbm(dbm), allowAllBookmarks(true)
{

}

bool BookmarksSortFilterProxyModel::SetFilter(const BookmarkFilter& filter)
{
    //If the number of bookmarks is big, we can show a busy cursor to user while filtering.
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
        if (!dbm->tags.GetBookmarkIDsForTags(m_filter.filterTIDs, bookmarkIDsForTags))
            success = false;
        if (first)
            filteredBookmarkIDs.unite(bookmarkIDsForTags);
        else
            filteredBookmarkIDs.intersect(bookmarkIDsForTags);
        first = false;
    }

    return success;
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
