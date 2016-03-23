#include "FileArchiveManager.h"

#include "TransactionalFileOperator.h"
#include "Util.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

FileArchiveManager::FileArchiveManager(QWidget* dialogParent, Config* conf,
                                       const QString& archiveName, const QString& archiveRoot,
                                       int fileLayout, TransactionalFileOperator* filesTransaction)
    : IArchiveManager(dialogParent, conf, archiveName, archiveRoot, fileLayout, filesTransaction)
{

}

FileArchiveManager::~FileArchiveManager()
{

}

bool FileArchiveManager::AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                                          const QString& folderHint, const QString& groupHint,
                                          const QString& errorWhileContext, QString& fileArchiveURL)
{
    //This is like an assert.
    if (!filesTransaction->isTransactionStarted())
        return Error(QString("Error while %1:\n"
                             "No file transaction was started before adding files to archive.")
                     .arg(errorWhileContext));

    //Check if file is valid.
    QFileInfo fi(filePathName);
    if (!fi.exists())
        return Error(QString("Error while %1:\nThe selected file \"%2\" does not exist!")
                     .arg(errorWhileContext, filePathName));
    else if (!fi.isFile())
        return Error(QString("Error while %1:\nThe path \"%2\" does not point to a valid file!")
                     .arg(errorWhileContext, filePathName));

    //Decide where should the file be copied.
    QString fileRelArchiveURL = CalculateFileArchiveURL(filePathName, folderHint, groupHint);
    if (fileRelArchiveURL.isEmpty())
        return false;

    QString targetFilePathName = GetFullArchivePathForRelativeURL(fileRelArchiveURL);
    fileArchiveURL = m_archiveName + "/" + fileRelArchiveURL; //Out param

    //Create its directory if doesn't exist.
    QString targetFileDir = QFileInfo(targetFilePathName).absolutePath();
    QFileInfo tdi(targetFileDir);
    if (!tdi.exists())
    {
        //Can NOT use `canonicalFilePath`, since the directory still doesn't exist, it will just
        //  return an empty string.
        if (!filesTransaction->MakePath(".", tdi.absoluteFilePath()))
            return Error(QString("Error while %1:\nCould not create the directory for placing "
                                 "the attached file.\n\nDirectory: %2")
                         .arg(errorWhileContext, tdi.absoluteFilePath()));
    }
    else if (!tdi.isDir())
    {
        return Error(QString("Error while %1:\nThe path for placing the attached file is not a directory!"
                     "\n\nDirectory: %2").arg(errorWhileContext, tdi.absoluteFilePath()));
    }

    //Copy the file.
    bool success = filesTransaction->CopyFile(filePathName, targetFilePathName);
    if (!success)
        return Error(QString("Error while %1:\n"
                             "Could not copy the source file to destination directory!"
                             "\n\nSource File: %2\nDestination File: %3")
                             .arg(errorWhileContext, filePathName, targetFilePathName));

    //Remove the original file.
    if (systemTrashOriginalFile)
    {
        bool Trashsuccess = filesTransaction->SystemTrashFile(filePathName);
        //We do NOT return FALSE in case of failure.
        if (!Trashsuccess)
        {
            Error(QString("Error while %1:\nCould not delete the original file from your filesystem. "
                          "You should manually delete it yourself.\n\nFile: %2")
                  .arg(errorWhileContext, filePathName));
        }
    };

    return true;
}

bool FileArchiveManager::RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash,
                                               const QString& errorWhileContext)
{
    //This is like an assert.
    if (!filesTransaction->isTransactionStarted())
        return Error(QString("Error while %1:\n"
                             "No file transaction was started before removing files from archive.")
                     .arg(errorWhileContext));

    bool success;
    QString fileOperationError = "Error while %1:\n"
                                 "Unable to remove a file from the FileArchive.\n\n"
                                 "File name: %2";

    QString fullFilePathName = GetFullArchivePathForRelativeURL(fileRelArchiveURL);

    if (trash)
        success = filesTransaction->SystemTrashFile(fullFilePathName);
    else
        success = filesTransaction->DeleteFile(fullFilePathName);

    if (!success)
        return Error(fileOperationError.arg(errorWhileContext, fullFilePathName));

    return true;
}

QString FileArchiveManager::CalculateFileArchiveURL(const QString& fileFullPathName,
                                                    const QString& folderHint, const QString& groupHint)
{
    QFileInfo fi(fileFullPathName);

    if (m_fileLayout == 0) //File hash layout
    {
        int fileNameHash = FileNameHash(fi.fileName());
        fileNameHash = fileNameHash % 16;

        //Prefix the randomHash with the already calculated fileNameHash.
        QString prefix = QString::number(fileNameHash, 16).toUpper();
        QString fileDir = fi.absolutePath();
        QString randomHash =
                Util::NonExistentRandomFileNameInDirectory(fileDir, 7, prefix, "." + fi.suffix());

        //Use `prefix` again to put files in different directories.
        QString fileArchiveURL = prefix + "/" + randomHash;
        return fileArchiveURL;
    }
    else if (m_fileLayout == 1 || m_fileLayout == 2) //Normal hierarchical file name layout
    {
        QString fileArchivePath;

        if (m_fileLayout == 1)
        {
            bool isFileName = (groupHint.isEmpty());
            QString uHierName = (isFileName ? fi.fileName() : groupHint);

            //Put files like 'f/fi/{GroupHint|@BM_Files}/filename.ext'.
            //  Files without groupHint go to '@BM_Files' dir instead of cluttering the folders.
            fileArchivePath = FolderHierForName(uHierName, isFileName);
            if (groupHint.isEmpty())
                fileArchivePath += "@BM_Files/";
            else
                fileArchivePath += SafeAndShortFSName(uHierName, isFileName) + "/";
        }
        else if (m_fileLayout == 2)
        {
            //It is IMPORTANT that folderHint be a valid fileSystem path part.
            fileArchivePath = folderHint;
            if (fileArchivePath.startsWith('/'))
                fileArchivePath = fileArchivePath.mid(1);
            if (!fileArchivePath.endsWith('/'))
                fileArchivePath += '/';

            //If folderHint is empty, append groupHint to it.
            if (fileArchivePath == "/" && !groupHint.isEmpty())
                fileArchivePath = groupHint + "/";

            //Make names Short and Safe
            QStringList uParts = fileArchivePath.split('/');
            QStringList sParts;
            foreach (const QString& part, uParts)
                sParts.append(SafeAndShortFSName(part, false));
            fileArchivePath = sParts.join('/');
        }

        //Generate final file name.
        QString safeFileName = SafeAndShortFSName(fi.fileName(), true);
        QString fileArchiveURL = fileArchivePath + safeFileName;

        //If file exists, put it in a hashed directory.
        QFileInfo calculatedFileURLInfo(GetFullArchivePathForRelativeURL(fileArchiveURL));
        //`exists` returns `false` if symlink exists but its target doesn't.
        if (calculatedFileURLInfo.exists() || calculatedFileURLInfo.isSymLink())
        {
            QString calculatedFileDir = calculatedFileURLInfo.absolutePath(); //i.e absolute `fileArchivePath`
            QString randomHash = Util::NonExistentRandomFileNameInDirectory(calculatedFileDir, 8);
            fileArchiveURL = fileArchivePath + randomHash + "/" + safeFileName;
        }

        return fileArchiveURL;
    }

    Error(QString("Invalid layout for file archive %1: %2.")
          .arg(m_archiveName, QString::number(m_fileLayout)));
    return QString();
}

int FileArchiveManager::FileNameHash(const QString& fileNameOnly)
{
    //For now just calculates the utf-8 sum of all bytes.
    QByteArray utf8 = fileNameOnly.toUtf8();
    int sum = 0;

    for (int i = 0; i < utf8.size(); i++)
        sum += utf8[i];

    return sum;
}

QString FileArchiveManager::FolderHierForName(const QString& name, bool isFileName)
{
    //Calculate the initial chars for folder names.
    //  Use the completeBaseName of the file name to not count e.g 'a.png' a two letter name.
    //    So if it's not a file name we simply don't calculate it's base name!
    //  The `if` check for file name length > 0 was put just in case we pass a file with an
    //    empty base name e.g '.gitconfig', or for future usecases.
    QString baseName = (isFileName ? QFileInfo(name).completeBaseName() : name);
    QString firstCharInit  = (baseName.length() > 0 ? FolderNameInitialsForChar(baseName.at(0).unicode()) : "-");
    QString secondCharInit = (baseName.length() > 1 ? FolderNameInitialsForChar(baseName.at(1).unicode()) : "-");
    return firstCharInit + "/" + firstCharInit + secondCharInit + "/";
}

QString FileArchiveManager::FolderNameInitialsForChar(int c)
{
    if ('A' <= c && c <= 'Z') //Uppercase
        return QString(QChar(c));
    else if ('a' <= c && c <= 'z') //Lowercase
        return QString(QChar(c - ('a' - 'A')));
    else if ('0' <= c && c <= '9') //Numbers
        return QString("@Digit");
    else if (32 <= c && c <= 126) //Not including 127
        return QString("@Sign");
    else
        return QString("@Unicode");
}

QString FileArchiveManager::SafeAndShortFSName(const QString& fsName, bool isFileName)
{
    //IMPORTANT: Must return only ASCII characters for the file name.

    QString safeFileName = fsName;

    //First remove consecutive dots; this prevents Qt from being able to copy files on Windows.
    int sfOldLength;
    do
    {
        sfOldLength = safeFileName.length();
        safeFileName.replace("..", ".");
    } while (sfOldLength != safeFileName.length());

    //Percent encode invalid and unicode characters.
    if (isFileName) //We know it is a valid file name then
        safeFileName = Util::PercentEncodeUnicodeChars(safeFileName);
    else //Arbitrary name
        safeFileName = Util::PercentEncodeUnicodeAndFSChars(safeFileName);

    if (safeFileName.length() > 64) //For WINDOWS!
    {
        if (isFileName)
        {
            //Shorten the file name
            //Using 'complete' base name but 'short' suffix to resist long file names with
            //  accidental dots in them.
            const QFileInfo safeFileNameInfo(safeFileName);
            const QString suffix = safeFileNameInfo.suffix();
            if (suffix.isEmpty() || suffix.length() > 20) //Too long suffixes can be just file names.
            {
                safeFileName = safeFileNameInfo.fileName().left(qMin(64, safeFileName.length()));
            }
            else
            {
                const QString cbaseName = safeFileNameInfo.completeBaseName();
                safeFileName = cbaseName.left(qMin(64, cbaseName.length()));
                safeFileName += "." + safeFileNameInfo.suffix();
            }
        }
        else
        {
            //Is e.g an arbitrary name or a folder name
            safeFileName = safeFileName.left(qMin(64, safeFileName.length()));
        }
    }
    return safeFileName.trimmed();
}

QString FileArchiveManager::GetFullArchivePathForRelativeURL(const QString& fileArchiveURL)
{
    //Do NOT canonical-ize or absolute-ize this file name, this function is also called for
    //  non-existent files, e.g at the end of `CalculateFileArchiveURL`.
    QString path = m_archiveRoot + "/" + fileArchiveURL;
    return path;
}
