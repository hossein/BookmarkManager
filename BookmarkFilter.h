#pragma once
#include <QList>
#include <QSet>

/// We use BookmarkFilter class and SetFilter for filtering the bookmarks, instead of individual
///   functions.
/// This allows for applying multiple filters, and we can make an string using the data here to
///   show to the user.
/// Implementation Note: To simplify memory management, this class is copied among users. Only
///   use member types with low copy overhead, e.g those using Qt's implicit sharing mechanism.
struct BookmarkFilter
{
private:
    friend class BookmarksSortFilterProxyModel;
    QSet<long long> filterFOIDs;
    QSet<long long> filterBIDs;
    QSet<long long> filterTIDs;

public:
    void ClearFilters()
    {
        filterFOIDs.clear();
        filterBIDs.clear();
        filterTIDs.clear();
    }

    void FilterSpecificFolderIDs(const QSet<long long>& FOIDs)
    {
        filterFOIDs = FOIDs;
    }

    void FilterSpecificBookmarkIDs(const QList<long long>& BIDs)
    {
        filterBIDs = QSet<long long>::fromList(BIDs);
    }

    void FilterSpecificTagIDs(const QSet<long long>& TIDs)
    {
        filterTIDs = TIDs;
    }

private:
    bool FilterEquals(const BookmarkFilter& another)
    {
        return (filterFOIDs== another.filterFOIDs)
            && (filterBIDs == another.filterBIDs)
            && (filterTIDs == another.filterTIDs);
    }
};
