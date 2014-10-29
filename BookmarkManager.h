#pragma once
#include "ISubManager.h"
#include "FileManager.h"
#include <QStringList>
#include <QtSql/QSqlQueryModel>
#include <QtSql/QSqlTableModel>

class DatabaseManager;

class BookmarkManager : public ISubManager
{
    friend class DatabaseManager;

public:
    QSqlQueryModel model;

    struct BookmarkExtraInfoIndexes
    {
        int BEIID;
        int BID;
        int Name;
        int Type;
        int Value;
    } beiidx;

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
        long long BID;
        QString Name;
        DataType Type;
        QString Value;
    };

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
        QSqlTableModel Ex_ExtraInfosModel;
        QList<BookmarkExtraInfoData> Ex_ExtraInfosList;
        QStringList Ex_TagsList;
        QSqlQueryModel Ex_FilesModel;
        QList<FileManager::BookmarkFile> Ex_FilesList;
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

    //Extra Infos with model
    /// The returned model has its EditStrategy set to OnManualSubmit,
    /// and client code must NEVER call submit()/submitAll() on it!
    /// Submitting must be done via the update function below, which does extra jobs.
    bool RetrieveBookmarkExtraInfosModel(long long BID, QSqlTableModel& extraInfosModel);
    /// Sets the BID on all rows BEFORE submitting.
    bool UpdateBookmarkExtraInfos(long long BID, QSqlTableModel& extraInfosModel);
    /// The following two are just utility functions that ease modifying the model.
    void InsertBookmarkExtraInfoIntoModel(QSqlTableModel& extraInfosModel, long long BID,
                                          const QString& Name, BookmarkExtraInfoData::DataType Type, const QString& Value);

    //Extra Infos with custom item lists
    bool RetrieveBookmarkExtraInfos(long long BID, QList<BookmarkExtraInfoData>& extraInfos);
    bool UpdateBookmarkExtraInfos(long long BID, const QList<BookmarkExtraInfoData>& originalExtraInfos,
                                  const QList<BookmarkExtraInfoData>& extraInfos);

private:
    void SetBookmarkExtraInfoIndexes(const QSqlRecord& record);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
