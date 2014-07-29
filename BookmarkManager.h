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
    } bidx;

    struct BookmarkData
    {
        long long BID;
        QString Name;
        QString URL;
        QString Desc;
        long long DefBFID;
        int Rating;

        /// The following members ARE NOT filled by the `RetrieveBookmark` function.
        /// and ARE NOT saved by the `AddOrEditBookmark` function.
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
    bool DeleteBookmark(long long BID);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
