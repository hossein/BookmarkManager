#pragma once
#include "ISubManager.h"
#include <QDateTime>
#include <QtSql/QSqlQueryModel>

class DatabaseManager;

class FileManager : public ISubManager
{
    friend class DatabaseManager;

public:
    struct BookmarkFileIndexes
    {
        int BFID;
        int BID;
        int FID;
        int OriginalName;
        int ArchiveURL;
        int ModifyDate;
        int Size;
        int MD5;
    } bfidx;

    struct BookmarkFile
    {
        long long BFID;
        long long BID;
        long long FID;
        QString OriginalName;
        QString ArchiveURL;
        QDateTime ModifyDate;
        long long Size;
        QString MD5;
    };

public:
    FileManager(QWidget* dialogParent, Config* conf);
    ~FileManager();

    bool InitializeFilesDirectory();

    bool RetrieveBookmarkFilesModel(long long BID, QSqlQueryModel& filesModel);

    /// NOTE: We return a QList. We could have made our own QAbstractItem/TableModel for
    ///       files, but we preferred this way and using QTableWidget for displaying, etc.
    bool RetrieveBookmarkFiles(long long BID, QList<BookmarkFile>& bookmarkFiles);

private:
    /// Modifies the `path` and puts the FileArchive address in it.
    bool PutInsideFileArchive(QString& filePathName, bool removeOriginalFile);
    bool IsInsideFileArchive(const QString& path);

private:
    QString StandardIndexedBookmarkFileByBIDQuery() const;
    void SetBookmarkFileIndexes(const QSqlRecord& record);

    int FileNameHash(const QString& fileNameOnly);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
