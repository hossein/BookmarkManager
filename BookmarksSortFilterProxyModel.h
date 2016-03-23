#pragma once
#include <QSortFilterProxyModel>
#include "IManager.h"

#include "BookmarkFilter.h"
#include "DatabaseManager.h"
#include <QSet>

class BookmarksSortFilterProxyModel : public QSortFilterProxyModel, public IManager
{
    Q_OBJECT

private:
    DatabaseManager* dbm;
    BookmarkFilter m_filter;
    bool allowAllBookmarks;
    QSet<long long> filteredBookmarkIDs;

public:
    BookmarksSortFilterProxyModel(DatabaseManager* dbm, QWidget* dialogParent,
                                  QObject* parent = NULL);

    //This function may change the state of the this class as the filterer, so IMPORTANT:
    //  Call `invalidateFilter()` after changing the state. This causes the `layoutChanged()`
    //  (on sorting) or `rowsInserted/Removed` (probably on filtering) signals be emitted and
    //  be caught by the view using this model, causing it to update itself.
    //  Without `layoutChanged()` the Bookmarks view doesn't update itself.
    //forceReset: If user adds/deletes a new bookmark to a folder/tag, refreshing the view doesn't
    //  show the new bookmark because although bookmarks changed, folders/tags filters are the same
    //  and the filter didn't change so this function skips filtering the bookmarks again. In such
    //  situations, we should `forceReset` the filter and filtering the bookmarks again to include
    //  the new bookmark.
    bool SetFilter(const BookmarkFilter& filter, bool forceReset);

    // QAbstractItemModel interface
public:
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual QStringList mimeTypes() const;
    virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
    virtual Qt::DropActions supportedDragActions() const;

private:
    bool populateFilteredBookmarkIDs();

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
};
