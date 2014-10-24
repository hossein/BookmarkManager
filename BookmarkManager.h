#pragma once
#include "ISubManager.h"
#include "FileManager.h"
#include <QStringList>
#include <QtSql/QSqlQueryModel>

class DatabaseManager;

class BookmarkManager : public ISubManager
{
    friend class DatabaseManager;

public:
    QSqlQueryModel model;

    struct BookmarkIndexes
    {
        int BID;
        int Name;
        int URL;
        int Desc;
        int DefBFID;
        int Rating;
        int AddDate;
    } bidx;

    struct BookmarkData
    {
        long long BID;
        QString Name;
        QString URL;
        QString Desc;
        long long DefBFID;
        int Rating;
        long long AddDate;

        /// The following members ARE NOT filled by the `RetrieveBookmark` function.
        /// and ARE NOT saved by the `AddOrEditBookmark` function.
        QList<long long> Ex_LinkedBookmarksList;
        QStringList Ex_TagsList;
        QSqlQueryModel Ex_FilesModel;
        QList<FileManager::BookmarkFile> Ex_FilesList;
    };

    struct BookmarkExtraInfoData
    {
        //These are JSON's basic non-array types.
        enum DataType
        {
            Type_Null = 0,
            Type_Text = 1,
            Type_Number = 2,
            Type_Boolean = 3
        };

        long long BEIID;
        QString Name;
        DataType Type;
        QString Value;
    };

public:
    BookmarkManager(QWidget* dialogParent, Config* conf);

    bool RetrieveBookmark(long long BID, BookmarkData& bdata);
    /// For adding bookmark (i.e when BID == -1), both the BID arg and bdata.BID will contain
    ///   the BID of the inserted bookmark.
    bool AddOrEditBookmark(long long& BID, BookmarkData& bdata);
    bool SetBookmarkDefBFID(long long BID, long long BFID);
    bool DeleteBookmark(long long BID);

    bool RetrieveLinkedBookmarks(long long BID, QList<long long>& linkedBIDs);
    bool UpdateLinkedBookmarks(long long BID, const QList<long long>& originalLinkedBIDs,
                               const QList<long long>& editedLinkedBIDs);
    bool LinkBookmarksTogether(long long BID1, long long BID2);
    bool RemoveBookmarksLink(long long BID1, long long BID2);

    bool RetrieveBookmarkExtraInfos(long long BID, QList<BookmarkExtraInfoData>& extraInfos);
    bool UpdateBookmarkExtraInfos(long long BID, const QList<BookmarkExtraInfoData>& extraInfos,
                                  const QList<BookmarkExtraInfoData>& originalExtraInfos);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
