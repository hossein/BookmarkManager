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
                                          const QString& groupHint, const QString& errorWhileContext,
                                          QString& fileArchiveURL)
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
    QString fileRelArchiveURL = CalculateFileArchiveURL(filePathName, groupHint);
    QString targetFilePathName = GetFullArchivePathForRelativeURL(fileRelArchiveURL);
    //Out param
    fileArchiveURL = m_archiveName + "/" + fileRelArchiveURL;

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

QString FileArchiveManager::CalculateFileArchiveURL(const QString& fileFullPathName, const QString& groupHint)
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
    else if (m_fileLayout == 1) //Normal hierarchical file name layout
    {
        bool isFileName = (groupHint.isEmpty());
        QString nameForHier = (isFileName ? fi.fileName() : groupHint);

        bool firstWasPercent  = (nameForHier.length() > 0 && nameForHier.at(0) == QChar('%'));
        bool SecondWasPercent = (nameForHier.length() > 1 && nameForHier.at(1) == QChar('%'));

        //Percent-encode as required and make it short.
        QString sHierName = SafeAndShortFSName(nameForHier, isFileName);

        //We know sHierName just contains ASCII characters.
        //  Use the completeBaseName of the file name to not count e.g 'a.png' a two letter name.
        //    So if it's not a file name we simply don't calculate it's base name!
        //  The `if` check for file name length > 0 was put just in case we pass a file with an
        //    empty base name e.g '.gitconfig', or for future usecases.
        QString sbaseName = sHierName;
        if (isFileName)
            sbaseName = QFileInfo(sHierName).completeBaseName();

        QString firstCharInit = "-";
        if (sbaseName.length() > 0)
            firstCharInit = FolderNameInitialsForASCIIChar(sbaseName.at(0).unicode(), firstWasPercent);

        QString secondCharInit = "-";
        if (sbaseName.length() > 1)
            secondCharInit = FolderNameInitialsForASCIIChar(sbaseName.at(1).unicode(), SecondWasPercent);

        //Put files like 'f/fi/{GroupHint|@BM_Files}/filename.ext'. Read commit msg for '@BM_Files' reason.
        QString fileArchivePath = firstCharInit + "/" + firstCharInit + secondCharInit + "/";
        if (groupHint.isEmpty())
            fileArchivePath += "@BM_Files/";
        else
            fileArchivePath += sHierName + "/";
        QString safeFileName = SafeAndShortFSName(fi.fileName(), true);
        QString fileArchiveURL = fileArchivePath + safeFileName;

        //If file exists, put it in a hashed directory.
        QFileInfo calculatedFileURLInfo(GetFullArchivePathForRelativeURL(fileArchiveURL));
        //`exists` returns `false` if symlink exists but its target doesn't.
        if (calculatedFileURLInfo.exists() || calculatedFileURLInfo.isSymLink())
        {
            QString calculatedFileDir = calculatedFileURLInfo.absolutePath();
            QString randomHash = Util::NonExistentRandomFileNameInDirectory(calculatedFileDir, 8);
            fileArchiveURL = fileArchivePath + randomHash + "/" + safeFileName;
        }

        return fileArchiveURL;
    }

    return QString(); //Error obviously
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

QString FileArchiveManager::SafeAndShortFSName(const QString& fsName, bool isFileName)
{
    //IMPORTANT: Must return only ASCII characters for the file name.
    //Same logic is used in BookmarkImporter::Import.
    QString safeFileName;
    if (isFileName) //We know it is a valid file name then
        safeFileName = Util::PercentEncodeUnicodeChars(fsName);
    else //Arbitrary name
        safeFileName = Util::PercentEncodeUnicodeAndFSChars(fsName);

    if (safeFileName.length() > 64) //For WINDOWS!
    {
        if (isFileName)
        {
            //Shorten the file name
            //Using 'complete' base name but 'short' suffix to resist long file names with
            //  accidental dots in them.
            const QFileInfo safeFileNameInfo(safeFileName);
            const QString cbaseName = safeFileNameInfo.completeBaseName();
            safeFileName = cbaseName.left(qMin(64, cbaseName.length()));
            if (!safeFileNameInfo.suffix().isEmpty())
                safeFileName += "." + safeFileNameInfo.suffix();
        }
        else
        {
            //Is e.g an arbitrary name or a folder name
            safeFileName = safeFileName.left(qMin(64, safeFileName.length()));
        }
    }
    return safeFileName;
}

QString FileArchiveManager::FolderNameInitialsForASCIIChar(char c, bool startedWithPercent)
{
    if ('A' <= c && c <= 'Z') //Uppercase
        return QString(QChar(c));
    else if ('a' <= c && c <= 'z') //Lowercase
        return QString(QChar(c - ('a' - 'A')));
    else if ('0' <= c && c <= '9') //Numbers
        return QString("@Digit");
    else if (c == '%') //Percent encoded unicode
        return QString(startedWithPercent ? "@Sign" : "@Unicode");
    else //All other signs, including '@'.
        return QString("@Sign");
}

QString FileArchiveManager::GetFullArchivePathForRelativeURL(const QString& fileArchiveURL)
{
    //Do NOT canonical-ize or absolute-ize this file name, this function is also called for
    //  non-existent files, e.g at the end of `CalculateFileArchiveURL`.
    QString path = m_archiveRoot + "/" + fileArchiveURL;
    return path;
}
