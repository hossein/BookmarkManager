#pragma once
#include "ISubManager.h"
#include "TransactionalFileOperator.h"

#include <QHash>
#include <QDateTime>
#include <QtSql/QSqlQueryModel>

class DatabaseManager;
class IArchiveManager;

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
        ///     file. If attached file changes, they must change, too. THEY ARE important for
        ///     `FileManager::UpdateFile` and `operator==(BookmarkFile,BookmarkFile)` and these two
        ///     functions must be updated if distinct properties change.
        // Note: For unattached files, ArchiveURL is "" and OriginalName is "C:\File.txt".
        //   For attached files, ArchiveURL is ":arch0:/F/F5AB32DA.txt" and OriginalName is "File.txt".
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

private:
    /// This must map LOWER-CASE archive names with ':' colons to the corresponding class.
    QHash<QString, IArchiveManager*> fileArchives;
    TransactionalFileOperator filesTransaction;

public:
    FileManager(QWidget* dialogParent, Config* conf);
    ~FileManager();

    bool InitializeFileArchives();

    /// //Files in file archive: Name convenience functions.
    /// bool IsInsideFileArchive(const QString& userReadablePath);
    //NOTE: The following two functions can have equivalent 'TrashFile' sisters.
    /// Get a path that only contains the Archive name and the file name, e.g for a file
    /// with ArchiveURL of ":arch0:/F/FA/FA3D4FBE.html" and OriginalName of "Index.html",
    /// it returns ":arch0:/Index.html". Must be called on ATTACHED files ONLY.
    QString GetUserReadableArchiveFilePath(const BookmarkFile& bf);
    //TODO: Save :archive:, etc in the file too? Then we can have :archive1: on disk,
    //  :archive2: on shared network, :sanbox:, :trash:, etc and unify many functions here!
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
                             QList<long long>& editedBFIDs,
                             const QString& fileArchiveNameForNewFiles);

    //Sandbox
public:
    bool ClearSandBox();
    /// This function gets and returns ABSOLUTE path names, NOT ArchiveURLs. This can change though!
    /// Returns empty QString on error. [Why we don't delete file after app]
    QString CopyFileToSandBoxAndGetAddress(const QString& filePathName);
    /// The following overload implements sandboxing files in theory using the `CopyFile` function,
    /// but isn't used throughout the code.
    QString CopyFileToSandBoxAndGetAddress(long long FID);

private:
    //Adding bookmarks
    bool AddBookmarkFile(long long BID, long long FID, long long& addedBFID);
    /// Merely updates OriginalName, ModifyDate, Size and MD5; i.e [DISTINCT PROPERTY]s.
    /// `bf.FID` WILL BE DISREGARDED! The `FID` argument will be used to determine the file.
    /// This functions can not be used to change ArchiveURL of BookmarkFiles to move files to
    /// other archives
    bool UpdateFile(long long FID, const BookmarkFile& bf);
    /// Adds the file into the FileArchive folder and Updates the "FID" and "ArchiveURL" fields.
    /// Make sure 'fileArchiveName` exists before calling this function.
    bool AddFile(BookmarkFile& bf, const QString& fileArchiveName);

    //Removing bookmarks
    /// This function will clean-up the no-more-used files automatically by calling "RemoveFile"
    /// for FIDs who are not in use by any other bookmarks.
    bool RemoveBookmarkFile(long long BFID, long long FID);
    /// Removes a file from database and the FileArchive folder.
    /// A File Transaction MUST HAVE BEEN STARTED before calling this function.
    bool TrashFile(long long FID);

    /// A convenience function that calls the appropriate ArchiveMan's 'RemoveFileFromArchive' function.
    /// A File Transaction MUST HAVE BEEN STARTED before calling this function.
    bool RemoveFileFromArchive(const QString& fileArchiveURL, bool trash);

    /// Moving files to trash, and opening files in sandboxed mode are done with the Move/Copy
    ///   functions below.
    /// These two do NOT handle the Database; work solely on physical files.
    /// Do they need File Transaction? If the archive that they want to write to is an ArchiveMan
    ///   that requires transactions, e.g FAM then yes, a File Transaction MUST HAVE BEEN STARTED
    ///   before using these functions (well actually we could do without transactions, but using
    ///   TransactionalFileOperator is a requirement of Updating the bookmark files in BMEditDialog
    ///   so we must do it).
    ///   If the ArchiveMan doesn't need transactions and we are simply copying, no transaction is
    ///   required.
    bool MoveFile(long long FID, const QString& destArchiveName, QString& newFileArchiveURL);
    bool CopyFile(long long FID, const QString& destArchiveName, QString& newFileArchiveURL);
    /// Auxiliary function used by the above functions.
    bool MoveOrCopyAux(long long FID, const QString& destArchiveName, bool removeOriginal,
                       QString& newFileArchiveURL);

private:
    //Standard queries
    QString StandardIndexedBookmarkFileByBIDQuery() const;
    void SetBookmarkFileIndexes(const QSqlRecord& record);

    /// Which archive a file is in? Returns empty QString if the URL is wrong and doesn't
    /// contain a valid archived file URL. Does NOT check to make sure the fileArchiveName
    /// it returns exists or not.
    QString GetArchiveNameOfFile(const QString& fileArchiveURL);

protected:
    // ISubManager interface
    void CreateTables();
    void PopulateModels();
};

inline bool operator==(const FileManager::BookmarkFile& lhs, const FileManager::BookmarkFile& rhs);
inline bool operator!=(const FileManager::BookmarkFile& lhs, const FileManager::BookmarkFile& rhs);
