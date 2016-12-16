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

    //For Folder or Tag filtering, the filterer just checks if there are entries or not.
    //However if the list of BIDs to filter is empty, it's not clear whether user has not set a
    //  filter, or the list of bookmarks to filter is empty. E.g when a search returns no bookmark
    //  results, or when the list of linked (related) bookmarks for a bookmark is empty, previously
    //  the filterer showed all bookmarks because filterBIDs was empty. Therefore, instead of
    //  checking if filterBIDs is empty, the following variable has to be used instead.
    bool hasFilterBIDs;

public:
    BookmarkFilter()
    {
        hasFilterBIDs = false;
    }

    void ClearFilters()
    {
        hasFilterBIDs = false;
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
        hasFilterBIDs = true;
        filterBIDs = QSet<long long>::fromList(BIDs);
    }

    void FilterSpecificTagIDs(const QSet<long long>& TIDs)
    {
        filterTIDs = TIDs;
    }

private:
    bool FilterEquals(const BookmarkFilter& another)
    {
        return (hasFilterBIDs == another.hasFilterBIDs)
            && (filterFOIDs== another.filterFOIDs)
            && (filterBIDs == another.filterBIDs)
            && (filterTIDs == another.filterTIDs);
    }
};
