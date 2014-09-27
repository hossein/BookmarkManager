#include "FileArchiveManager.h"

#include "TransactionalFileOperator.h"
#include "Util.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

//NOTE: In the process of I/FileArchiveManager'izing, the key phrases to search for are
//      the `conf->nominal/prefix` and `QDir::current` thingies.
FileArchiveManager::FileArchiveManager(QWidget* dialogParent, Config* conf,
                                       const QString& archiveName, const QString& archiveRoot,
                                       TransactionalFileOperator* filesTransaction)
    : IArchiveManager(dialogParent, conf, archiveName, archiveRoot, filesTransaction)
{

}

FileArchiveManager::~FileArchiveManager()
{

}

bool FileArchiveManager::AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                                          QString& fileArchiveURL)
{
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

int FileArchiveManager::FileNameHash(const QString& fileNameOnly)
{
    //For now just calculates the utf-8 sum of all bytes.
    QByteArray utf8 = fileNameOnly.toUtf8();
    int sum = 0;

    for (int i = 0; i < utf8.size(); i++)
        sum += utf8[i];

    return sum;
}

QString FileArchiveManager::GetFullArchivePathForRelativeURL(const QString& fileArchiveURL)
{
    QString path = m_archiveRoot + "/" + fileArchiveURL;
    return path;
}
