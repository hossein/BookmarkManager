#include "FileManager.h"

#include "Config.h"
#include "FileArchiveManager.h"
#include "WinFunctions.h"
#include "Util.h"

//TODO: Remove these includes later. ALSO remove util, winfuncs, conf, etc if not needed later.
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

FileManager::FileManager(QWidget* dialogParent, Config* conf)
    : ISubManager(dialogParent, conf)
{
    //TODO: Temporary
    FileArchiveManager* fam = new FileArchiveManager(
                dialogParent, conf, conf->fileArchivePrefix,
                QDir::currentPath() + "/" + conf->nominalFileArchiveDirName, &filesTransaction);
    fileArchives[conf->fileArchivePrefix] = fam;

    fam = new FileArchiveManager(
                dialogParent, conf, conf->fileTrashPrefix,
                QDir::currentPath() + "/" + conf->nominalFileTrashDirName, &filesTransaction);
    fileArchives[conf->fileTrashPrefix] = fam;
}

FileManager::~FileManager()
{
    foreach (FileArchiveManager* fam, fileArchives)
        delete fam;
}

bool FileManager::InitializeFilesDirectory()
{
    bool success = true;
    success &= CreateLocalFileDirectory(conf->nominalFileArchiveDirName);
    success &= CreateLocalFileDirectory(conf->nominalFileTrashDirName);
    success &= CreateLocalFileDirectory(conf->nominalFileSandBoxDirName);
    return success;
}

bool FileManager::ClearSandBox()
{
    bool success;
    QString sandboxDirPath = QDir::currentPath() + "/" + conf->nominalFileSandBoxDirName;
    success = RemoveDirectoryRecursively(sandboxDirPath, false); //Don't remove the SandBox itself.

    if (!success)
        return Error("Can not remove the contents of File SandBox!)");

    return success;
}

bool FileManager::IsInsideFileArchive(const QString& userReadablePath)
{
    int faPrefixLength = conf->fileArchivePrefix.length();
    if (userReadablePath.length() > faPrefixLength &&
        userReadablePath.left(faPrefixLength) == conf->fileArchivePrefix &&
        (userReadablePath[faPrefixLength] == '/' || userReadablePath[faPrefixLength] == '\\'))
        return true;
    return false;
}

QString FileManager::GetUserReadableArchiveFilePath(const QString& originalName)
{
    return conf->fileArchivePrefix + "/" + originalName;
}

QString FileManager::GetFullArchiveFilePath(const QString& fileArchiveURL)
{
    //TODO: Need to test to make sure the URL is a valid colonized archive URL and that
    //      the archive exists(user can change the DB, or simply can remove the archives?, errors may happen, etc)
    //      , etc AND should we set these as requirements on this function?
    foreach (const QString& famName, fileArchives.keys())
    {
        if (fileArchiveURL.left(famName.length()) == famName)
        {
            //Strip the ":arhiveName:/" part from the file url.
            QString relativeFileURLToArchive = fileArchiveURL.mid(famName.length() + 1);
            return fileArchives[famName]->GetFullArchivePathForRelativeURL(relativeFileURLToArchive);
        }
    }
}

QString FileManager::GetFileNameOnlyFromOriginalNameField(const QString& originalName)
{
    //In the DB the original only-file-name of the file is stored without prefixes and suffixes
    //  and stuff, so it is simply the only-file-name of the file.
    return originalName;
}

QString FileManager::ChangeOriginalNameField(const QString& originalName, const QString& newName)
{
    //In the DB the original only-file-name of the file is stored without prefixes and suffixes
    //  and stuff, so we can simply change it to what we want.
    Q_UNUSED(originalName);
    return newName;
}

bool FileManager::BeginFilesTransaction()
{
    if (!filesTransaction.BeginTransaction())
        return Error("Could not start a transactional file operation session.");
    return true;
}

bool FileManager::CommitFilesTransaction()
{
    if (!filesTransaction.CommitTransaction())
        return Error("Could not start a transactional file operation session.");
    return true;
}

bool FileManager::RollBackFilesTransaction()
{
    if (!filesTransaction.RollBackTransaction())
        return Error("Could not roll back the applied changes to your file system. "
                     "Your files will be in a non-consistent state.");
    return true;
}

bool FileManager::RetrieveBookmarkFilesModel(long long BID, QSqlQueryModel& filesModel)
{
    QString retrieveError =
            "Could not get attached files information for the bookmark "
            "from the database.";
    QSqlQuery query(db);
    query.prepare(StandardIndexedBookmarkFileByBIDQuery());
    query.addBindValue(BID);

    if (!query.exec()) //IMPORTANT For the model and for setting the indexes.
        return Error(retrieveError, query.lastError());

    //Since the query is already executed, we think no errors happened in here.
    filesModel.setQuery(query);

    while (filesModel.canFetchMore())
        filesModel.fetchMore();

    SetBookmarkFileIndexes(filesModel.record());

    filesModel.setHeaderData(bfidx.BFID        , Qt::Horizontal, "BFID"         );
    filesModel.setHeaderData(bfidx.BID         , Qt::Horizontal, "BID"          );
    filesModel.setHeaderData(bfidx.FID         , Qt::Horizontal, "FID"          );
    filesModel.setHeaderData(bfidx.OriginalName, Qt::Horizontal, "Original Name");
    filesModel.setHeaderData(bfidx.ArchiveURL  , Qt::Horizontal, "Archive URL"  );
    filesModel.setHeaderData(bfidx.ModifyDate  , Qt::Horizontal, "Modify Date"  );
    filesModel.setHeaderData(bfidx.Size        , Qt::Horizontal, "Size"         );
    filesModel.setHeaderData(bfidx.MD5         , Qt::Horizontal, "MD5"          );

    return true;
}

bool FileManager::RetrieveBookmarkFiles(long long BID, QList<FileManager::BookmarkFile>& bookmarkFiles)
{
    QString retrieveError =
            "Could not get attached files information for the bookmark "
            "from the database.";
    QSqlQuery query(db);
    query.prepare(StandardIndexedBookmarkFileByBIDQuery());
    query.addBindValue(BID);

    if (!query.exec()) //IMPORTANT For the model.
        return Error(retrieveError, query.lastError());

    SetBookmarkFileIndexes(query.record());

    while (query.next())
    {
        BookmarkFile bf;
        bf.BFID         = query.value(bfidx.BFID        ).toLongLong();
        bf.BID          = query.value(bfidx.BID         ).toLongLong();
        bf.FID          = query.value(bfidx.FID         ).toLongLong();
        bf.OriginalName = query.value(bfidx.OriginalName).toString();
        bf.ArchiveURL   = query.value(bfidx.ArchiveURL  ).toString();
        long long msse  = query.value(bfidx.ModifyDate  ).toLongLong();
        bf.ModifyDate   = QDateTime::fromMSecsSinceEpoch(msse);
        bf.Size         = query.value(bfidx.Size        ).toLongLong();
        bf.MD5          = query.value(bfidx.MD5         ).toByteArray();

        bookmarkFiles.append(bf);
    }

    return true;
}

bool FileManager::UpdateBookmarkFiles(long long BID,
                                      const QList<BookmarkFile>& originalBookmarkFiles,
                                      const QList<BookmarkFile>& editedBookmarkFiles,
                                      QList<long long>& editedBFIDs,
                                      const QString& fileArchiveNameForNewFiles)
{
    QSqlQuery query(db);

    //Find the bookmarks that were removed. We used BFID, not FID. No difference. Not even in
    //  supporting sharing a file between two bookmarks.
    foreach (const BookmarkFile& obf, originalBookmarkFiles)
    {
        bool originalBookmarkRelationUsed = false;
        foreach (const BookmarkFile& nbf, editedBookmarkFiles)
        {
            if (nbf.BFID == obf.BFID)
            {
                originalBookmarkRelationUsed = true;

                //The user might have changed the file information as a result of renaming the file
                //  or editing it.
                if (nbf != obf)
                    if (!UpdateFile(nbf.FID, nbf))
                        return false;

                break;
            }
        }

        if (!originalBookmarkRelationUsed)
            if (!RemoveBookmarkFile(obf.BFID, obf.FID))
                return false;
    }

    //Add the new bookmarks
    foreach (const BookmarkFile& nbf, editedBookmarkFiles)
    {
        if (nbf.BFID != -1)
        {
            //Already attached.
            editedBFIDs.append(nbf.BFID);
            continue;
        }

        //Make a writable copy, such that addSuccess can modify its `.FID` field.
        BookmarkFile bf = nbf;

        //Insert new files into our FileArchive.
        if (bf.FID)
        {
            if (!AddFile(bf, fileArchiveNameForNewFiles))
                return false;
        }

        //Associate the bookmark-file relationship.
        long long addedBFID;
        if (!AddBookmarkFile(BID, bf.FID, addedBFID))
            return false;

        editedBFIDs.append(addedBFID);
    }

    return true;
}

QString FileManager::CopyFileToSandBoxAndGetAddress(const QString& filePathName)
{
    QFileInfo originalfi(filePathName);
    QString sandBoxFilePathName = QDir::currentPath() + "/" + conf->nominalFileSandBoxDirName
            + "/" + originalfi.fileName();

    //Try
    QFileInfo sbfi(sandBoxFilePathName);
    if (sbfi.isDir())
    {
        bool deleteSuccess = RemoveDirectoryRecursively(sandBoxFilePathName);
        if (!deleteSuccess)
        {
            Error("The target for creating sandboxed file '" + sandBoxFilePathName +
                  "' is a directory and cannot be deleted!");
            return QString();
        }
    }
    else if (sbfi.exists()) //and is a file
    {
        bool deleteSuccess = QFile::remove(sandBoxFilePathName);
        if (!deleteSuccess)
        {
            Error("A file already exists at the target for creating sandboxed file '"
                  + sandBoxFilePathName + "' and cannot be deleted!");
            return QString();
        }
    }
    //else if (sbfi not exists)
    //  fine;

    bool copySuccess = QFile::copy(filePathName, sandBoxFilePathName);
    if (!copySuccess)
    {
        Error(QString("Could not create a temporary, sandboxed file for read-only opening!\n"
                      "Can not continue\nSource File: %1\nDestination File: %2")
              .arg(filePathName, sandBoxFilePathName));
        return QString();
    }

    return sandBoxFilePathName;
}

bool FileManager::AddBookmarkFile(long long BID, long long FID, long long& addedBFID)
{
    QString attachError = "Could not set attached files information for the bookmark in the database.";
    QSqlQuery query(db);

    query.prepare("INSERT INTO BookmarkFile(BID, FID) VALUES( ? , ? )");
    query.addBindValue(BID);
    query.addBindValue(FID);
    if (!query.exec())
        return Error(attachError, query.lastError());

    addedBFID = query.lastInsertId().toLongLong();

    return true;
}

bool FileManager::UpdateFile(long long FID, const FileManager::BookmarkFile& bf)
{
    QString updateFileError = "Unable To alter the information of attached files in the database.";
    QSqlQuery query(db);

    query.prepare("UPDATE File "
                  "SET OriginalName = ?, ModifyDate = ?, Size = ?, MD5 = ? "
                  "WHERE FID = ?");

    query.addBindValue(bf.OriginalName);
    query.addBindValue(bf.ModifyDate);
    query.addBindValue(bf.Size);
    query.addBindValue(bf.MD5);
    query.addBindValue(FID);

    if (!query.exec())
        return Error(updateFileError, query.lastError());

    return true;
}

bool FileManager::AddFile(FileManager::BookmarkFile& bf, const QString& fileArchiveName)
{
    //Add file to our FileArchive directory and also set the `bf.ArchiveURL` field.
    bool addFileToArchiveSuccess =
            fileArchives[fileArchiveName]->
            AddFileToArchive(bf.OriginalName, bf.Ex_RemoveAfterAttach, bf.ArchiveURL);

    if (!addFileToArchiveSuccess)
        return false;

    //IDEAL:
    //We save original FILE NAME ONLY instead of the full name in DB and IDEALLY WE DON'T WANT TO
    //  TOUCH the original `bf` struct in case later transactions fail, but even if we don't touch
    //  it, the `bf.FID` field is being changed so we touch it anyway! So we touch it, and caller
    //  functions must be careful to give writable COPIES of const references to this.
    bf.OriginalName = QFileInfo(bf.OriginalName).fileName();

    QString addFileDBError = "Unable To add file information to the database.";
    QSqlQuery query(db);

    //Get the required file names from the DB.
    query.prepare("INSERT INTO File (OriginalName, ArchiveURL, ModifyDate, Size, MD5) "
                  "VALUES ( ? , ? , ? , ? , ? )");
    query.addBindValue(bf.OriginalName);
    query.addBindValue(bf.ArchiveURL);
    query.addBindValue(bf.ModifyDate);
    query.addBindValue(bf.Size);
    query.addBindValue(bf.MD5);
    if (!query.exec())
        return Error(addFileDBError, query.lastError());

    long long addedFID = query.lastInsertId().toLongLong();
    bf.FID = addedFID;

    return true;
}

bool FileManager::RemoveBookmarkFile(long long BFID, long long FID)
{
    QString attachedRemoveError = "Unable To remove an old attached file from database.";
    QSqlQuery query(db);

    //Trash the bookmark-attached file relation.
    query.prepare("INSERT INTO BookmarkFileTrash(BFID, BID, FID) "
                  "SELECT BFID, BID, FID FROM BookmarkFile WHERE BFID = ?");
    query.addBindValue(BFID);
    if (!query.exec())
        return Error(attachedRemoveError, query.lastError());

    query.prepare("DELETE FROM BookmarkFile WHERE BFID = ?");
    query.addBindValue(BFID);
    if (!query.exec())
        return Error(attachedRemoveError, query.lastError());

    //If file FID is not used by other bookmarks (shared), remove the file altogether.
    QString attachedRemoveCheckForUseError =
            "Unable To clean-up after removing an old attached file from database.";
    query.prepare("SELECT * FROM BookmarkFile WHERE FID = ?");
    query.addBindValue(FID);
    if (!query.exec())
        return Error(attachedRemoveCheckForUseError, query.lastError());

    if (!query.first())
    {
        //This shows no other bookmarks rely on this file!
        //Remove the file completely from db and the archive.
        return RemoveFile(FID); //TODO: TrashFile
    }

    return true;
}

bool FileManager::RemoveFile(long long FID)
{
    QString removeFileError = "Unable To remove an old file information from database.";
    QSqlQuery query(db);

    //Get the required file names from the DB.
    query.prepare("SELECT * FROM File WHERE FID = ?");
    query.addBindValue(FID);
    if (!query.exec())
        return Error(removeFileError, query.lastError());

    query.first();
    //QString originalFileName = query.record().value("OriginalName");
    QString fileArchiveURL = query.record().value("ArchiveURL").toString();

    //Trash the file from the DB.
    query.prepare("INSERT INTO FileTrash(FID, OriginalName, ArchiveURL, ModifyDate, Size, MD5) "
                  "SELECT FID, OriginalName, ArchiveURL, ModifyDate, Size, MD5 FROM File WHERE FID = ?");
    query.addBindValue(FID);
    if (!query.exec())
        return Error(removeFileError, query.lastError());

    query.prepare("DELETE FROM File WHERE FID = ?");
    query.addBindValue(FID);
    if (!query.exec())
        return Error(removeFileError, query.lastError());

    if (!RemoveFileFromArchive(fileArchiveURL))
        return false;

    return true;
}

bool FileManager::RemoveFileFromArchive(const QString& fileArchiveURL)
{
    /**
    QString fileOperationError = "Unable to remove a file from the FileArchive.";

    QString fullFilePathName = GetFullArchivePathForFile(fileArchiveURL, conf->nominalFileArchiveDirName);
    QString fullTrashPathName = GetFullArchivePathForFile(fileArchiveURL, conf->nominalFileTrashDirName);

    QString fullTrashPath = QFileInfo(fullTrashPathName).absolutePath();
    if (!QDir::current().mkpath(fullTrashPath))
        return Error(fileOperationError + QString("\n\nFile Name: ") + fullFilePathName);

    if (!filesTransaction.RenameFile(fullFilePathName, fullTrashPathName))
        return Error(fileOperationError + QString("\n\nFile Name: ") + fullFilePathName);

    return true;
    **/

    QString originalFileArchiveName = GetArchiveNameOfFile(originalFileArchiveURL);
    if (!fileArchives.keys().contains(originalFileArchiveName))
        return Error(QString("The source file archive '%1' does not exist!")
                     .arg(originalFileArchiveName));

    QString relativeFileURLToArchive = fileArchiveURL.mid(originalFileArchiveName.length() + 1);
    return fileArchives[originalFileArchiveName]->RemoveFileFromArchive(relativeFileURLToArchive);
}

bool FileManager::MoveFile(long long FID, const QString& destinationArchiveName)
{
    QString retrieveFileError = "Unable to retrieve file information from the database.";
    QString moveFileError = "Unable to record file moving information in the database.";
    QSqlQuery query(db);

    //Get the required file names from the DB.
    query.prepare("SELECT * FROM File WHERE FID = ?");
    query.addBindValue(FID);
    if (!query.exec())
        return Error(retrieveFileError, query.lastError());

    query.first();
    QString fileArchiveURL = query.record().value("ArchiveURL").toString();

    QString fullArchiveFilePath = GetFullArchiveFilePath(fileArchiveURL);
    if (fullArchiveFilePath.isEmpty())
        return Error(QString("Archive file \"%1\" is invalid and does not match a real file.")
                     .arg(fileArchiveURL));

    //Add the file to destinationArchiveName (e.g ':trash:').
    //Note: We could just set the second parameter of `AddFileToArchive` to true to remove the
    //  original file from the old archive. This is fine with the current implementation as
    //  FAM doesn't store extra information about the files. However we do it in two-steps of
    //  adding to new archive then removing from the old archive for more integrity, although
    //  that way we didn't even needed to get `originalFileArchiveName`.
    QString newFileArchiveURL;
    //NOTE: The errors of the following (and many other calls here, incl. Remove) might not be evident to user,
    //  maybe set a prefix or title like 'Error while trashing file/adding new bookmark file, etc'
    //  string on the errors?
    bool success = fileArchives[destinationArchiveName]->AddFileToArchive(
                fullArchiveFilePath, false, newFileArchiveURL);
    if (!success)
        return false;

    //Remove the file from the old file archive.
    success = RemoveFileFromArchive(fileArchiveURL);
    if (!success)
        return false;

    //Change the file archive name accordingly.
    query.prepare("Update File SET ArchiveURL = ? WHERE FID = ?");
    query.addBindValue(newFileArchiveURL);
    query.addBindValue(FID);
    if (!query.exec())
        return Error(moveFileError, query.lastError());

    return true;
}

bool FileManager::CopyFile(long long FID, const QString& destinationArchiveName)
{

}

QString FileManager::StandardIndexedBookmarkFileByBIDQuery() const
{
    //Returns a query such that the `bfidx` values remain consistent accross different functions
    //  in this class.
    return QString("SELECT * FROM BookmarkFile NATURAL JOIN File WHERE BID = ?");
}

void FileManager::SetBookmarkFileIndexes(const QSqlRecord& record)
{
    bfidx.BFID         = record.indexOf("BFID"        );
    bfidx.BID          = record.indexOf("BID"         );
    bfidx.FID          = record.indexOf("FID"         );
    bfidx.OriginalName = record.indexOf("OriginalName");
    bfidx.ArchiveURL   = record.indexOf("ArchiveURL"  );
    bfidx.ModifyDate   = record.indexOf("ModifyDate"  );
    bfidx.Size         = record.indexOf("Size"        );
    bfidx.MD5          = record.indexOf("MD5"         );
}

bool FileManager::GetArchiveNameOfFile(const QString& fileArchiveURL)
{
    int indexOfSlash = fileArchiveURL.indexOf('/');
    if (indexOfSlash == -1)
        return QString();

    QString fileArchiveName = fileArchiveURL.left(indexOfSlash);
    return fileArchiveName;
}

bool FileManager::RemoveDirectoryRecursively(const QString& dirPathName, bool removeParentDir)
{
    // http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/

    bool success = true;
    QDir dir(dirPathName);
    if (!dir.exists())
        return true; //We wanted it deleted; it's already deleted, success!

    QFileInfoList entriesInfo = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System |
                                                  QDir::Hidden | QDir::AllDirs | QDir::Files);

    foreach (const QFileInfo& ei, entriesInfo)
    {
        if (ei.isDir())
            success = RemoveDirectoryRecursively(ei.absoluteFilePath());
        else
            success = QFile::remove(ei.absoluteFilePath());

        if (!success)
            return success;
    }

    if (removeParentDir)
        success = dir.rmdir(dirPathName);
    return success;
}

bool FileManager::CreateLocalFileDirectory(const QString& archiveFolderName)
{
    QString faDirPath = QDir::currentPath() + "/" + archiveFolderName;
    QFileInfo faDirInfo(faDirPath);

    if (!faDirInfo.exists())
    {
        if (!QDir::current().mkpath(faDirPath))
            return Error(QString("'%1' directory could not be created!").arg(archiveFolderName));
    }
    else if (!faDirInfo.isDir())
    {
        return Error(QString("'%1' is not a directory!").arg(archiveFolderName));
    }

    return true;
}

void FileManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE File"
               "( FID INTEGER PRIMARY KEY AUTOINCREMENT, OriginalName TEXT, ArchiveURL TEXT, "
               "  ModifyDate INTEGER, Size Integer, MD5 BLOB )");

    query.exec("CREATE TABLE FileTrash"
               "( FID INTEGER PRIMARY KEY              , OriginalName TEXT, ArchiveURL TEXT, "
               "  ModifyDate INTEGER, Size Integer, MD5 BLOB )");

    query.exec("CREATE TABLE BookmarkFile"
               "( BFID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, FID INTEGER )");

    query.exec("CREATE TABLE BookmarkFileTrash"
               "( BFID INTEGER PRIMARY KEY              , BID INTEGER, FID INTEGER )");
}

void FileManager::PopulateModels()
{
    //FileManager does not have a model.
}


bool operator==(const FileManager::BookmarkFile& lhs, const FileManager::BookmarkFile& rhs)
{
    //Note: Since we don't want to check the `Ex_` fields, we have to override `operator==`.
    //      Also we don't have to check BFID, BID, FID and ArchiveURL for the purposes of comparing
    //      BookmarkFiles in this program.

    bool isSame = true;
    isSame = isSame && (lhs.OriginalName == rhs.OriginalName);
    isSame = isSame && (lhs.ModifyDate   == rhs.ModifyDate  );
    isSame = isSame && (lhs.Size         == rhs.Size        );
    isSame = isSame && (lhs.MD5          == rhs.MD5         );

    return isSame;
}

bool operator!=(const FileManager::BookmarkFile& lhs, const FileManager::BookmarkFile& rhs)
{
    return !operator==(lhs, rhs);
}
