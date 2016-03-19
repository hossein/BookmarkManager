#include "FileSandBoxManager.h"

#include "Util.h"
#include "WinFunctions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

FileSandBoxManager::FileSandBoxManager(QWidget* dialogParent, Config* conf,
                                       const QString& archiveName, const QString& archiveRoot,
                                       TransactionalFileOperator* filesTransaction)
    : IArchiveManager(dialogParent, conf, archiveName, archiveRoot, -1, filesTransaction)
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
                                          const QString& groupHint, const QString& errorWhileContext,
                                          QString& fileArchiveURL)
{
    Q_UNUSED(groupHint);
    QFileInfo originalfi(filePathName);

    //Check if file is valid.
    if (!originalfi.exists())
        return Error(QString("Error while %1:\nThe selected file \"%2\" does not exist!")
                     .arg(errorWhileContext, filePathName));
    else if (!originalfi.isFile())
        return Error(QString("Error while %1:\nThe path \"%2\" does not point to a valid file!")
                     .arg(errorWhileContext, filePathName));

    const QString randomHash = Util::NonExistentRandomFileNameInDirectory(m_archiveRoot, 8);
    const QString sandBoxFileRelPathName = randomHash + "/" + originalfi.fileName();
    const QString sandBoxFilePathName = GetFullArchivePathForRelativeURL(sandBoxFileRelPathName);
    fileArchiveURL = m_archiveName + "/" + sandBoxFileRelPathName; //Out param

    //Let's create the hashed directory
    if (!QDir(m_archiveRoot).mkdir(randomHash))
        return Error(QString("Error while %1:\nTemporary sandbox sub-directory could not be created!\n"
                             "Path: %2").arg(errorWhileContext, m_archiveRoot + "/" + randomHash));

    //Try
    QFileInfo sbfi(sandBoxFilePathName);
    if (sbfi.isDir())
    {
        bool deleteSuccess = Util::RemoveDirectoryRecursively(sandBoxFilePathName);
        if (!deleteSuccess)
            return Error(QString("Error while %1:\nThe target for creating sandboxed file '%2' is a "
                         "directory and cannot be deleted!").arg(errorWhileContext, sandBoxFilePathName));
    }
    else if (sbfi.exists()) //and is a file
    {
        bool deleteSuccess = QFile::remove(sandBoxFilePathName);
        if (!deleteSuccess)
            return Error(QString("Error while %1:\nA file already exists at the target for creating "
                         "sandboxed file '%2' and cannot be deleted!")
                         .arg(errorWhileContext, sandBoxFilePathName));
    }
    //else if (sbfi not exists)
    //  fine;

    //Copy the file.
    bool copySuccess = QFile::copy(filePathName, sandBoxFilePathName);
    if (!copySuccess)
    {
        return Error(QString("Error while %1:\n"
                             "Could not create a temporary, sandboxed file for read-only opening!\n"
                             "Can not continue\nSource File: %2\nDestination File: %3")
                     .arg(errorWhileContext, filePathName, sandBoxFilePathName));
    }

    //Remove the original file.
    if (systemTrashOriginalFile)
    {
        bool Trashsuccess = WinFunctions::MoveFileToRecycleBin(filePathName);
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

bool FileSandBoxManager::RemoveFileFromArchive(const QString& fileRelArchiveURL, bool trash,
                                               const QString& errorWhileContext)
{
    bool success;
    QString fileOperationError = "Error while %1:\n"
                                 "Unable to remove a file from the file sandbox.\n\n"
                                 "File name: %2";

    QString fullFilePathName = GetFullArchivePathForRelativeURL(fileRelArchiveURL);

    if (trash)
        success = WinFunctions::MoveFileToRecycleBin(fullFilePathName);
    else
        success = QFile::remove(fullFilePathName);

    if (!success)
        return Error(fileOperationError.arg(errorWhileContext, fullFilePathName));

    return true;
}

QString FileSandBoxManager::GetFullArchivePathForRelativeURL(const QString& fileArchiveURL)
{
    QString path = m_archiveRoot + "/" + fileArchiveURL;
    return path;
}
