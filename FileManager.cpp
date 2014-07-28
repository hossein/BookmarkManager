#include "FileManager.h"

#include "Config.h"
#include "WinFunctions.h"
#include "Util.h"

#include <QDir>
#include <QFile>
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
}

bool FileManager::InitializeFilesDirectory()
{
    QString faDirPath = QDir::currentPath() + "/" + conf->nominalFileArchiveDirName;
    QFileInfo faDirInfo(faDirPath);

    if (!faDirInfo.exists())
    {
        if (QDir::current().mkpath(faDirPath))
        {
            //We assume creating the directory was successful, so subdirectories will also
            //  be successfully created.
            for (int i = 0; i < 16; i++)
                QDir(faDirPath).mkdir(QString::number(i));
        }
        else
        {
            return Error("FileArchive directory could not be created!");
        }
    }
    else if (!faDirInfo.isDir())
    {
        return Error("FileArchive is not a directory!");
    }

    return true;
}

QString FileManager::makeUserReadableArchiveFilePath(const QString& originalFileName)
{
    return conf->fileArchivePrefix + "/" + originalFileName;
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

bool FileManager::BeginFilesTransaction()
{
    if (!filesTransaction.BeginTransaction())
        return Error("Could not start a transactional file operation session.");
}

bool FileManager::CommitFilesTransaction()
{
    if (!filesTransaction.CommitTransaction())
        return Error("Could not start a transactional file operation session.");
}

bool FileManager::RollBackFilesTransaction()
{
    if (!filesTransaction.RollBackTransaction())
        return Error("Could not roll back the applied changes to your file system. "
                     "Your files will be in a non-consistent state.");
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

    return true; //NOTE: We checked for model.setQuery errors nowhere.
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

    return true; //NOTE: We checked for model.setQuery errors nowhere.
}

bool FileManager::UpdateBookmarkFiles(long long BID,
                                      const QList<BookmarkFile>& originalBookmarkFiles,
                                      const QList<BookmarkFile>& editedBookmarkFiles)
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
                break;
            }
        }

        if (!originalBookmarkRelationUsed)
            RemoveBookmarkFile(obf.BFID, obf.FID);
    }

    //Add the new bookmarks
    foreach (const BookmarkFile& nbf, editedBookmarkFiles)
    {
        if (nbf.BFID != -1)
            continue; //Already attached.

        //Make a writable copy, such that addSuccess can modify its `.FID` field.
        BookmarkFile bf = nbf;

        //Insert new files into our FileArchive.
        if (bf.FID)
        {
            if (!AddFile(bf))
                return false;
        }

        //Associate the bookmark-file relationship.
        if (!AddBookmarkFile(BID, bf.FID))
            return false;
    }

    return true;
}

bool FileManager::AddBookmarkFile(long long BID, long long FID)
{
    QString attachError = "Could not set attached files information for the bookmark in the database.";
    QSqlQuery query(db);

    query.prepare("INSERT INTO BookmarkFile(BID, FID) VALUES( ? , ? )");
    query.addBindValue(BID);
    query.addBindValue(FID);
    if (!query.exec())
        return Error(attachError, query.lastError());

    return true;
}

bool FileManager::AddFile(FileManager::BookmarkFile& bf)
{
    //Add file to our FileArchive directory and also set the `bf.ArchiveURL` field.
    bool addFileToArchiveSuccess =
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

bool FileManager::AddFileToArchive(const QString& filePathName, bool removeOriginalFile,
                                   QString& fileArchiveURL)
{
    //TODO: 1. don't generate duplicate names
    //      2. mkpath the full path before.
    //      3. don't mkpath the subdirectories 0..15 in the constructor.
    //      4. mkpath the subdirectories as 0..F instead of 0..15.
    //      5. append the file extension to the hash.

    //Check if file is valid.
    QFileInfo fi(filePathName);
    if (!fi.exists())
        return Error("The selected file \"" + filePathName + "\" does not exist!");
    else if (!fi.isFile())
        return Error("The path \"" + filePathName + "\" does not point to a valid file!");

    //Copy the file.
    fileArchiveURL = CalculateFileArchiveURL(filePathName);
    QString targetFilePathName = GetFullArchiveFilePath(fileArchiveURL, conf->nominalFileArchiveDirName);
    bool success = filesTransaction.CopyFile(filePathName, targetFilePathName);
    if (!success)
        return Error(QString("Could not copy the source file to destination directory!"
                             "\n\nSource File: %1\nDestination File: %2")
                             .arg(filePathName, targetFilePathName));

    //Remove the original file.
    if (removeOriginalFile)
    {
        success = filesTransaction.SystemTrashFile(filePathName);
        //We do NOT return FALSE in case of failure.
        Error(QString("Could not delete the original file from your filesystem. "
                      "You should manually delete it yourself.\n\nFile: %1").arg(filePathName));
    };

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

    //If file FID is not used by other bookmarks, remove the file altogether.
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
        return RemoveFile(FID);
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
    QString fileOperationError = "Unable to remove a file from the FileArchive.";

    QString faDirPath = QDir::currentPath() + "/" + conf->nominalFileArchiveDirName;
    QString ftDirPath = QDir::currentPath() + "/" + conf->nominalFileTrashDirName;

    QString fullFilePathName = GetFullArchiveFilePath(fileArchiveURL, faDirPath);
    QString fullTrashPathName = GetFullArchiveFilePath(fileArchiveURL, ftDirPath);

    QString fullTrashPath = QFileInfo(fullTrashPathName).absolutePath();
    if (!QDir::current().mkpath(fullTrashPath))
        return Error(fileOperationError + QString("\n\nFile Name: ") + fullFilePathName);

    if (!filesTransaction.RenameFile(fullFilePathName, fullTrashPathName))
        return Error(fileOperationError + QString("\n\nFile Name: ") + fullFilePathName);

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

QString FileManager::CalculateFileArchiveURL(const QString& fileFullPathName)
{
    int fileNameHash = FileNameHash(QFileInfo(fileFullPathName).fileName());
    fileNameHash = fileNameHash % 16;

    //Prefix the randomHash with the already calculated fileNameHash.
    QString randomHash = QString::number(fileNameHash, 16).toUpper();
    char randomHashChars[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
    };
    int randomHashCharsSize = sizeof(randomHashChars) / sizeof(char);

    Util::SeedRandomWithTime(); //We do it for each file we are adding.

    for (int i = 0; i < 7; i++)
    {
        int charIdx = Util::Random() % randomHashCharsSize;
        randomHash += randomHashChars[charIdx];
    }

    QString fileArchiveURL = QString::number(fileNameHash) + "/" + randomHash;

    return fileArchiveURL;
}

QString FileManager::GetFullArchiveFilePath(const QString& fileArchiveURL,
                                            const QString& archiveFolderName)
{
    QString path = QDir::currentPath() + "/" + archiveFolderName + "/" + fileArchiveURL;
    return path;
}

int FileManager::FileNameHash(const QString& fileNameOnly)
{
    //For now just calculates the utf-8 sum of all bytes.
    QByteArray utf8 = fileNameOnly.toUtf8();
    int sum = 0;

    for (int i = 0; i < utf8.size(); i++)
        sum += utf8[i];

    return sum;
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
