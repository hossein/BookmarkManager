#include "BookmarksSortFilterProxyModel.h"

#include "Config.h"

#include <QMimeData>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

BookmarksSortFilterProxyModel::BookmarksSortFilterProxyModel
    (DatabaseManager* dbm, QWidget* dialogParent, QObject* parent)
    : QSortFilterProxyModel(parent), IManager(dialogParent, dbm->conf), dbm(dbm), allowAllBookmarks(true)
{

}

bool BookmarksSortFilterProxyModel::SetFilter(const BookmarkFilter& filter, bool forceReset)
{
    //If the number of bookmarks is big, we can show a busy cursor to user while filtering.
    if (forceReset || !m_filter.FilterEquals(filter))
    {
        m_filter = filter;
        bool success = populateFilteredBookmarkIDs();
        invalidateFilter(); //Read function header docs
        return success;
    }
    return true;
}

Qt::ItemFlags BookmarksSortFilterProxyModel::flags(const QModelIndex& index) const
{
    //This is required to make items draggable. However the drag pixmap is ugly; researching it led
    //  to the need reimplement QAbstractItemView::startDrag, which uses D-pointers so we stopped.

    //As per docs: 'Using Drag and Drop with Item Views'
    Qt::ItemFlags defaultFlags = QSortFilterProxyModel::flags(index);
    return index.isValid() ? Qt::ItemIsDragEnabled | defaultFlags : defaultFlags;
}

QStringList BookmarksSortFilterProxyModel::mimeTypes() const
{
    return QStringList() << conf->mimeTypeBookmarks;
}

QMimeData* BookmarksSortFilterProxyModel::mimeData(const QModelIndexList& indexes) const
{
    //indexes contains a list of all rows and each column separately. So we first collect all BIDs.
    QSet<long long> BIDs;
    foreach (const QModelIndex& index, indexes)
    {
        if (!index.isValid())
            continue;
        long long BID = this->index(index.row(), dbm->bms.bidx.BID).data().toLongLong();
        BIDs.insert(BID);
    }

    QByteArray encData;
    foreach (long long BID, BIDs)
    {
        encData += QByteArray::number(BID);
        encData += ',';
    }
    encData.chop(1);

    QMimeData *mimeData = new QMimeData();
    mimeData->setData(conf->mimeTypeBookmarks, encData);
    return mimeData;
}

Qt::DropActions BookmarksSortFilterProxyModel::supportedDragActions() const
{
    //QSortFilterProxyModel::supportedDragActions() supports Copy only.
    //This must match BookmarkFoldersTreeWidget::supportedDropActions.
    return Qt::MoveAction;
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

    if (!m_filter.filterFOIDs.empty())
    {
        allowAllBookmarks = false;
        QSet<long long> bookmarkIDsForFolders;
        if (!dbm->bms.RetrieveBookmarksInFolders(bookmarkIDsForFolders, m_filter.filterFOIDs))
            success = false;
        if (first)
            filteredBookmarkIDs.unite(bookmarkIDsForFolders);
        else
            filteredBookmarkIDs.intersect(bookmarkIDsForFolders);
        first = false;
    }

    if (m_filter.hasFilterBIDs)
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
