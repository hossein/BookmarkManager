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
        /// [DISTINCT PROPERTY]'s are the properties that show the properties of the real attached
        ///     file. If attached file changes, they must change, too.
        /// THEY ARE important for `FileManager::UpdateFile` and `operator==(BookmarkFile,BookmarkFile)`
        ///     and these two functions must be updated if distinct properties change.
        // Note: For unattached files, ArchiveURL is "" and Original URL is "C:\File.txt".
        //   For already attached files, ArchiveURL is "F/F5AB32DA.txt" and Original URL is "File.txt".
        long long BFID;
        long long BID;
        long long FID;
        QString OriginalName; //[DISTINCT PROPERTY]
        QString ArchiveURL;
        QDateTime ModifyDate; //[DISTINCT PROPERTY]
        long long Size;       //[DISTINCT PROPERTY]
        QByteArray MD5;       //[DISTINCT PROPERTY]

        bool Ex_IsDefaultFileForEditedBookmark;
        bool Ex_RemoveAfterAttach;
    };

    TransactionalFileOperator filesTransaction;

public:
    FileManager(QWidget* dialogParent, Config* conf);
    ~FileManager();

    bool InitializeFilesDirectory();

    //Files in file archive: Name convenience functions.
    bool IsInsideFileArchive(const QString& userReadablePath);
    //NOTE: The following two functions can have equivalent 'TrashFile' sisters.
    QString GetUserReadableArchiveFilePath(const QString& originalName);
    //NOTE: Maybe save :archive:, etc in the file too? Then we can have :archive1: on disk,
    //  :archive2: on shared network, etc!
    QString GetFullArchiveFilePath(const QString& fileArchiveURL);
    static QString GetFileNameOnlyFromOriginalNameField(const QString& originalName);
    static QString ChangeOriginalNameField(const QString& originalName, const QString& newName);

    //Transaction functions
    bool BeginFilesTransaction();
    bool CommitFilesTransaction();
    bool RollBackFilesTransaction();

    //Bookmark retrieving functions.
    bool RetrieveBookmarkFilesModel(long long BID, QSqlQueryModel& filesModel);
    /// Note: We return a QList. We could have made our own QAbstractItem/TableModel for
    ///       files, but we preferred this way and using QTableWidget for displaying, etc.
    /// This function DOES NOT set any `Ex_` fields in the returned structs.
    bool RetrieveBookmarkFiles(long long BID, QList<BookmarkFile>& bookmarkFiles);

    //Bookmark updating: involves BOTH adding and deleting.
    //NOTE: "Shar"e and "Multiple" and "Two" are keywords for file sharing between multiple bookmarks.
    /// Although the interface currently doesn't allow sharing a file between multiple bookmarks,
    ///     this function uses BFID for its calculations and allows it.
    /// IMPORTANT: Do NOT make `editedBookmarkFiles` non-const; in transactions where consecutive
    ///     actions are happening, this can't just change the parameters it wants.
    ///     This function just returns the BFIDs that are added or edited in EQUIVALENT INDEXES TO
    ///     the `editedBookmarkFiles` arg in `editedBFIDs` list.
    bool UpdateBookmarkFiles(long long BID,
                             const QList<BookmarkFile>& originalBookmarkFiles,
                             const QList<BookmarkFile>& editedBookmarkFiles,
                             QList<long long>& editedBFIDs);

    /// This function gets and returns ABSOLUTE path names, NOT ArchiveURLs. This can change though!
    /// Returns empty QString on error.
    QString CopyFileToSandBoxAndGetAddress(const QString& filePathName);

private:
    //Adding bookmarks
    bool AddBookmarkFile(long long BID, long long FID, long long& addedBFID);
    /// Merely updates OriginalName, ModifyDate, Size and MD5.
    /// `bf.FID` WILL BE DISREGARDED! The `FID` argument will be used to determine the file.
    bool UpdateFile(long long FID, const BookmarkFile& bf);
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
    /// Note: This only happens ONCE, and later if file name in archive, or any other property
    ///       that is used to calculate the hash or anyhting in the FileArchive changes, the file
    ///       remains in the folder that it always was and doesn't change location.
    ///       Also, remaining the file extension does NOT change the extension that is used with
    ///       the file in the FileArchive.
    QString CalculateFileArchiveURL(const QString& fileFullPathName);
    QString GetFullArchivePathForFile(const QString& fileArchiveURL, const QString& archiveFolderName);
    int FileNameHash(const QString& fileNameOnly);

    bool RemoveDirectoryRecursively(const QString& dirPathName);

private:
    bool CreateLocalFileDirectory(const QString& archiveFolderName);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};

inline bool operator==(const FileManager::BookmarkFile& lhs, const FileManager::BookmarkFile& rhs);
inline bool operator!=(const FileManager::BookmarkFile& lhs, const FileManager::BookmarkFile& rhs);
