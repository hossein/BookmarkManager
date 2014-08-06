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
    QSet<long long> filteredBookmarkIDs;

public:
    BookmarksFilteredByTagsSortProxyModel(DatabaseManager* dbm, const QSet<long long>& tagIDs,
                                         QWidget* dialogParent, Config* conf, QObject* parent = NULL);

private:
    bool populateValidBookmarkIDs(const QSet<long long>& tagIDs);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
};
