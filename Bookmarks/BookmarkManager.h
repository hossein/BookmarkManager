#pragma once
#include "Database/ISubManager.h"
#include "Files/FileManager.h"
#include <QHash>
#include <QStringList>
#include <QtSql/QSqlQueryModel>
#include <QtSql/QSqlTableModel>

class DatabaseManager;
class BookmarksView;

class BookmarkManager : public ISubManager
{
    friend class DatabaseManager;
    friend class BookmarksView;

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
            //Upon changing, BookmarkExtraInfoTypeChooser must be updated.
            Type_Null = 0,
            Type_Text = 1,
            Type_Number = 2,
            Type_Boolean = 3
        };

        static QString DataTypeName(DataType dataType)
        {
            QString ret;
            switch (dataType)
            {
            case Type_Null   : ret = "Null"   ; break;
            case Type_Text   : ret = "Text"   ; break;
            case Type_Number : ret = "Number" ; break;
            case Type_Boolean: ret = "Boolean"; break;
            default          : ret = "Unknown"; break;
            }
            return ret;
        }

        long long BEIID;
        long long BID;
        QString Name;
        DataType Type;
        QString Value;
    };

    struct BookmarkIndexes
    {
        int BID;
        int FOID;
        int Name;
        int URLs;
        int Desc;
        int DefBFID;
        int Rating;
        int AddDate;
    } bidx;

    struct BookmarkData
    {
        long long BID;
        long long FOID;
        QString Name;
        QString URLs;
        QString Desc;
        long long DefBFID;
        int Rating;
        long long AddDate;

        /// The following members are for storing additional information related to bookmarks from
        /// other tables in the same struct as the bookmark data. They ARE NOT filled by the
        /// `RetrieveBookmark` function and ARE NOT saved by the `AddOrEditBookmark` function.
        /// However `BookmarksBusinessLogic` functions are designed to handle them.
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
    bool RemoveBookmark(long long BID);

    bool CountBookmarksInFolder(int& count, const long long FOID);
    bool CountBookmarksInFolders(int& count, const QSet<long long>& FOIDs);
    bool RetrieveBookmarksInFolder(QList<long long>& BIDs, const long long FOID);
    bool RetrieveBookmarksInFolders(QSet<long long>& BIDs, const QSet<long long>& FOIDs);

    bool InsertBookmarkIntoTrash(
            const QString& Folder, const QString& Name, const QString& URLs, const QString& Description,
            const QString& Tags, const QString& AttachedFIDs, const long long DefFID, const int Rating,
            long long AddDate, const QString& ExtraInfos);

    bool RetrieveLinkedBookmarks(long long BID, QList<long long>& linkedBIDs);
    bool UpdateLinkedBookmarks(long long BID, const QList<long long>& originalLinkedBIDs,
                               const QList<long long>& editedLinkedBIDs);
    bool LinkBookmarksTogether(long long BID1, long long BID2);
    bool RemoveBookmarksLink(long long BID1, long long BID2);

    //Extra Infos with model
    /// The returned model has its EditStrategy set to OnManualSubmit,
    /// and client code must NEVER call submit()/submitAll() on it!
    /// Submitting must be done via the update function below, which does extra jobs.
    bool GetEmptyExtraInfosModel(QSqlTableModel& extraInfosModel);
    bool RetrieveBookmarkExtraInfosModel(long long BID, QSqlTableModel& extraInfosModel);
    /// Sets the BID on all rows BEFORE submitting.
    bool UpdateBookmarkExtraInfos(long long BID, QSqlTableModel& extraInfosModel);
    /// The following two are just utility functions that ease modifying the model.
    void InsertBookmarkExtraInfoIntoModel(QSqlTableModel& extraInfosModel, long long BID,
                                          const QString& Name, BookmarkExtraInfoData::DataType Type, const QString& Value);
    void RemoveBookmarkExtraInfoFromModel(QSqlTableModel& extraInfosModel, const QModelIndex& index);

    //Extra Infos with custom item lists
    bool RetrieveBookmarkExtraInfos(long long BID, QList<BookmarkExtraInfoData>& extraInfos);
    bool UpdateBookmarkExtraInfos(long long BID, const QList<BookmarkExtraInfoData>& originalExtraInfos,
                                  const QList<BookmarkExtraInfoData>& extraInfos);

    /// Convenience function mainly to get linked bookmarks' names. Preserves the order.
    bool RetrieveBookmarkNames(const QList<long long>& BIDs, QStringList& names);
    /// Convenience functions used during importing.
    bool RetrieveAllFullURLs(QMultiHash<long long, QString>& bookmarkURLs);
    /// Works case-sensitively.
    bool RetrieveSpecificExtraInfoForAllBookmarks(const QString& extraInfoName, QList<BookmarkExtraInfoData>& extraInfos);

private:
    void SetBookmarkExtraInfoIndexes(const QSqlRecord& record);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModelsAndInternalTables();
};
