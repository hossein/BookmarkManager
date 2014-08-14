#pragma once
#include <QSortFilterProxyModel>
#include "IManager.h"

#include "DatabaseManager.h"
#include <QSet>

class BookmarksFilteredByTagsSortProxyModel : public QSortFilterProxyModel, public IManager
{
    Q_OBJECT

private:
    DatabaseManager* dbm;
    bool allowAllTags;
    QSet<long long> filteredBookmarkIDs;

public:
    BookmarksFilteredByTagsSortProxyModel(DatabaseManager* dbm, QWidget* dialogParent, Config* conf,
                                          QObject* parent = NULL);

    //These functions change the state of the this class as the filterer, so IMPORTANT:
    //  Call `invalidateFilter()` after changing the state. This causes the `layoutChanged()`
    //  or `rowsInserted/Removed` signals be emitted and be caught by the view using this model,
    //  causing it to update itself. Without `layoutChanged()` the view doesn't update itself.
    void ClearFilters();
    bool FilterSpecificTagIDs(const QSet<long long>& tagIDs);

private:
    bool populateValidBookmarkIDs(const QSet<long long>& tagIDs);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
};
