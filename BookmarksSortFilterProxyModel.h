#pragma once
#include <QSortFilterProxyModel>
#include "IManager.h"

#include "DatabaseManager.h"
#include <QSet>

class BookmarksSortFilterProxyModel : public QSortFilterProxyModel, public IManager
{
    Q_OBJECT

private:
    DatabaseManager* dbm;
    bool allowAllBookmarks;
    QSet<long long> filteredBookmarkIDs;

public:
    BookmarksSortFilterProxyModel(DatabaseManager* dbm, QWidget* dialogParent, Config* conf,
                                          QObject* parent = NULL);

    //These functions change the state of the this class as the filterer, so IMPORTANT:
    //  Call `invalidateFilter()` after changing the state. This causes the `layoutChanged()`
    //  (on sorting) or `rowsInserted/Removed` (probably on filtering) signals be emitted and
    //  be caught by the view using this model, causing it to update itself.
    //  Without `layoutChanged()` the Bookmarks view doesn't update itself.
    void ClearFilters();
    bool FilterSpecificBookmarkIDs(const QList<long long>& BIDs);
    bool FilterSpecificTagIDs(const QSet<long long>& tagIDs);

private:
    bool populateValidBookmarkIDs(const QSet<long long>& tagIDs);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
};
