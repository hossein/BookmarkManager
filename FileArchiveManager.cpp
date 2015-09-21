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
                                          QString& fileArchiveURL)
{
    //This is like an assert.
    if (!filesTransaction->isTransactionStarted())
        return Error("No file transaction was started before adding files to archive.");

    //Check if file is valid.
    QFileInfo fi(filePathName);
    if (!fi.exists())
        return Error("The selected file \"" + filePathName + "\" does not exist!");
    else if (!fi.isFile())
        return Error("The path \"" + filePathName + "\" does not point to a valid file!");

    //Decide where should the file be copied.
    QString fileRelArchiveURL = CalculateFileArchiveURL(filePathName);
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
            return Error("Could not create the directory for placing the attached file."
                         "\n\nDirectory: " + tdi.absoluteFilePath());
    }
    else if (!tdi.isDir())
    {
        return Error("The path for placing the attached file is not a directory!"
                     "\n\nDirectory: " + tdi.absoluteFilePath());
    }

    //Copy the file.
    bool success = filesTransaction->CopyFile(filePathName, targetFilePathName);
    if (!success)
        return Error(QString("Could not copy the source file to destination directory!"
                             "\n\nSource File: %1\nDestination File: %2")
                             .arg(filePathName, targetFilePathName));

    //Remove the original file.
    if (systemTrashOriginalFile)
    {
        success = filesTransaction->SystemTrashFile(filePathName);
        //We do NOT return FALSE in case of failure.
        Error(QString("Could not delete the original file from your filesystem. "
                      "You should manually delete it yourself.\n\nFile: %1").arg(filePathName));
    };

    return true;
}

bool FileArchiveManager::RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash)
{
    //This is like an assert.
    if (!filesTransaction->isTransactionStarted())
        return Error("No file transaction was started before removing files from archive.");

    bool success;
    QString fileOperationError = "Unable to remove a file from the FileArchive.";

    QString fullFilePathName = GetFullArchivePathForRelativeURL(fileRelArchiveURL);

    if (trash)
        success = filesTransaction->SystemTrashFile(fullFilePathName);
    else
        success = filesTransaction->DeleteFile(fullFilePathName);

    if (!success)
        return Error(fileOperationError + QString("\n\nFile Name: ") + fullFilePathName);

    return true;
}

QString FileArchiveManager::CalculateFileArchiveURL(const QString& fileFullPathName)
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
        const QString nubaseName = fi.baseName();
        bool firstWasPercent  = (nubaseName.length() > 0 && nubaseName.at(0) == QChar('%'));
        bool SecondWasPercent = (nubaseName.length() > 1 && nubaseName.at(1) == QChar('%'));

        QString safeFileName = Util::PercentEncodeUnicodeChars(fi.fileName());
        if (safeFileName.length() > 50) //For WINDOWS!
        {
            //Shorten the file name
            //Using 'complete' base name but 'short' suffix to resist long file names with
            //  accidental dots in them.
            const QFileInfo safeFileNameInfo(safeFileName);
            const QString cbaseName = safeFileNameInfo.completeBaseName();
            safeFileName = cbaseName.left(qMin(50, cbaseName.length()));
            if (!safeFileNameInfo.suffix().isEmpty())
                safeFileName += "." + safeFileNameInfo.suffix();
        }

        //We know safeFileName just contains ASCII characters.
        //  Use the completeBaseName of the file name to not count e.g 'a.png' a two letter name.
        //  The `if` check for file name length > 0 was put just in case we pass a file with an
        //    empty base name e.g '.gitconfig', or for future usecases.
        const QString cbaseName = QFileInfo(safeFileName).completeBaseName();
        QString firstCharInit = "-";
        if (cbaseName.length() > 0)
            firstCharInit = FolderNameInitialsForASCIIChar(cbaseName.at(0).unicode(), firstWasPercent);

        QString secondCharInit = "-";
        if (cbaseName.length() > 1)
            secondCharInit = FolderNameInitialsForASCIIChar(cbaseName.at(1).unicode(), SecondWasPercent);

        //Put files like 'f/fi/filename.ext'.
        QString fileArchiveURL = firstCharInit + "/" + firstCharInit + secondCharInit + "/" + safeFileName;

        //If file exists, put it in a hashed directory.
        QFileInfo calculatedFileURLInfo(GetFullArchivePathForRelativeURL(fileArchiveURL));
        //`exists` returns `false` if symlink exists but its target doesn't.
        if (calculatedFileURLInfo.exists() || calculatedFileURLInfo.isSymLink())
        {
            QString calculatedFileDir = calculatedFileURLInfo.absolutePath();
            QString randomHash = Util::NonExistentRandomFileNameInDirectory(calculatedFileDir, 8);
            fileArchiveURL = firstCharInit + "/" + firstCharInit + secondCharInit + "/"
                           + randomHash + "/" + safeFileName;
        }

        return fileArchiveURL;
    }
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
