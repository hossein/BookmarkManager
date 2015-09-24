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
    bool SetFilter(const BookmarkFilter& filter);

private:
    bool populateFilteredBookmarkIDs();
    bool getBookmarkIDsForTags(const QSet<long long>& tagIDs, QSet<long long>& bookmarkIDs);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
};
