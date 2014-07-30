#pragma once
#include "ISubManager.h"
#include "TransactionalFileOperator.h"

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
        QByteArray MD5;

        bool Ex_RemoveAfterAttach;
    };

    TransactionalFileOperator filesTransaction;

public:
    FileManager(QWidget* dialogParent, Config* conf);
    ~FileManager();

    bool InitializeFilesDirectory();

    //Files in file archive: Name convenience functions.
    QString makeUserReadableArchiveFilePath(const QString& originalFileName);
    bool IsInsideFileArchive(const QString& userReadablePath);

    //Transaction functions
    bool BeginFilesTransaction();
    bool CommitFilesTransaction();
    bool RollBackFilesTransaction();

    //Bookmark retrieving functions.
    bool RetrieveBookmarkFilesModel(long long BID, QSqlQueryModel& filesModel);
    /// NOTE: We return a QList. We could have made our own QAbstractItem/TableModel for
    ///       files, but we preferred this way and using QTableWidget for displaying, etc.
    bool RetrieveBookmarkFiles(long long BID, QList<BookmarkFile>& bookmarkFiles);

    //Bookmark updating: involves BOTH adding and deleting.
    /// Although the interface currently doesn't allow sharing a file between two bookmarks,
    ///     this function uses BFID for its calculations and allows it.
    /// IMPORTANT: Do NOT make `editedBookmarkFiles` non-const; in transactions where consecutive
    ///     actions are happening, this can't just change the parameters it wants.
    bool UpdateBookmarkFiles(long long BID,
                             const QList<BookmarkFile>& originalBookmarkFiles,
                             const QList<BookmarkFile>& editedBookmarkFiles);

private:
    //Adding bookmarks
    bool AddBookmarkFile(long long BID, long long FID);
    /// Adds the file into the FileArchive folder and Updates the "FID" and "ArchiveURL" fields.
    bool AddFile(BookmarkFile& bf);
    /// Only to be called by `AddFile`.
    bool AddFileToArchive(const QString& filePathName, bool removeOriginalFile,
                          QString& fileArchiveURL);

    //Removing bookmarks
    /// This function will clean-up the no-more-used files automatically by calling "RemoveFile"
    /// for FIDs who are not in use by any other bookmarks.
    bool RemoveBookmarkFile(long long BFID, long long FID);
    /// Removes a file from database and the FileArchive folder.
    bool RemoveFile(long long FID);
    /// Only to be called by `RemoveFile`.
    bool RemoveFileFromArchive(const QString& fileArchiveURL);

private:
    //Standard queries
    QString StandardIndexedBookmarkFileByBIDQuery() const;
    void SetBookmarkFileIndexes(const QSqlRecord& record);

    //Files in file archive handling
    /// Could be called `CreateFileArchiveURL` too.
    QString CalculateFileArchiveURL(const QString& fileFullPathName);
    QString GetFullArchiveFilePath(const QString& fileArchiveURL, const QString& archiveFolderName);
    int FileNameHash(const QString& fileNameOnly);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};
