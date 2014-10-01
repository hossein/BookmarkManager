#include "FileSandBoxManager.h"

#include "Util.h"
#include "WinFunctions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

FileSandBoxManager::FileSandBoxManager(QWidget* dialogParent, Config* conf,
                                       const QString& archiveName, const QString& archiveRoot,
                                       TransactionalFileOperator* filesTransaction)
    : IArchiveManager(dialogParent, conf, archiveName, archiveRoot, filesTransaction)
{

}

FileSandBoxManager::~FileSandBoxManager()
{

}

bool FileSandBoxManager::ClearSandBox()
{
    //Don't remove the SandBox directory itself.
    bool success = Util::RemoveDirectoryRecursively(m_archiveRoot, false);
    if (!success)
        return Error("Can not remove the contents of File SandBox!)");
    return success;
}

bool FileSandBoxManager::AddFileToArchive(const QString& filePathName, bool systemTrashOriginalFile,
                                          QString& fileArchiveURL)
{
    QFileInfo originalfi(filePathName);

    //Check if file is valid.
    if (!originalfi.exists())
        return Error("The selected file \"" + filePathName + "\" does not exist!");
    else if (!originalfi.isFile())
        return Error("The path \"" + filePathName + "\" does not point to a valid file!");

    //We don't use `GetFullArchivePathForRelativeURL` here, we are generating the path now!
    QString sandBoxFilePathName = m_archiveRoot + "/" + originalfi.fileName();

    //Try
    QFileInfo sbfi(sandBoxFilePathName);
    if (sbfi.isDir())
    {
        bool deleteSuccess = Util::RemoveDirectoryRecursively(sandBoxFilePathName);
        if (!deleteSuccess)
        {
            fileArchiveURL = QString();
            return Error("The target for creating sandboxed file '" + sandBoxFilePathName +
                         "' is a directory and cannot be deleted!");
        }
    }
    else if (sbfi.exists()) //and is a file
    {
        bool deleteSuccess = QFile::remove(sandBoxFilePathName);
        if (!deleteSuccess)
        {
            fileArchiveURL = QString();
            return Error("A file already exists at the target for creating sandboxed file '"
                         + sandBoxFilePathName + "' and cannot be deleted!");
        }
    }
    //else if (sbfi not exists)
    //  fine;

    //Copy the file.
    bool copySuccess = QFile::copy(filePathName, sandBoxFilePathName);
    if (!copySuccess)
    {
        fileArchiveURL = QString();
        return Error(QString("Could not create a temporary, sandboxed file for read-only opening!\n"
                             "Can not continue\nSource File: %1\nDestination File: %2")
                     .arg(filePathName, sandBoxFilePathName));
    }

    //Set the Output
    //fileArchiveURL = sandBoxFilePathName; No: must be colon-ized :ArchiveName: URL.
    fileArchiveURL = m_archiveName + "/" + originalfi.fileName();

    //Remove the original file.
    if (systemTrashOriginalFile)
    {
        WinFunctions::MoveFileToRecycleBin(filePathName);
        //We do NOT return FALSE in case of failure.
        Error(QString("Could not delete the original file from your filesystem. "
                      "You should manually delete it yourself.\n\nFile: %1").arg(filePathName));
    };

    return true;
}

bool FileSandBoxManager::RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash)
{
    bool success;
    QString fileOperationError = "Unable to remove a file from the file sandbox.";

    QString fullFilePathName = GetFullArchivePathForRelativeURL(fileRelArchiveURL);

    if (trash)
        success = WinFunctions::MoveFileToRecycleBin(fullFilePathName);
    else
        success = QFile::remove(fullFilePathName);

    if (!success)
        return Error(fileOperationError + QString("\n\nFile Name: ") + fullFilePathName);

    return true;
}

QString FileSandBoxManager::GetFullArchivePathForRelativeURL(const QString& fileArchiveURL)
{
    QString path = m_archiveRoot + "/" + fileArchiveURL;
    return path;
}
