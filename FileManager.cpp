#include "FileManager.h"

#include "Config.h"
#include "WinFunctions.h"

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

bool FileManager::RetrieveBookmarkFilesModel(long long BID, QSqlQueryModel& filesModel)
{
    QString retrieveError = "Could not get attached files information for the bookmark from the database.";
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
    QString retrieveError = "Could not get attached files information for the bookmark from the database.";
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
        bf.ModifyDate   = query.value(bfidx.ModifyDate  ).toDateTime();
        bf.Size         = query.value(bfidx.Size        ).toLongLong();
        bf.MD5          = query.value(bfidx.MD5         ).toString();

        bookmarkFiles.append(bf);
    }

    return true; //NOTE: We checked for model.setQuery errors nowhere.
}

bool FileManager::PutInsideFileArchive(QString& filePathName, bool removeOriginalFile)
{
    QFileInfo fi(filePathName);
    if (!fi.exists())
        return Error("The selected file \"" + filePathName + "\" does not exist!");
    else if (!fi.isFile())
        return Error("The path \"" + filePathName + "\" does not point to a valid file!");

    int fileNameHash = FileNameHash(fi.fileName());
    fileNameHash = fileNameHash % 16;

    QString targetFilePathName = QDir::currentPath()
            + "/" + conf->nominalFileArchiveDirName
            + "/" + QString::number(fileNameHash)
            + "/" + fi.fileName();

    bool success = QFile::copy(filePathName, targetFilePathName);
    if (!success)
        return Error(QString("Could not copy the source file to destination directory!"
                             "\nSource File: %1\nDestination File: %2")
                     .arg(filePathName, targetFilePathName));

    if (removeOriginalFile)
    {
        if (!WinFunctions::MoveFileToRecycleBin(filePathName))
        {
            //Note: We just show the dialog, we don't return an error or sth at all.
            QMessageBox::warning(dialogParent, "Warning", "The source file \"" + filePathName +
                                 "\" could not be deleted. You should manually delete it yourself.");
        }
    }

    //TODO: Save the file in archive as "5r3edz5" but in the db keep its original name and show
    //      ":archive:/original utf8 file name.mht" to user!
    if (success)
    {
        //TODO: According to the above todo, save file name in db as utf8 and change the copied
        //      file name before copying.
        filePathName = conf->fileArchivePrefix + "/" + fi.fileName();
    }

    return success;
}

bool FileManager::IsInsideFileArchive(const QString& path)
{
    int faPrefixLength = conf->fileArchivePrefix.length();
    if (path.length() > faPrefixLength &&
        path.left(faPrefixLength) == conf->fileArchivePrefix &&
        (path[faPrefixLength] == '/' || path[faPrefixLength] == '\\'))
        return true;
    return false;
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
               "  ModifyDate INTEGER, Size Integer, MD5 TEXT )");

    query.exec("CREATE TABLE BookmarkFile"
               "( BFID INTEGER PRIMARY KEY AUTOINCREMENT, BID INTEGER, FID INTEGER )");
}

void FileManager::PopulateModels()
{
    //FileManager does not have a model.
}
