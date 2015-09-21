#include "FileManager.h"

#include "Config.h"
#include "IArchiveManager.h"
//#include "FileArchiveManager.h"
#include "FileSandBoxManager.h"

#include <QDir>
#include <QFileInfo>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlResult>

FileManager::FileManager(QWidget* dialogParent, Config* conf)
    : ISubManager(dialogParent, conf)
{

}

FileManager::~FileManager()
{
    foreach (IArchiveManager* iam, fileArchives)
        delete iam;
}

bool FileManager::InitializeFileArchives()
{
    bool success = true;
    success &= PopulateAndRegisterFileArchives();
    success &= DoFileArchiveInitializations();
    return success;
}

QString FileManager::GetUserReadableArchiveFilePath(const FileManager::BookmarkFile& bf)
{
    QString archiveName = GetArchiveNameOfFile(bf.ArchiveURL);
    return archiveName + "/" + bf.OriginalName;
}

QString FileManager::GetFullArchiveFilePath(const QString& fileArchiveURL)
{
    //TODO: Need to test to make sure the URL is a valid colonized archive URL and that
    //      the archive exists(user can change the DB, or simply can remove the archives?, errors may happen, etc)
    //      , etc AND should we set these as requirements on this function?
    foreach (const QString& archiveName, fileArchives.keys())
    {
        if (fileArchiveURL.left(archiveName.length()) == archiveName)
        {
            //Strip the ":ArhiveName:/" part from the file url.
            QString relativeFileURLToArchive = fileArchiveURL.mid(archiveName.length() + 1);
            return fileArchives[archiveName]->GetFullArchivePathForRelativeURL(relativeFileURLToArchive);
        }
    }

    //TODO: If not found return what? or do the error handling where?
    return QString();
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
            "Could not get attached files information for the bookmark from the database.";

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

bool FileManager::TrashAllBookmarkFiles(long long BID)
{
    QString retrieveBookmarkFilesError =
            "Unable to get attached files information for bookmark in order to delete them.";

    QSqlQuery query(db);
    query.prepare("SELECT * FROM BookmarkFile WHERE BID = ?");
    query.addBindValue(BID);

    if (!query.exec())
        return Error(retrieveBookmarkFilesError, query.lastError());

    while (query.next())
    {
        const QSqlRecord& record = query.record();
        long long BFID = record.value("BFID").toLongLong();
        long long FID = record.value("FID").toLongLong();

        //The following call will remove the attachment information and trash the files ONLY IF
        //  they are not shared.
        if (!RemoveBookmarkFile(BFID, FID))
            return false;
    }

    return true;
}

bool FileManager::ClearSandBox()
{
    //dynamic_cast as an assertion.
    FileSandBoxManager* fsbm =
            dynamic_cast<FileSandBoxManager*>(fileArchives[conf->sandboxArchiveName]);
    return fsbm->ClearSandBox();
}

QString FileManager::CopyFileToSandBoxAndGetAddress(const QString& filePathName)
{
    //FileSandBoxManager archive doesn't need transactions for its `AddFileToArchive`.
    QString fileArchiveURL;
    bool success = fileArchives[conf->sandboxArchiveName]
            ->AddFileToArchive(filePathName, false, fileArchiveURL);
    if (!success)
        return QString();

    return GetFullArchiveFilePath(fileArchiveURL);
}

QString FileManager::CopyFileToSandBoxAndGetAddress(long long FID)
{
    QString fileArchiveURL;
    bool success = CopyFile(FID, conf->sandboxArchiveName, fileArchiveURL);
    if (!success)
        return QString();

    return GetFullArchiveFilePath(fileArchiveURL);
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
    QString updateFileError = "Unable to alter the information of attached files in the database.";
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

    QString addFileDBError = "Unable to add file information to the database.";
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
    QString attachedRemoveError = "Unable to remove an old attached file from database.";
    QSqlQuery query(db);

    //Trash the bookmark-attached file relation.
    /// No more needed after business logic doing stuff.
    /// query.prepare("INSERT INTO BookmarkFileTrash(BFID, BID, FID) "
    ///               "SELECT BFID, BID, FID FROM BookmarkFile WHERE BFID = ?");
    /// query.addBindValue(BFID);
    /// if (!query.exec())
    ///     return Error(attachedRemoveError, query.lastError());

    query.prepare("DELETE FROM BookmarkFile WHERE BFID = ?");
    query.addBindValue(BFID);
    if (!query.exec())
        return Error(attachedRemoveError, query.lastError());

    //If file FID is not used by other bookmarks (shared), remove the file altogether.
    QString attachedRemoveCheckForUseError =
            "Unable to clean-up after removing an old attached file from database.";
    query.prepare("SELECT * FROM BookmarkFile WHERE FID = ?");
    query.addBindValue(FID);
    if (!query.exec())
        return Error(attachedRemoveCheckForUseError, query.lastError());

    if (!query.first())
    {
        //This shows no other bookmarks rely on this file!
        //Remove the file completely from db and the archive.
        return TrashFile(FID);
    }

    return true;
}

bool FileManager::TrashFile(long long FID)
{
    //Note: Since we are in a files transaction, we think DB transaction is started, too;
    //  although even without DB transaction this function would be okay.
    QString trashFileError = "Unable to record file trashing information in the database.";
    QSqlQuery query(db);

    //First move the physical file
    //TODO: Set an error prefix or re-design some aspects?
    QString newFileArchiveURL;
    bool success = MoveFile(FID, conf->trashArchiveName, newFileArchiveURL);
    if (!success)
        return false;

    //Now update DB to change the file archive name accordingly.
    query.prepare("Update File SET ArchiveURL = ? WHERE FID = ?");
    query.addBindValue(newFileArchiveURL);
    query.addBindValue(FID);
    if (!query.exec())
        return Error(trashFileError, query.lastError());

    return true;
}

bool FileManager::RemoveFileFromArchive(const QString& fileArchiveURL, bool trash)
{
    QString originalFileArchiveName = GetArchiveNameOfFile(fileArchiveURL);
    if (!fileArchives.keys().contains(originalFileArchiveName))
        return Error(QString("The source file archive '%1' does not exist!")
                     .arg(originalFileArchiveName));

    QString relativeFileURLToArchive = fileArchiveURL.mid(originalFileArchiveName.length() + 1);
    return fileArchives[originalFileArchiveName]->
            RemoveFileFromArchive(relativeFileURLToArchive, trash);
}

bool FileManager::MoveFile(long long FID, const QString& destArchiveName, QString& newFileArchiveURL)
{
    return MoveOrCopyAux(FID, destArchiveName, true, newFileArchiveURL);
}

bool FileManager::CopyFile(long long FID, const QString& destArchiveName, QString& newFileArchiveURL)
{
    return MoveOrCopyAux(FID, destArchiveName, false, newFileArchiveURL);
}

bool FileManager::MoveOrCopyAux(long long FID, const QString& destArchiveName,
                                bool removeOriginal, QString& newFileArchiveURL)
{
    QString retrieveFileError = "Unable to retrieve file information from the database.";
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

    //Add the file to destArchiveName (e.g ':trash:').
    //NOTE: We could just set the second parameter of `AddFileToArchive` to true to remove the
    //  original file from the old archive. This is fine with the current implementation as
    //  ArchiveMans don't store extra information about the files. However we do it in two-steps of
    //  adding to new archive then removing from the old archive for more integrity, although
    //  that way we didn't even needed to get `originalFileArchiveName`.

    //NOTE: The errors of the following (and many other calls here, incl. Remove) might not be evident to user,
    //  maybe set a prefix or title like 'Error while trashing file/adding new bookmark file, etc'
    //  string on the errors?
    bool success = fileArchives[destArchiveName]->AddFileToArchive(
                fullArchiveFilePath, false, newFileArchiveURL);
    if (!success)
        return false;

    if (removeOriginal)
    {
        //Remove the file from the old file archive.
        success = RemoveFileFromArchive(fileArchiveURL, false);
        if (!success)
            return false;
    }

    return true;
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

QString FileManager::GetArchiveNameOfFile(const QString& fileArchiveURL)
{
    int indexOfSlash = fileArchiveURL.indexOf('/');
    if (indexOfSlash == -1)
        return QString();

    QString fileArchiveName = fileArchiveURL.left(indexOfSlash);
    return fileArchiveName;
}

bool FileManager::PopulateAndRegisterFileArchives()
{
    QString retrieveError = "Could not get file archives information from the database.";

    QSqlQuery query(db);
    query.prepare("SELECT * FROM FileArchive");
    if (!query.exec())
        return Error(retrieveError, query.lastError());

    const QSqlRecord record = query.record();
    int faidx_Name = record.indexOf("Name");
    int faidx_Type = record.indexOf("Type");
    int faidx_Path = record.indexOf("Path");
    int faidx_Layout = record.indexOf("FileLayout");

    ArchiveManagerFactory archiveManFactory(dialogParent, conf, &filesTransaction);
    while (query.next())
    {
        IArchiveManager::ArchiveType aType =
                static_cast<IArchiveManager::ArchiveType>(query.value(faidx_Type).toInt());
        QString aName = query.value(faidx_Name).toString();
        QString aPath = GetAbsoluteFileArchivePath(query.value(faidx_Path).toString());
        int aFileLayout = query.value(faidx_Layout).toInt();

        IArchiveManager* archiveMan = archiveManFactory.CreateArchiveManager(aType, aName, aPath, aFileLayout);
        fileArchives[aName] = archiveMan;
    }

    //Check if all default required archives are present. They are :arch0:, :trash: and :sandbox:.
    QString archiveNotPresentError = "The required file archive '%1' does not exist in the database.";
    QStringList requiredArchiveNames;
    requiredArchiveNames
            << conf->fileArchiveNamePATTERN.arg(0)
            << conf->trashArchiveName
            << conf->sandboxArchiveName;
    foreach (const QString& requiredArchiveName, requiredArchiveNames)
        if (!fileArchives.keys().contains(requiredArchiveName))
            return Error(archiveNotPresentError.arg(requiredArchiveName));

    return true;
}

bool FileManager::DoFileArchiveInitializations()
{
    bool success = true;
    foreach (IArchiveManager* iam, fileArchives)
        //Use `success &=` to initialize the rest of the dirs even if one encountered error.
        success &= iam->InitializeFilesDirectory();
    return success;
}

void FileManager::CreateTables()
{
    QSqlQuery query(db);

    query.exec("CREATE TABLE File"
               "( FID INTEGER PRIMARY KEY AUTOINCREMENT, OriginalName TEXT, ArchiveURL TEXT, "
               "  ModifyDate INTEGER, Size Integer, MD5 BLOB )");

    //Having a separate `FileLayout` field allows for separation of archive's type from how it
    //  stores its files, allowing it to be configurable and update-able in case of new versions.
    query.exec("CREATE TABLE FileArchive"
               "( FAID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, Type INTEGER, Path TEXT,"
               "  FileLayout INTEGER )");
    CreateDefaultArchives(query);

    query.exec("CREATE TABLE BookmarkFile"
               "( BFID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, FID INTEGER )");
}

void FileManager::PopulateModels()
{
    //FileManager does not have a model.
}

QString FileManager::GetAbsoluteFileArchivePath(const QString& fileArchivePathWithVars)
{
    //Read `FileManager::CreateDefaultArchives` comments to know why we use special variables
    //  in FileArchive paths.
    QString reprPath = fileArchivePathWithVars;

    //It is mentioned in the program plan that we'd better use QApplication::applicationDirPath()
    //  for this!
    QString varValue_appdir = QDir::currentPath();
    reprPath.replace("%appdir%", varValue_appdir, Qt::CaseInsensitive);
    return reprPath;
}

void FileManager::CreateDefaultArchives(QSqlQuery& query)
{
    //It's important that for portability and easy copy-pasting the program and its data, we use
    //  some representing variable instead of the absolute path of the executable. We could also
    //  use relative path names, but that's prone to errors (e.g user starting the executable in
    //  a working folder other than executable's own).
    //QString currentPath = QDir::currentPath() + "/";
    QString currentPath = "%appdir%/";
    QString path_arch0   = currentPath + conf->nominalFileArchiveDirName,
            path_trash   = currentPath + conf->nominalFileTrashDirName,
            path_sandbox = currentPath + conf->nominalFileSandBoxDirName;

    //Insert multiple values at once requires SQLite 3.7.11+: stackoverflow.com/a/5009740/656366
    //So we don't do it.
    //query.prepare("INSERT INTO FileArchive(Name, Type, Path, FileLayout) VALUES "
    //              "(?, ?, ?, ?), (?, ?, ?, ?), (?, ?, ?, ?);");

    query.prepare("INSERT INTO FileArchive(Name, Type, Path, FileLayout) VALUES (?, ?, ?, ?);");
    query.addBindValue(conf->fileArchiveNamePATTERN.arg(0));
    query.addBindValue((int)IArchiveManager::AT_FileArchive);
    query.addBindValue(path_arch0);
    query.addBindValue(1);
    query.exec();

    query.prepare("INSERT INTO FileArchive(Name, Type, Path, FileLayout) VALUES (?, ?, ?, ?);");
    query.addBindValue(conf->trashArchiveName);
    query.addBindValue((int)IArchiveManager::AT_FileArchive);
    query.addBindValue(path_trash);
    query.addBindValue(0);
    query.exec();

    query.prepare("INSERT INTO FileArchive(Name, Type, Path, FileLayout) VALUES (?, ?, ?, ?);");
    query.addBindValue(conf->sandboxArchiveName);
    query.addBindValue((int)IArchiveManager::AT_SandBox);
    query.addBindValue(path_sandbox);
    query.addBindValue(0);
    query.exec();
}



bool operator==(const FileManager::BookmarkFile& lhs, const FileManager::BookmarkFile& rhs)
{
    //This only checks [DISTINCT PROPERTY]s.
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
