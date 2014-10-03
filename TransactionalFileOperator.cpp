#include "TransactionalFileOperator.h"

#include "Util.h"
#include "WinFunctions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

TransactionalFileOperator::TransactionalFileOperator()
{
    fileTransactionStarted = false;
}

bool TransactionalFileOperator::BeginTransaction()
{
    if (fileTransactionStarted)
        return false;

    fileTransactionStarted = true;
    return true;
}

bool TransactionalFileOperator::CommitTransaction()
{
    if (!fileTransactionStarted)
        return false;

    EndTransaction();
    return true;
}

bool TransactionalFileOperator::RollBackTransaction()
{
    if (!fileTransactionStarted)
        return false;

    bool overallResult = true;
    foreach (const FileOp& fileOp, fileOps)
    {
        switch (fileOp.action)
        {
        case FileOp::FAT_MakePath:
            overallResult &= QDir(fileOp.srcFile).rmpath(fileOp.destFile);
            break;
        case FileOp::FAT_Rename:
            overallResult &= QFile::rename(fileOp.destFile, fileOp.srcFile);
            break;
        case FileOp::FAT_Copy:
            overallResult &= QFile::remove(fileOp.destFile);
            break;
        case FileOp::FAT_Move:
            overallResult &= QFile::copy(fileOp.destFile, fileOp.srcFile);
            overallResult &= QFile::remove(fileOp.destFile);
            break;
        case FileOp::FAT_SystemTrash:
            //Note: We can't undo our own recycle-bin deleting, so we have to save a backup file
            //      in temp. This backup file is the destFile.
            overallResult &= QFile::copy(fileOp.destFile, fileOp.srcFile);
            break;
        case FileOp::FAT_Delete:
            overallResult &= QFile::copy(fileOp.destFile, fileOp.srcFile);
            break;
        }
    }

    EndTransaction();
    return overallResult;
}

bool TransactionalFileOperator::isTransactionStarted()
{
    return fileTransactionStarted;
}

bool TransactionalFileOperator::MakePath(const QString& basePath, const QString& pathToMake)
{
    if (!fileTransactionStarted)
        return false;

    QDir baseDir(basePath);
    bool result = baseDir.mkpath(pathToMake);

    if (result)
        //fileOps.append(FileOp(FileOp::FAT_MakePath, baseDir.absoluteFilePath(pathToMake), ""));
        fileOps.append(FileOp(FileOp::FAT_MakePath, basePath, pathToMake));

    return result;
}

bool TransactionalFileOperator::RenameFile(const QString& oldName, const QString& newName)
{
    if (!fileTransactionStarted)
        return false;

    bool result = QFile::rename(oldName, newName);

    if (result)
        fileOps.append(FileOp(FileOp::FAT_Rename, oldName, newName));

    return result;
}

bool TransactionalFileOperator::CopyFile(const QString& oldPath, const QString& newPath)
{
    if (!fileTransactionStarted)
        return false;

    bool result = QFile::copy(oldPath, newPath);

    if (result)
        fileOps.append(FileOp(FileOp::FAT_Copy, oldPath, newPath));

    return result;
}

bool TransactionalFileOperator::MoveFile(const QString& oldPath, const QString& newPath)
{
    if (!fileTransactionStarted)
        return false;

    bool result = QFile::copy(oldPath, newPath);

    if (result)
        result = QFile::remove(oldPath);

    if (result)
        fileOps.append(FileOp(FileOp::FAT_Move, oldPath, newPath));

    return result;
}

bool TransactionalFileOperator::SystemTrashFile(const QString& filePath)
{
    if (!fileTransactionStarted)
        return false;

    QString backUpFilePath;
    bool result = backupFileInTemp(filePath, backUpFilePath);

    if (result)
        result = WinFunctions::MoveFileToRecycleBin(filePath);

    if (result)
        fileOps.append(FileOp(FileOp::FAT_SystemTrash, filePath, backUpFilePath));

    return result;
}

bool TransactionalFileOperator::DeleteFile(const QString& filePath)
{
    if (!fileTransactionStarted)
        return false;

    QString backUpFilePath;
    bool result = backupFileInTemp(filePath, backUpFilePath);

    if (result)
        result = QFile::remove(filePath);

    if (result)
        fileOps.append(FileOp(FileOp::FAT_Delete, filePath, backUpFilePath));

    return result;
}

void TransactionalFileOperator::EndTransaction()
{
    //Note: We try to remove the backup files in temp and don't care about their removal success.
    foreach (const FileOp& fileOp, fileOps)
        if (fileOp.action == FileOp::FAT_SystemTrash || fileOp.action == FileOp::FAT_Delete)
            QFile::remove(fileOp.destFile); //destFile is the backup file.

    fileOps.clear();
    fileTransactionStarted = false;
}

bool TransactionalFileOperator::backupFileInTemp(const QString& filePath, QString& backUpFilePath)
{
    QFileInfo fi(filePath);
    QString tempDir = QDir::tempPath();
    QString backUpFileNameOnly =
            Util::NonExistentRandomFileNameInDirectory(tempDir, 8, "", "." + fi.suffix());
    backUpFilePath = tempDir + "/" + backUpFileNameOnly;

    bool result = QFile::copy(filePath, backUpFilePath);
    return result;
}
